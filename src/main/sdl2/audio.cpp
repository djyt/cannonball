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
#include <mutex>

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
// audio_paused: keeps track of when audio is paused:
// 0 - running, 1 - waiting to start when buffer has something ready, 2 - paused on command
static int audio_paused = 1;
std::mutex soundMutex;

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
        desired.samples  = SAMPLES; // number of samples to be filled on each callback
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

        // Start Generating Audio
        sound_enabled = true;

        int specified_delay_samps = (FREQ * SND_DELAY) / 1000;
        int dsp_buffer_samps = SAMPLES * DSP_BUFFER_FRAGS; // + specified_delay_samps;
        dsp_buffer_bytes = CHANNELS * dsp_buffer_samps * (BITS / 8);
        dsp_buffer = new uint8_t[dsp_buffer_bytes];

        // Create Buffer For Mixing
        uint16_t buffer_size = dsp_buffer_bytes / DSP_BUFFER_FRAGS;
        mix_buffer = new uint16_t[buffer_size];

        clear_buffers();
        clear_wav();

        audio_paused = 1; // flags to audio_tick() to start playing when data becomes available
    }
}

void Audio::clear_buffers()
{
    dsp_read_pos  = 0;
    int specified_delay_samps = (FREQ * SND_DELAY) / 1000;
    dsp_write_pos = (specified_delay_samps) * bytes_per_sample;
    avg_gap = 0.0;
    gap_est = 0;

    for (int i = 0; i < dsp_buffer_bytes; i++)
        dsp_buffer[i] = 0;

    uint16_t buffer_size = dsp_buffer_bytes / DSP_BUFFER_FRAGS;
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
        audio_paused = 2;
        SDL_CloseAudioDevice(dev);

        delete[] dsp_buffer;
        delete[] mix_buffer;
    }
}

void Audio::pause_audio()
{
    if (sound_enabled)
    {
        soundMutex.lock();
        SDL_PauseAudioDevice(dev,1);
        audio_paused = 2;
        soundMutex.unlock();
    }
}

void Audio::resume_audio()
{
    if (sound_enabled)
    {
        clear_buffers();
//        SDL_PauseAudioDevice(dev,0);
        audio_paused = 1; // audio_tick() will commence playback when there's something to play
    }
}

// Called every audio frame
void Audio::tick()
{
    int ready = 0;
    int bytes_written = 0;
    int gap, newpos;
    double bytes_per_ms;

    if ((!sound_enabled) || (audio_paused==2)) return;

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

        // JJP clipping
        mix_data = (mix_data > 32767) ? 32767 : mix_data;
        mix_data = (mix_data < -32768) ? -32768 : mix_data;
        mix_buffer[i] = mix_data;

        // Loop wav files
        if (++wavfile.pos >= wavfile.length)
            wavfile.pos = 0;
    }

    // Cast mix_buffer to a byte array, to align it with internal SDL format
    uint8_t* mbuf8 = (uint8_t*) mix_buffer;

    // produce samples from the sound emulation
    bytes_per_ms = (double(bytes_per_sample) * double(FREQ)) / 1000.0; // JJP - avoid precision error
    bytes_written = (BITS == 8 ? samples_written : samples_written * 2);

    // suspend the callback
    SDL_LockAudio();
    // and wait for the callback to complete, to avoid errors updating the pointers (dsp_read_pos specifically)
    soundMutex.lock();

    if (audio_paused) {
        // clear to start calling off samples; we will have data available
        SDL_PauseAudioDevice(dev,0);
        audio_paused = 0;
    }

    // this is the gap as of the most recent callback
    while (!ready) {
        if (dsp_write_pos == dsp_read_pos) gap = dsp_buffer_bytes;
        else if (dsp_write_pos > dsp_read_pos) gap = dsp_buffer_bytes - dsp_write_pos + dsp_read_pos;
        else gap = dsp_read_pos - dsp_write_pos;
/*    // an estimation of the current gap, adding time since then
    if (callbacktick != 0)
        gap_est = (int) (gap - (bytes_per_ms)*(SDL_GetTicks() - callbacktick));
*/
    // if there isn't enough room...
//    while ((gap + bytes_written) > dsp_buffer_bytes) {
        if (gap < bytes_written) {
            printf("Sound buffer overflow:%d %d %d %d %d\n",gap, dsp_buffer_bytes, bytes_written,
                   dsp_read_pos, dsp_write_pos);
            // then we allow the callback to run..
            soundMutex.unlock();
            SDL_UnlockAudio();
            // and delay until it runs and allows space.
            SDL_Delay(1);
            SDL_LockAudio();
            soundMutex.lock();
        } else ready = 1;
    }
    // now we copy the data into the buffer and adjust the positions
    newpos = dsp_write_pos + bytes_written;

    if (newpos < dsp_buffer_bytes) // No wrap
        memcpy(dsp_buffer+(dsp_write_pos), mbuf8, bytes_written);

    else // Wrap around
    {
        int first_part_size = dsp_buffer_bytes - dsp_write_pos;
        memcpy(dsp_buffer+(dsp_write_pos), mbuf8, first_part_size);
        memcpy(dsp_buffer, mbuf8+first_part_size, bytes_written - first_part_size);
        newpos = bytes_written-first_part_size;
    }

    dsp_write_pos = newpos;

    // Sound callback has not yet been called, update pointer here
//    if (callbacktick == 0) dsp_read_pos += bytes_written;

    while (dsp_read_pos >= dsp_buffer_bytes)
        dsp_read_pos -= dsp_buffer_bytes;

//    while (dsp_write_pos > dsp_buffer_bytes)
//        dsp_write_pos -= dsp_buffer_bytes;

    soundMutex.unlock();
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
    int available_bytes;
    int newpos;
    int underflow = 0;

    // check for another thread running, and wait for it to avoid
    // errors updating the pointers
    soundMutex.lock();

    if (dsp_write_pos == dsp_read_pos) available_bytes = 0;
    else if (dsp_write_pos > dsp_read_pos) available_bytes = dsp_write_pos - dsp_read_pos;
    else available_bytes = dsp_buffer_bytes - dsp_read_pos + dsp_write_pos;
    if (available_bytes < len) {
        underflow = len - available_bytes;
        memset(stream, 0, len); // silence the buffer requested, and any valid data held will be filled below.
        printf("Underflow! (%i bytes)\n",underflow);
        printf("DSP Buffer: %i, read_pos: %i, write_pos: %i, Available Bytes: %i\n",
            dsp_buffer_bytes,dsp_read_pos,dsp_write_pos,available_bytes);
        printf("audio_paused: %i\n",audio_paused);
        len = available_bytes;
    }
    newpos = dsp_read_pos + len;
//printf("dsp_buffer_bytes %i newpos %i dsp_read_pos %i + len %i\n",dsp_buffer_bytes,newpos,dsp_read_pos,len);

    // No Wrap
    if (newpos < dsp_buffer_bytes)
        memcpy(stream, dsp_buffer + dsp_read_pos, len);

/*    if (newpos/dsp_buffer_bytes == dsp_read_pos/dsp_buffer_bytes)
    {
        memcpy(stream, dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), len);
    } */
    // Wrap
    else {
        int first_part_size = dsp_buffer_bytes - dsp_read_pos;
        memcpy(stream,  dsp_buffer + dsp_read_pos, first_part_size);
        memcpy(stream + first_part_size,  dsp_buffer, len - first_part_size);
//        newpos = len - first_part_size;
/*        int first_part_size = dsp_buffer_bytes - (dsp_read_pos%dsp_buffer_bytes);
        memcpy(stream,  dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), first_part_size);
        if (len>0)
            memcpy(stream + first_part_size, dsp_buffer, len - first_part_size); //error this line
*/    }
/*    if (underflow) {
        // Just repeat the last good sample if underflow
        for (int i = 0; i < underflow; i++) {
            if ((newpos + i) < dsp_buffer_bytes)
                *(stream + newpos + i) = 0; // zero fill missing samples; effectively mutes the audio
            else // wrap
                *(stream + newpos + i - dsp_buffer_bytes) = 0;
        }
    }
*/
/*        if (available_bytes >= bytes_per_sample)
            memcpy(last_bytes, stream + len - bytes_per_sample, bytes_per_sample);
        for (int i = 0; i < underflow_amount/bytes_per_sample; i++) {
            // potential seg-fault here, if underflow wraps buffer
            memcpy(stream + len +i*bytes_per_sample, last_bytes, bytes_per_sample);
        }
    }*/
    dsp_read_pos = newpos;

    while (dsp_read_pos >= dsp_buffer_bytes)
        dsp_read_pos -= dsp_buffer_bytes;

    soundMutex.unlock();

    // Record the tick at which the callback occured.
    callbacktick = SDL_GetTicks();
}

#endif
