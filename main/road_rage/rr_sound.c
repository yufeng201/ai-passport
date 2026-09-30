#include "rr_sound.h"
int16_t rr_sound_sample(int effect, unsigned sample)
{
    if (sample >= RR_SOUND_SAMPLES || effect < 1 || effect > 3) return 0;
    int amplitude = (RR_SOUND_SAMPLES - sample) * 4500 / RR_SOUND_SAMPLES;
    if (effect == 2) {
        uint32_t noise = sample * 747796405u + 2891336453u;
        noise = ((noise >> ((noise >> 28) + 4)) ^ noise) * 277803737u;
        return (noise & 1) ? amplitude : -amplitude;
    }
    unsigned frequency = effect == 1 ? 180 + (RR_SOUND_SAMPLES-sample)/8 : sample < 1600 ? 440 : 660;
    return ((sample*frequency/RR_SOUND_HZ)&1) ? amplitude : -amplitude;
}
