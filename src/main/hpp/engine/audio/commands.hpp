#pragma once

// ----------------------------------------------------------------------------
// Commands to send from main program code
// ----------------------------------------------------------------------------

namespace sound
{
    enum
    {
        RESET  = 0x80,          // Reset sound code
        MUSIC_BREEZE = 0x81,    // Music: Passing Breeze
        MUSIC_SPLASH = 0x82,    // Music: Splash Wave
        COIN_IN = 0x84,         // Coin IN Effect
        MUSIC_MAGICAL = 0x85,   // Music: Magical Sound Shower
        YM_CHECKPOINT = 0x86,   // YM: Checkpoint Ding
        INIT_SLIP = 0x8A,       // Slip (Looped)
        STOP_SLIP = 0x8B,
        INIT_CHEERS = 0x8D,
        STOP_CHEERS = 0x8E,
        INIT_CRASH1 = 0x8F,
        INIT_REBOUND = 0x90,
        INIT_CRASH2 = 0x92,
        NEW_COMMAND = 0x93,
        INIT_SIGNAL1 = 0x94,
        INIT_SIGNAL2 = 0x95,
        INIT_WEIRD = 0x96,
        STOP_WEIRD = 0x97,
        BEEP1 = 0x99,            // YM Beep
        UFO = 0x9A,              // Unused sound. Note that the z80 code to play this is not implemented in this conversion.
        BEEP2 = 0x9B,            // YM Double Beep
        INIT_CHEERS2     = 0x9C, // Cheers (Looped)   
        VOICE_CHECKPOINT = 0x9D, // Voice: Checkpoint
        VOICE_CONGRATS   = 0x9E, // Voice: Congratulations
        VOICE_GETREADY   = 0x9F, // Voice: Get Ready
        INIT_SAFETYZONE  = 0xA0,
        STOP_SAFETYZONE  = 0xA1,
        YM_SET_LEVELS    = 0xA2,
        // 0xA3 Unused - Should be voice 4, but isn't hooked up
        PCM_WAVE = 0xA4,        // Wave Sample
        MUSIC_LASTWAVE = 0xA5,  // Music: Last Wave
    };

    // ----------------------------------------------------------------------------
    // Engine Commands to send from main program code
    // ----------------------------------------------------------------------------

    enum
    {
        UNUSED,
        ENGINE_PITCH_H,
        ENGINE_PITCH_L,
        ENGINE_VOL,
        TRAFFIC1,
        TRAFFIC2,
        TRAFFIC3,
        TRAFFIC4,
    };

};

