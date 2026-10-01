#include "cb_game.h"
#include "game_audio.h"
static cb_game_t game;
static uint16_t frame[CB_WIDTH*CB_HEIGHT]; /* Browser-only, absent from firmware. */
void game_init(uint32_t seed){cb_init(&game,0,seed);}
void game_edge(int key,int down,uint32_t now){cb_edge(&game,key,down,now);}
void game_tick(uint32_t now){cb_update(&game,now);}
void game_cancel(uint32_t now){cb_cancel(&game,now);}
uint16_t *game_frame(void){cb_render_strip(&game,frame,0,CB_HEIGHT);return frame;}
uint32_t game_hash(void){return cb_hash(&game);}
int game_phase(void){return game.phase;}
int game_effect(void){int effect=game.effect;game.effect=0;return effect;}
int game_sound_sample(int effect,unsigned sample){return game_audio_sample(effect,sample);}
