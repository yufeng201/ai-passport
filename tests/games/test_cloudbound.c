#include "cb_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t full[CB_WIDTH*CB_HEIGHT],strips[CB_WIDTH*CB_HEIGHT+2];
static void advance(cb_game_t *g,uint32_t *now,int ms){while(ms>0){int n=ms>20?20:ms;*now+=(uint32_t)n;cb_update(g,*now);ms-=n;}}
static void tap(cb_game_t *g,int key,uint32_t *now){cb_edge(g,key,1,*now);advance(g,now,40);cb_edge(g,key,0,*now);}
static int choose_hold(const cb_game_t *g){
    int chosen=-1,error=999;
    for(int t=100;t<=1200;t++){int x=cb_landing_x(g,t),d=x-(g->target.x+g->target.w/2);if(d<0)d=-d;
        if(cb_can_land(g,t)&&d<error){chosen=t;error=d;}}
    return chosen;
}
static void input_tests(void){
    cb_game_t g;uint32_t now=0;cb_init(&g,now,1);tap(&g,1,&now);assert(g.phase==CB_PLAY);
    cb_edge(&g,1,1,now);advance(&g,&now,2000);assert(g.charging&&g.charge_ms==1200&&!g.flying);
    cb_edge(&g,1,0,now);assert(g.flying&&g.jump_dx==160);
    cb_cancel(&g,now);assert(g.phase==CB_PAUSED&&!g.charging);
    int flight=g.flight_ms;advance(&g,&now,1000);assert(g.flight_ms==flight);
    tap(&g,1,&now);assert(g.phase==CB_PLAY&&g.flying);
    cb_start(&g);cb_edge(&g,1,1,now);cb_edge(&g,1,1,now+100);cb_edge(&g,1,0,now+500);
    assert(g.jump_dx==cb_jump_distance(500));now+=500;
    cb_start(&g);cb_edge(&g,1,1,now);cb_cancel(&g,now+80);now+=80;
    cb_edge(&g,1,0,now);assert(g.phase==CB_PAUSED&&!g.flying);
    tap(&g,1,&now);assert(g.phase==CB_PLAY&&!g.flying&&!g.charging);
    cb_edge(&g,2,1,now);advance(&g,&now,800);assert(g.phase==CB_PAUSED);
    cb_edge(&g,2,0,now);assert(g.phase==CB_PAUSED);tap(&g,1,&now);assert(g.phase==CB_PLAY);
    cb_cancel(&g,now);g.stage=2;g.unlocked=3;
    cb_edge(&g,0,1,now);advance(&g,&now,800);assert(g.phase==CB_TITLE);
    cb_edge(&g,0,0,now);assert(g.stage==2); /* Long release must not select stage 1. */
    cb_edge(&g,0,1,now);advance(&g,&now,800);cb_edge(&g,0,0,now);assert(g.muted==1&&g.stage==2);
    tap(&g,2,&now);assert(g.stage==3);tap(&g,2,&now);assert(g.stage==3);
    cb_start(&g);cb_update(&g,now+100);uint32_t before=g.elapsed_ms;
    cb_edge(&g,1,1,now+95);assert(g.elapsed_ms==before);cb_edge(&g,1,0,now+100);
    assert(g.jump_dx==cb_jump_distance(5)); /* Stale queue edges do not simulate a wrap. */
    /* Both short edges can arrive after a frame sampled a much later clock. */
    cb_init(&g,0,9);cb_start(&g);cb_update(&g,1200);
    cb_edge(&g,2,1,100);cb_edge(&g,2,0,140);
    assert(g.phase==CB_PLAY&&!g.held); /* A 40ms C tap must not become long pause. */
    now=UINT32_MAX-400;cb_init(&g,now,7);tap(&g,1,&now);cb_edge(&g,1,1,now);
    advance(&g,&now,700);cb_edge(&g,1,0,now);assert(g.flying&&g.jump_dx==cb_jump_distance(700));
    cb_start(&g);cb_edge(&g,1,1,now);cb_edge(&g,2,1,now);assert(g.held==2);cb_edge(&g,2,0,now);assert(g.held==2);
    cb_edge(&g,1,0,now);assert(!g.held);
}
static void campaign_tests(void){
    for(unsigned seed=1;seed<=64;seed++){
        cb_game_t g;uint32_t now=0;cb_init(&g,now,seed);tap(&g,1,&now);
        for(int stage=1;stage<=5;stage++){
            assert(g.stage==stage);
            for(int p=0;p<CB_GOAL;p++){
                int run=0,longest=0;
                for(int t=100;t<=1200;t++){run=cb_can_land(&g,t)?run+1:0;if(run>longest)longest=run;}
                assert(longest>=150&&g.target.x>g.current.x+g.current.w);
                assert(g.target.x+g.target.w<=288&&g.target.y>=135&&g.target.y<=185);
                int hold=choose_hold(&g);assert(hold>=100);
                cb_edge(&g,1,1,now);advance(&g,&now,hold);cb_edge(&g,1,0,now);
                /* Pressing during flight is never buffered into another jump. */
                cb_edge(&g,1,1,now);advance(&g,&now,CB_FLIGHT_MS+300);cb_edge(&g,1,0,now);
                assert(g.landings==p+1&&!g.flying);
                if(p<CB_GOAL-1)assert(g.phase==CB_PLAY&&g.x==70);
            }
            assert(g.phase==CB_CLEAR&&g.unlocked==(stage<5?stage+1:5)&&g.score>0);
            int best=g.best;tap(&g,1,&now);assert(g.phase==CB_PLAY&&g.best==best&&g.landings==0);
        }
    }
    cb_game_t g;uint32_t now=0;cb_init(&g,now,19);g.stage=g.unlocked=3;g.best=500;tap(&g,1,&now);
    cb_edge(&g,1,1,now);advance(&g,&now,100);cb_edge(&g,1,0,now);advance(&g,&now,600);
    assert(g.phase==CB_FAILED&&g.best==500&&g.unlocked==3);tap(&g,1,&now);assert(g.stage==3&&g.unlocked==3);
    g.phase=CB_CLEAR;cb_edge(&g,1,1,now);advance(&g,&now,800);assert(g.phase==CB_TITLE);
    cb_edge(&g,1,0,now);assert(g.phase==CB_TITLE);
}
static void geometry_tests(void){
    cb_game_t g;cb_init(&g,0,1);cb_start(&g);g.target=(cb_platform_t){70+cb_jump_distance(500)-4,152,40,16};
    assert(cb_can_land(&g,500));g.jump_rise=18;assert(cb_jump_y(&g,0)==170&&cb_jump_y(&g,600)==152);
    assert(cb_jump_y(&g,300)<152);
    g.target.x=70+cb_jump_distance(500)-36;assert(!cb_can_land(&g,500));
}
static void rendering_tests(void){
    cb_game_t g;
    for(int stage=1;stage<=5;stage++)for(int phase=0;phase<=4;phase++){
        cb_init(&g,0,17);g.stage=stage;cb_start(&g);g.phase=(cb_phase_t)phase;
        g.scene_ms=3780;g.battery=75;g.charging=phase==CB_PLAY;g.charge_ms=870;
        cb_render_strip(&g,full,0,CB_HEIGHT);
        for(int rows=8;rows<=40;rows+=8){
            strips[0]=strips[CB_WIDTH*CB_HEIGHT+1]=0xA55A;
            for(int y=0;y<CB_HEIGHT;y+=rows)cb_render_strip(&g,strips+1+y*CB_WIDTH,y,rows);
            assert(strips[0]==0xA55A&&strips[CB_WIDTH*CB_HEIGHT+1]==0xA55A);
            assert(!memcmp(full,strips+1,sizeof(full)));
        }
        uint16_t guard=0xAA55;cb_render_strip(&g,&guard,-1,1);assert(guard==0xAA55);
    }
}
static void replay(void){
    cb_game_t g;cb_init(&g,0,0xC10D);
    for(int i=0;i<2000;i++){
        uint32_t now=(uint32_t)i*20;
        if(i%90==1)cb_edge(&g,1,1,now);
        if(i%90==40)cb_edge(&g,1,0,now);
        if(i==500)cb_edge(&g,2,1,now);
        if(i==545)cb_edge(&g,2,0,now);
        if(i==1000)cb_cancel(&g,now);
        cb_update(&g,now);
        if(i%100==0||i==1999){
            cb_render_strip(&g,full,0,CB_HEIGHT);uint32_t h=2166136261u;
            for(int p=0;p<CB_WIDTH*CB_HEIGHT;p++){h=(h^(full[p]&255))*16777619u;h=(h^(full[p]>>8))*16777619u;}
            printf("%d %u %u\n",i,cb_hash(&g),h);
        }
    }
}
static void upgrade_tests(void){
 cb_game_t g;cb_init(&g,0,1);cb_start(&g);g.phase=CB_FAILED;g.landings=4;g.score=500;g.combo=2;g.x=250;g.flying=0;
 cb_edge(&g,0,1,20);cb_edge(&g,0,0,60);
 assert(g.phase==CB_PLAY&&g.rescued&&g.landings==4&&g.score==500&&g.combo==0&&g.x==70&&g.y==g.current.y);
 g.phase=CB_FAILED;cb_edge(&g,0,1,80);cb_edge(&g,0,0,120);assert(g.phase==CB_FAILED);
 cb_edge(&g,1,1,140);cb_edge(&g,1,0,180);assert(g.phase==CB_PLAY&&!g.rescued&&g.landings==0);
}
int main(int argc,char **argv){upgrade_tests();
    if(argc>1&&!strcmp(argv[1],"replay")){replay();return 0;}
    input_tests();campaign_tests();geometry_tests();rendering_tests();
    printf("Cloudbound: PASS (64 five-stage campaigns, reachability, input timing, pause/reset, geometry, strips); state=%zu bytes\n",sizeof(cb_game_t));
}
