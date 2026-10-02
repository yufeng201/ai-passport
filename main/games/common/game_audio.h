#pragma once
#include <stdint.h>
#define GAME_AUDIO_HZ 16000
#define GAME_AUDIO_SAMPLES 3200
/* Procedural 200 ms mono sound: 1=strike, 2=crash, 3=start/finish, 4=collect,
 * 5=perfect, 6=guard, 7=supply/rescue. Pure and bounded. */
int16_t game_audio_sample(int effect, unsigned sample);
/* Quiet looping music. Themes 1..5: road, clouds, ninja, bricks, rooftops.
 * 0/invalid is silence; caller streams samples and owns pause/mute behavior. */
int16_t game_music_sample(int theme,uint32_t sample);
