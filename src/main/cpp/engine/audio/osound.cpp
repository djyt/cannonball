#include <iostream> // needed for debugging only. Can be removed.
#include "engine/audio/osound.hpp"

/***************************************************************************
    Ported Z80 Sound Playing Code.
    Controls Sega PCM and YM2151 Chips.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// Use YM2151 Timing
#define TIMER_CODE

// Enable Unused code block warnings
#define UNUSED_WARNINGS

using namespace z80_adr;

OSound::OSound()
{

}

OSound::~OSound()
{
}

void OSound::init(YM2151* ym, uint8_t* pcm_ram)
{
    this->ym      = ym;
    this->pcm_ram = pcm_ram;

    command_input = 0;
    sound_props   = 0;
    pos           = 0;
    counter1      = 0;
    counter2      = 0;
    counter3      = 0;
    counter4      = 0;

    // Clear AM RAM 0xF800 - 0xFFFF
    for (uint16_t i = 0; i < CHAN_RAM_SIZE; i++)
        chan_ram[i] = 0;

    // Enable all PCM channels by default
    for (int8_t i = 0; i < 16; i++)
    {
        pcm_ram[6 + (i *8)] = 1; // Channel Active
    }

    init_fm_chip();
}

// Initialize FM Chip. Initalize and start Timer A.
// Source: 0x79
void OSound::init_fm_chip()
{
   command_input = sound::RESET;

   // Initialize the FM Chip with the set of default commands
   fm_write_block(0, YM_INIT_CMDS, 0);

   // Start Timer A & enable its IRQ, and do an IRQ reset (%00110101)
   fm_write_reg(0x14, 0x35);
}

void OSound::tick()
{
    fm_dotimera();      // FM: Process Timer A. Stop Timer B
    process_command();  // Process Command sent by main program code (originally the main 68k processor)
    process_channels(); // Run logic on individual sound channel (both YM & PCM channels)
}

// PCM RAM Read/Write Helper Functions
uint8_t OSound::pcm_r(uint16_t adr)
{
    return pcm_ram[adr & 0xFF];
}

void OSound::pcm_w(uint16_t adr, uint8_t v)
{
    pcm_ram[adr & 0xFF] = v;
}

// RAM Read/Write Helper Functions
uint16_t OSound::r16(uint8_t* adr)
{
    return ((*(adr+1) << 8) | *adr);
}

void OSound::w16(uint8_t* adr, uint16_t v)
{
    *adr     = v & 0xFF;
    *(adr+1) = v >> 8;
}

// Process Command Sent By 68K CPU
// Source: 0x74F
void OSound::process_command()
{
    // Reset FM Chip
    if (command_input == 0 || command_input == 0xFF)
    {
        fm_reset();
    }
    else if (command_input == sound::RESET)
        return;
    // Clear Z80 Command
    else if (command_input < 0x80 || command_input >= 0xFF)
    {
        command_input = sound::RESET;
        new_command();
    }
    else
    {
        uint8_t cmd   = command_input;
        command_input = sound::RESET;

        switch (cmd)
        {
            case sound::RESET:
                break;

            case sound::MUSIC_BREEZE:
                fm_reset();
                sound_props |= BIT_1; // Trigger rev effect
                init_sound(cmd, DATA_BREEZE, channel::YM1);
                break;

            case sound::MUSIC_SPLASH:
                fm_reset();
                sound_props |= BIT_1; // Trigger rev effect
                init_sound(cmd, DATA_SPLASH, channel::YM1);
                break;

            case sound::COIN_IN:
                init_sound(cmd, DATA_COININ, channel::YM_FX1);
                break;

            case sound::MUSIC_MAGICAL:
                fm_reset();
                sound_props |= BIT_1; // Trigger rev effect
                init_sound(cmd, DATA_MAGICAL, channel::YM1);
                break;

            case sound::YM_CHECKPOINT:
                init_sound(cmd, DATA_CHECKPOINT, channel::YM_FX1);
                break;

            case sound::INIT_SLIP:
                init_sound(cmd, DATA_SLIP, channel::PCM_FX3);
                break;

            case sound::INIT_CHEERS:
                init_sound(cmd, DATA_CHEERS, channel::PCM_FX1);
                break;

            case sound::STOP_CHEERS:
                chan_ram[channel::PCM_FX1] = 0;
                chan_ram[channel::PCM_FX2] = 0;
                pcm_w(0xF9E0, 1); // Set inactive flag on channels
                pcm_w(0xFA00, 1);
                break;

            case sound::INIT_CRASH1:
                init_sound(cmd, DATA_CRASH1, channel::PCM_FX5);
                break;

            case sound::INIT_REBOUND:
                init_sound(cmd, DATA_REBOUND, channel::PCM_FX5);
                break;

            case sound::INIT_CRASH2:
                init_sound(cmd, DATA_CRASH2, channel::PCM_FX5);
                break;

            case sound::NEW_COMMAND:
                new_command();
                break;

            case sound::INIT_SIGNAL1:
                init_sound(cmd, DATA_SIGNAL1, channel::YM_FX1);
                break;

            case sound::INIT_SIGNAL2:
                sound_props &= ~BIT_0; // Clear rev effect
                init_sound(cmd, DATA_SIGNAL2, channel::YM_FX1);
                break;

            case sound::INIT_WEIRD:
                init_sound(cmd, DATA_WEIRD, channel::PCM_FX5);
                break;

            case sound::STOP_WEIRD:
                chan_ram[channel::PCM_FX5] = 0;
                chan_ram[channel::PCM_FX6] = 0;
                pcm_w(0xF0CE, 1); // Set inactive flag on channels
                pcm_w(0xF0DE, 1);
                break;

            case sound::BEEP1:
                init_sound(cmd, DATA_BEEP1, channel::YM_FX1);
                break;

            case sound::UFO:
                fm_reset();
                init_sound(cmd, DATA_UFO, channel::YM_FX1);
                break;

            case sound::BEEP2:
                fm_reset();
                init_sound(cmd, DATA_BEEP2, channel::YM1);
                break;

            case sound::INIT_CHEERS2:
                init_sound(cmd, DATA_CHEERS2, channel::PCM_FX1);
                break;

            case sound::VOICE_CHECKPOINT:
                pcm_backup();
                init_sound(cmd, DATA_VOICE1, channel::PCM_FX7);
                break;

            case sound::VOICE_CONGRATS:
                pcm_backup();
                init_sound(cmd, DATA_VOICE2, channel::PCM_FX7);
                break;

            case sound::VOICE_GETREADY:
                pcm_backup();
                init_sound(cmd, DATA_VOICE3, channel::PCM_FX7);
                break;

            case sound::INIT_SAFETYZONE:
                init_sound(cmd, DATA_SAFETY, channel::PCM_FX3);
                break;

            case sound::STOP_SLIP:
            case sound::STOP_SAFETYZONE:
                chan_ram[channel::PCM_FX3] = 0;
                chan_ram[channel::PCM_FX4] = 0;
                pcm_w(0xF0AE, 1); // Set inactive flag on channels
                pcm_w(0xF0BE, 1);
                break;

            case sound::YM_SET_LEVELS:
                ym_set_levels();
                break;

            case sound::PCM_WAVE:
                init_sound(cmd, DATA_WAVE, channel::PCM_FX1);
                break;

            case sound::MUSIC_LASTWAVE:
                init_sound(cmd, DATA_LASTWAVE, channel::YM1);
                break;

            #ifdef UNUSED_WARNINGS
            default:
                std::cout << "Missing command: " << cmd << std::endl;
                break;
            #endif
        }
    }
}

// Called before initalizing a new command (PCM standalone sample, FM sample, New Music)
// Source: 0x833
void OSound::new_command()
{
    // ------------------------------------------------------------------------
    // FM Sound Effects Only (Increase Volume)
    // ------------------------------------------------------------------------
    if (chan_ram[channel::YM_FX1] & BIT_7)
    {
        chan_ram[channel::YM_FX1] = 0;

        uint16_t adr = YM_LEVEL_CMDS2;

        // Send four level commands
        for (uint8_t i = 0; i < 4; i++)
        {
            uint8_t reg = roms.z80.read8(&adr);
            uint8_t val = roms.z80.read8(&adr);
            fm_write_reg(reg, val);
        }
    }

    // Clear channel memory area used for internal format of PCM sound data
    for (int16_t i = channel::PCM_FX1; i < channel::YM_FX1; i++)
        chan_ram[i] = 0;

    // PCM Channel Enable
    uint16_t pcm_enable = 0xF088 + 6;

    // Disable 6 PCM Channels
    for (int16_t i = 0; i < 6; i++)
    {
        pcm_w(pcm_enable, pcm_r(pcm_enable) | BIT_0); 
        pcm_enable += 0x10; // Advance to next channel
    }
}

// Copy PCM Channel RAM contents to Channel RAM
// Source: 0x961
void OSound::pcm_backup()
{
    // Return if PCM Channel contents already backed up
    if (sound_props & BIT_1)
        return;

    memcpy(chan_ram + CH09_CMDS1, pcm_ram + 0x40, 8); // Channel 9 Blocks
    memcpy(chan_ram + CH09_CMDS2, pcm_ram + 0xC0, 8);
    memcpy(chan_ram + CH11_CMDS1, pcm_ram + 0x50, 8); // Channel 11 Blocks
    memcpy(chan_ram + CH11_CMDS1, pcm_ram + 0xD0, 8);

    sound_props |= BIT_1; // Denote contents backed up
}

// Check whether FM Channel is in use
// Map back to corresponding music channel
//
// Source: 0x95
void OSound::check_fm_mapping()
{
    uint16_t chan_id = channel::MAP1;

    // 8 Channels
    for (uint8_t i = 0; i < 8; i++)
    {
        if (chan_ram[chan_id] & BIT_7)
        {
            // Map back to corresponding music channel
            chan_ram[chan_id - 0x2C0] |= BIT_2;
        }
        chan_id += 0x20;
    }
}

// Process and update all sound channels
//
// Source: 0xB3
void OSound::process_channels()
{
    // Allows FM Music & FM Effect to be played simultaneously
    check_fm_mapping();

    // Channel to process
    uint16_t chan_id = channel::YM1;

    for (uint8_t c = 0; c < 30; c++)
    {
        // If channel is enabled, process the channel
        if (chan_ram[chan_id] & BIT_7)
            process_channel(chan_id);

        chan_id += 0x20; // Advance to next channel in memory
    }
}

// Update Individual Channel.
//
// Inputs:
// chan_id = Channel ID to process
//
// Source: 0xCD
void OSound::process_channel(uint16_t chan_id)
{
    // Get correct offset in RAM
    uint8_t* chan = &chan_ram[chan_id];

    // Increment sequence position
    pos = r16(&chan[ch::SEQ_POS]) + 1;
    w16(&chan[ch::SEQ_POS], pos);

    // Sequence end marker
    uint16_t seq_end = r16(&chan[ch::SEQ_END]);

    //std::cout << std::hex << "seq pos: " << seq_pos << " seq end: " << seq_end << std::endl;

    if (pos == seq_end)
    {
        pos = r16(&chan[ch::SEQ_CMD]);
        process_section(chan);
        
        // Hack to return when last command was a PCM/YM finalize
        // This is here to facilitate program flow. 
        // The Z80 code does loads of funky (or nasty depending on your point of view) stuff with the stack. 
        if (cmd_prev == 0x84 || cmd_prev == 0x99) 
            return;
    }

    // Return if not FM channel
    if (chan[ch::FM_FLAGS] & BIT_6) return;

    // ------------------------------------------------------------------------
    // FM CHANNELS
    // ------------------------------------------------------------------------

    #ifdef UNUSED_WARNINGS
    if (chan[ch::CTRL])
        std::cout << "process_channel - unimplemented code 0x167" << std::endl;

    if (chan[ch::FLAGS] & BIT_5)
        std::cout << "process_channel - unimplemented code 0x21A" << std::endl;
    #endif

    // 0xF9:  
    uint8_t reg;
    uint8_t chan_index = chan[ch::FM_FLAGS] & 7;

    // Use Phase and Amplitude Modulation Sensitivity Table?
    if (chan[ch::FM_PHASETBL])
    {
        read_mod_table(chan);
    }

    // If note set to 0xFF, turn off the channel output
    if (chan[ch::FM_NOTE] == 0xFF)
    {
        fm_write_reg_c(chan[ch::FLAGS], 8, chan_index);
        return;
    }

    // FM Noise Channel
    if (chan[ch::FLAGS] & BIT_1)
    {
        reg = 0xF; // Register = Noise Enable & Frequency
    }
    // Set Volume or Mute if PHASETBL not setup
    else
    {
        // 0x119
        // Register = Phase and Amplitude Modulation Sensitivity ($38-$3F)
        fm_write_reg_c(chan[ch::FLAGS], 0x30 + chan_index, chan[ch::FM_PHASETBL] ? chan[ch::FM_PHASE_AMP] : 0);

        // Register = Create Note ($28-$2F)
        reg = 0x28 + chan_index;
    }

    // set_octave_note: 
    fm_write_reg_c(chan[ch::FLAGS], reg, chan[ch::FM_NOTE]);

    // Check position in sequence. If expired, set channels on/off
    if (r16(&chan[ch::SEQ_POS])) return;

    // Turn channels off
    fm_write_reg_c(chan[ch::FLAGS], 8, chan_index);

    // Turn modulator and carry channels off
    fm_write_reg_c(chan[ch::FLAGS], 8, chan_index | 0x78);
}

// Process Channel Section
//
// Source: 0x2E1
void OSound::process_section(uint8_t* chan)
{
    uint8_t cmd = roms.z80.read8(&pos);
    cmd_prev = cmd;
    if (cmd >= 0x80)
    {
        do_command(chan, cmd);
        return;
    }

    // ------------------------------------------------------------------------
    // FM Only Code From Here Onwards
    // ------------------------------------------------------------------------

    #ifdef UNUSED_WARNINGS
    // Not sure, unused?
    if (chan[ch::FLAGS] & BIT_5)
    {
        std::cout << "Warning: process_section - unimplemented code 0x36D!" << std::endl;
        return;
    }
    
    // Is FM Noise Channel?
    if (chan[ch::FLAGS] & BIT_1)
    {
        std::cout << "Warning: process_section - unimplemented code 2!" << std::endl;
        return;
    }
    #endif
    
    // 0x30d set_note_octave
    // Command is an offset into the Note Offset table in ROM.
    if (cmd)
    {
        uint16_t adr = YM_NOTE_OCTAVE + (cmd - 1 + (int8_t) chan[ch::NOTE_OFFSET]);
        chan[ch::FM_NOTE] = roms.z80.read8(adr);
    }
    // If Channel is mute, clear note information.
    else if (!(chan[ch::FM_FLAGS] & BIT_6))
    {
        chan[ch::FM_NOTE] = 0xFF;
    }

    calc_end_marker(chan);
}

// Source: 0x31C
void OSound::calc_end_marker(uint8_t* chan)
{
    uint16_t end_marker = roms.z80.read8(pos);
    
    // 32d
    if (chan[ch::FM_MARKER] & BIT_1)
    {
        // Mask on high bit
        if (chan[ch::FM_MARKER] & BIT_0)
        {
            chan[ch::FM_MARKER] &= ~BIT_0;
            end_marker += (roms.z80.read8(++pos) << 8); 
        }
    }
    // 325
    else
    {
        end_marker = chan[ch::END_MARKER] * end_marker;
    }

    // Set End Marker
    w16(&chan[ch::SEQ_END], end_marker);
    // Set Next Sequence Command
    w16(&chan[ch::SEQ_CMD], ++pos);

    w16(&chan[ch::UNKNOWN], 0);
    w16(&chan[ch::SEQ_POS], 0);
}


// Trigger New Command From Sound Data.
//
// Source: 0x3A8
void OSound::do_command(uint8_t* chan, uint8_t cmd)
{
    // Play New PCM Sample
    if (cmd >= 0xBF)
    {
        play_pcm_index(chan, cmd);
        return;
    }

    // Trigger New Command On Channel
    switch (cmd & 0x3F)
    {
        // YM & PCM: Set Volume
        case 0x02:
            setvol(chan);
            break;

        case 0x04:
            ym_finalize(chan);
            return;

        // YM: Enable/Disable Modulation table
        case 0x07:
            chan[ch::FM_PHASETBL] = roms.z80.read8(pos);
            break;

        // Write Sequence Address (Used by PCM Drum Samples)
        case 0x08:
            write_seq_adr(chan);
            break;

        // Set Next Sequence Address [pos] (Used by PCM Drum Samples)
        case 0x09:
            pos = r16(&chan[chan[ch::MEM_OFFSET]]);
            chan[ch::MEM_OFFSET] += 2;
            break;

        case 0x0A:
            set_loop_adr();
            break;

        // YM: Set Note/Octave Offset
        case 0x0B:
            chan[ch::NOTE_OFFSET] += roms.z80.read8(pos);
            break;

        case 0x0C:
            do_loop(chan);
            break;

        case 0x11:
            ym_set_block(chan);
            break;

        case 0x13:
            pcm_setpitch(chan);
            break;

        // FM: End Marker - Do not calculate, use value from data
        case 0x14:
            chan[ch::FM_MARKER] |= BIT_1;
            pos--;
            break;

        // FM: End Marker - Set High Byte From Data
        case 0x15:
            chan[ch::FM_MARKER] |= BIT_0;
            pos--;
            break;

        // FM: Connect Channel to Right Speaker
        case 0x16:
            ym_set_connect(chan, PAN_RIGHT);
            break;

        // FM: Connect Channel to Left Speaker
        case 0x17:
            ym_set_connect(chan, PAN_LEFT);
            break;

        // FM: Connect Channel to Both Speakers
        case 0x18:
            ym_set_connect(chan, PAN_CENTRE);
            break;

        case 0x19:
            pcm_finalize(chan);
            return;
        
        #ifdef UNUSED_WARNINGS
        default:
            std::cout << std::hex << "do_command(...) Unsupported command: " << (int16_t) cmd << " : " << (int16_t) (cmd & 0x3F) << std::endl;
            break;
        #endif
    }

    pos++;
    process_section(chan);
}

// Set Volume (Left & Right Channels)
// Source: 0x3D7
void OSound::setvol(uint8_t* chan)
{  
    uint8_t vol_l = roms.z80.read8(pos);

    // PCM
    if (chan[ch::FM_FLAGS] & BIT_6)
    {
        const uint8_t VOL_MAX = 0x40;

        // Set left volume
        chan[ch::VOL_L] = vol_l > VOL_MAX ? 0 : vol_l;

        // Set right volume
        uint8_t vol_r = roms.z80.read8(++pos);
        chan[ch::VOL_R] = vol_r > VOL_MAX ? 0 : vol_r;
    }
    // YM
    else
    {
        chan[ch::FM_MARKER] = vol_l;
    }
}

// PCM Command: Set Pitch
// Source: 0x3C4
void OSound::pcm_setpitch(uint8_t* chan)
{
    // PCM Channel
    if (chan[ch::FM_FLAGS] & BIT_6)
        chan[ch::PCM_PITCH] = roms.z80.read8(pos);
}

// Sets Loop Address For FM & PCM Commands.
// For example, used by Checkpoint FM sample and Crowd PCM Sample
// Source: 0x446
void OSound::set_loop_adr()
{
    pos = roms.z80.read16(pos) - 1;
}

// Set Loop Counter For FM & PCM Data
// Source: 0x454
void OSound::do_loop(uint8_t* chan)
{
    uint8_t offset = roms.z80.read8(pos++) + 0x18;
    uint8_t a = chan[offset];

    // Reload counter
    if (a == 0)
    {
        chan[offset] = roms.z80.read8(pos);
    }

    pos++;
    if (--chan[offset] != 0)
        set_loop_adr();
    else
        pos++;
}

// Write Commands to PCM Channel (For Individual Sound Effects)
// Source: 0x483
void OSound::pcm_finalize(uint8_t* chan)
{
    sound_props &= ~BIT_1; // Clear PCM sound effect triggered

    memcpy(pcm_ram + 0x40, chan_ram + CH09_CMDS1, 8); // Channel 9 Blocks
    memcpy(pcm_ram + 0xC0, chan_ram + CH09_CMDS2, 8);
    memcpy(pcm_ram + 0x50, chan_ram + CH11_CMDS1, 8); // Channel 11 Blocks
    memcpy(pcm_ram + 0xD0, chan_ram + CH11_CMDS1, 8);
    
    ym_finalize(chan);
}

// Percussion sample properties
// Sample Start Address, Sample End Address (High Byte Only), Pitch, Sample Flags
const static uint16_t PCM_PERCUSSION [] =
{
    0x17C0, 0x42, 0x84, 0xD6,
    0x302F, 0x3A, 0x84, 0xC6,
    0x0090, 0x0B, 0x84, 0xC2,
    0x00F0, 0x08, 0x84, 0xD2,
    0x49DE, 0x4C, 0x84, 0xD2,
    0x437C, 0x48, 0x84, 0xD2,
    0x29AB, 0x2E, 0x84, 0xC2,
    0x1C03, 0x28, 0x84, 0xC2,
    0x0951, 0x16, 0x84, 0xD2,
    0x3BE6, 0x7F, 0x90, 0xC2,
    0x5830, 0x5F, 0x84, 0xD2,
    0x4DA0, 0x57, 0x84, 0xD2,
    0x0C1D, 0x1B, 0x78, 0xC2,
    0x6002, 0x7F, 0x84, 0xD2,
    0x0C1D, 0x1B, 0x40, 0xC2,
};

// Play PCM Sample.
// This routine is used both for standalone samples, and samples used by the music.
//
// +0 [Word]: Start Address In Bank
// +1 [Byte]: End Address (<< 8 + 1)
// +2 [Byte]: Sample Flags
//
// Bank 0: opr10193.66
// Bank 1: opr10192.67
// Bank 2: opr10191.68
//
// Source: 0x57A
void OSound::play_pcm_index(uint8_t* chan, uint8_t cmd)
{
    if (cmd == 0)
    {
        calc_end_marker(chan);
        return;
    }

    // ------------------------------------------------------------------------
    // Initalize PCM Effects
    // ------------------------------------------------------------------------
    if (cmd >= 0xD0)
    {
        // First sample is at index 0xD0, so reset to 0
        uint16_t pcm_index = PCM_INFO + ((cmd - 0xD0) << 2);

        chan[ch::PCM_ADR1L] = roms.z80.read8(&pcm_index);           // Wave Start Address
        chan[ch::PCM_ADR1H] = roms.z80.read8(&pcm_index);
        chan[ch::PCM_ADR2]  = roms.z80.read8(&pcm_index);           // Wave End Address
        chan[ch::CTRL]      = roms.z80.read8(&pcm_index);           // Sample Flags
    }
    // ------------------------------------------------------------------------
    // Initalize PCM Percussion Samples
    // ------------------------------------------------------------------------
    else
    {
        uint16_t pcm_index = (cmd - 0xC0) << 2;
        w16(&chan[ch::PCM_ADR1L], PCM_PERCUSSION[pcm_index]);        // Wave Start Address
        chan[ch::PCM_ADR2]  = (uint8_t) PCM_PERCUSSION[pcm_index+1]; // Wave End Address
        chan[ch::PCM_PITCH] = (uint8_t) PCM_PERCUSSION[pcm_index+2]; // Wave Pitch
        chan[ch::CTRL]      = (uint8_t) PCM_PERCUSSION[pcm_index+3]; // Sample Flags
    }

    process_pcm(chan);
}

// Initalize Sound from ROM to RAM. Used for both PCM and YM sounds.
//
// Inputs: 
// a = Command, de = dst, hl = src
//
// Source: 0x9E8
void OSound::init_sound(uint8_t cmd, uint16_t src, uint16_t dst)
{
    command_index = cmd - 0x81;
    
    // Get offset to channel setup
    src = roms.z80.read16(src);

    // Get number of channels
    uint8_t channels = roms.z80.read8(&src);

    // next_channel
    for (uint8_t ch = 0; ch < channels; ch++)
    {
        // Address of default channel setup data
        uint16_t adr = roms.z80.read16(&src);

        // Copy default setup code for block of sound (14 bytes)
        for (int i = 0; i < 0xE; i++)
            chan_ram[dst++] = roms.z80.read8(&adr);

        // Write command byte (at position 0xE)
        chan_ram[dst++] = command_index;

        // Write zero 17 times (essentially pad out the 0x20 byte entry)
        // This empty space is updated later
        for (int i = 0xF; i < 0x20; i++)
            chan_ram[dst++] = 0;
    }
}

// Source: 0xCC3
void OSound::process_pcm(uint8_t* chan)
{
    // ------------------------------------------------------------------------
    // PCM Music Sample (Drums)
    // ------------------------------------------------------------------------
    if (chan[ch::CTRL] & BIT_7)
    {
        const uint16_t BASE_ADR = 0xF088; // Channel 2 Base Address
        const uint16_t CHAN_SIZE = 0x10;  // Size of each channel entry

        uint16_t adr = BASE_ADR; 

        // Check Wave End Address
        if (chan[ch::CTRL] & BIT_2)
        {
            // get_chan_adr2:
            for (int i = 0; i < 6; i++)
            {
                uint8_t channel_pair = pcm_r(adr + 6);

                // If channel active, play sample
                if ((channel_pair & 0x84) == 0x84 && (channel_pair & BIT_0) == 0)
                {
                    pcm_send_cmds(chan, adr, channel_pair);
                    return;
                }
                // d79
                adr += CHAN_SIZE; // Advance to next channel
            }
            adr = BASE_ADR;
        }
        // get_chan_adr3:
        for (int i = 0; i < 6; i++)
        {
            uint8_t channel_pair = pcm_r(adr + 6);

            if (channel_pair & BIT_0)
            {
                pcm_send_cmds(chan, adr, channel_pair);
                return;
            }

            adr += CHAN_SIZE; // Advance to next channel
        }

        adr = BASE_ADR;
        // get_chan_adr4:
        for (int i = 0; i < 6; i++)
        {
            uint8_t channel_pair = pcm_r(adr + 6);

            if (channel_pair & BIT_7)
            {
                pcm_send_cmds(chan, adr, channel_pair);
                return;
            }
            adr += CHAN_SIZE; // Advance to next channel
        }

        // No need to restore positioning info from stack, as stored in 'pos' variable
        calc_end_marker(chan);
    }

    // ------------------------------------------------------------------------
    // Standard PCM Samples
    // ------------------------------------------------------------------------
    else
    {
        // Mask on channel pair select
        uint8_t channel_pair = chan[ch::CTRL] & 0xC;
        // Channel selected [b]
        uint8_t selected = 0;

        // select_ch_8or10:
        if (channel_pair < 4)
        {
            // Denote PCM sound effect triggered.
            sound_props |= BIT_1;
            if (++counter1 & BIT_0)
            {
                selected = 10;      
                pcm_w(0xF0D6, channel_pair = 1); // Set flags for channel 10 (active, loop disabled)
            }
            else
            {
                selected = 8;
                pcm_w(0xF0C6, channel_pair = 1); // Set flags for channel 10 (active, loop disabled)
            }
        }
        // select_ch_1or3
        else if (channel_pair == 4)
        {
            if (++counter2 & BIT_0)
                selected = 3;      
            else
                selected = 1;
        }
        // select_ch_9or11
        else if (channel_pair == 8)
        {
            if (++counter3 & BIT_0)
                selected = 11;      
            else
                selected = 9;
        }
        // select_ch_5or7
        else
        {
            if (++counter4 & BIT_0)
                selected = 7;      
            else
                selected = 5;
        }

        // Channel Address = Channel 1 Base Address 
        uint16_t pcm_adr = 0xF080 + (selected * 8);

        pcm_send_cmds(chan, pcm_adr, channel_pair);
    }
}

// Source: 0xDA8
void OSound::pcm_send_cmds(uint8_t* chan, uint16_t pcm_adr, uint8_t channel_pair)
{
    pcm_w(pcm_adr + 0x80, channel_pair);        // Write channel pair selected value
    pcm_w(pcm_adr + 0x82, chan[ch::VOL_L]);     // Volume left
    pcm_w(pcm_adr + 0x83, chan[ch::VOL_R]);     // Volume Right
    pcm_w(pcm_adr + 0x84, chan[ch::PCM_ADR1L]); // PCM Start Address Low
    pcm_w(pcm_adr + 0x4,  chan[ch::PCM_ADR1L]); // PCM Loop Address Low
    pcm_w(pcm_adr + 0x85, chan[ch::PCM_ADR1H]); // PCM Start Address High
    pcm_w(pcm_adr + 0x5,  chan[ch::PCM_ADR1H]); // PCM Loop Address High
    pcm_w(pcm_adr + 0x86, chan[ch::PCM_ADR2]);  // PCM End Address High
    pcm_w(pcm_adr + 0x87, chan[ch::PCM_PITCH]); // PCM Pitch
    pcm_w(pcm_adr + 0x6,  chan[ch::CTRL]);      // PCM Flags

    // No need to restore positioning info from stack, as stored in 'pos' variable
    calc_end_marker(chan);
}

void OSound::fm_dotimera()
{
    #ifdef TIMER_CODE
    // Return if YM2151 is busy
    if (!(ym->read_status() & BIT_0))
        return;
    #endif
    // Set Timer A, Enable its IRQ and also reset its IRQ
    ym->write_reg(0x14, 0x15); // %10101
}

// Reset Yamaha YM2151 Chip.
// Called before inititalizing a new music track, or when z80 has just initalized and has no specific command.
// Source: 0x561
void OSound::fm_reset()
{
    // Clear YM & Drum Channels in RAM (0xF820 - 0xF9DF)
    for (uint16_t i = channel::YM1; i < channel::PCM_FX1; i++)
        chan_ram[i] = 0;

    fm_write_block(0, YM_INIT_CMDS, 0);
}

// Write to FM Register With Check
// Source: 0xA70
void OSound::fm_write_reg_c(uint8_t ix0, uint8_t reg, uint8_t value)
{
    // Is corresponding music channel enabled?
    if (ix0 & BIT_2)
        return;

    fm_write_reg(reg, value);
}

// Write to FM Register
// Source: 0xA75
void OSound::fm_write_reg(uint8_t reg, uint8_t value)
{
    #ifdef TIMER_CODE
    // Return if YM2151 is busy
    if (ym->read_status() & BIT_7)
        return;
    #endif
    ym->write_reg(reg, value);
}

// Write Block of FM Data From ROM
//
// Inputs:
//
// adr  = Address of commands and data to write to FM
// chan = Register offset
//
// Format:
// 2 = End of data block
// 3 = Next word specifies next address in memory
//
// Source: 0xA84
void OSound::fm_write_block(uint8_t ix0, uint16_t adr, uint8_t chan)
{
    uint8_t cmd = roms.z80.read8(&adr);

    // Return if end of data block
    if (cmd == 2) return;

    // Next word specifies next address in memory
    if (cmd == 3)
    {
        adr = roms.z80.read16(adr);
    }
    else
    {
        uint8_t reg = cmd + chan;
        uint8_t val = roms.z80.read8(&adr);
        fm_write_reg_c(0, reg, val);
    }

    fm_write_block(ix0, adr, chan);
}

// Write level info to YM Channels
// Source: 0x91A
void OSound::ym_set_levels()
{
    // Clear YM & Drum Channels in RAM (0xF820 - 0xF9DF)
    for (uint16_t i = channel::YM1; i < channel::PCM_FX1; i++)
        chan_ram[i] = 0;

    // FM Sound Effects: Write fewer levels
    uint8_t entries = (chan_ram[channel::YM_FX1] & BIT_7) ? 28 : 32;
    uint16_t adr = YM_LEVEL_CMDS1;

    // Write Level Info
    for (uint8_t i = 0; i < entries; i++)
    {
        uint8_t reg = roms.z80.read8(&adr);
        uint8_t val = roms.z80.read8(&adr);
        fm_write_reg(reg, val);
    }
}

const static uint16_t FM_DATA_TABLE[] =
{
    DATA_BREEZE,
    DATA_SPLASH,
    0,
    DATA_COININ,
    DATA_MAGICAL,
    DATA_CHECKPOINT,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    DATA_SIGNAL1,
    DATA_SIGNAL2,
    0,
    0,
    0,
    DATA_BEEP1,
    DATA_UFO,
    DATA_BEEP2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    DATA_LASTWAVE,
};

// This is called first, when setting up YM Samples.
// The global 'pos' variable stores the location of the block table.
// Source: 0x515
void OSound::ym_set_block(uint8_t* chan)
{
    // Set block address
    chan[ch::FM_BLOCK] = roms.z80.read8(pos);
    
    if (!chan[ch::FM_BLOCK])
        return;

    uint16_t adr = ym_lookup_data(chan[ch::COMMAND], 3, chan[ch::FM_BLOCK]); // Use Routine 3.
    fm_write_block(chan[ch::FLAGS], adr, chan[ch::FM_FLAGS] & 7);
}

// Source: 0xAAA
uint16_t OSound::ym_lookup_data(uint8_t cmd, uint8_t offset, uint8_t block)
{
    block = (block - 1) << 1;
    
    // Address of data for FM routine
    uint16_t adr = roms.z80.read16((uint16_t) (FM_DATA_TABLE[cmd] + (offset << 1))); // ok
    return roms.z80.read16((uint16_t) (adr + block));
}

// "Connect" Channels To Play Out of Left/Right Speakers.
// Source: 0x534
void OSound::ym_set_connect(uint8_t* chan, uint8_t pan)
{
    uint8_t block = chan[ch::FM_BLOCK];                          // FM Routine To Choose from Data Block
    uint16_t adr = ym_lookup_data(chan[ch::COMMAND], 3, block);  // Send Block of FM Commands
    adr += 0x33;

    // c = Channel Control Register (0x20 - 0x27)
    uint8_t chan_ctrl_reg = (chan[ch::FM_FLAGS] & 7) + 0x20;

    // Register Value
    uint8_t reg_value = (roms.z80.read8(adr) & 0x3F) | pan;

    pos--;

    // Write Register
    fm_write_reg_c(chan[ch::FLAGS], chan_ctrl_reg, reg_value);
}

// iy = chan_ram
//
// Source: 0x4BF
void OSound::ym_finalize(uint8_t* chan)
{
    // Get channel number
    uint8_t chan_index = chan[ch::FM_FLAGS] & 7;

    // Write block of release commands
    fm_write_block(chan[ch::FLAGS], YM_RELEASE_RATE, chan_index);
    
    // Register: KEY ON Turns on and off output from each operator of each channel. (Disable in this case)
    fm_write_reg(0x8, chan_index);
    // Register: noise mode enable, noise period (Disable in this case)
    fm_write_reg(0xf, 0);

    chan[ch::FLAGS] = 0;

    // Check whether YM channel is also playing music
    int32_t subadr = 0xFADF - 0xFBC0;

    if (subadr >= 0)
    {
        // pop and return
        return;
    }
    
    *(chan -= 0x2C0); // = corresponding music channel

    // Return if no sound playing on corresponding channel
    if (!(chan[ch::FLAGS] & BIT_7))
        return;

    // ------------------------------------------------------------------------
    // FM Sound effect is playing & Music is playing simultaneously
    // ------------------------------------------------------------------------

    chan[ch::FLAGS] &= ~BIT_2;

    // Write remaining FM Data block, if specified
    uint8_t block = chan[ch::FM_BLOCK];

    if (!block)
        return;

    uint16_t adr = ym_lookup_data(chan[ch::COMMAND], 3, block);
    fm_write_block(chan[ch::FLAGS], adr, chan[ch::FM_FLAGS] & 7);
}

// Use Phase and Amplitude Modulation Sensitivity Table
// Enabled with FM_PHASETBL flag.
// Source: 0x1D6
void OSound::read_mod_table(uint8_t* chan)
{
    uint16_t adr = ym_lookup_data(chan[ch::COMMAND], 2, chan[ch::FM_PHASETBL]); // Use Routine 2.

    while (true)
    {
        uint16_t offset = chan[ch::FM_PHASEOFF];
        uint8_t table_entry = roms.z80.read8((uint16_t) (adr + offset));

        // Reset table position
        if (table_entry == 0xFD)
            chan[ch::FM_PHASEOFF] = 0;
        // Decrement table position
        else if (table_entry == 0xFE)
            chan[ch::FM_PHASEOFF]--;
        // Unused special case
        else if (table_entry == 0xFC)
        {
            #ifdef UNUSED_WARNINGS
            // Missing code here
            std::cout<< "read_mod_table: table_entry 0xFC not supported" << std::endl;
            #endif
        }
        // Increment table position
        else
        {
            chan[ch::FM_PHASEOFF]++;
            uint8_t carry = (table_entry < 0xFC) ? 2 : 0;
            // rotate table_entry left through 9-bits twice
            chan[ch::FM_PHASE_AMP] = ((table_entry << 2) + carry) + ((table_entry & 0x80) >> 7);
            return;
        }
    }
}

// 1/ bc = Read next value in sequence (from address in de) 
// 2/ Store value on stack
// 3/ Decrement internal pointer 
// 4/ hl =  Address in block + bc
// 5/ Increment and store next 'de' value in sequence within 0x20 block
// Source: 0x418
void OSound::write_seq_adr(uint8_t* chan)
{
    uint16_t value = roms.z80.read16(pos);
    pos++;

    chan[ch::MEM_OFFSET]--;
    uint8_t offset = chan[ch::MEM_OFFSET];
    chan[ch::MEM_OFFSET]--;

    chan[offset]   = pos >> 8;
    chan[offset-1] = pos & 0xFF;

    pos = value - 1;
}