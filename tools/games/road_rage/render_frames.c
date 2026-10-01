/* Offline visual review of the shared renderer, not browser/device acceptance. */
#include "rr_render.h"
#include <stdio.h>
static uint16_t pixels[320*240];
static void save(rr_game_t *g,const char *name)
{
    rr_render_strip(g,pixels,0,240);
    FILE *file=fopen(name,"wb");
    if(!file)return;
    fprintf(file,"P6\n320 240\n255\n");
    for(int i=0;i<320*240;++i){
        unsigned char rgb[]={((pixels[i]>>11)&31)*255/31,((pixels[i]>>5)&63)*255/63,(pixels[i]&31)*255/31};
        fwrite(rgb,1,3,file);
    }
    fclose(file);
}
int main(void)
{
    rr_game_t g;rr_init(&g,0xD057);save(&g,"build/games/road_rage/title.ppm");
    rr_input(&g,RR_ACTION);
    for(int i=0;i<200;i++)rr_tick(&g,20);
    g.entities[0]=(rr_entity_t){.active=1,.lane=1,.depth=870,.color=1};
    g.entities[1]=(rr_entity_t){.active=1,.lane=-1,.depth=490,.car=1};
    g.entities[2]=(rr_entity_t){.active=1,.lane=0,.depth=290,.color=2};
    save(&g,"build/games/road_rage/race.ppm");
    for(int stage=1;stage<=RR_STAGES;++stage) {
        char name[80];g.stage=stage;
        snprintf(name,sizeof(name),"build/games/road_rage/stage%d.ppm",stage);save(&g,name);
    }
    g.stage=1;
    rr_input(&g,RR_ACTION);save(&g,"build/games/road_rage/strike.ppm");
    rr_input(&g,RR_PAUSE);save(&g,"build/games/road_rage/pause.ppm");
    g.phase=RR_FINISHED;g.score=2680;g.knockouts=8;g.overtakes=15;g.metres_mm=RR_FINISH_METRES*1000;
    save(&g,"build/games/road_rage/finish.ppm");
    g.phase=RR_WRECKED;g.health=0;save(&g,"build/games/road_rage/wreck.ppm");
    return 0;
}
