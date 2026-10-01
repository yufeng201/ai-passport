#include "sg_game.h"
#include "game_audio.h"
static sg_game_t game;
static uint16_t frame[SG_WIDTH*SG_HEIGHT]; /* Browser-only, absent from firmware. */
void game_init(uint32_t seed){sg_init(&game,0,seed);}
void game_edge(int key,int down,uint32_t now){sg_edge(&game,key,down,now);}
void game_tick(uint32_t now){sg_update(&game,now);}
void game_cancel(uint32_t now){sg_cancel(&game,now);}
uint16_t *game_frame(void){sg_render_strip(&game,frame,0,SG_HEIGHT);return frame;}
uint32_t game_hash(void){return sg_hash(&game);}
int game_phase(void){return game.phase;}
int game_effect(void){int effect=game.effect;game.effect=0;return effect;}
int game_sound_sample(int effect,unsigned sample){return game_audio_sample(effect,sample);}
