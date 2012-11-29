/***************************************************************************
    Sound Chip  

    This is an abstract class, used by the Sega PCM and YM2151 chips.
    It facilitates writing to a buffer of sound data.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.hpp"
#include "hwaudio/soundchip.hpp"

SoundChip::SoundChip()
{

}

SoundChip::~SoundChip()
{

}

void SoundChip::init(uint8_t channels, int32_t sample_freq, int32_t fps)
{
    this->fps         = fps;
    this->sample_freq = sample_freq;
    this->channels    = channels;

    frame_size =  sample_freq / fps;
    buffer_size = frame_size * channels;

    // Clean up existing array implementation if we've reinitalized
    //if (buffer != NULL)
        //delete[] buffer;

    buffer = new int16_t[buffer_size];
}

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