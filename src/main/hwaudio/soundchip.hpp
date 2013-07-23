/***************************************************************************
    Sound Chip  

    This is an abstract class, used by the Sega PCM and YM2151 chips.
    It facilitates writing to a buffer of sound data.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

class SoundChip
{
public:
    bool initalized;

    // Sample Frequency in use
    uint32_t sample_freq;

    // How many channels to support (mono/stereo)
    uint8_t channels;

    // Size of the buffer (including channel info)
    uint32_t buffer_size;

    SoundChip();
    ~SoundChip();

    void init(uint8_t, int32_t, int32_t);
    
    void set_frame_size(int32_t);

    // Pure virtual function. Denotes virtual class.
    virtual void stream_update() = 0;

    int16_t* get_buffer();

protected:
    const static uint8_t MONO             = 1;
    const static uint8_t STEREO           = 2;

    const static uint8_t LEFT             = 0;
    const static uint8_t RIGHT            = 1;

    //  Buffer size for one frame (excluding channel info)
    uint32_t frame_size;

    void clear_buffer();
    void write_buffer(const uint8_t, uint32_t, int16_t);
    int16_t read_buffer(const uint8_t, uint32_t);

private:
    // Sound buffer stream
    int16_t* buffer;

    // Frames per second
    uint32_t fps; 
};