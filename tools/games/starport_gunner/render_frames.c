#include "sg_game.h"
#include <stdio.h>
static uint16_t pixels[SG_WIDTH*SG_HEIGHT];
static int save(sg_game_t *g,const char *name){
    sg_render_strip(g,pixels,0,SG_HEIGHT);FILE *f=fopen(name,"wb");if(!f)return 0;
    fprintf(f,"P6\n320 240\n255\n");
    for(int i=0;i<SG_WIDTH*SG_HEIGHT;i++){uint16_t p=pixels[i];unsigned char rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}
    return fclose(f)==0;
}
int main(void){
    sg_game_t g;char path[160];
    for(int stage=1;stage<=5;stage++)for(int state=0;state<=4;state++){
        sg_init(&g,0,0x57A2);g.stage=stage;sg_start(&g);g.phase=(sg_phase_t)state;
        g.battery=74;g.scene_ms=3500;g.charging=state==SG_PLAY;g.charge_ms=800;
        g.score=720;g.best=2100;
        g.enemies[0]=(sg_enemy_t){1,118,86,2,2,1,1,500,1};
        g.enemies[1]=(sg_enemy_t){1,208,106,stage==5?16:1,stage==5?16:1,stage==5?2:0,-1,1800,2};
        g.threats[0]=(sg_threat_t){1,118,115,500};
        snprintf(path,sizeof(path),"build/games/starport_gunner/stage%d-state%d.ppm",stage,state);
        if(!save(&g,path))return 1;
    }
    return 0;
}
