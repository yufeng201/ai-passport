#include "rp_game.h"
#include "game_runtime.h"
static rp_game_t game;
static void init(uint32_t now){rp_init(&game,now,0xA11Eu);}
static void update(uint32_t now){rp_update(&game,now);}
static void edge(int key,int down,uint32_t now){rp_edge(&game,key,down,now);}
static void cancel(uint32_t now){rp_cancel(&game,now);}
static void render(uint16_t *pixels,int y,int rows){rp_render_strip(&game,pixels,y,rows);}
static void battery(int percent){game.battery=percent;}
static void restore(const game_progress_t *p){
    if(p->unlocked>=1&&p->unlocked<=RP_STAGES)game.unlocked=(int)p->unlocked;
    if(p->best<=1000000)game.best=(int)p->best;
    game.muted=p->muted==1;game.medals=p->medals&1023u;
}
static void progress(game_progress_t *p){*p=(game_progress_t){(uint32_t)game.best,(uint32_t)game.unlocked,(uint32_t)game.muted,game.medals};}
static int effect(void){int value=game.effect;game.effect=0;return value;}
static int music(void){return game.phase==RP_PLAY&&!game.muted?5:0;}
void rp_device_run(void){
    static const game_app_t app={"rooftop_runner",init,update,edge,cancel,render,battery,restore,progress,effect,music};
    game_runtime_run(&app);
}
