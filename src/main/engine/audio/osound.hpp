#pragma once

#include "stdint.hpp"
#include "globals.hpp"
#include "roms.hpp"

#include "hwaudio/ym2151.hpp"

#include "engine/audio/commands.hpp"
#include "engine/audio/osoundadr.hpp"

// PCM Sample Indexes
namespace pcm_sample
{
    enum
    {
        CRASH1  = 0xD0, // 0xD0 - Crash 1
        GURGLE  = 0xD1, // 0xD1 - Gurgle
        SLIP    = 0xD2, // 0xD2 - Slip
        CRASH2  = 0xD3, // 0xD3 - Crash 2
        CRASH3  = 0xD4, // 0xD4 - Crash 3
        SKID    = 0xD5, // 0xD5 - Skid
        REBOUND = 0xD6, // 0xD6 - Rebound
        HORN    = 0xD7, // 0xD7 - Horn
        TYRES   = 0xD8, // 0xD8 - Tyre Squeal
        SAFETY  = 0xD9, // 0xD9 - Safety Zone
        LOSKID  = 0xDA, // 0xDA - Lofi skid (is this used)
        CHEERS  = 0xDB, // 0xDB - Cheers
        VOICE1  = 0xDC, // 0xDC - Voice 1, Checkpoint
        VOICE2  = 0xDD, // 0xDD - Voice 2, Congratulations
        VOICE3  = 0xDE, // 0xDE - Voice 3, Get Ready
        VOICE4  = 0xDF, // 0xDF - Voice 4, You're doing great (unused, plays at wrong pitch)
        WAVE    = 0xE0, // 0xE0 - Wave
        CRASH4  = 0xE1, // 0xE1 - Crash 4
    };
};

// Internal Channel Offsets in RAM
namespace channel
{
    // Channels 0-7: YM Channels
    const static uint16_t YM1 = 0x020; // f820
    const static uint16_t YM2 = 0x040;
    const static uint16_t YM3 = 0x060;
    const static uint16_t YM4 = 0x080;
    const static uint16_t YM5 = 0x0A0;
    const static uint16_t YM6 = 0x0C0;
    const static uint16_t YM7 = 0x0E0;
    const static uint16_t YM8 = 0x100; // f900

    // Channels 8-13: PCM Drum Channels for music
    const static uint16_t PCM_DRUM1 = 0x120;
    const static uint16_t PCM_DRUM2 = 0x140;
    const static uint16_t PCM_DRUM3 = 0x160;
    const static uint16_t PCM_DRUM4 = 0x180;
    const static uint16_t PCM_DRUM5 = 0x1A0;
    const static uint16_t PCM_DRUM6 = 0x1C0;

    // Channels 14-21: PCM Sound Effects
    const static uint16_t PCM_FX1 = 0x1E0; // f9e0: Crowd, Cheers, Wave
    const static uint16_t PCM_FX2 = 0x200;
    const static uint16_t PCM_FX3 = 0x220; // fa20: Slip, Safety Zone
    const static uint16_t PCM_FX4 = 0x240;
    const static uint16_t PCM_FX5 = 0x260; // fa60: Crash 1, Rebound, Crash2
    const static uint16_t PCM_FX6 = 0x280;
    const static uint16_t PCM_FX7 = 0x2A0; // faa0: Voices
    const static uint16_t PCM_FX8 = 0x2C0;

    // Channel Mapping Info. Used to play sound effects and music at the same time. 
    const static uint16_t MAP1 = 0x2E0;
    const static uint16_t MAP2 = 0x300;
    const static uint16_t MAP3 = 0x320;
    const static uint16_t MAP4 = 0x340;
    const static uint16_t MAP5 = 0x360;
    const static uint16_t MAP6 = 0x380;
    const static uint16_t MAP7 = 0x3A0;

    // Channels 22-23: YM Sound Effects
    const static uint16_t YM_FX1 = 0x3C0; // fbc0: Signal 1, Signal 2
    const static uint16_t YM_FX2 = 0x3E0;

    // Engine Commands in RAM
    const static int16_t ENGINE_CH1 = 0x400; // 0xFC00: Engine Channel - Player's Car
    const static int16_t ENGINE_CH2 = 0x420; // 0xFC20: Engine Channel - Traffic 1
    const static int16_t ENGINE_CH3 = 0x440; // 0xFC40: Engine Channel - Traffic 2
    const static int16_t ENGINE_CH4 = 0x460; // 0xFC60: Engine Channel - Traffic 3
    const static int16_t ENGINE_CH5 = 0x480; // 0xFC80: Engine Channel - Traffic 4

};

// ------------------------------------------------------------------------------------------------
// Internal Format of Sound Data in RAM before sending to hardware
// ------------------------------------------------------------------------------------------------
//
//0x20 byte chunks of information per channel in memory. Format is as follows:
//
//+0x00: [Byte] Flags e65--cn-
//              n = FM noise channel (1 = yes, 0 = no)
//              c = Corresponding music channel is enabled
//              5 = Pitch Bend (Only used by Step On Beat track, not the standard music)
//              6 = ???
//              e = channel enable (1 = active, 0 = disabled). 
//+0x01: [Byte] Flags -m---ccc
//	            c = YM Channel Number
//              m = possibly a channel mute? 
//                  Counters and positions still tick.  (1 = active, 0 = disabled).
//+0x02: [Byte] Used as end marker when bit 1 of 0x0D is set
//+0x03: [Word] Position in sequence
//+0x05: [Word] Sequence End Marker
//+0x07: [Word] Address of next command (see 0x2E7)
//              This is essentially within the same block of sound information
//+0x09: [Byte] Note Offset: From lowest note. Essentially an index into a table.
//+0x0A: [Byte] Offset into 0x20 block of memory. Used to store positioning info.
//+0x0B: [Byte] Use Phase and Amplitude Modulation Sensitivity Table
//+0x0C: [Byte] FM: Select FM Data Block To Send. 0 = No Block.
//+0x0D: [Byte] FM: End Marker Flags
//              Flags ------10
//              0 = Set high byte of end marker from data.
//              1 = Do not calculate end marker. Use value from data.
//+0x0E: [Byte] Sample Index / Command
//+0x0F: [Word] ?
//+0x10: [Byte] Offset into Phase and Amplitude Modulation Sensitivity Table (see 0x1DF)
//+0x11: [Byte] Volume: Left Channel
//+0x12: [Byte] Volume: Right Channel
//+0x13: [Word] PCM: Wave Start Address / Loop Address 
//              FM:  Note & Octave Info (top bit denotes noise channel?)
//+0x14: [Byte] FM Channels only. Phase and Amplitude Modulation Sensitivity
//+0x15: [Byte] Wave End Address HIGH 8 bits
//+0x16: [Byte] PCM Pitch
//+0x17: [Byte] Flags m-bbccla
//              a = active (0 = active,  1 = inactive) 
//              l = loop   (0 = enabled, 1 = disabled)
//              c = channel pair select
//              b = bank
//              m = Music Sample (Drums etc.)
//+0x18: [Byte] FM Loop Counter. Specifies number of times to trigger command sequence. 
//              Counter used at 0x45f
//
//+0x1C: [Word] Sequence Address #1
//+0x1E: [Word] Sequence Address #2

namespace ch
{
    enum
    {
        FLAGS       = 0x00,
        FM_FLAGS    = 0x01,
        END_MARKER  = 0x02,
        SEQ_POS     = 0x03,
        SEQ_END     = 0x05,
        SEQ_CMD     = 0x07,
        NOTE_OFFSET = 0x09,
        MEM_OFFSET  = 0x0A,
        FM_PHASETBL = 0x0B,
        FM_BLOCK    = 0x0C,
        FM_MARKER   = 0x0D,
        COMMAND     = 0x0E,
        UNKNOWN     = 0x0F,
        FM_PHASEOFF = 0x10,
        VOL_L       = 0x11,
        VOL_R       = 0x12,
        PCM_ADR1L   = 0x13,
        FM_NOTE     = 0x13,
        PCM_ADR1H   = 0x14,
        FM_PHASE_AMP= 0x14,
        PCM_ADR2    = 0x15,
        PCM_PITCH   = 0x16,
        CTRL        = 0x17,
        FM_LOOP     = 0x18,
        SEQ_ADR1    = 0x1C,
        SEQ_ADR2    = 0x1E,
    };
};

// +0x00: [Byte] Engine Volume
// +0x01: [Byte] Engine Volume (seems same as 0x00)
// +0x02: [Byte] Flags -6543210
//               6 = 
//               5 = Set mutes channel completely
//               4 = 
//               3 = Set denotes loop address has been set
//               2 = Set denotes loop disabled
//               1 = Denote engine volume set
//               0 = Set denotes start address / end address has been set
// +0x03: [Byte] Engine Sample Loop counter (0 - 8) 
//               Used as offset into separate 0x20 block to store data at (e.g. Engine Pitch1, Pitch2, Vol)
// +0x04: [Byte] Engine Pitch Low
// +0x05: [Byte] Engine Pitch High
// +0x06: [Byte] Volume adjusted by 0x76D7 routine
// +0x07: [Byte]
// +0x08: [Byte] 0 = Channel Muted, 1 = Channel Active

namespace ch_engines
{
    enum
    {
        VOL0    = 0x00,
        VOL1    = 0x01,
        FLAGS   = 0x02,
        OFFSET  = 0x03,
        LOOP    = 0x03,
        PITCH_L = 0x04,
        PITCH_H = 0x05,
        VOL6    = 0x06,
        ACTIVE  = 0x08,
    };
};

// MML Command Languge Defines
namespace mml
{
    enum
    {
        TEMPO            = 0x01, // change tempo of track(only possible for non - FIXED_TEMPO tracks)
        SAMPLE_LEVEL	 = 0x02, // set sample left / right volume
        SEAMLESS		 = 0x03, // make use of unused command $83 for 'seamless' note transitions
        END_FM_TRACK	 = 0x04, // end playback of this FM track(DO NOT USE ON PCM TRACKS)
        NOISE_ON		 = 0x05, // set noise bit(only works on FM track 7)
        SET_TL		     = 0x06, // only partially working(can't use bits 0 and 1)
        KEY_FRACTIONS	 = 0x07, // little to no audible difference
        CALL		     = 0x08, // call a subroutine
        RET			     = 0x09, // return from a subroutine
        LOOP_FOREVER	 = 0x0a, // self explanatory
        TRANSPOSE		 = 0x0b, // transpose subsequent notes up or down
        LOOP		     = 0x0c, // loop n number of times(allows nested loops via second parameter)
        PITCH_BEND_START = 0x0d, // pitch bend start
        PITCH_BEND_END	 = 0x0e, // pitch bend AND 'seamless' end
        LOAD_PATCH		 = 0x11, // load new FM patch data into the sound chip registers
        NOISE_OFF		 = 0x12, // clear noise bit(set by cmd $85)
        VOICE_PITCH		 = 0x13, // set pitch of PCM voice like 'Checkpoint' or 'Get Ready'
        FIXED_TEMPO		 = 0x14, // note duration is read directly from track rather than being computed
        LONG		     = 0x15, // used for 'long' notes, restsand percussion samples
        RIGHT_CH_ONLY	 = 0x16, // send FM output to right channel / speaker only
        LEFT_CH_ONLY	 = 0x17, // send FM output to left channel / speaker only
        BOTH_CH		     = 0x18, // send FM output to both channels / speakers
    };
};



class OSound
{
public:
    // Command to process
    uint8_t command_input;

    // [+0] Unused
    // [+1] Engine pitch high
    // [+2] Engine pitch low
    // [+3] Engine pitch vol
    // [+4] Traffic data #1 
    // [+5] Traffic data #2 
    // [+6] Traffic data #3 
    // [+7] Traffic data #4
    uint8_t engine_data[8];

    OSound();
    ~OSound();

    void init(YM2151* ym, uint8_t* pcm_ram);
    void init_fm_chip();
    void tick();

private:
    const static uint16_t PCM_RAM_SIZE  = 0x100;
    const static uint16_t CHAN_RAM_SIZE = 0x800;

    // Internal channel format
    uint8_t chan_ram[CHAN_RAM_SIZE];

    // Size of each internal channel entry
    const static uint8_t CHAN_SIZE = 0x20;

    // ------------------------------------------------------------------------------------------------
    // Format of Data in PCM RAM
    // ------------------------------------------------------------------------------------------------

    // RAM DESCRIPTION ===============
    //
    // 0x00 - 0x07, 0x80 - 0x87 : CHANNEL #1  
    // 0x08 - 0x0F, 0x88 - 0x8F : CHANNEL #2
    // 0x10 - 0x17, 0x90 - 0x97 : CHANNEL #3  
    // 0x18 - 0x1F, 0x98 - 0x9F : CHANNEL #4
    // 0x20 - 0x27, 0xA0 - 0xA7 : CHANNEL #5  
    // 0x28 - 0x2F, 0xA8 - 0xAF : CHANNEL #6
    // 0x30 - 0x37, 0xB0 - 0xB7 : CHANNEL #7  
    // 0x38 - 0x3F, 0xB8 - 0xBF : CHANNEL #8
    // 0x40 - 0x47, 0xC0 - 0xC7 : CHANNEL #9  
    // 0x48 - 0x4F, 0xC8 - 0xCF : CHANNEL #10
    // 0x50 - 0x57, 0xD0 - 0xD7 : CHANNEL #11 
    // 0x58 - 0x5F, 0xD8 - 0xDF : CHANNEL #12
    // 0x60 - 0x67, 0xE0 - 0xE7 : CHANNEL #13 
    // 0x68 - 0x6F, 0xE8 - 0xEF : CHANNEL #14
    // 0x70 - 0x77, 0xF0 - 0xF7 : CHANNEL #15 
    // 0x78 - 0x7F, 0xF8 - 0xFF : CHANNEL #16
    //
    //
    // CHANNEL DESCRIPTION ===================
    //  
    // OFFS | BITS     | DESCRIPTION 
    // -----+----------+---------------------------------
    // 0x00 | -------- | (unknown) <- scratch space for pitch engine 1 noise or vol
    // 0x01 | -------- | (unknown) <- scratch space for pitch engine 2 noise or vol
    // 0x02 | vvvvvvvv | Volume LEFT 
    // 0x03 | vvvvvvvv | Volume RIGHT 
    // 0x04 | aaaaaaaa | Wave Start Address LOW 8 bits 
    // 0x05 | aaaaaaaa | Wave Start Address HIGH 8 bits 
    // 0x06 | eeeeeeee | Wave End Address HIGH 8 bits 
    // 0x07 | dddddddd | Delta (pitch) 
    // 0x80 | -------- | (unknown) Traffic Volume Boost & Pitch Table Entry (Distance of Traffic)
    //      |          |           OR for Engine Channels: Offset into engine_adr_table [Start addresses]
    // 0x81 | -------- | (unknown) Traffic Panning Table Entry
    // 0x82 | -------- | (unknown) <- scratch space. 
    //      |          |           bit 5: Mute channel (engine sounds)
    //      |          |           bit 4: pitch slide.
    //      |          |           bit 3: set to enable traffic sound
    //      |          |           bit 2: set to denote wave start/end address setup. 
    //      |          |           bit 1: set to denote traffic pan is unchanged. unset denotes change.
    //      |          |           bit 0: set to denote traffic vol is unchanged. unset denotes change.
    //      |          |
    //      |          |           OR for Engine Channels:
    //      |          |           bit 2: clear to denote offset into engine_adr_table has been reset. 
    // 0x83 | -------- | (unknown) Traffic Volume Multiplier (read from table specified by 0x80).
    // 0x84 | llllllll | Wave Loop Address LOW 8 bits
    // 0x85 | llllllll | Wave Loop Address HIGH 8 bits 
    // 0x86 | ------la | Flags: a = active (0 = active, 1 = inactive) 
    //      |          |        l = loop   (0 = enabled, 1 = disabled)

    // Reference to 0xFF bytes of PCM Chip RAM
    uint8_t* pcm_ram;

    // SoundChip: Yamaha YM2151
    YM2151*  ym;

    // Bit 0: Set denotes car stationary do rev sample when revs high enough
    // Bit 1: Set to denote PCM sound effect triggered.
    uint8_t sound_props;

    // Stored Command
    uint8_t command_index;

    // F810 - F813
    uint8_t counter1, counter2, counter3, counter4;

    // Position in sequence [de]
    uint16_t pos;

    // Store last command to assist program flow
    uint8_t cmd_prev;

    // Store last chan ID
    uint16_t chanid_prev;

    // PCM Channel Commands in RAM to send
    const static uint16_t CH09_CMDS1 = 0x570; // 0xFD70;
    const static uint16_t CH09_CMDS2 = 0x578;
    const static uint16_t CH11_CMDS1 = 0x580; // 0xFD80;
    const static uint16_t CH11_CMDS2 = 0x588;

    // Panning flags
    const static uint8_t PAN_LEFT  = 0x40;
    const static uint8_t PAN_RIGHT = 0x80;
    const static uint8_t PAN_CENTRE = PAN_LEFT | PAN_RIGHT;

    // ------------------------------------------------------------------------
    // ENGINE TONE CODE
    // ------------------------------------------------------------------------
    // Used to skip the engine code 1/2 times
    uint8_t engine_counter;

    // Engine Channel: Selects Channel at offset 0xF800 for engine tones
    uint8_t engine_channel;

    inline uint8_t pcm_r(uint16_t adr);
    inline void    pcm_w(uint16_t adr, uint8_t v);
    inline uint16_t r16(uint8_t* adr);
    inline void     w16(uint8_t* adr, uint16_t v);
    void process_command();
    void pcm_backup();
    void check_fm_mapping();
    void process_channels();
    void process_channel(uint16_t chan_id);
    void process_section(uint8_t* chan);
    void calc_end_marker(uint8_t* chan);
    void next_mml_cmd(uint8_t* chan, uint8_t cmd);
    void new_command();
    void play_pcm_index(uint8_t* chan, uint8_t cmd);
    void setvol(uint8_t* chan);
    inline void pcm_setpitch(uint8_t* chan);
    inline void set_loop_adr();
    void do_loop(uint8_t* chan);
    void pcm_finalize(uint8_t* chan);
    //void pcm_send_cmds(uint16_t src, uint16_t pcm_adr);
    void init_sound(uint8_t cmd, uint16_t src, uint16_t dst);
    void process_pcm(uint8_t* chan);
    void pcm_send_cmds(uint8_t* chan, uint16_t pcm_adr, uint8_t channel_pair);
    void fm_dotimera();
    void fm_reset();
    void fm_write_reg_c(uint8_t ix0, uint8_t reg, uint8_t value);
    void fm_write_reg(uint8_t reg, uint8_t value);
    void fm_write_block(uint8_t ix0, uint16_t adr, uint8_t chan);
    void ym_set_levels();
    void ym_load_patch(uint8_t* chan);
    uint16_t ym_lookup_data(uint8_t cmd, uint8_t offset, uint8_t block);
    void ym_set_connect(uint8_t* chan, uint8_t pan);
    void ym_end_track(uint8_t* chan);
    void read_mod_table(uint8_t* chan);
    void call_adr(uint8_t* chan);

    // ------------------------------------------------------------------------
    // ENGINE TONE FUNCTIONS
    // ------------------------------------------------------------------------
    void engine_process();
    void engine_process_chan(uint8_t* chan, uint8_t* pcm);
    void vol_thicken(uint16_t& pos, uint8_t* chan, uint8_t* pcm);
    uint8_t get_adjusted_vol(uint16_t& pos, uint8_t* chan);
    void engine_set_pitch(uint16_t& pos, uint8_t* pcm);
    void engine_mute_channel(uint8_t* chan, uint8_t* pcm, bool do_check = true);
    void unk78c7(uint8_t* chan, uint8_t* pcm);
    void ferrari_vol_pan(uint8_t* chan, uint8_t* pcm);
    uint16_t engine_get_table_adr(uint8_t* chan, uint8_t* pcm);
    void engine_adjust_volume(uint8_t* chan);
    uint16_t engine_set_adr(uint16_t& pos, uint8_t* chan, uint8_t* pcm);
    void engine_set_adr_end(uint16_t& pos, uint16_t loop_adr, uint8_t* chan, uint8_t* pcm);
    void engine_set_pan(uint16_t& pos, uint8_t* chan, uint8_t* pcm);
    void engine_read_data(uint8_t* chan, uint8_t* pcm);

    // ----------------------------------------------------------------------------
    //                               PASSING TRAFFIC FX 
    // ----------------------------------------------------------------------------
    void traffic_process();
    void traffic_process_chan(uint8_t* pcm);
    void traffic_process_entry(uint8_t* pcm);
    void traffic_disable(uint8_t* pcm);
    void traffic_set_vol(uint8_t* pcm);
    void traffic_set_pan(uint8_t* pcm);
    uint8_t traffic_get_vol(uint16_t pos, uint8_t* pcm);
    void traffic_note_changes(uint8_t new_vol, uint8_t* pcm);
    void traffic_read_data(uint8_t* pcm);
};