#include "game_audio.h"
int16_t game_audio_sample(int effect, unsigned sample)
{
    if (sample >= GAME_AUDIO_SAMPLES || effect < 1 || effect > 3) return 0;
    int amplitude = (GAME_AUDIO_SAMPLES - sample) * 4500 / GAME_AUDIO_SAMPLES;
    if (effect == 2) {
        uint32_t noise = sample * 747796405u + 2891336453u;
        noise = ((noise >> ((noise >> 28) + 4)) ^ noise) * 277803737u;
        return (noise & 1) ? amplitude : -amplitude;
    }
    unsigned frequency = effect == 1 ? 180 + (GAME_AUDIO_SAMPLES-sample)/8 : sample < 1600 ? 440 : 660;
    return ((sample*frequency/GAME_AUDIO_HZ)&1) ? amplitude : -amplitude;
}
