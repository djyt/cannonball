/***************************************************************************
    Sound Chip  

    This is an abstract class, used by the Sega PCM and YM2151 chips.
    It facilitates writing to a buffer of sound data.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../stdint.hpp"
#include "soundchip.hpp"

SoundChip::SoundChip()
{
    buffer = 0;
    initalized = false;
}

SoundChip::~SoundChip()
{
    delete[] buffer;
}

void SoundChip::init(uint8_t channels, int32_t sample_freq, int32_t fps)
{
    this->fps         = fps;
    this->sample_freq = sample_freq;
    this->channels    = channels;

    #ifndef EMSCRIPTEN
    frame_size =  sample_freq / fps;
    buffer_size = frame_size * channels;

    if (initalized)
        delete[] buffer;
    
    buffer = new int16_t[buffer_size];

    initalized = true;
    #endif
}

// Used by implementations that directly set the frame size
#ifdef EMSCRIPTEN
void SoundChip::set_frame_size(int32_t s)
{
    frame_size  = s;
    buffer_size = frame_size * channels;
    
    if (buffer != 0)
        delete[] buffer;
        
    buffer = new int16_t[buffer_size];
    
    initalized = true;
}
#endif

void SoundChip::clear_buffer()
{
    for (uint32_t i = 0; i < buffer_size; i++)
        buffer[i] = 0;
}

void SoundChip::write_buffer(const uint8_t channel, uint32_t address, int16_t value)
{
    buffer[channel + (address * channels)] = value;
}

int16_t SoundChip::read_buffer(const uint8_t channel, uint32_t address)
{
    return buffer[channel + (address * channels)];
}

int16_t* SoundChip::get_buffer()
{
    return buffer;
}