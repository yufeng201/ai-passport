#include "an_game.h"
#include <stdio.h>
static uint16_t pixels[AN_WIDTH*AN_HEIGHT];
static int save(an_game_t *g,const char *name){
    an_render_strip(g,pixels,0,AN_HEIGHT);FILE *f=fopen(name,"wb");if(!f)return 0;
    fprintf(f,"P6\n320 240\n255\n");
    for(int i=0;i<AN_WIDTH*AN_HEIGHT;i++){uint16_t p=pixels[i];unsigned char rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}
    return fclose(f)==0;
}
int main(void){
    an_game_t g;char path[160];
    for(int stage=1;stage<=5;stage++)for(int state=0;state<=4;state++){
        an_init(&g,0,0xA11E);g.stage=stage;an_start(&g);g.phase=(an_phase_t)state;
        g.battery=74;g.scene_ms=3500;g.guarding=state==AN_PLAY;g.stamina=72;
        g.score=720;g.best=2100;g.enemy=(an_enemy_t){.active=1,.x=196,.side=1,.hp=stage==5?5:2,.max_hp=stage==5?5:2,.kind=stage==5?2:1,.phase=AN_WINDUP,.timer_ms=400,.warning_ms=1000};
        snprintf(path,sizeof(path),"build/games/alley_ninja/stage%d-state%d.ppm",stage,state);
        if(!save(&g,path))return 1;
    }
    return 0;
}
