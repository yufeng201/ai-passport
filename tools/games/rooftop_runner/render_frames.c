#include "rp_game.h"
#include <stdio.h>
static uint16_t pixels[RP_WIDTH*RP_HEIGHT];
static int save(rp_game_t *g,const char *name){
    rp_render_strip(g,pixels,0,RP_HEIGHT);FILE *f=fopen(name,"wb");if(!f)return 0;
    fprintf(f,"P6\n320 240\n255\n");
    for(int i=0;i<RP_WIDTH*RP_HEIGHT;i++){uint16_t p=pixels[i];unsigned char rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}
    return fclose(f)==0;
}
int main(void){
    rp_game_t g;char path[160];
    for(int stage=1;stage<=5;stage++)for(int state=0;state<=4;state++){
        rp_init(&g,0,0xA11E);g.stage=stage;rp_start(&g);g.phase=(rp_phase_t)state;
        g.battery=74;g.scene_ms=3500;g.elapsed_ms=3600;g.score=600;g.best=1400;
        g.x=(g.platforms[2].x+50)*RP_Q;g.y=(g.platforms[2].y-56)*RP_Q;g.grounded=0;g.platform=-1;g.vx=RP_SPEED;g.camera=g.x/RP_Q-112;
        snprintf(path,sizeof(path),"build/games/rooftop_runner/stage%d-state%d.ppm",stage,state);
        if(!save(&g,path))return 1;
    }
    rp_init(&g,0,1);if(!save(&g,"build/games/rooftop_runner/title-initial.ppm"))return 1;
    rp_init(&g,0,1);rp_start(&g);g.x=146*RP_Q;
    if(!save(&g,"build/games/rooftop_runner/edge-hint.ppm"))return 1;
    g.x=60*RP_Q;g.facing=-1;
    if(!save(&g,"build/games/rooftop_runner/backward-idle.ppm"))return 1;
    return 0;
}
