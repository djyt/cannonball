#pragma once

class Audio
{
public:
    // Enable/Disable Sound
    bool sound_enabled;

    Audio();
    ~Audio();

    void init();
    void tick();
    void stop_audio();
    double adjust_speed();

private:
    // Sample Rate. Can't be changed easily for now, due to lack of SDL resampling.
    static const uint32_t FREQ = 44100;

    // Stereo. Could be changed, requires some recoding.
    static const uint32_t CHANNELS = 2;

    // 16-Bit Audio Output. Could be changed, requires some recoding.
    static const uint32_t BITS = 16;

    // Low value  = Responsiveness, chance of drop out.
    // High value = Laggy, less chance of drop out.
    static const uint32_t SAMPLES  = 1024;

    // Buffer used to mix PCM and YM channels together
    uint16_t* mix_buffer;

    // Latency (in ms) and thus target buffer size
    const static int SND_DELAY = 20;

    // allowed "spread" between too many and too few samples in the buffer (ms)
    const static int SND_SPREAD = 7;

    // Estimated gap
    int gap_est;

    // Cumulative audio difference
    double avg_gap;
};