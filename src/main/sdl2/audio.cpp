/***************************************************************************
    SDL Audio Code.
    
    This is the SDL specific audio code.
    If porting to a non-SDL platform, you would need to replace this class.
    
    It takes the output from the PCM and YM chips, mixes them and then
    outputs appropriately.
    
    In order to achieve seamless audio, when audio is enabled the framerate
    is adjusted to essentially sync the video to the audio output.
    
    This is based upon code from the Atari800 emulator project.
    Copyright (c) 1998-2008 Atari800 development team
***************************************************************************/

#include <iostream>
#include <SDL.h>
#include <time.h>

#ifdef SDL2
#include "sdl2/audio.hpp"
#else
#include "sdl/audio.hpp"
#endif

#include "frontend/config.hpp" // fps
#include "engine/audio/osoundint.hpp"

#ifdef COMPILE_SOUND_CODE

/* ----------------------------------------------------------------------------
   SDL Sound Implementation & Callback Function
   ----------------------------------------------------------------------------*/

// Note that these variables are accessed by two separate threads.
uint8_t* dsp_buffer;
static int dsp_buffer_bytes;
static int dsp_write_pos;
static int dsp_read_pos;
static int callbacktick;     // tick at which callback occured
static int bytes_per_sample; // Number of bytes per sample entry (usually 4 bytes if stereo and 16-bit sound)
static int spinlock = 0;     // used to prevent concurrent updates in tick() and fillaudio()

// SDL Audio Callback Function
extern void fill_audio(void *udata, Uint8 *stream, int len);

// ----------------------------------------------------------------------------

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
    {
        // Since many GNU/Linux distros are infected with PulseAudio, SDL2 could chose PA as first
	// driver option before ALSA, and PA doesn't obbey our sample number requests, resulting
	// in audio gaps, if we're on a GNU/Linux we force ALSA.
	// Else we accept whatever SDL2 wants to give us or what the user specifies on SDL_AUDIODRIVER
	// enviroment variable.
	std::string platform = SDL_GetPlatform();
	if (platform=="Linux"){

	    if (SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) {

		std::cout << "Error initalizing audio subsystem: " << SDL_GetError() << std::endl;
	    }

	    if (SDL_AudioInit("alsa")!=0) {
		std::cout << "Error initalizing audio using ALSA: " << SDL_GetError() << std::endl;
		return;
	    }

	}
	else {
	    if(SDL_Init(SDL_INIT_AUDIO) == -1) 
	    {
		std::cout << "Error initalizing audio: " << SDL_GetError() << std::endl;
		return;
	    }		
	}

        // SDL Audio Properties
        SDL_AudioSpec desired, obtained;

        desired.freq     = FREQ;
        desired.format   = AUDIO_S16SYS;
        desired.channels = CHANNELS;
        desired.samples  = SAMPLES;
        desired.callback = fill_audio;
        desired.userdata = NULL;
	
	// SDL2 block
	dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, /*SDL_AUDIO_ALLOW_FORMAT_CHANGE*/0);
	if (dev == 0)
	{
            std::cout << "Error opening audio device: " << SDL_GetError() << std::endl;
            return;
        }

        if (desired.samples != obtained.samples) {
            std::cout << "Error initalizing audio: number of samples not supported." << std::endl
                      << "Please compare desired vs obtained. Look at what audio driver SDL2 is using." << std::endl;
	    return;
	}

        bytes_per_sample = CHANNELS * (BITS / 8);

        // Start Audio
        sound_enabled = true;

        // how many fragments in the dsp buffer
        const int DSP_BUFFER_FRAGS = 5;
        int specified_delay_samps = (FREQ * SND_DELAY) / 1000;
        int dsp_buffer_samps = SAMPLES * DSP_BUFFER_FRAGS + specified_delay_samps;
        dsp_buffer_bytes = CHANNELS * dsp_buffer_samps * (BITS / 8);
        dsp_buffer = new uint8_t[dsp_buffer_bytes];

        // Create Buffer For Mixing
        uint16_t buffer_size = (FREQ / config.fps) * CHANNELS;
        mix_buffer = new uint16_t[buffer_size];

        clear_buffers();
        clear_wav();

        SDL_PauseAudioDevice(dev,0);
    }
}

void Audio::clear_buffers()
{
    dsp_read_pos  = 0;
    int specified_delay_samps = (FREQ * SND_DELAY) / 1000;
    dsp_write_pos = (specified_delay_samps+SAMPLES) * bytes_per_sample;
    avg_gap = 0.0;
    gap_est = 0;

    for (int i = 0; i < dsp_buffer_bytes; i++)
        dsp_buffer[i] = 0;

    uint16_t buffer_size = (FREQ / config.fps) * CHANNELS;
    for (int i = 0; i < buffer_size; i++)
        mix_buffer[i] = 0;

    callbacktick = 0;
}

void Audio::stop_audio()
{
    if (sound_enabled)
    {
        sound_enabled = false;

        SDL_PauseAudioDevice(dev,1);
        SDL_CloseAudioDevice(dev);

        delete[] dsp_buffer;
        delete[] mix_buffer;
    }
}

void Audio::pause_audio()
{
    if (sound_enabled)
    {
        SDL_PauseAudioDevice(dev,1);
    }
}

void Audio::resume_audio()
{
    if (sound_enabled)
    {
        clear_buffers();
        SDL_PauseAudioDevice(dev,0);
    }
}

// Called every frame to update the audio
void Audio::tick()
{
    int bytes_written = 0;
    int newpos;
    double bytes_per_ms;

    if (!sound_enabled) return;

    // check for another thread running, and wait for it to avoid
    // errors updating the pointers
    struct timespec ts;
    int res;
    ts.tv_sec = 0; // whole seconds to sleep
    ts.tv_nsec = 100; // 100ns sleep period, if we need to wait
    while (spinlock==1) {
      res = nanosleep(&ts, &ts);
    } // wait
    spinlock = 1; // hold the lock

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

        mix_buffer[i] = mix_data;

        // Loop wav files
        if (++wavfile.pos >= wavfile.length)
            wavfile.pos = 0;
    }

    // Cast mix_buffer to a byte array, to align it with internal SDL format 
    uint8_t* mbuf8 = (uint8_t*) mix_buffer;

    // produce samples from the sound emulation
    bytes_per_ms = (bytes_per_sample) * (FREQ/1000.0);
    bytes_written = (BITS == 8 ? samples_written : samples_written*2);
    
    SDL_LockAudio();

    // this is the gap as of the most recent callback
    int gap = dsp_write_pos - dsp_read_pos;
    // an estimation of the current gap, adding time since then
    if (callbacktick != 0)
        gap_est = (int) (gap - (bytes_per_ms)*(SDL_GetTicks() - callbacktick));

    // if there isn't enough room...
    while (gap + bytes_written > dsp_buffer_bytes) 
    {
        // then we allow the callback to run..
        SDL_UnlockAudio();
        // and delay until it runs and allows space.
        SDL_Delay(1);
        SDL_LockAudio();
        //printf("sound buffer overflow:%d %d\n",gap, dsp_buffer_bytes);
        gap = dsp_write_pos - dsp_read_pos;
    }
    // now we copy the data into the buffer and adjust the positions
    newpos = dsp_write_pos + bytes_written;
    if (newpos/dsp_buffer_bytes == dsp_write_pos/dsp_buffer_bytes) 
    {
        // no wrap
        memcpy(dsp_buffer+(dsp_write_pos%dsp_buffer_bytes), mbuf8, bytes_written);
    }
    else 
    {
        // wraps
        int first_part_size = dsp_buffer_bytes - (dsp_write_pos%dsp_buffer_bytes);
        memcpy(dsp_buffer+(dsp_write_pos%dsp_buffer_bytes), mbuf8, first_part_size);
        memcpy(dsp_buffer, mbuf8+first_part_size, bytes_written-first_part_size);
    }
    dsp_write_pos = newpos;

    // Sound callback has not yet been called
    if (callbacktick == 0)
        dsp_read_pos += bytes_written;

    while (dsp_read_pos > dsp_buffer_bytes) 
    {
        dsp_write_pos -= dsp_buffer_bytes;
        dsp_read_pos -= dsp_buffer_bytes;
    }
    spinlock = 0; // release the lock
    SDL_UnlockAudio();
}

// Adjust the speed of the emulator, based on audio streaming performance.
// This ensures that we avoid pops and crackles (in theory). 
double Audio::adjust_speed()
{
    if (!sound_enabled)
        return 1.0;

    double alpha = 2.0 / (1.0+40.0);
    int gap_too_small;
    int gap_too_large;
    bool inited = false;

    if (!inited) 
    {
        inited = true;
        avg_gap = gap_est;
    }
    else 
    {
        avg_gap = avg_gap + alpha * (gap_est - avg_gap);
    }

    gap_too_small = (SND_DELAY * FREQ * bytes_per_sample)/1000;
    gap_too_large = ((SND_DELAY + SND_SPREAD) * FREQ * bytes_per_sample)/1000;
    
    if (avg_gap < gap_too_small) 
    {
        double speed = 0.9;
        return speed;
    }
    if (avg_gap > gap_too_large)
    {
        double speed = 1.1;
        return speed;
    }
    return 1.0;
}

// Empty Wav Buffer
static int16_t EMPTY_BUFFER[] = {0, 0, 0, 0};

void Audio::load_wav(const char* filename)
{
    if (sound_enabled)
    {
        clear_wav();

        // Load Wav File
        SDL_AudioSpec wave;
    
        uint8_t *data;
        uint32_t length;

        pause_audio();

        if( SDL_LoadWAV(filename, &wave, &data, &length) == NULL)
        {
            wavfile.loaded = 0;
            resume_audio();
            std::cout << "Could not load wav: " << filename << std::endl;
            return;
        }
        
        SDL_LockAudio();

        // Halve Volume Of Wav File
        uint8_t* data_vol = new uint8_t[length];
	SDL_MixAudioFormat(data_vol, data, wave.format, length, SDL_MIX_MAXVOLUME / 2);

        // WAV File Needs Conversion To Target Format
        if (wave.format != AUDIO_S16 || wave.channels != 2 || wave.freq != FREQ)
        {
            SDL_AudioCVT cvt;
            SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq,
                                    AUDIO_S16,   CHANNELS,      FREQ);

            cvt.buf = (uint8_t*) malloc(length*cvt.len_mult);
            memcpy(cvt.buf, data_vol, length);
            cvt.len = length;
            SDL_ConvertAudio(&cvt);
            SDL_FreeWAV(data);
            delete[] data_vol;

            wavfile.data = (int16_t*) cvt.buf;
            wavfile.length = cvt.len_cvt / 2;
            wavfile.pos = 0;
            wavfile.loaded = 1;
        }
        // No Conversion Needed
        else
        {
            SDL_FreeWAV(data);
            wavfile.data = (int16_t*) data_vol;
            wavfile.length = length / 2;
            wavfile.pos = 0;
            wavfile.loaded = 2;
        }

        resume_audio();
        SDL_UnlockAudio();
    }
}

void Audio::clear_wav()
{
    if (wavfile.loaded)
    {
        if (wavfile.loaded == 1)
            free(wavfile.data);
        else
            delete[] wavfile.data;        
    }

    wavfile.length = 1;
    wavfile.data   = EMPTY_BUFFER;
    wavfile.pos    = 0;
    wavfile.loaded = false;
}

// SDL Audio Callback Function
//
// Called when the audio device is ready for more data.
//
// stream:  A pointer to the audio buffer to be filled
// len:     The length (in bytes) of the audio buffer

void fill_audio(void *udata, Uint8 *stream, int len)
{
    int gap;
    int newpos;
    int underflow_amount = 0;
#define MAX_SAMPLE_SIZE 4
    static char last_bytes[MAX_SAMPLE_SIZE];

    // check for another thread running, and wait for it to avoid
    // errors updating the pointers
    struct timespec ts;
    int res;
    ts.tv_sec = 0; // whole seconds to sleep
    ts.tv_nsec = 100; // 100ns sleep period, if we need to wait
    while (spinlock==1) {
      res = nanosleep(&ts, &ts);
    } // wait
    spinlock = 1; // hold the lock

    gap = dsp_write_pos - dsp_read_pos;
    if (gap < len) 
    {
        underflow_amount = len - gap;
        len = gap;
    }
    newpos = dsp_read_pos + len;

    // No Wrap
    if (newpos/dsp_buffer_bytes == dsp_read_pos/dsp_buffer_bytes) 
    {
        memcpy(stream, dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), len);
    }
    // Wrap
    else 
    {
        int first_part_size = dsp_buffer_bytes - (dsp_read_pos%dsp_buffer_bytes);
        memcpy(stream,  dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), first_part_size);
        memcpy(stream + first_part_size, dsp_buffer, len - first_part_size);
    }
    // Save the last sample as we may need it to fill underflow
    if (gap >= bytes_per_sample) 
    {
        memcpy(last_bytes, stream + len - bytes_per_sample, bytes_per_sample);
    }
    // Just repeat the last good sample if underflow
    if (underflow_amount > 0 ) 
    {
        int i;
        for (i = 0; i < underflow_amount/bytes_per_sample; i++) 
        {
            memcpy(stream + len +i*bytes_per_sample, last_bytes, bytes_per_sample);
        }
    }
    dsp_read_pos = newpos;

    spinlock = 0; // release the lock
    // Record the tick at which the callback occured.
    callbacktick = SDL_GetTicks();
}

#endif
