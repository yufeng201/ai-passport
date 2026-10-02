#include "bw_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t full[320*240],part[320*240+2];
static void tick(bw_game_t *g){bw_update(g,g->last_ms+20);}
static void tap(bw_game_t *g,int k){bw_edge(g,k,1,g->last_ms);bw_edge(g,k,0,g->last_ms+40);}
static void start(bw_game_t *g,int stage){bw_init(g,0,0xB21C);g->stage=stage;g->unlocked=stage;bw_start(g);}
static void input(void){
 bw_game_t g;start(&g,1);bw_edge(&g,0,1,0);tick(&g);assert(g.paddle_x==152);bw_edge(&g,2,1,g.last_ms);assert(g.held==1);
 bw_edge(&g,0,0,g.last_ms);tick(&g);assert(g.paddle_x==152);bw_edge(&g,2,0,g.last_ms);
 bw_edge(&g,0,1,g.last_ms);for(int i=0;i<30;i++)tick(&g);assert(g.paddle_x==44);bw_edge(&g,0,0,g.last_ms);
 tap(&g,1);assert(!g.ready&&g.vy<0);tap(&g,1);assert(g.slow_uses==1&&g.slow_ms==3000);tap(&g,1);assert(g.slow_uses==1);
 bw_edge(&g,1,1,g.last_ms);for(int i=0;i<40;i++)tick(&g);assert(g.phase==BW_PAUSED);bw_edge(&g,1,0,g.last_ms);
 int x=g.ball_x,y=g.ball_y,t=g.elapsed_ms;for(int i=0;i<100;i++)tick(&g);assert(g.ball_x==x&&g.ball_y==y&&g.elapsed_ms==(unsigned)t);
 tap(&g,1);assert(g.phase==BW_PLAY&&g.slow_uses==1);
 start(&g,1);bw_edge(&g,1,1,100);bw_update(&g,80);assert(g.phase==BW_PLAY);bw_edge(&g,1,0,120);assert(!g.ready);
 bw_init(&g,UINT32_MAX-10,1);bw_start(&g);bw_update(&g,9);assert(g.elapsed_ms==20);
 start(&g,1);bw_edge(&g,1,1,0);for(int i=0;i<40;i++)tick(&g);assert(g.phase==BW_PAUSED&&g.ready&&g.slow_uses==2);bw_edge(&g,1,0,g.last_ms);assert(g.phase==BW_PAUSED);
 bw_edge(&g,0,1,g.last_ms);for(int i=0;i<40;i++)tick(&g);assert(g.phase==BW_TITLE);bw_edge(&g,0,0,g.last_ms);
 g.unlocked=4;g.best=900;g.muted=1;tap(&g,1);assert(g.best==900&&g.unlocked==4&&g.muted);
}
static void collisions(void){
 bw_game_t g;start(&g,5);g.ready=0;g.ball_x=160*256;g.ball_y=201*256;g.vx=0;g.vy=7*256;tick(&g);assert(g.vy<0&&g.lives==3&&g.ball_y<=203*256);
 start(&g,1);g.ready=0;g.ball_x=195*256;g.ball_y=202*256;g.vx=0;g.vy=7*256;tick(&g);tick(&g);assert(g.lives==2&&g.ready);assert(g.ball_x==g.paddle_x*256);tap(&g,1);assert(!g.ready);
 start(&g,1);g.ready=0;g.ball_x=19*256;g.ball_y=37*256;g.vx=-6*256;g.vy=-5*256;tick(&g);assert(g.vx>0&&g.vy>0);
 start(&g,2);for(int i=1;i<24;i++)g.bricks[i].hp=0;g.remaining=1;g.ready=0;g.ball_x=54*256;g.ball_y=64*256;g.vx=0;g.vy=-7*256;tick(&g);assert(g.bricks[0].hp==1&&g.score==50&&g.vy>0);
 start(&g,3);g.ready=0;g.ball_x=160*256;g.ball_y=150*256;g.vx=0;g.vy=-5*256;g.stale_ms=7000;tick(&g);assert(g.vy>0&&g.ball_y>=149*256);
 start(&g,1);for(int i=1;i<24;i++)g.bricks[i].hp=0;g.remaining=1;g.ready=0;g.ball_x=54*256;g.ball_y=64*256;g.vy=-7*256;tick(&g);assert(g.phase==BW_CLEAR&&g.unlocked==2&&g.best==50);tap(&g,1);assert(g.stage==2&&g.lives==3&&g.score==0);
 start(&g,1);g.lives=1;g.ready=0;g.ball_y=215*256;tick(&g);assert(g.phase==BW_FAILED);tap(&g,1);assert(g.stage==1&&g.lives==3);
 start(&g,4);g.ready=0;g.ball_x=36*256;g.ball_y=55*256;g.vx=0;g.vy=-256;tick(&g);assert(g.ball_x<=33*256||g.ball_y<=47*256||g.ball_y>=63*256);
}
/* Legal single-button tracking pilot. It chooses an offset on the paddle to
 * aim the next bounce at a surviving brick; it never moves the ball directly. */
static void campaigns(void){
 int total=0;
 for(int seed=1;seed<=12;seed++)for(int stage=1;stage<=5;stage++){
  bw_game_t g;start(&g,stage);g.rng=(unsigned)seed;int key=-1;
  for(int n=0;n<30000&&g.phase==BW_PLAY;n++){
   if(g.ready){if(key>=0){bw_edge(&g,key,0,g.last_ms);key=-1;}tap(&g,1);}
   int bx=g.ball_x/256,target=bx;
   if(g.vy>0){
    int nearest=-1,distance=10000;for(int i=0;i<24;i++)if(g.bricks[i].hp){int d=bw_brick_x(&g,i)+18-bx;if(d<0)d=-d;if(d<distance){distance=d;nearest=i;}}
    if(nearest>=0){int dx=bw_brick_x(&g,nearest)+18-bx;target=bx-(dx>0?12:dx<0?-12:0);}
   }
   int desired=target<g.paddle_x-4?0:target>g.paddle_x+4?2:-1;
   if(desired!=key){if(key>=0)bw_edge(&g,key,0,g.last_ms);key=desired;if(key>=0)bw_edge(&g,key,1,g.last_ms);}
   tick(&g);
  }
  if(g.phase!=BW_CLEAR){fprintf(stderr,"campaign seed=%d stage=%d phase=%d left=%d lives=%d time=%u\n",seed,stage,g.phase,g.remaining,g.lives,g.elapsed_ms);assert(g.phase==BW_CLEAR);}
  total++;
 }
 printf("Brick Workshop: %d complete stages with single-button pilot, no slow field\n",total);
}
static void strips(void){
 bw_game_t g;int sizes[]={1,7,40,61,240};
 for(int stage=1;stage<=5;stage++)for(int phase=0;phase<5;phase++){
  start(&g,stage);g.phase=(bw_phase_t)phase;bw_render_strip(&g,full,0,240);
  for(int s=0;s<5;s++)for(int y=0;y<240;y+=sizes[s]){int rows=sizes[s]<240-y?sizes[s]:240-y;part[0]=123;part[rows*320+1]=456;bw_render_strip(&g,part+1,y,rows);assert(part[0]==123&&part[rows*320+1]==456);assert(!memcmp(part+1,full+y*320,(size_t)rows*640));}
 }
 bw_init(&g,0,1);bw_render_strip(&g,full,0,240);part[0]=987;bw_render_strip(&g,part,-1,1);assert(part[0]==987);bw_render_strip(&g,part,239,2);assert(part[0]==987);
}
static void replay(void){
 bw_game_t g;bw_init(&g,0,0xB21C);
 for(int i=0;i<2000;i++){unsigned now=(unsigned)i*20;if(i%90==1)bw_edge(&g,1,1,now);if(i%90==40)bw_edge(&g,1,0,now);if(i==500)bw_edge(&g,2,1,now);if(i==545)bw_edge(&g,2,0,now);if(i==1800)bw_edge(&g,1,1,now);if(i==1000)bw_cancel(&g,now);bw_update(&g,now);
  if(i%100==0||i==1999){bw_render_strip(&g,full,0,240);uint32_t h=2166136261u;for(int p=0;p<320*240;p++){h=(h^(full[p]&255))*16777619u;h=(h^(full[p]>>8))*16777619u;}printf("%d %u %u\n",i,bw_hash(&g),h);}
 }
}
static void upgrade_tests(void){
 bw_game_t g;
 for(int uses=0;uses<=3;uses++){
  start(&g,1);for(int i=1;i<24;i++)g.bricks[i].hp=0;g.remaining=1;g.destroyed=5;g.slow_uses=uses;g.ready=0;g.ball_x=54*BW_Q;g.ball_y=64*BW_Q;g.vx=0;g.vy=-7*BW_Q;
  tick(&g);assert(g.destroyed==6&&g.slow_uses==(uses<3?uses+1:3));
  assert((g.supply_ms>0)==(uses<3));
 }
}
int main(int argc,char **argv){upgrade_tests();if(argc>1&&!strcmp(argv[1],"replay")){replay();return 0;}input();collisions();campaigns();strips();printf("Brick Workshop host tests: PASS (state %zu bytes)\n",sizeof(bw_game_t));return 0;}
