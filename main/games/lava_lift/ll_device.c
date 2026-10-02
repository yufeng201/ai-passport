#include "ll_game.h"
#include "game_runtime.h"
static ll_game_t game;
static void init(uint32_t now){ll_init(&game,now,0xC10Du);}
static void update(uint32_t now){ll_update(&game,now);}
static void edge(int key,int down,uint32_t now){ll_edge(&game,key,down,now);}
static void cancel(uint32_t now){ll_cancel(&game,now);}
static void render(uint16_t *pixels,int y,int rows){ll_render_strip(&game,pixels,y,rows);}
static void battery(int percent){game.battery=percent;}
static void restore(const game_progress_t *p){if(p->best<=LL_GOAL)game.best=(int)p->best;}
static void progress(game_progress_t *p){*p=(game_progress_t){(uint32_t)game.best,1,0,0};}
static int effect(void){int value=game.effect;game.effect=0;return value;}
static int music(void){return 0;}
void ll_device_run(void){static const game_app_t app={"lava_lift",init,update,edge,cancel,render,battery,restore,progress,effect,music};game_runtime_run(&app);}
