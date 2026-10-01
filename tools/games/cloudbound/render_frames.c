#include "cb_game.h"
#include <stdio.h>
static uint16_t pixels[CB_WIDTH*CB_HEIGHT];
static int save(cb_game_t *g,const char *name){
    cb_render_strip(g,pixels,0,CB_HEIGHT);FILE *f=fopen(name,"wb");if(!f)return 0;
    fprintf(f,"P6\n320 240\n255\n");
    for(int i=0;i<CB_WIDTH*CB_HEIGHT;i++){uint16_t p=pixels[i];unsigned char rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}
    return fclose(f)==0;
}
int main(void){
    cb_game_t g;char path[160];
    for(int stage=1;stage<=5;stage++)for(int state=0;state<=4;state++){
        cb_init(&g,0,0xC10D);g.stage=stage;cb_start(&g);g.phase=(cb_phase_t)state;
        g.battery=74;g.scene_ms=3500;g.charging=state==CB_PLAY;g.charge_ms=800;
        g.score=720;g.best=2100;
        snprintf(path,sizeof(path),"build/games/cloudbound/stage%d-state%d.ppm",stage,state);
        if(!save(&g,path))return 1;
    }
    return 0;
}
