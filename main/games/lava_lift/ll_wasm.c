#include "ll_game.h"
#include "game_audio.h"
static ll_game_t game;
static uint16_t frame[320*240]; /* Browser-only, absent from firmware. */
void game_init(uint32_t seed){ll_init(&game,0,seed);}
void game_edge(int key,int down,uint32_t now){ll_edge(&game,key,down,now);}
void game_tick(uint32_t now){ll_update(&game,now);}
void game_cancel(uint32_t now){ll_cancel(&game,now);}
uint16_t *game_frame(void){ll_render_strip(&game,frame,0,240);return frame;}
uint32_t game_hash(void){return ll_hash(&game);}
int game_phase(void){return game.phase;}
int game_effect(void){int effect=game.effect;game.effect=0;return effect;}
int game_sound_sample(int effect,unsigned sample){return game_audio_sample(effect,sample);}

int game_music_theme(void) { return 0; }
int game_music(unsigned theme,unsigned sample) {return game_music_sample((int)theme,sample);}
