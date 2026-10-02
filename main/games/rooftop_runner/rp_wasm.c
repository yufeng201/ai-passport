#include "rp_game.h"
#include "game_audio.h"
static rp_game_t game;
static uint16_t frame[RP_WIDTH*RP_HEIGHT]; /* Browser-only, absent from firmware. */
void game_init(uint32_t seed){rp_init(&game,0,seed);}
void game_edge(int key,int down,uint32_t now){rp_edge(&game,key,down,now);}
void game_tick(uint32_t now){rp_update(&game,now);}
void game_cancel(uint32_t now){rp_cancel(&game,now);}
uint16_t *game_frame(void){rp_render_strip(&game,frame,0,RP_HEIGHT);return frame;}
uint32_t game_hash(void){return rp_hash(&game);}
int game_phase(void){return game.phase;}
int game_effect(void){int effect=game.effect;game.effect=0;return effect;}
int game_sound_sample(int effect,unsigned sample){return game.muted?0:game_audio_sample(effect,sample);}

int game_music_theme(void) { return game.phase==RP_PLAY&&!game.muted?5:0; }
int game_music(unsigned theme,unsigned sample) {return game_music_sample((int)theme,sample);}
