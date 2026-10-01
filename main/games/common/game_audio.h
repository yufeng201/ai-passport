#pragma once
#include <stdint.h>
#define GAME_AUDIO_HZ 16000
#define GAME_AUDIO_SAMPLES 3200
/* Procedural 200 ms mono sound: 1=strike, 2=crash, 3=start/finish. Pure and bounded. */
int16_t game_audio_sample(int effect, unsigned sample);
