#include "bw_game.h"
#include <stdio.h>
static uint16_t pixels[BW_WIDTH*BW_HEIGHT];
static int save(bw_game_t *g,const char *name){
    bw_render_strip(g,pixels,0,BW_HEIGHT);FILE *f=fopen(name,"wb");if(!f)return 0;
    fprintf(f,"P6\n320 240\n255\n");
    for(int i=0;i<BW_WIDTH*BW_HEIGHT;i++){uint16_t p=pixels[i];unsigned char rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}
    return fclose(f)==0;
}
int main(void){
    bw_game_t g;char path[160];
    for(int stage=1;stage<=5;stage++)for(int state=0;state<=4;state++){
        bw_init(&g,0,0xB21C);g.stage=stage;bw_start(&g);g.phase=(bw_phase_t)state;
        g.battery=74;g.scene_ms=3500;g.score=720;g.best=2100;
        g.ready=0;g.ball_x=180*BW_Q;g.ball_y=160*BW_Q;g.vx=2*BW_Q;g.vy=-4*BW_Q;
        snprintf(path,sizeof(path),"build/games/brick_workshop/stage%d-state%d.ppm",stage,state);
        if(!save(&g,path))return 1;
    }
    bw_init(&g,0,1);if(!save(&g,"build/games/brick_workshop/title-initial.ppm"))return 1;
    return 0;
}
