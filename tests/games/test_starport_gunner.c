#include "sg_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t frame[320*240],strips[320*240+2];
static int active_shots(const sg_game_t *g){int n=0;for(int i=0;i<SG_SHOTS;i++)n+=g->shots[i].active;return n;}
static void tap(sg_game_t *g,int key,uint32_t now){sg_edge(g,key,1,now);sg_edge(g,key,0,now);}
static void inputs(void){
    sg_game_t g;sg_init(&g,0,4);tap(&g,1,0);assert(g.phase==SG_PLAY&&g.health==3);
    tap(&g,0,20);assert(g.aim==3);sg_update(&g,400);assert(g.aim==3);
    sg_edge(&g,2,1,420);sg_update(&g,1220);assert(g.phase==SG_PAUSED&&g.aim==3);
    sg_edge(&g,2,0,1240);assert(g.aim==3);
    uint32_t elapsed=g.elapsed_ms;sg_update(&g,2000);assert(g.elapsed_ms==elapsed);
    tap(&g,1,2020);assert(g.phase==SG_PLAY&&!g.charging);
    sg_edge(&g,1,1,2040);sg_update(&g,3540);assert(g.charge_ms==1200&&active_shots(&g)==0);
    sg_edge(&g,1,0,3560);assert(active_shots(&g)==1&&g.cooldown_ms==SG_COOLDOWN);
    int power=0;for(int i=0;i<SG_SHOTS;i++)if(g.shots[i].active)power=g.shots[i].power;
    assert(power==3);sg_update(&g,3780);tap(&g,1,3780);assert(active_shots(&g)==2);
    sg_edge(&g,1,1,3800);sg_cancel(&g,4000);assert(g.phase==SG_PAUSED&&!g.charging);
    sg_edge(&g,1,0,4020);tap(&g,1,4040);assert(g.phase==SG_PLAY&&!g.charging);
    sg_init(&g,0,1);sg_start(&g);sg_update(&g,1200);
    sg_edge(&g,2,1,100);sg_edge(&g,2,0,140);assert(g.phase==SG_PLAY&&g.aim==5);
    sg_init(&g,UINT32_MAX-100,1);sg_start(&g);sg_edge(&g,1,1,UINT32_MAX-50);sg_edge(&g,1,0,600);assert(g.cooldown_ms==SG_COOLDOWN);
    sg_init(&g,0,1);sg_start(&g);sg_edge(&g,0,1,0);sg_edge(&g,1,1,0);sg_edge(&g,1,0,20);assert(!g.charging&&!active_shots(&g));
    sg_edge(&g,0,0,40);assert(g.aim==3);g.best=500;g.unlocked=4;g.phase=SG_FAILED;tap(&g,1,60);assert(g.best==500&&g.unlocked==4);
}
static void collisions(void){
    assert(sg_segment_hits(160,190,160,70,148,90,172,106));
    assert(!sg_segment_hits(120,190,120,70,148,90,172,106));
    sg_game_t g;sg_init(&g,0,1);sg_start(&g);g.spawn_ms=9999;
    g.enemies[0]=(sg_enemy_t){.active=1,.x=160,.y=90,.hp=2,.max_hp=2,.attack_ms=9999};
    g.shots[0]=(sg_shot_t){.active=1,.x=160,.y=115,.dy=-40,.power=3,.pierce=3};
    sg_update(&g,20);assert(g.kills==1&&!g.enemies[0].active);
    g.enemies[1]=(sg_enemy_t){.active=1,.x=160,.y=60,.hp=5,.max_hp=5,.attack_ms=9999};
    g.shots[0]=(sg_shot_t){.active=1,.x=160,.y=69,.dy=-2,.power=3,.pierce=3};
    sg_update(&g,40);assert(g.enemies[1].hp==2);sg_update(&g,60);assert(g.enemies[1].hp==2);
    g.threats[0]=(sg_threat_t){1,160,160,500};g.shots[2]=(sg_shot_t){.active=1,.x=160,.y=180,.dy=-25,.power=1,.pierce=1};
    sg_update(&g,80);assert(!g.threats[0].active&&g.score==125);
    g.threats[0]=(sg_threat_t){1,100,197,0};g.threats[1]=(sg_threat_t){1,200,197,0};
    sg_update(&g,100);assert(g.health==2&&g.damage_ms==700);
    g.health=1;g.damage_ms=0;g.threats[0]=(sg_threat_t){1,100,197,0};sg_update(&g,120);assert(g.phase==SG_FAILED);
}
static unsigned fh(void){unsigned h=2166136261u;for(int i=0;i<320*240;i++){h=(h^(frame[i]&255))*16777619u;h=(h^(frame[i]>>8))*16777619u;}return h;}
static void render(void){
    sg_game_t g;
    for(int stage=1;stage<=5;stage++)for(int phase=0;phase<=4;phase++){
        sg_init(&g,0,7);g.stage=stage;sg_start(&g);g.phase=(sg_phase_t)phase;
        g.enemies[0]=(sg_enemy_t){1,170,88,16,16,2,1,400,1};g.threats[0]=(sg_threat_t){1,122,125,600};
        g.charging=1;g.charge_ms=700;sg_render_strip(&g,frame,0,240);
        const int sizes[]={8,16,24,32,40};
        for(int i=0;i<5;i++){
            strips[0]=strips[320*240+1]=0xF00D;
            for(int y=0;y<240;y+=sizes[i])sg_render_strip(&g,strips+1+y*320,y,240-y<sizes[i]?240-y:sizes[i]);
            assert(strips[0]==0xF00D&&strips[320*240+1]==0xF00D);assert(!memcmp(frame,strips+1,sizeof(frame)));
        }
    }
    strips[0]=0xABCD;sg_render_strip(&g,strips,-1,1);sg_render_strip(&g,strips,239,2);sg_render_strip(&g,strips,1,2147483647);assert(strips[0]==0xABCD);
}
static int ab(int x){return x<0?-x:x;}
/* A deterministic pilot uses only legal single-button edges and aims at the
 * projected target position. This tests all five complete win paths. */
static void campaigns(void){
    for(unsigned seed=1;seed<=8;seed++)for(int stage=1;stage<=5;stage++){
        sg_game_t g;sg_init(&g,0,seed);g.stage=stage;g.unlocked=stage;tap(&g,1,0);
        int holding=0;uint32_t release=0,next_action=0;
        for(uint32_t now=20;now<=180000&&g.phase==SG_PLAY;now+=20){
            sg_update(&g,now);if(g.phase!=SG_PLAY)break;
            if(holding){if(now>=release){sg_edge(&g,1,0,now);holding=0;}continue;}
            if(now<next_action)continue;
            int tx=0,ty=0,dir=0,speed=0,boss=0;
            for(int i=0;i<SG_THREATS;i++)if(g.threats[i].active&&g.threats[i].y>125){tx=g.threats[i].x;ty=g.threats[i].y;break;}
            if(!tx)for(int i=0;i<SG_ENEMIES;i++)if(g.enemies[i].active){
                tx=g.enemies[i].x;ty=g.enemies[i].y;dir=g.enemies[i].dir;speed=g.stage>=2?(g.stage>=4?2:4):0;boss=g.enemies[i].kind==2;break;
            }
            if(!tx)continue;
            int aim=4,best=10000;
            for(int a=0;a<=8;a++){
                int ticks=(189-ty)/(-sg_aim_dy(a));int predicted=tx+(speed?dir*ticks/speed:0);
                int delta=ab(160+sg_aim_dx(a)*ticks-predicted);if(delta<best){best=delta;aim=a;}
            }
            if(g.aim!=aim){tap(&g,g.aim<aim?2:0,now);next_action=now+100;continue;}
            if(g.shot_ms==0){sg_edge(&g,1,1,now);holding=1;release=now+(boss&&g.cooldown_ms==0?500:40);}
        }
        if(g.phase!=SG_CLEAR){fprintf(stderr,"pilot failed seed=%u stage=%d kills=%d hp=%d time=%u\n",seed,stage,g.kills,g.health,g.elapsed_ms);}
        assert(g.phase==SG_CLEAR&&g.kills==g.goal&&g.best==g.score);
        assert(g.unlocked==(stage<5?stage+1:5));
        sg_edge(&g,1,0,g.last_ms+20);tap(&g,1,g.last_ms+40);assert(g.phase==SG_PLAY&&g.health==3&&g.score==0);
    }
}
static void replay(void){
    sg_game_t g;sg_init(&g,0,0x57A2);
    for(int i=0;i<2000;i++){
        uint32_t now=(uint32_t)i*20;
        if(i%90==1)sg_edge(&g,1,1,now);if(i%90==40)sg_edge(&g,1,0,now);
        if(i==500)sg_edge(&g,2,1,now);if(i==545)sg_edge(&g,2,0,now);if(i==1000)sg_cancel(&g,now);
        sg_update(&g,now);
        if(i%100==0||i==1999){sg_render_strip(&g,frame,0,240);printf("%d %u %u\n",i,sg_hash(&g),fh());}
    }
}
int main(int argc,char **argv){if(argc>1&&!strcmp(argv[1],"replay")){replay();return 0;}inputs();collisions();render();campaigns();printf("Starport Gunner: PASS (40 complete stages; strip/input/collision checks; state %zu bytes)\n",sizeof(sg_game_t));return 0;}
