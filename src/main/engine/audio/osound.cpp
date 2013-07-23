/***************************************************************************
    Ported Z80 Sound Playing Code.
    Controls Sega PCM and YM2151 Chips.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

/*
TODO:

- Double check passing car tones, especially when going through checkpoint areas
X Finish Ferrari sound code
- Engine tones seem to jump channels. How do we stop this? Is it a bug in the original?
X More cars seem to be high pitched than on MAME. (Fixed - engine channel setup)

*/

//#include <iostream> // needed for debugging only. Can be removed.
#include <cstring> // For memset on GCC
#include "osound.hpp"

// Use YM2151 Timing
#define TIMER_CODE 1

// Enable Unused code block warnings
//#define UNUSED_WARNINGS 1

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

    engine_counter= 0;

    // Clear AM RAM 0xF800 - 0xFFFF
    for (uint16_t i = 0; i < CHAN_RAM_SIZE; i++)
        chan_ram[i] = 0;

    // Enable all PCM channels by default
    for (int8_t i = 0; i < 16; i++)
        pcm_ram[0x86 + (i * 8)] = 1; // Channel Active

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
    fm_dotimera();          // FM: Process Timer A. Stop Timer B
    process_command();      // Process Command sent by main program code (originally the main 68k processor)
    process_channels();     // Run logic on individual sound channel (both YM & PCM channels)
    engine_process();       // Ferrari Engine Tone & Traffic Noise
    traffic_process();      // Traffic Volume/Panning & Pitch
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
    if (command_input == sound::RESET)
        return;
    // Clear Z80 Command
    else if (command_input < 0x80 || command_input >= 0xFF)
    {
        // Reset FM Chip
        if (command_input == sound::FM_RESET || command_input == 0xFF)
            fm_reset();

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
                sound_props |= BIT_0; // Trigger rev effect
                init_sound(cmd, DATA_BREEZE, channel::YM1);
                break;

            case sound::MUSIC_SPLASH:
                fm_reset();
                sound_props |= BIT_0; // Trigger rev effect
                init_sound(cmd, DATA_SPLASH, channel::YM1);
                break;

            case sound::COIN_IN:
                init_sound(cmd, DATA_COININ, channel::YM_FX1);
                break;

            case sound::MUSIC_MAGICAL:
                fm_reset();
                sound_props |= BIT_0; // Trigger rev effect
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
                pcm_w(0xF08E, 1); // Set inactive flag on channels
                pcm_w(0xF09E, 1);
                break;

            case sound::CRASH1:
                init_sound(cmd, DATA_CRASH1, channel::PCM_FX5);
                break;

            case sound::REBOUND:
                init_sound(cmd, DATA_REBOUND, channel::PCM_FX5);
                break;

            case sound::CRASH2:
                init_sound(cmd, DATA_CRASH2, channel::PCM_FX5);
                break;

            case sound::NEW_COMMAND:
                new_command();
                break;

            case sound::SIGNAL1:
                init_sound(cmd, DATA_SIGNAL1, channel::YM_FX1);
                break;

            case sound::SIGNAL2:
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

            case sound::REVS:
                fm_reset();
                sound_props |= BIT_0; // Trigger rev effect
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
    for (uint8_t c = 0; c < 8; c++)
    {
        // Map back to corresponding music channel
        if (chan_ram[chan_id] & BIT_7)    
            chan_ram[chan_id - 0x2C0] |= BIT_2;
        chan_id += CHAN_SIZE;
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

        chan_id += CHAN_SIZE; // Advance to next channel in memory
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
    chanid_prev = chan_id;

    // Get correct offset in RAM
    uint8_t* chan = &chan_ram[chan_id];

    // Increment sequence position
    pos = r16(&chan[ch::SEQ_POS]) + 1;
    w16(&chan[ch::SEQ_POS], pos);

    // Sequence end marker
    uint16_t seq_end = r16(&chan[ch::SEQ_END]);

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
  
    w16(&chan[ch::SEQ_END], end_marker); // Set End Marker    
    w16(&chan[ch::SEQ_CMD], ++pos);      // Set Next Sequence Command
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

    // PCM Percussion Sample
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
    uint16_t dst_backup = dst;

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
        for (int i = 0xF; i < CHAN_SIZE; i++)
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
        const uint16_t CHAN_SIZE = 0x10;  // Size of each channel entry (2 Channel Increment)

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
    uint16_t adr = roms.z80.read16((uint16_t) (FM_DATA_TABLE[cmd] + (offset << 1)));
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
    if (chanid_prev < channel::MAP1)
    {
        // pop and return
        return;
    }
    
    // HACKED OUT
    *chan -= 0x2C0; // = corresponding music channel

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

// ----------------------------------------------------------------------------
//                                ENGINE TONE CODE 
// ----------------------------------------------------------------------------

// Process Ferrari Engine Tone & Traffic Sound Effects
//
// Original addresses used: 
// 0xFC00: Engine Channel - Player's Car
// 0xFC20: Engine Channel - Traffic 1
// 0xFC40: Engine Channel - Traffic 2
// 0xFC60: Engine Channel - Traffic 3
// 0xFC80: Engine Channel - Traffic 4
//
// counter = Increments every tick (0 - 0xFF)
//
// Source: 0x7501
void OSound::engine_process()
{
    // DEBUG
    // bpset 7501,1,{b@0xf801 = 0xe; b@0xf802 = 0xf1; b@f803 = 0x3f;}
    // engine_data[sound::ENGINE_PITCH_H] = 0xE;
    // engine_data[sound::ENGINE_PITCH_L] = 0xF1;
    // engine_data[sound::ENGINE_VOL]     = 0x3F;
    // END DEBUG

    // Return 1 in 2 times when this routine is called
    if ((++engine_counter & 1) == 0)
        return;

    uint16_t ix = 0;                    // PCM Channel RAM Address
    uint16_t iy = channel::ENGINE_CH1;  // Internal Channel RAM Address

    for (engine_channel = 6; engine_channel > 0; engine_channel--)
    {
        engine_process_chan(&chan_ram[iy], &pcm_ram[ix]);
        ix += 0x10;
        iy += CHAN_SIZE;
    }
}

// Source: 0x7531
void OSound::engine_process_chan(uint8_t* chan, uint8_t* pcm)
{
    // Return if PCM Sample Being Played On Channel
    if (engine_channel < 3)
    {
        if (sound_props & BIT_1)
            return;
    }

    // Read Engine Data that has been sent by 68K CPU
    engine_read_data(chan, pcm);

    // ------------------------------------------------------------------------
    // Car is stationary, do rev effect at start line.
    // This bit is set whenever the music start is triggered
    // ------------------------------------------------------------------------
    if (sound_props & BIT_0)
    {
        // 0x7663
        uint16_t revs = r16(pcm); // Read Revs/Pitch which has just been stored by engine_read_data

        // No revs, mute engine channel and get out of here
        if (revs == 0)
        {
            engine_mute_channel(chan, pcm);
            return;
        }
        // 0x766E
        // Do High Pitched Rev Sound When Car Is At Starting Line
        // Set Volume & Pitch, Then Return
        if (revs >= 0xFA)
        {
            // 0x7682
            // Return if channel already active
            if (!(pcm[0x86] & BIT_0))
                return;

            if (engine_channel < 3)
            {
                pcm[2] = 0x20; // l
                pcm[3] = 0;    // r
                pcm[7] = 0x41; // pitch
            }
            else if (engine_channel < 5)
            {
                pcm[2] = 0x10; // l
                pcm[3] = 0x10; // r
                pcm[7] = 0x42; // pitch
            }
            else
            {
                pcm[2] = 0x20; // l
                pcm[3] = 0;    // r
                pcm[7] = 0x40; // pitch
            }

            pcm[4] = pcm[0x84] = 0;    // start/loop low
            pcm[5] = pcm[0x85] = 0x36; // start/loop high
            pcm[6] = 0x55;             // end address high
            pcm[0x86] = 2;             // Flags: Enable loop, set active

            return;
        }
        // Some code relating to 0xFD08 that I don't think is used
    }
    // 0x754A
    if (engine_channel & BIT_0)
    {
        unk78c7(chan, pcm);
    }

    // Check engine volume and mute channel if disabled
    if (!chan[ch_engines::VOL0])
    {
        engine_mute_channel(chan, pcm);
        return;
    }

    // Set Engine Volume
    if (chan[ch_engines::VOL0] == chan[ch_engines::VOL1])
    {
        // Denote volume already set
        chan[ch_engines::FLAGS] |= BIT_1; 
    }
    else
    {
        chan[ch_engines::FLAGS] &= ~BIT_1; 
        chan[ch_engines::VOL1] = chan[ch_engines::VOL0];
    }

    // 0x755C
    // Check we have some revs
    uint16_t revs = r16(pcm);
    if (revs == 0)
    {
        engine_mute_channel(chan, pcm);
        return;
    }

    // Rev Change Setup
    // 0x774E routine rolled in here
    uint16_t old_revs = r16(pcm + 0x80);

    // Revs Unchanged
    if (revs == old_revs)
    {
        chan[ch_engines::FLAGS] |= BIT_0; // denotes start address / end address has been set
    }
    // Revs Changed
    else 
    {
        if (revs - old_revs < 0)
            chan[ch_engines::FLAGS] &= ~BIT_2; // loop enabled
        else
            chan[ch_engines::FLAGS] |= BIT_2;  // loop disabled

        chan[ch_engines::FLAGS] &= ~BIT_0;     // Start end address not set
        w16(pcm + 0x80, revs);                 // Write new revs value
    }

    // 0x756A
    chan[ch_engines::ACTIVE] &= ~BIT_0; // Mute

    // Return if addresses have already been set
    if ((chan[ch_engines::FLAGS] & BIT_0) && chan[ch_engines::FLAGS] & BIT_1)
        return;

    // 0x757A
    // PLAYER'S CAR
    if (engine_channel >= 5)
    {
        int16_t off = r16(pcm) - 0x30;

        if (off >= 0)
        {
            if (chan[ch_engines::FLAGS] & BIT_4)
            {
                chan[ch_engines::FLAGS] &= ~BIT_3; // Loop Address Not Set
                chan[ch_engines::FLAGS] &= ~BIT_4;
            }

            ferrari_vol_pan(chan, pcm);
            return;
        }
        else
        {
            if (!(chan[ch_engines::FLAGS] & BIT_4))
            {
                chan[ch_engines::FLAGS] &= ~BIT_3; // Loop Address Not Set
                chan[ch_engines::FLAGS] |=  BIT_4;
            }
        }
    }
    // 0x75B2
    uint16_t engine_pos = engine_get_table_adr(chan, pcm); // hl
    
    // Mute Engine Channel
    if (chan[ch_engines::FLAGS] & BIT_5)
    {
        engine_mute_channel(chan, pcm, false);
        return;
    }

    // 0x75bc Has Start Address Been Set Already?
    // Used on start line
    if (chan[ch_engines::FLAGS] & BIT_0)
    {
        engine_pos += 2;
    }
    else
    {
        uint16_t start_adr = engine_set_adr(engine_pos, chan, pcm); // Set Start Address
        engine_set_adr_end(engine_pos, start_adr, chan, pcm);       // Set End Address
    }

    vol_thicken(engine_pos, chan, pcm); // Thicken engine effect by panning left/right dependent on channel.
    engine_set_pitch(engine_pos, pcm);  // Set Engine Pitch from lookup table specified by hl
    pcm[0x86] = 0;                      // Set Active & Loop Enabled
}

// Only called for odd number channels
// I've not really worked out what this does yet
// Source: 0x78C7
void OSound::unk78c7(uint8_t* chan, uint8_t* pcm)
{
    uint16_t adr; // Channel address in RAM

    if (engine_channel == 1)
    {
        adr = 0xFD10;
    }
    else if (engine_channel == 3)
    {
        adr = 0xFD30;
    }
    else
    {
        adr = 0xFD50;
    }

    // STORE: Calculate offset into Channel Block To Store Data At
    uint16_t adr_offset = adr + (chan[ch_engines::OFFSET] * 3);
    adr_offset &= 0x7FF;
    chan_ram[adr_offset++] = pcm[0x0];               // Copy Engine Pitch Low
    chan_ram[adr_offset++] = pcm[0x1];               // Copy Engine Pitch High
    chan_ram[adr_offset++] = chan[ch_engines::VOL0]; // Copy Engine Volume

    // Wrap around block of three entries
    if (++chan[ch_engines::OFFSET] >= 8)
    {
        chan[ch_engines::OFFSET] = 0;
        adr_offset = adr & 0x7FF;
    }

    // RESTORE: 7915
    pcm[0x0] = chan_ram[adr_offset++];
    pcm[0x1] = chan_ram[adr_offset++];
    chan[ch_engines::VOL0] = chan_ram[adr_offset++];
}

// Source: 0x75DA
void OSound::ferrari_vol_pan(uint8_t* chan, uint8_t* pcm)
{
    // Adjust Engine Volume and write to new memory area (0x6)
    engine_adjust_volume(chan);

    // Set Pitch Table Details
    int16_t pitch_table_index = r16(pcm + 0x80) - 0x30;

    if (pitch_table_index < 0)
    {
        engine_mute_channel(chan, pcm, false);
        return;
    }

    w16(chan + ch_engines::PITCH_L, pitch_table_index);

    // Set PCM Sample Addresses
    uint16_t pos = ENGINE_ADR_TABLE;
    engine_set_adr(pos, chan, pcm);

    // Set PCM Sample End Address
    pcm[0x6] = roms.z80.read8(++pos);

    // Set Volume Pan
    engine_set_pan(pos, chan, pcm);

    // Set Pitch
    pos = ENGINE_ADR_TABLE + 4; // Set position to pitch offset
    uint16_t pitch = roms.z80.read8(pos); // bc
    pitch += r16(chan + ch_engines::PITCH_L) >> 1;
    if (pitch > 0xFF) pitch = 0xFF;

    // Tweak pitch slightly based on channel id
    if (engine_channel & BIT_0)
        pitch -= 2;

    pcm[0x7] = (uint8_t) pitch;
    pcm[0x86] = 0x10; // Set channel active and enabled
}

// Set Table Index For Engine Sample Start / End Addresses
// Table starts at ENGINE_ADR_TABLE offset in ROM.
// Source: 0x7819
uint16_t OSound::engine_get_table_adr(uint8_t* chan, uint8_t* pcm)
{
    int16_t off = r16(pcm + 0x80) - 0x52;
    int16_t table_offset;

    if (off < 0)
    {
        chan[ch_engines::FLAGS] &= ~BIT_5; // Unmute Engine Sounds
        table_offset = r16(pcm + 0x80);
        w16(pcm + 0x82, 0);
    }
    else
    {
        chan[ch_engines::FLAGS] |= BIT_5; // Mute Engine Sounds
        table_offset = 1;
        w16(pcm + 0x82, off);
    }
    // get_adr:
    table_offset--;

    const static uint8_t ENTRY = 5; // bytes per entry
    return (ENGINE_ADR_TABLE + ENTRY) + (table_offset * ENTRY); // table has 54 entries
}

// Setup engine addresses from table (START, LOOP)
// Source: 0x77AD

//bpset 77b0,1,{printf "start adr:%02x pos:=%02x",bc, hl; g}
uint16_t OSound::engine_set_adr(uint16_t& pos, uint8_t* chan, uint8_t* pcm)
{
    uint16_t start_adr = roms.z80.read16(pos++);
    w16(pcm + 0x4, start_adr); // Set Wave Start Address

    // TRAFFIC
    if (engine_channel < 5)
    {
        if (chan[ch_engines::FLAGS] & BIT_5) // Mute
        {
            if (chan[ch_engines::FLAGS] & BIT_6)
            {
                chan[ch_engines::FLAGS] |= BIT_6;               
                chan[ch_engines::FLAGS] |= BIT_3; // Denote loop address set                
                w16(pcm + 0x84, start_adr);       // Set default loop address to start address
                return start_adr;
            }
        }
        else
        {
            chan[ch_engines::FLAGS] &= ~BIT_6;
        }
    }

    // Return if loop address already set
    if (chan[ch_engines::FLAGS] & BIT_3)
        return start_adr;

    chan[ch_engines::FLAGS] |= BIT_3; // Denote loop address set  
    w16(pcm + 0x84, start_adr);       // Set default loop address to start address
    return start_adr;
}

// Source: 0x7853
void OSound::engine_set_adr_end(uint16_t& pos, uint16_t loop_adr, uint8_t* chan, uint8_t* pcm)
{
    // Set wave end address from table
    pcm[0x6] = roms.z80.read8(++pos);

    // Loop Disabled
    if (chan[ch_engines::FLAGS] & BIT_2)
        return;

    // Loop Address >= End Address
    if (pcm[0x6] >= pcm[0x85])
        return;

    // Set loop address
    w16(pcm + 0x84, loop_adr);
}

// Thicken engine effect by panning left/right dependent on channel.
// Source: 0x77EA
void OSound::vol_thicken(uint16_t& pos, uint8_t* chan, uint8_t* pcm)
{
    pos++; // Address of volume multiplier

    // Odd Channels: Pan Left
    if (engine_channel & BIT_0)
    {
        pcm[0x2] = pcm[0x82] & BIT_5 ? 0 : get_adjusted_vol(pos, chan); // left (if enabled)
        pcm[0x3] = 0; // right
    }
    // Even Channels: Pan Right
    else
    {
        pcm[0x2] = 0; // left
        pcm[0x3] = pcm[0x83] & BIT_5 ? 0 : get_adjusted_vol(pos, chan); // right (if enabled)
    }
}

// Get Adjusted Volume
// Source: 0x78A7
uint8_t OSound::get_adjusted_vol(uint16_t& pos, uint8_t* chan)
{
    uint8_t multiply =  roms.z80.read8(pos);
    uint16_t vol = (chan[ch_engines::VOL1] * multiply) >> 6;

    if (vol > 0x3F)
        vol = 0x3F;

    return (uint8_t) vol;
}

// bpset 7877,1,{printf "bc=%02x hl=%02x",bc, hl; g}
// Set Engine Pitch From Table
// Source: 0x7870
void OSound::engine_set_pitch(uint16_t& pos, uint8_t* pcm)
{
    pos++; // Increment to pitch entry in table

    uint16_t bc = r16(pcm + 0x82);
    bc >>= 2;

    if (bc & 0xFF00)
        bc = (bc & 0xFF00) | 0xFF;

    uint16_t pitch = roms.z80.read8(pos);

    //std::cout << std::hex << pos << std::endl;

    // Read pitch from table
    if (bc)
    {
        pitch += (bc & 0xFF);
        if (pitch > 0xFF)
            pitch = 0xFC;
    }

    // Adjust the pitch slightly dependent on the channel selected
    if (engine_channel & BIT_0)
        pcm[0x7] = (uint8_t) pitch;
    else
        pcm[0x7] = (uint8_t) pitch + 3;
}

// Mute an engine channel
// Source: 0x7639
void OSound::engine_mute_channel(uint8_t* chan, uint8_t* pcm, bool do_check)
{
    // Return if already muted
    if (do_check && (chan[ch_engines::ACTIVE] & BIT_0))
        return;

    // Denote channel muted
    chan[ch_engines::ACTIVE] |= BIT_0;

    pcm[0x02] = 0;      // Clear volume left
    pcm[0x03] = 0;      // Clear volume right
    pcm[0x07] = 0;      // Clear pitch
    pcm[0x86] |= BIT_0; // Denote not active

    // Clear some stuff
    chan[ch_engines::VOL0]    = 0;
    chan[ch_engines::VOL1]    = 0;
    chan[ch_engines::FLAGS]   = 0;
    chan[ch_engines::PITCH_L] = 0;
    chan[ch_engines::PITCH_H] = 0;
    chan[ch_engines::VOL6]    = 0;
}

// Adjust engine volume and write to new memory area
// Source: 0x76D7
void OSound::engine_adjust_volume(uint8_t* chan)
{
    uint16_t vol = (chan[ch_engines::VOL1] * 0x18) >> 6;

    if (vol > 0x3F)
        vol = 0x3F;

    chan[ch_engines::VOL6] = (uint8_t) vol;
}   

// Set engine pan. 
// Adjust Volume and write to new memory area
// Also write to ix (PCM Channel RAM)
// Source: 0x76FD
void OSound::engine_set_pan(uint16_t& pos, uint8_t* chan, uint8_t* pcm)
{
    uint16_t pitch = r16(chan + ch_engines::PITCH_L) >> 1;
    pitch += roms.z80.read8(++pos);

    uint16_t vol = (chan[ch_engines::VOL1] * pitch) >> 6;

    if (vol > 0x3F)
        vol = 0x3F;

    if (vol >= chan[ch_engines::VOL6])
        vol = chan[ch_engines::VOL6];

    // Pan Left
    if (engine_channel & BIT_0)
    {
        pcm[0x2] = (uint8_t) vol;      // left
        pcm[0x3] = (uint8_t) vol >> 1; // right
    }
    // Pan Right
    else
    {
        pcm[0x2] = (uint8_t) vol >> 1; // left
        pcm[0x3] = (uint8_t) vol;      // right;
    }
}

// Read Engine Data & Store the engine pitch and volume to PCM Channel RAM
// Source: 0x778D
void OSound::engine_read_data(uint8_t* chan, uint8_t* pcm)
{
    uint16_t pitch = (engine_data[sound::ENGINE_PITCH_H] << 8) + engine_data[sound::ENGINE_PITCH_L];

    pitch = (pitch >> 5) & 0x1FF;
    
    // Store pitch in scratch space of channel (due to mirroring this wraps round to 0x00 in the channel)
    pcm[0x0] = pitch & 0xFF;
    pcm[0x1] = (pitch >> 8) & 0xFF;
    chan[ch_engines::VOL0] = engine_data[sound::ENGINE_VOL];
}

// ----------------------------------------------------------------------------
//                               PASSING TRAFFIC FX 
// ----------------------------------------------------------------------------

// should be 0x9b 0x9b 0xe3 0xe3 for starting traffic

// Process Passing Traffic Channels
// Source: 0x7AFB
void OSound::traffic_process()
{
    if ((engine_counter & 1) == 0)
        return;

    uint16_t pcm_adr = 0x60; // Channel 13: PCM Channel RAM Address

    // Iterate PCM Channels 13 to 16
    for (engine_channel = 4; engine_channel > 0; engine_channel--)
    {
        traffic_process_chan(&pcm_ram[pcm_adr]);
        pcm_adr += 0x8; // Advance to next channel
    }

}

// Process Single Channel Of Traffic Sounds
// Source: 0x7B1F
void OSound::traffic_process_chan(uint8_t* pcm)
{
    // No slide/pitch reduction applied yet
    if (!(pcm[0x82] & BIT_4))
    {
        traffic_read_data(pcm); // Read Traffic Data that has been sent by 68K CPU
        
        uint8_t vol = pcm[0x00];
        
        // vol on
        if (vol)
        {
            traffic_note_changes(vol, pcm); // Record changes to traffic volume and panning
            uint8_t flags = pcm[0x82];

            // Change in Volume or Panning: Set volume, panning & pitch based on distance of traffic.
            if (!(flags & BIT_0) || !(flags & BIT_1))
            {
                traffic_process_entry(pcm);
                return;
            }
            // Return if start/end position of wave is already setup
            else if (flags & BIT_2)
                return;

            // Set volume, panning & pitch based on distance of traffic.
            traffic_process_entry(pcm); 
            return;
        }
        // vol off: decrease pitch
        else
        {
            // Check whether to instantly disable channel
            if (!(pcm[0x82] & BIT_3))
            {
                traffic_disable(pcm);
                return;
            }

            pcm[0x82] |= BIT_4; // Denote pitch reduction

            // Adjust pitch
            if (pcm[0x07] < 0x81)
                pcm[0x07] -= 4;
            else
                pcm[0x07] -= 6;
        }
    }

    // Reduce Volume Right Channel
    if (pcm[0x03])
        pcm[0x03]--;

    // Reduce Volume Left Channel
    if (pcm[0x02])
        pcm[0x02]--;

    // Once both channels have been reduced to zero, disable the sample completely
    if (!pcm[0x02] && !pcm[0x03])
        traffic_disable(pcm);
}

const uint8_t TRAFFIC_PITCH_H[] = { 0, 2, 4, 4, 0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8 };

// Process traffic entry. Set volume, panning & pitch based on distance of traffic.
// Source: 0x7B82
void OSound::traffic_process_entry(uint8_t* pcm)
{
    // Wave Start/End Address has not been setup yet
    if (!(pcm[0x82] & BIT_2))
    {
        pcm[0x82] |= BIT_2;           // Denote set       
        pcm[0x04] = pcm[0x84] = 0x82; // Set Wave Start & Loop Addresses (Word)
        pcm[0x05] = pcm[0x85] = 0x00;
        pcm[0x06] = 0x6;              // Set Wave End    
    }
    // do_pan_vol
    traffic_set_vol(pcm); // Set Traffic Volume Multiplier
    traffic_set_pan(pcm); // Set Traffic Volume / Panning on each channel

    int8_t vol_boost = pcm[0x80] - 0x16;  
    uint8_t pitch = 0;

    if (vol_boost >= 0)
        pitch = TRAFFIC_PITCH_H[vol_boost];

    pitch += (engine_channel & 1) ? 0x60 : 0x80;
    
    // set_pitch2:
    pcm[0x07] = pitch; // Set Pitch
    pcm[0x86] = 0x10;  // Set Active & Enabled

    //std::cout << std::hex << (uint16_t) pitch << std::endl;
}

// Disable Traffic PCM Channel
// Source: 0x7BDC
void OSound::traffic_disable(uint8_t* pcm)
{
    pcm[0x86] |= BIT_0; // Disable sound
    pcm[0x82] = 0;      // Clear Flags
    pcm[0x02] = 0;      // Clear Volume Left
    pcm[0x03] = 0;      // Clear Volume Right
    pcm[0x07] = 0;      // Clear Delta (Pitch)
    pcm[0x00] = 0;      // Clear New Vol Index
    pcm[0x80] = 0;      // Clear Old Vol Index
    pcm[0x01] = 0;      // Clear New Pan Index
    pcm[0x81] = 0;      // Clear Old Pan Index
}

// Set Traffic Volume Multiplier
// Source: 0x7C28
void OSound::traffic_set_vol(uint8_t* pcm)
{
    // Return if volume index is not set
    uint8_t vol_entry = pcm[0x80];

    if (!vol_entry)
        return;

    uint16_t multiply = TRAFFIC_VOL_MULTIPLY + vol_entry - 1;

    // Set traffic volume multiplier
    pcm[0x83] = roms.z80.read8(multiply);

    if (pcm[0x83] < 0x10)
        pcm[0x82] &= ~BIT_3; // Disable Traffic Sound
    else
        pcm[0x82] |= BIT_3;  // Enable Traffic Sound
}

// Traffic Panning Indexes are as follows:
// 0 = Both
// 1 = Pan Left
// 2 = Pan Left More
// 3 = Hard Left Pan
//
// 5 = Both
// 6 = Hard Pan Right
// 7 = Pan Right More
// 8 = Pan Right

const uint8_t TRAFFIC_PANNING[] = 
{ 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x08, 0x0D, // Right Channel
    0x10, 0x0D, 0x08, 0x00, 0x10, 0x10, 0x10, 0x10  // Left Channel
};

// Set Traffic Panning On Channel From Table
// Source: 0x7BFA
void OSound::traffic_set_pan(uint8_t* pcm)
{
    pcm[0x03] = traffic_get_vol(pcm[0x81] + 0, pcm); // Set Volume Right
    pcm[0x02] = traffic_get_vol(pcm[0x81] + 8, pcm); // Set Volume Left
}

// Read Traffic Volume Value from Table And Multiply Appropriately
// Source: 0x7C16
uint8_t OSound::traffic_get_vol(uint16_t pos, uint8_t* pcm)
{
    // return volume from table * multiplier
    return (TRAFFIC_PANNING[pos] * pcm[0x83]) >> 4;
}

// Has Traffic Volume or Pitch changed?
// Set relevant flags when it has to denote the fact.
// Source: 0x7C48
void OSound::traffic_note_changes(uint8_t new_vol, uint8_t* pcm)
{
    // Denote no volume entry change
    if (new_vol == pcm[0x80])
        pcm[0x82] |= BIT_0;
    // Record entry change
    else
    {
        pcm[0x82] &= ~BIT_0;
        pcm[0x80] = new_vol;
    }

    // Denote no pan entry change
    if (pcm[0x01] == pcm[0x81])
        pcm[0x82] |= BIT_1;
    else
    {
        pcm[0x82] &= ~BIT_1;
        pcm[0x81] = pcm[0x01];
    }
}

// Read Traffic Data that has been sent by 68K CPU
void OSound::traffic_read_data(uint8_t* pcm)
{
    // Get volume of traffic for channel
    uint8_t vol = engine_data[sound::ENGINE_VOL + engine_channel];
    //std::cout << std::hex << "ch: " << (int16_t) engine_channel << " vol: " << (int16_t) vol << std::endl;    
    pcm[0x01] = vol & 7;  // Put bottom 3 bits in 01    (pan entry)
    pcm[0x00] = vol >> 3; // And remaining 5 bits in 00 (used as vol entry)
}
