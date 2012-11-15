#pragma once

// ----------------------------------------------------------------------------
// Z80 Addresses
// ----------------------------------------------------------------------------

namespace z80_adr
{
    // FM Note & Octave Lookup Table
    const static uint16_t YM_NOTE_OCTAVE = 0xAC9;

    // Command Lists to send to FM Chip.
    // Format is register, value pairs
    const static uint16_t YM_INIT_CMDS = 0xB29;

    // Format is register, value pairs
    // Address is ($60 + (8 * Operator) + Channel). Total Level affects the total output volume of the sound.
    const static uint16_t YM_LEVEL_CMDS1  = 0xB49;
    const static uint16_t YM_LEVEL_CMDS2  = 0xB81;
    const static uint16_t YM_RELEASE_RATE = 0xB8A;

    // PCM Sample Properties
    const static uint16_t PCM_INFO = 0xDDD;

    // YM: Passing Breeze
    const static uint16_t DATA_BREEZE = 0xE26;

    // YM: Splash Wave
    const static uint16_t DATA_SPLASH = 0x20C8;

    // YM: Magical Sound Shower
    const static uint16_t DATA_MAGICAL = 0x3D5F;

    // YM: Last Wave
    const static uint16_t DATA_LASTWAVE = 0x5F2D;

    // PCM: Safety Zone
    const static uint16_t DATA_SAFETY = 0x69A9;

    // PCM: Slip Sound
    const static uint16_t DATA_SLIP   = 0x69E6;

    // YM: Coin IN Sound
    const static uint16_t DATA_COININ = 0x6A24;

    // YM: Checkpoint beeps
    const static uint16_t DATA_CHECKPOINT = 0x6A60;

    // YM: Signal 1 Sound
    const static uint16_t DATA_SIGNAL1 = 0x6A87;

    // YM: Signal 2 Sound
    const static uint16_t DATA_SIGNAL2 = 0x6AA7;

    // YM: Beep 1 Sound
    const static uint16_t DATA_BEEP1 = 0x6AC7;

    // PCM: Crash Sound
    const static uint16_t DATA_CRASH1 = 0x6C15;

    // PCM: Rebound Sound
    const static uint16_t DATA_REBOUND = 0x6CFF;

    // PCM: Crash Sound
    const static uint16_t DATA_CRASH2 = 0x6C8A;

    // PCM: Weird Sound
    const static uint16_t DATA_WEIRD = 0x6D61;

    // PCM: Crowd Cheers
    const static uint16_t DATA_CHEERS = 0x6F16;

    // PCM: Crowd Cheers 2
    const static uint16_t DATA_CHEERS2 = 0x6F53;

    // PCM: Voice 1, Checkpoint (Sound Data)
    const static uint16_t DATA_VOICE1 = 0x6F91;

    // PCM: Voice 2, Congratulations
    const static uint16_t DATA_VOICE2 = 0x6FCA;

    // PCM: Voice 3, Get Ready
    const static uint16_t DATA_VOICE3 = 0x7003;

    // YM: UFO (Unused?)
    const static uint16_t DATA_UFO = 0x703D;

    // YM: Beep 2 Sound
    const static uint16_t DATA_BEEP2 = 0x71F9;

    // PCM: Wave Sample
    const static uint16_t DATA_WAVE = 0x748B;
};
