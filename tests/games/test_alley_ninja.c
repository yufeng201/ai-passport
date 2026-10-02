#include "an_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t frame[320*240],strips[320*240+2];
static void tap(an_game_t *g,int key,uint32_t now){an_edge(g,key,1,now);an_edge(g,key,0,now);}
static void target(an_game_t *g,int side,int hp,int kind,int warning){
    g->spawn_ms=9999;g->enemy=(an_enemy_t){.active=1,.x=160+side*36,.side=side,.hp=hp,.max_hp=hp,.kind=kind,.phase=AN_WINDUP,.timer_ms=warning,.warning_ms=warning};
}
static void inputs(void){
    an_game_t g;an_init(&g,0,1);tap(&g,1,0);assert(g.phase==AN_PLAY);
    target(&g,1,1,0,1000);tap(&g,0,20);assert(g.enemy.hp==1&&g.feedback==4);
    g.cooldown_ms=0;g.enemy.x=250;tap(&g,2,40);assert(g.enemy.hp==1&&g.feedback==4);
    g.cooldown_ms=0;g.enemy.x=196;tap(&g,2,60);assert(!g.enemy.active&&g.kills==1);
    target(&g,1,2,1,1000);g.cooldown_ms=0;tap(&g,2,80);assert(g.enemy.hp==2&&g.feedback==3);
    an_edge(&g,2,1,100);an_update(&g,900);assert(g.phase==AN_PAUSED&&g.enemy.hp==2);an_edge(&g,2,0,920);assert(g.enemy.hp==2);
    uint32_t elapsed=g.elapsed_ms;int timer=g.enemy.timer_ms;an_update(&g,1400);assert(g.elapsed_ms==elapsed&&g.enemy.timer_ms==timer);
    tap(&g,1,1420);assert(g.phase==AN_PLAY&&!g.guarding);
    an_edge(&g,1,1,1440);assert(g.guarding);an_cancel(&g,1460);assert(g.phase==AN_PAUSED&&!g.guarding);
    an_edge(&g,1,0,1480);tap(&g,1,1500);assert(g.phase==AN_PLAY&&!g.guarding);
    an_init(&g,0,1);an_start(&g);an_update(&g,1200);an_edge(&g,2,1,100);an_edge(&g,2,0,140);assert(g.phase==AN_PLAY&&g.slash_side==1);
    an_init(&g,UINT32_MAX-100,1);an_start(&g);an_edge(&g,2,1,UINT32_MAX-50);an_edge(&g,2,0,800);assert(g.phase==AN_PAUSED);
    an_init(&g,0,1);an_start(&g);an_edge(&g,1,1,0);an_edge(&g,2,1,20);an_edge(&g,2,0,40);assert(g.guarding&&g.slash_ms==0);
    an_edge(&g,1,0,60);g.best=777;g.unlocked=4;g.phase=AN_FAILED;tap(&g,1,80);assert(g.best==777&&g.unlocked==4&&g.health==3&&g.score==0);
}
static void guard(void){
    an_game_t g;an_init(&g,0,1);an_start(&g);target(&g,1,2,1,120);
    an_edge(&g,1,1,0);for(int i=20;i<=120;i+=20)an_update(&g,(uint32_t)i);
    assert(g.health==3&&g.feedback==1&&g.combo==1&&g.enemy.vulnerable_ms==1000);
    an_edge(&g,1,0,140);tap(&g,2,160);assert(g.enemy.hp==1);tap(&g,2,180);assert(g.enemy.hp==1);
    tap(&g,2,420);assert(!g.enemy.active&&g.kills==1);
    an_init(&g,0,1);an_start(&g);target(&g,-1,2,1,500);an_edge(&g,1,1,0);
    for(int i=20;i<=500;i+=20)an_update(&g,(uint32_t)i);
    assert(g.health==3&&g.feedback==2&&g.combo==0&&g.enemy.vulnerable_ms>0&&g.stamina<80);
    an_init(&g,0,1);an_start(&g);g.spawn_ms=9999;an_edge(&g,1,1,0);
    for(int i=20;i<=5000;i+=20)an_update(&g,(uint32_t)i);
    assert(!g.guarding&&g.stamina<100); /* A held depleted guard cannot auto-rearm. */
    an_edge(&g,1,0,5020);for(int i=5040;i<=5600;i+=20)an_update(&g,(uint32_t)i);an_edge(&g,1,1,5620);assert(g.guarding);
    an_init(&g,0,1);an_start(&g);target(&g,1,1,0,20);g.stamina=24;an_edge(&g,1,1,0);g.stamina=10;an_update(&g,20);
    assert(g.health==2&&g.broken_ms==800&&!g.guarding);
    an_init(&g,0,1);an_start(&g);target(&g,1,2,1,20);g.score=999999;g.combo=99;an_edge(&g,1,1,0);an_update(&g,20);assert(g.score==1000000&&g.combo==99);
    an_init(&g,0,1);an_start(&g);g.health=1;target(&g,1,1,0,20);an_update(&g,20);assert(g.phase==AN_FAILED);
}
static unsigned fh(void){unsigned h=2166136261u;for(int i=0;i<320*240;i++){h=(h^(frame[i]&255))*16777619u;h=(h^(frame[i]>>8))*16777619u;}return h;}
static void render(void){
    an_game_t g;for(int stage=1;stage<=5;stage++)for(int p=0;p<=4;p++){
        an_init(&g,0,1);g.stage=stage;an_start(&g);target(&g,stage%2?1:-1,5,2,600);g.phase=(an_phase_t)p;g.guarding=1;g.slash_ms=100;g.slash_side=-1;
        an_render_strip(&g,frame,0,240);const int sizes[]={8,16,24,32,40};
        for(int i=0;i<5;i++){
            strips[0]=strips[320*240+1]=0xF00D;
            for(int y=0;y<240;y+=sizes[i])an_render_strip(&g,strips+1+y*320,y,240-y<sizes[i]?240-y:sizes[i]);
            assert(strips[0]==0xF00D&&strips[320*240+1]==0xF00D&&!memcmp(frame,strips+1,sizeof(frame)));
        }
    }
    strips[0]=0xABCD;an_render_strip(&g,strips,-1,1);an_render_strip(&g,strips,239,2);an_render_strip(&g,strips,1,2147483647);assert(strips[0]==0xABCD);
}
static void campaigns(void){
    for(unsigned seed=1;seed<=16;seed++)for(int stage=1;stage<=5;stage++)for(int perfect=0;perfect<=1;perfect++){
        an_game_t g;an_init(&g,0,seed);g.stage=stage;g.unlocked=stage;tap(&g,1,0);
        int pending_side=0;uint32_t attack_at=0;
        for(uint32_t now=20;now<=180000&&g.phase==AN_PLAY;now+=20){
            an_update(&g,now);if(g.phase!=AN_PLAY)break;
            if(g.enemy.active&&g.enemy.phase==AN_WINDUP&&!g.guarding&&g.stamina>=40&&g.enemy.timer_ms<=(perfect?120:420))an_edge(&g,1,1,now);
            if(g.guarding&&g.enemy.phase==AN_RECOVER){pending_side=g.enemy.side;an_edge(&g,1,0,now);attack_at=now+100;}
            if(pending_side&&now>=attack_at&&g.cooldown_ms==0){tap(&g,pending_side<0?0:2,now);attack_at=now+300;if(!g.enemy.active||g.enemy.vulnerable_ms<=0)pending_side=0;}
        }
        if(g.phase!=AN_CLEAR)fprintf(stderr,"failed seed=%u stage=%d perfect=%d hp=%d kills=%d\n",seed,stage,perfect,g.health,g.kills);
        assert(g.phase==AN_CLEAR&&g.kills==g.goal&&g.best==g.score&&g.health==3);
        assert(g.unlocked==(stage<5?stage+1:5));an_edge(&g,1,0,g.last_ms+20);tap(&g,1,g.last_ms+40);assert(g.phase==AN_PLAY&&g.health==3&&g.stamina==100);
    }
}
static void replay(void){
    an_game_t g;an_init(&g,0,0xA11E);
    for(int i=0;i<2000;i++){
        uint32_t now=(uint32_t)i*20;
        if(i%90==1)an_edge(&g,1,1,now);if(i%90==40)an_edge(&g,1,0,now);
        if(i==500)an_edge(&g,2,1,now);if(i==545)an_edge(&g,2,0,now);if(i==1000)an_cancel(&g,now);an_update(&g,now);
        if(i%100==0||i==1999){an_render_strip(&g,frame,0,240);printf("%d %u %u\n",i,an_hash(&g),fh());}
    }
}
static void upgrade_tests(void){
 an_game_t g;an_init(&g,0,1);an_start(&g);target(&g,1,3,2,120);g.combo=1;
 an_edge(&g,1,1,0);an_update(&g,120);assert(g.riposte_ms==1000&&g.combo==2);
 an_edge(&g,1,0,140);tap(&g,0,160);assert(g.enemy.hp==3&&g.riposte_ms>0);
 tap(&g,2,420);assert(g.enemy.hp==1&&g.riposte_ms==0);
 an_start(&g);assert(g.riposte_ms==0);
}
int main(int argc,char **argv){upgrade_tests();if(argc>1&&!strcmp(argv[1],"replay")){replay();return 0;}inputs();guard();render();campaigns();printf("Alley Ninja: PASS (160 complete stages, normal/perfect guard, strips/input; state %zu bytes)\n",sizeof(an_game_t));return 0;}
