#pragma once
#include <stdint.h>
#define RR_SOUND_HZ 16000
#define RR_SOUND_SAMPLES 3200
/* Procedural 200 ms mono sound: 1=strike, 2=crash, 3=start/finish. Pure and bounded. */
int16_t rr_sound_sample(int effect, unsigned sample);
