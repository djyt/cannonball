#pragma once

#include "hwaudio/segapcm.hpp"
#include "hwaudio/ym2151.hpp"

#include "engine/audio/commands.hpp"

class SegaPCM;
class YM2151;

class OSoundInt
{
public:
    // SoundChip: Sega Custom Sample Generator
    SegaPCM* pcm;

    // SoundChip: Yamaha YM2151
    YM2151*  ym;

    const static uint16_t PCM_RAM_SIZE = 0x100;

    // Note whether the game has booted
    bool has_booted;

    // [+0] Unused
    // [+1] Engine pitch high
    // [+2] Engine pitch low
    // [+3] Engine pitch vol
    // [+4] Traffic data #1 
    // [+5] Traffic data #2 
    // [+6] Traffic data #3 
    // [+7] Traffic data #4
    uint8_t engine_data[8];

    OSoundInt();
    ~OSoundInt();

    void init();
    void tick();

    void play_queued_sound();
    void queue_sound_service(uint8_t snd);
    void queue_sound(uint8_t snd);
    void queue_clear();

private:
    // 4 MHz
    static const uint32_t SOUND_CLOCK = 4000000;

    // Reference to 0xFF bytes of PCM Chip RAM
    uint8_t* pcm_ram;

    // Controls what type of sound we're going to process in the interrupt routine
    uint8_t sound_counter;

    static const uint8_t QUEUE_LENGTH = 0x1F;
    uint8_t queue[QUEUE_LENGTH];

    // Number of sounds queued
    uint8_t sounds_queued;

    // Positions in the queue
    uint8_t sound_head, sound_tail;

    void add_to_queue(uint8_t snd);
};

extern OSoundInt osoundint;