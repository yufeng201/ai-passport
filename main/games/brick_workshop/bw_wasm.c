#include "bw_game.h"
#include "game_audio.h"
static bw_game_t game;
static uint16_t frame[BW_WIDTH*BW_HEIGHT]; /* Browser-only, absent from firmware. */
void game_init(uint32_t seed){bw_init(&game,0,seed);}
void game_edge(int key,int down,uint32_t now){bw_edge(&game,key,down,now);}
void game_tick(uint32_t now){bw_update(&game,now);}
void game_cancel(uint32_t now){bw_cancel(&game,now);}
uint16_t *game_frame(void){bw_render_strip(&game,frame,0,BW_HEIGHT);return frame;}
uint32_t game_hash(void){return bw_hash(&game);}
int game_phase(void){return game.phase;}
int game_effect(void){int effect=game.effect;game.effect=0;return effect;}
int game_sound_sample(int effect,unsigned sample){return game.muted?0:game_audio_sample(effect,sample);}

int game_music_theme(void) { return game.phase==BW_PLAY&&!game.muted?4:0; }
int game_music(unsigned theme,unsigned sample) {return game_music_sample((int)theme,sample);}
