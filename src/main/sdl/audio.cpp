/***************************************************************************
    
    Emscripten Audio Handler.  
    
***************************************************************************/

#include "audio.hpp"
#include "../frontend/config.hpp" // fps
#include "../engine/audio/osoundint.hpp"

#ifdef COMPILE_SOUND_CODE


Audio::Audio()
{

}

Audio::~Audio()
{

}

void Audio::init()
{
    if (config.sound.enabled)
        start_audio();
}

void Audio::start_audio()
{
    if (!sound_enabled)
        sound_enabled = true;
}

void Audio::stop_audio()
{
    if (sound_enabled)
        sound_enabled = false;
}

void Audio::pause_audio()
{
}

void Audio::resume_audio()
{
}

// Called every frame to update the audio
void Audio::tick(float* mix_buffer_l, float* mix_buffer_r)
{
    if (!sound_enabled) return;

    // Update audio streams from PCM & YM Devices
    osoundint.pcm->stream_update();
    osoundint.ym->stream_update();

    // Get the audio buffers we've just output
    int16_t *pcm_buffer = osoundint.pcm->get_buffer();
    int16_t *ym_buffer  = osoundint.ym->get_buffer();
    int16_t *wav_buffer = wavfile.data;

    int samples_written = osoundint.pcm->buffer_size;

    // And mix them into the mix_buffer
    for (int i = 0; i < samples_written; i++)
    {
        int32_t mix_data = wav_buffer[wavfile.pos] + pcm_buffer[i] + ym_buffer[i];

        // Clip mix data
        if (mix_data >= (1 << 15))
            mix_data = (1 << 15);
        else if (mix_data < -(1 << 15))
            mix_data = -(1 << 15);

        // Right Channel
        if (i & 1)
        {
            mix_buffer_r[i / 2] = (float) (mix_data / 32768.0);
        }
        // Left Channel
        else
        {
            mix_buffer_l[i / 2] = (float) (mix_data / 32768.0);
        }

        // Loop wav files
        //if (++wavfile.pos >= wavfile.length)
        //    wavfile.pos = 0;
    }
}

// Emscripten: Not necessary as this is handled by the Javascript calling code
double Audio::adjust_speed()
{
    return 1.0;
}

// Empty Wav Buffer
//static int16_t EMPTY_BUFFER[] = {0, 0, 0, 0};

void Audio::load_wav(const char* filename)
{

}

void Audio::clear_wav()
{

}
#endif