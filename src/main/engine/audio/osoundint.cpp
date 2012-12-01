/***************************************************************************
    Interface to Ported Z80 Code.
    Handles the interface between 68000 program code and Z80.

    Also abstracted here, so the more complex OSound class isn't exposed
    to the main code directly
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/outrun.hpp"
#include "engine/audio/osound.hpp"
#include "engine/audio/osoundint.hpp"

OSoundInt osoundint;
OSound osound;

OSoundInt::OSoundInt()
{
    pcm_ram = new uint8_t[PCM_RAM_SIZE];
    has_booted = false;
}

OSoundInt::~OSoundInt()
{
    delete[] pcm_ram;
}

void OSoundInt::init()
{
    if (pcm == NULL)
    {
        pcm = new SegaPCM(SOUND_CLOCK, &roms.pcm, pcm_ram, SegaPCM::BANK_512);
        pcm->init(FRAMES_PER_SECOND);
    }

    if (ym == NULL)
    {
        ym = new YM2151(0.5f, (uint32_t) (SOUND_CLOCK * 1.024));
        ym->init(44100, FRAMES_PER_SECOND);
    }

    reset();

    // Clear PCM Chip RAM
    for (uint16_t i = 0; i < PCM_RAM_SIZE; i++)
        pcm_ram[i] = 0;

    osound.init(ym, pcm_ram);
}

// Clear sound queue
// Source: 0x5086
void OSoundInt::reset()
{
    sound_counter = 0;
    sound_head    = 0;
    sound_tail    = 0;
    sounds_queued = 0;
}

void OSoundInt::tick()
{
    if (FRAMES_PER_SECOND == 30)
    {
        play_queued_sound(); // Process audio commands from main program code
        osound.tick();
        play_queued_sound();
        osound.tick();
        play_queued_sound();
        osound.tick();
        play_queued_sound();
        osound.tick();
    }
    else if (FRAMES_PER_SECOND == 60)
    {
        play_queued_sound(); // Process audio commands from main program code
        osound.tick();
        play_queued_sound();
        osound.tick();
    }
}

// ----------------------------------------------------------------------------
// Sound Queuing Code
// ----------------------------------------------------------------------------

// Play Queued Sounds & Send Engine Noise Commands to Z80
// Was called by horizontal interrupt routine
// Source: 0x564E
void OSoundInt::play_queued_sound()
{
    if (!has_booted)
    {
        sound_head = 0;
        sounds_queued = 0;
        return;
    }

    // Process the lot in one go. 
    for (int counter = 0; counter < 8; counter++)
    {
        // Process queued sound
        if (counter == 0)
        {
            if (sounds_queued != 0)
            {
                osound.command_input = queue[sound_head];
                sound_head = (sound_head + 1) & QUEUE_LENGTH;
                sounds_queued--;
            }
            else
            {
                osound.command_input = sound::RESET;
            }
        }
        // Process player engine sounds and passing traffic
        else
        {
            osound.engine_data[counter] = engine_data[counter];
        }
    }
}

// Queue a sound in service mode
// Used to trigger both sound effects and music
// Source: 0x56C6
void OSoundInt::queue_sound_service(uint8_t snd)
{
    if (has_booted)
        add_to_queue(snd);
    else
        queue_clear();
}

// Queue a sound in-game
// Note: This version has an additional check, so that certain sounds aren't played depending on game mode
// Source: 0x56D4
void OSoundInt::queue_sound(uint8_t snd)
{
    if (has_booted)
    {
        if (outrun.game_state == GS_ATTRACT)
        {
            // Return if we are not playing sound in attract mode
            if (!DIP_ADVERTISE) return;

            // Do not play music in attract mode, even if attract sound enabled
            if (snd == sound::MUSIC_BREEZE || snd == sound::MUSIC_MAGICAL ||
                snd == sound::MUSIC_SPLASH || snd == sound::MUSIC_LASTWAVE)
                return;
        }
        add_to_queue(snd);
    }
    else
        queue_clear();
}

void OSoundInt::add_to_queue(uint8_t snd)
{
    // Add sound to the tail end of the queue
    queue[sound_tail] = snd;
    sound_tail = (sound_tail + 1) & QUEUE_LENGTH;
    sounds_queued++;
}

void OSoundInt::queue_clear()
{
    sound_tail = 0;
    sounds_queued = 0;
}