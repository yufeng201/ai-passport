#include "ll_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t frame[320*240],strips[320*240];
static void tick(ll_game_t *g,int ms){for(int i=0;i<ms;i+=20)ll_update(g,g->last_ms+20);}
static void safe_gates(ll_game_t *g){for(int i=0;i<3;i++)g->gates[i].altitude=100000+i*160;}
int main(int argc,char **argv){
 ll_game_t g;ll_init(&g,0,0xC10D);
 if(argc>1&&!strcmp(argv[1],"replay")){
  for(int i=0;i<2000;i++){
   uint32_t now=(uint32_t)i*20;
   if(i%90==1)ll_edge(&g,2,1,now);
   if(i%90==40)ll_edge(&g,2,0,now);
   if(i==500)ll_edge(&g,0,1,now);
   if(i==545)ll_edge(&g,0,0,now);
   if(i==1000)ll_cancel(&g,now);
   ll_update(&g,now);
   if(i%100==0||i==1999){uint32_t h=2166136261u;ll_render_strip(&g,frame,0,240);
    for(int j=0;j<320*240;j++){h=(h^(frame[j]&255))*16777619u;h=(h^(frame[j]>>8))*16777619u;}
    printf("%d %u %u\n",i,ll_hash(&g),h);
   }
  }return 0;
 }
 assert(g.phase==LL_TITLE);ll_edge(&g,2,1,0);ll_edge(&g,2,0,0);assert(g.phase==LL_PLAY);
 safe_gates(&g);ll_edge(&g,0,1,0);tick(&g,100);assert(g.x==145&&g.best==0);ll_edge(&g,0,0,100);
 ll_edge(&g,1,1,100);tick(&g,100);assert(g.x==160);ll_edge(&g,1,0,200);
 ll_edge(&g,2,1,200);tick(&g,340);assert(ll_jump_height(&g)==54);
 ll_edge(&g,2,0,g.last_ms);tick(&g,380);assert(g.jump_ms==0);
 ll_cancel(&g,g.last_ms);int height=g.height;tick(&g,1000);assert(g.phase==LL_PAUSED&&g.height==height&&!g.held);
 ll_edge(&g,2,1,g.last_ms);assert(g.phase==LL_PLAY);ll_edge(&g,2,0,g.last_ms);
 // A simulation gap pauses rather than teleporting through hazards.
 ll_update(&g,g.last_ms+500);assert(g.phase==LL_PAUSED);
 ll_start(&g);safe_gates(&g);ll_edge(&g,0,1,g.last_ms);tick(&g,1000);assert(g.x==52);ll_edge(&g,0,0,g.last_ms);
 ll_edge(&g,1,1,g.last_ms);tick(&g,2000);assert(g.x==268);ll_edge(&g,1,0,g.last_ms);
 // A rock hit consumes one life, and invulnerability covers the next contact.
 ll_start(&g);safe_gates(&g);g.gates[0]=(ll_gate_t){0,72,0};tick(&g,20);assert(g.hp==2);
 g.gates[1]=(ll_gate_t){0,72,0};tick(&g,20);assert(g.hp==2);
 // Walking into a monster hurts; descending onto it stomps it.
 ll_start(&g);safe_gates(&g);g.monster_active=1;g.monster_x=g.x;tick(&g,20);assert(g.hp==2&&!g.monster_active);
 ll_start(&g);safe_gates(&g);g.monster_active=1;g.monster_x=g.x;g.jump_ms=640;tick(&g,20);assert(g.kills==1&&g.hp==3&&!g.monster_active);
 ll_start(&g);safe_gates(&g);g.hp=1;g.monster_active=1;g.monster_x=g.x;tick(&g,20);assert(g.phase==LL_FAILED);
 ll_edge(&g,2,1,g.last_ms);ll_edge(&g,2,0,g.last_ms);assert(g.phase==LL_PLAY&&g.hp==3);
 // Goal transition and persisted best are independent of frame cadence.
 safe_gates(&g);g.elapsed_ms=59980;g.monster_wait=100000;tick(&g,20);assert(g.phase==LL_WON&&g.height==LL_GOAL&&g.best==LL_GOAL);
 for(int phase=LL_TITLE;phase<=LL_FAILED;phase++){
  g.phase=(ll_phase_t)phase;ll_render_strip(&g,frame,0,240);
  for(int y=0;y<240;y+=40)ll_render_strip(&g,strips+y*320,y,40);
  assert(!memcmp(frame,strips,sizeof(frame)));
 }
 ll_game_t a,b;ll_init(&a,0,11);ll_init(&b,0,11);ll_start(&a);ll_start(&b);safe_gates(&a);safe_gates(&b);
 for(int i=1;i<=100;i++)ll_update(&a,(uint32_t)i*20);
 for(int i=1;i<=50;i++)ll_update(&b,(uint32_t)i*40);
 assert(ll_hash(&a)==ll_hash(&b));
 puts("Lava Lift movement/jump/rocks/monsters/pause/restart/goal/strip rendering: PASS");return 0;
}
