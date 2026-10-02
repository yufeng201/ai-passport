#include "rr_game.h"
#include "rr_render.h"
#include "game_audio.h"
#include "rr_controls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint16_t full[RR_WIDTH*RR_HEIGHT], strips[RR_WIDTH*RR_HEIGHT+2];

/* Controlled input sequences cover all state transitions and exact attack/collision limits. */
static void logic_tests(void)
{
    rr_game_t g;
    rr_init(&g,0);
    assert(g.phase == RR_TITLE && g.battery == -1);
    rr_input(&g,RR_LEFT); assert(g.lane == 0);
    rr_input(&g,RR_ACTION); assert(g.phase == RR_RACING);
    for (int i = 0; i < 10; ++i) rr_input(&g,RR_LEFT);
    assert(g.lane == -1); rr_tick(&g,160); assert(g.lane_q8 == -256);
    rr_input(&g,RR_PAUSE); uint32_t before = g.elapsed_ms;
    rr_tick(&g,200); assert(g.elapsed_ms == before && g.phase == RR_PAUSED);
    rr_input(&g,RR_ACTION); assert(g.phase == RR_RACING);
    g.entities[0] = (rr_entity_t){ .active=1,.lane=0,.depth=880 };
    rr_input(&g,RR_ACTION); assert(g.knockouts == 1 && g.entities[0].active == 2 && g.attack_side == 1);
    g.entities[1] = (rr_entity_t){ .active=1,.lane=0,.depth=880 };
    rr_input(&g,RR_ACTION); assert(g.knockouts == 1); /* cooldown */
    rr_tick(&g,250); rr_tick(&g,250);
    g.entities[1] = (rr_entity_t){ .active=1,.lane=0,.depth=900,.car=1 };
    rr_input(&g,RR_ACTION); assert(g.knockouts == 1 && g.entities[1].active);
    g.entities[0] = (rr_entity_t){ .active=1,.lane=-1,.depth=925,.car=1 };
    rr_tick(&g,20); assert(g.health == 70 && g.hurt_ms > 0);
    g.entities[0] = (rr_entity_t){ .active=1,.lane=-1,.depth=925,.car=1 };
    rr_tick(&g,20); assert(g.health == 70); /* damage grace window */
    g.hurt_ms=0;g.health=20;
    rr_tick(&g,20); assert(g.phase == RR_WRECKED && g.health == 0);
    before = g.elapsed_ms; rr_tick(&g,200); assert(g.elapsed_ms == before);
    rr_input(&g,RR_ACTION); assert(g.health == 100 && g.metres_mm == 0);
    g.metres_mm=RR_FINISH_METRES*1000-1;rr_tick(&g,20);
    assert(g.phase == RR_FINISHED && g.metres_mm == RR_FINISH_METRES*1000);
    rr_input(&g,RR_ACTION); assert(g.phase == RR_RACING && g.knockouts == 0);
    g.battery=72;rr_input(&g,RR_HOME);assert(g.phase == RR_TITLE && g.battery == 72);
    rr_input(&g,RR_ACTION);before=g.elapsed_ms;rr_tick(&g,100000);
    assert(g.elapsed_ms-before == 240); /* stalled clocks have bounded catch-up */
    assert(game_audio_sample(1,3200)==0 && game_audio_sample(0,20)==0);
}

/* Regressions for immediate release, repeated taps and long holds. */
static void control_tests(void)
{
    rr_action_key_t key={0};
    assert(rr_action_edge(&key,1,100)==-1);
    assert(rr_action_edge(&key,0,150)==RR_ACTION);
    assert(rr_action_edge(&key,1,180)==-1);
    assert(rr_action_edge(&key,0,220)==RR_ACTION); /* no double-click wait */
    assert(rr_action_edge(&key,0,221)==-1); /* no duplicate release */
    assert(rr_action_edge(&key,1,300)==-1);
    assert(rr_action_poll(&key,799)==-1);
    assert(rr_action_poll(&key,800)==RR_PAUSE);
    assert(rr_action_poll(&key,900)==-1);
    assert(rr_action_edge(&key,0,950)==-1); /* no attack after long hold */
    assert(rr_action_edge(&key,1,1000)==-1);
    assert(rr_action_edge(&key,0,1500)==RR_PAUSE); /* release before next poll */
    assert(rr_action_edge(&key,1,UINT32_MAX-100)==-1);
    assert(rr_action_edge(&key,0,10)==RR_ACTION);
}
static void geometry_tests(void)
{
    uint32_t m=game_medal_record(0,5,3);assert(m==768&&game_medal_count(m,5)==3);assert(game_medal_record(m,5,1)==m&&game_medal_record(m,0,3)==m&&game_medal_record(m,6,3)==m);
 rr_game_t g;rr_init(&g,1);rr_input(&g,RR_HOME);assert(g.muted);rr_input(&g,RR_ACTION);assert(g.muted);
    g.elapsed_ms=5000;g.scenery_ms=9600;g.lane=0;g.lane_q8=160;
    g.entities[0]=(rr_entity_t){.active=1,.lane=0,.depth=920};
    rr_tick(&g,20);
    assert(g.health==100 && !rr_contact(&g,&g.entities[0])); /* visible gap */
    rr_init(&g,1);rr_input(&g,RR_ACTION);g.elapsed_ms=5000;
    g.lane=0;g.lane_q8=160;
    g.entities[0]=(rr_entity_t){.active=1,.lane=0,.depth=1045};
    rr_tick(&g,100);assert(g.health==80); /* visible overlap after old cutoff */
    rr_init(&g,1);rr_input(&g,RR_ACTION);g.lane=1;g.lane_q8=96;
    g.entities[0]=(rr_entity_t){.active=1,.lane=1,.depth=880};
    assert(rr_attackable(&g,&g.entities[0])); /* inclusive 160 edge */
    rr_input(&g,RR_ACTION);assert(g.knockouts==1);
    g.entities[1]=(rr_entity_t){.active=1,.lane=1,.depth=880};
    assert(!rr_attackable(&g,&g.entities[1])); /* cooldown hides cue */
    rr_input(&g,RR_ACTION);assert(g.knockouts==1);
    g.cooldown_ms=0;g.entities[1].depth=780;
    assert(!rr_attackable(&g,&g.entities[1])); /* beyond fist height */
    g.entities[1].depth=880;g.entities[1].car=1;
    assert(!rr_attackable(&g,&g.entities[1]));
    rr_init(&g,1);rr_input(&g,RR_ACTION);
    g.entities[0]=(rr_entity_t){.active=1,.car=1,.lane=0,.depth=1100};
    rr_render_strip(&g,full,0,240);
    unsigned car_body=((116>>3)<<11)|((138>>2)<<5)|(172>>3);
    assert(full[205*320+160]==car_body); /* foreground car covers player */
}

/* Complete seeded races using only legal discrete lane/attack inputs. This is a
 * simulation regression, not a substitute for a human browser/device playtest. */
static void complete_races(void)
{
    for (uint32_t seed=1;seed<=16;++seed) {
        rr_game_t g;rr_init(&g,seed);
        rr_input(&g,RR_RIGHT);assert(g.stage==1); /* locked selection */
        rr_input(&g,RR_ACTION);g.rng=seed;
        for(int stage=1;stage<=RR_STAGES;++stage) {
            assert(g.stage==stage && g.health==100 && g.metres_mm==0);
            for(int frame=0;frame<7000 && g.phase==RR_RACING;++frame) {
                /* Predict contact during a legal 400 ms path, including the
                 * interpolated lane position, rather than the old depth gate. */
                int best=-1,best_health=-1;
                for(int choice=0;choice<3;++choice) {
                    rr_game_t future=g;
                    if(choice==1)rr_input(&future,RR_LEFT);
                    if(choice==2)rr_input(&future,RR_RIGHT);
                    rr_tick(&future,200);rr_tick(&future,200);
                    if(future.health>best_health) { best_health=future.health;best=choice; }
                }
                if(best==1)rr_input(&g,RR_LEFT);
                if(best==2)rr_input(&g,RR_RIGHT);
                rr_tick(&g,20); /* No attacks: combat never gates advancement. */
                assert(g.health>=0 && g.health<=100 && g.lane>=-1 && g.lane<=1);
            }
            assert(g.phase==RR_FINISHED);
            assert(g.metres_mm==rr_stage(&g)->metres*1000 && g.score>0);
            assert(g.knockouts==0 && rr_stars(&g)>=1);
            assert(g.unlocked==(stage<RR_STAGES ? stage+1 : RR_STAGES));
            if(stage<RR_STAGES)rr_input(&g,RR_ACTION);
        }
        rr_input(&g,RR_HOME);assert(g.unlocked==5 && g.stage==5 && g.phase==RR_TITLE);
        rr_input(&g,RR_LEFT);assert(g.stage==4);
        rr_input(&g,RR_ACTION);g.phase=RR_WRECKED;rr_input(&g,RR_ACTION);
        assert(g.stage==4 && g.health==100 && g.phase==RR_RACING);
    }
}

/* A full draw must exactly match strips, and strip buffers retain guard words. */
static void render_tests(void)
{
    rr_game_t g;
    for (int stage=1;stage<=RR_STAGES;++stage)
    for (int phase = RR_TITLE; phase <= RR_WRECKED; ++phase) {
        rr_init(&g,0xD057);g.phase=(rr_phase_t)phase;g.battery=100;g.stage=stage;
        g.entities[0]=(rr_entity_t){.active=1,.depth=900,.lane=1};
        g.attack_ms=200;g.attack_side=1;
        rr_render_strip(&g,full,0,240);
        strips[0]=0xABCD;strips[320*240+1]=0x1234;
        const int sizes[]={8,24,40};
        for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) {
            for (int y=0;y<240;y+=sizes[i]) rr_render_strip(&g,strips+1+y*320,y,sizes[i]);
            assert(memcmp(full,strips+1,sizeof(full))==0);
        }
        assert(strips[0]==0xABCD && strips[320*240+1]==0x1234);
        assert(full[0]==0 && full[319]==0 && full[320*240-1]==0);
    }
}

/* Produce the same seeded event/tick replay that the Wasm parity runner uses. */
static void replay(void)
{
    rr_game_t g;rr_init(&g,0xD057);
    for (int i=0;i<1400;++i) {
        if(i==1||i%61==0)rr_input(&g,RR_ACTION);
        if(i%37==0)rr_input(&g,RR_LEFT);
        if(i%53==0)rr_input(&g,RR_RIGHT);
        if(i==120||i==131)rr_input(&g,RR_PAUSE);
        if(i==900)rr_input(&g,RR_HOME);
        rr_tick(&g,(unsigned)(16+i%41));
        if(i%70==0||i==1399) {
            rr_render_strip(&g,full,0,240);
            uint32_t h=2166136261u;
            for(int p=0;p<320*240;++p){h=(h^(full[p]&255))*16777619u;h=(h^(full[p]>>8))*16777619u;}
            printf("%d %u %u\n",i,rr_state_hash(&g),h);
        }
    }
}

static void upgrade_tests(void){
 uint32_t m=game_medal_record(0,5,3);assert(m==768&&game_medal_count(m,5)==3);assert(game_medal_record(m,5,1)==m&&game_medal_record(m,0,3)==m&&game_medal_record(m,6,3)==m);
 rr_game_t g;rr_init(&g,1);rr_input(&g,RR_HOME);assert(g.muted);rr_input(&g,RR_ACTION);assert(g.muted);g.lane=-1;g.lane_q8=-256;g.spawn_ms=30000;
 for(int i=0;i<3;i++){g.entities[0]=(rr_entity_t){.active=1,.lane=1,.depth=1119};rr_tick(&g,20);}
 assert(g.boost_ms==3000&&g.chain==0&&g.overtakes==3&&g.score==180);
 rr_input(&g,RR_PAUSE);int boost=g.boost_ms;rr_tick(&g,200);assert(g.boost_ms==boost);
 rr_input(&g,RR_PAUSE);g.chain=2;g.entities[0]=(rr_entity_t){.active=1,.lane=-1,.depth=1000};rr_tick(&g,20);
 assert(g.health<100&&g.boost_ms==0&&g.chain==0);
 for(int theme=1;theme<=5;theme++){long energy=0;for(unsigned n=0;n<16000;n++){int v=game_music_sample(theme,n);assert(v>=-900&&v<=900);energy+=v<0?-v:v;}assert(energy>0);}
 assert(game_music_sample(0,20)==0&&game_music_sample(6,20)==0);
 for(int effect=1;effect<=7;effect++){assert(game_audio_sample(effect,0)==0&&game_audio_sample(effect,GAME_AUDIO_SAMPLES-1)==0);int energy=0;for(unsigned n=0;n<GAME_AUDIO_SAMPLES;n++){int v=game_audio_sample(effect,n);assert(v>=-4500&&v<=4500);energy+=v<0?-v:v;}assert(energy>0);}
}
int main(int argc,char **argv)
{upgrade_tests();
    (void)argv;
    if(argc>1){replay();return 0;}
    logic_tests();control_tests();geometry_tests();complete_races();render_tests();
    printf("Road Rage host tests: PASS (state, combat, collision, reset, strip bounds)\n");
    return 0;
}
