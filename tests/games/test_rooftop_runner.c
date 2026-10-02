#include "rp_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t full[320*240],part[320*240+2];
static void tick(rp_game_t *g){rp_update(g,g->last_ms+20);}
static void tap(rp_game_t *g,int k){rp_edge(g,k,1,g->last_ms);rp_edge(g,k,0,g->last_ms+40);}
static void start(rp_game_t *g,int stage){rp_init(g,0,0xA11E);g->stage=stage;g->unlocked=stage;rp_start(g);}
static void input(void){
 rp_game_t g;start(&g,1);int x=g.x;
 rp_edge(&g,0,1,0);tick(&g);assert(g.x==x+RP_SPEED);rp_edge(&g,1,1,g.last_ms);assert(g.held==1);
 rp_edge(&g,0,0,g.last_ms);tick(&g);assert(g.x==x+RP_SPEED&&g.vx==0);rp_edge(&g,1,0,g.last_ms);
 rp_edge(&g,1,1,g.last_ms);tick(&g);assert(g.x==x);rp_edge(&g,1,0,g.last_ms);
 tap(&g,2);assert(!g.grounded&&g.vx==-RP_SPEED&&g.vy<0);
 rp_edge(&g,0,1,g.last_ms);tick(&g);assert(g.vx==RP_SPEED);rp_edge(&g,0,0,g.last_ms);
 for(int i=0;i<60&&!g.grounded&&g.phase==RP_PLAY;i++)tick(&g);
 assert(g.grounded&&g.vx==0);x=g.x;for(int i=0;i<20;i++)tick(&g);assert(g.x==x);
 tap(&g,2);assert(g.vx==0);int vy=g.vy;tap(&g,2);assert(g.vy>vy); /* No double jump. */
 start(&g,1);rp_edge(&g,2,1,0);for(int i=0;i<40;i++)tick(&g);assert(g.phase==RP_PAUSED&&g.grounded);rp_edge(&g,2,0,g.last_ms);
 int y=g.y,elapsed=(int)g.elapsed_ms;x=g.x;for(int i=0;i<30;i++)tick(&g);assert(g.x==x&&g.y==y&&g.elapsed_ms==(unsigned)elapsed);
 tap(&g,1);assert(g.phase==RP_PLAY&&g.grounded);rp_edge(&g,0,1,g.last_ms);tick(&g);assert(g.x>x);rp_cancel(&g,g.last_ms);rp_edge(&g,0,0,g.last_ms);assert(g.phase==RP_PAUSED);tap(&g,1);x=g.x;tick(&g);assert(g.x==x);
 start(&g,1);rp_edge(&g,2,1,100);rp_update(&g,80);assert(g.phase==RP_PLAY);rp_edge(&g,2,0,140);assert(!g.grounded&&g.vx==0);
 rp_init(&g,UINT32_MAX-10,1);rp_start(&g);rp_update(&g,9);assert(g.elapsed_ms==20);
 start(&g,1);g.unlocked=4;g.best=1200;g.muted=1;rp_start(&g);assert(g.unlocked==4&&g.best==1200&&g.muted);
}
static void forgiving_jumps(void){
 rp_game_t g;start(&g,1);
 /* Late edge jump still works; repeating C in flight cannot jump again. */
 g.x=164*RP_Q;rp_edge(&g,0,1,g.last_ms);tick(&g);assert(!g.grounded&&g.coyote_ms==100);
 rp_edge(&g,0,0,g.last_ms);tap(&g,2);assert(g.vy==-10*RP_Q&&g.coyote_ms==0&&g.vx==RP_SPEED);
 int vy=g.vy;tap(&g,2);assert(g.vy>vy&&g.jump_buffer_ms==100);
 for(int i=0;i<6;i++)tick(&g);assert(!g.jump_buffer_ms);
 start(&g,1);rp_edge(&g,0,1,0);tick(&g);rp_edge(&g,0,0,g.last_ms);for(int i=0;i<15;i++)tick(&g);tap(&g,2);assert(g.vx==RP_SPEED);
 /* Near touchdown, a short release is remembered and consumed exactly once. */
 start(&g,1);g.grounded=0;g.platform=-1;g.y=(196-30)*RP_Q;g.vy=3*RP_Q;g.vx=0;
 tap(&g,2);assert(g.jump_buffer_ms==100);for(int i=0;i<5&&g.vy>=0;i++)tick(&g);
 assert(g.vy<0&&!g.grounded&&g.jump_buffer_ms==0);
 start(&g,1);g.grounded=0;g.platform=-1;g.y=100*RP_Q;g.vy=0;tap(&g,2);assert(g.jump_buffer_ms);rp_cancel(&g,g.last_ms);assert(!g.jump_buffer_ms);tap(&g,1);for(int i=0;i<35&&!g.grounded;i++)tick(&g);assert(g.grounded&&g.vy==0);
 start(&g,1);rp_edge(&g,1,1,0);tick(&g);rp_edge(&g,1,0,g.last_ms);tick(&g);assert(g.facing==-1&&g.vx==0);
}
static void collisions(void){
 rp_game_t g;start(&g,5);rp_platform_t *p=&g.platforms[1];
 g.x=(p->x+65)*RP_Q;g.y=(p->y-2)*RP_Q;g.vy=14*RP_Q;g.grounded=0;g.platform=-1;tick(&g);assert(g.grounded&&g.y==p->y*RP_Q&&g.vy==0);
 start(&g,2);p=&g.platforms[1];g.x=(p->x+65)*RP_Q;g.y=(p->y+8)*RP_Q;g.grounded=0;g.platform=-1;g.vy=RP_Q;tick(&g);assert(!g.grounded&&g.y>p->y*RP_Q); /* No upward snap. */
 start(&g,3);p=&g.platforms[1];g.x=(p->x-RP_HALF-1)*RP_Q;g.y=(p->y+10)*RP_Q;g.grounded=0;g.platform=-1;g.vx=RP_SPEED;g.vy=0;tick(&g);assert(g.x<=(p->x-RP_HALF)*RP_Q&&g.vx==0);
 start(&g,4);g.checkpoint=3;g.score=300;g.collected=14;g.x=300*RP_Q;g.y=259*RP_Q;g.grounded=0;g.platform=-1;g.vy=10*RP_Q;tick(&g);assert(g.phase==RP_FAILED&&g.falls==1);
 for(int i=0;i<30;i++)tick(&g);assert(g.phase==RP_PLAY&&g.checkpoint==3&&g.collected==14&&g.score==300&&g.x==(g.platforms[3].x+18)*RP_Q);
 start(&g,5);g.platform=11;g.grounded=1;g.x=(g.platforms[11].x+g.platforms[11].w-24)*RP_Q;g.y=g.platforms[11].y*RP_Q;tick(&g);assert(g.phase==RP_CLEAR&&g.best==500);tap(&g,1);assert(g.stage==5&&g.score==0);
 start(&g,4);g.platform=11;g.grounded=1;g.x=(g.platforms[11].x+g.platforms[11].w-24)*RP_Q;g.y=g.platforms[11].y*RP_Q;tick(&g);assert(g.phase==RP_CLEAR&&g.unlocked==5);tap(&g,1);assert(g.stage==5&&g.unlocked==5);
 start(&g,2);g.x=rp_hazard_x(&g,2)*RP_Q;g.y=g.platforms[2].y*RP_Q;g.platform=2;tick(&g);assert(g.phase==RP_FAILED);
}
/* Controls use only real edges and never teleport the runner. Vary the switch
 * delay and launch position to establish a usable window, not one exact frame. */
static void campaigns(void){
 int count=0;
 for(int delay=20;delay<=100;delay+=20)for(int margin=-2;margin<=4;margin+=2)for(int stage=1;stage<=5;stage++){
  rp_game_t g;start(&g,stage);int held=0;rp_edge(&g,0,1,0);
  for(int n=0;n<20000&&g.phase==RP_PLAY;n++){
   if(g.grounded&&g.platform<11){rp_platform_t *p=&g.platforms[g.platform];
    if(g.x/RP_Q>=p->x+p->w-margin){
     if(held){rp_edge(&g,0,0,g.last_ms);held=0;}
     rp_edge(&g,2,1,g.last_ms);rp_edge(&g,2,0,g.last_ms+(unsigned)delay);
    }else if(!held){rp_edge(&g,0,1,g.last_ms);held=1;}
   }else if(g.grounded&&g.platform==11&&!held){rp_edge(&g,0,1,g.last_ms);held=1;}
   tick(&g);
  }
  if(g.phase!=RP_CLEAR){fprintf(stderr,"route delay=%d margin=%d stage=%d phase=%d platform=%d x=%d y=%d falls=%d\n",delay,margin,stage,g.phase,g.platform,g.x/RP_Q,g.y/RP_Q,g.falls);assert(g.phase==RP_CLEAR);}
  assert(g.falls==0);count++;
 }
 printf("Rooftop Runner: %d complete stages with legal sequential input\n",count);
}
static void backward_routes(void){
 for(int stage=1;stage<=5;stage++){
  rp_game_t g;start(&g,stage);g.platform=10;g.x=(g.platforms[10].x+85)*RP_Q;g.y=g.platforms[10].y*RP_Q;
  int held=1;rp_edge(&g,1,1,g.last_ms);
  for(int n=0;n<12000&&g.phase==RP_PLAY;n++){
   if(g.grounded&&g.platform==0&&g.x<80*RP_Q)break;
   if(g.grounded&&g.platform>0){rp_platform_t *p=&g.platforms[g.platform];
    if(g.x/RP_Q<=p->x+55){if(held){rp_edge(&g,1,0,g.last_ms);held=0;}tap(&g,2);}
    else if(!held){rp_edge(&g,1,1,g.last_ms);held=1;}
   }else if(g.grounded&&!held){rp_edge(&g,1,1,g.last_ms);held=1;}
   int camera=g.camera;tick(&g);assert(g.camera-camera<=8&&camera-g.camera<=8);
  }
  assert(g.phase==RP_PLAY&&g.grounded&&g.platform==0&&g.x<80*RP_Q&&g.falls==0);
 }
 puts("Rooftop Runner: five backward routes, camera movement bounded");
}
static void strips(void){
 rp_game_t g;int sizes[]={1,7,40,61,240};
 for(int stage=1;stage<=5;stage++)for(int phase=0;phase<5;phase++){
  start(&g,stage);g.phase=(rp_phase_t)phase;g.elapsed_ms=3400;g.camera=150;rp_render_strip(&g,full,0,240);
  for(int s=0;s<5;s++)for(int y=0;y<240;y+=sizes[s]){int rows=sizes[s]<240-y?sizes[s]:240-y;part[0]=123;part[rows*320+1]=456;rp_render_strip(&g,part+1,y,rows);assert(part[0]==123&&part[rows*320+1]==456);assert(!memcmp(part+1,full+y*320,(size_t)rows*640));}
 }
 part[0]=987;rp_render_strip(&g,part,-1,1);assert(part[0]==987);rp_render_strip(&g,part,239,2);assert(part[0]==987);
}
static void replay(void){
 rp_game_t g;rp_init(&g,0,0xA11E);
 for(int i=0;i<2000;i++){unsigned now=(unsigned)i*20;if(i%90==1)rp_edge(&g,1,1,now);if(i%90==40)rp_edge(&g,1,0,now);if(i==500)rp_edge(&g,2,1,now);if(i==545)rp_edge(&g,2,0,now);if(i==1800)rp_edge(&g,1,1,now);if(i==1000)rp_cancel(&g,now);rp_update(&g,now);
  if(i%100==0||i==1999){rp_render_strip(&g,full,0,240);uint32_t h=2166136261u;for(int p=0;p<320*240;p++){h=(h^(full[p]&255))*16777619u;h=(h^(full[p]>>8))*16777619u;}printf("%d %u %u\n",i,rp_hash(&g),h);}
 }
}
static void upgrade_tests(void){
 rp_game_t g;start(&g,1);g.collected=0xffe;g.score=1100;g.platform=11;g.grounded=1;
 g.x=(g.platforms[11].x+g.platforms[11].w-24)*RP_Q;g.y=g.platforms[11].y*RP_Q;
 tick(&g);assert(g.phase==RP_CLEAR&&g.score==1900&&g.best==1900);
 tick(&g);assert(g.score==1900);
}
int main(int argc,char **argv){upgrade_tests();if(argc>1&&!strcmp(argv[1],"replay")){replay();return 0;}input();forgiving_jumps();collisions();campaigns();backward_routes();strips();printf("Rooftop Runner host tests: PASS (state %zu bytes)\n",sizeof(rp_game_t));return 0;}
