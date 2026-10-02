#include "ll_game.h"
static int abs_i(int x){return x<0?-x:x;}
static uint32_t random_next(ll_game_t *g){g->rng=g->rng*1664525u+1013904223u;return g->rng;}
int ll_platform_y(const ll_game_t *g){return 196-(g->height<20?g->height:20);}
int ll_gate_y(const ll_game_t *g,int i){return ll_platform_y(g)+g->height-g->gates[i].altitude;}
int ll_jump_height(const ll_game_t *g){int t=g->jump_ms;return t?4*54*t*(LL_JUMP_MS-t)/(LL_JUMP_MS*LL_JUMP_MS):0;}
void ll_init(ll_game_t *g,uint32_t now,uint32_t seed){*g=(ll_game_t){0};g->last_ms=now;g->rng=seed;g->x=160;g->hp=3;g->battery=-1;}
void ll_start(ll_game_t *g){
 g->phase=LL_PLAY;g->height=0;g->elapsed_ms=0;g->accumulator_ms=0;g->x=160;g->jump_ms=0;g->hp=3;
 g->invincible_ms=0;g->held=0;g->kills=0;g->monster_active=0;g->monster_wait=4000;g->effect=3;
 for(int i=0;i<3;i++)g->gates[i]=(ll_gate_t){140+i*160,90+(int)(random_next(g)%141),0};
}
static void damage(ll_game_t *g){if(g->invincible_ms)return;g->hp--;g->invincible_ms=1400;g->effect=2;
 if(g->hp<=0){g->phase=LL_FAILED;g->held=0;if(g->height>g->best)g->best=g->height;}}
static void step(ll_game_t *g){
 g->elapsed_ms+=20;g->height=(int)(g->elapsed_ms/50);
 int direction=((g->held&2)!=0)-((g->held&1)!=0);g->x+=direction*3;
 if(g->x<52)g->x=52;
 if(g->x>268)g->x=268;
 if(g->invincible_ms>0)g->invincible_ms-=20;
 int previous=g->jump_ms;
 if(g->jump_ms){g->jump_ms+=20;if(g->jump_ms>=LL_JUMP_MS)g->jump_ms=0;}
 for(int i=0;i<3;i++){
  ll_gate_t *gate=&g->gates[i];int y=ll_gate_y(g,i),feet=ll_platform_y(g)-ll_jump_height(g);
  int platform_hit=y+14>=ll_platform_y(g)&&y<=ll_platform_y(g)+12&&(g->x-32<gate->gap-58||g->x+32>gate->gap+58);
  int player_hit=y+14>=feet-24&&y<=feet&&(g->x-8<gate->gap-58||g->x+8>gate->gap+58);
  if(!gate->hit&&(platform_hit||player_hit)){gate->hit=1;damage(g);}
  if(y>240){gate->altitude+=480;gate->gap=90+(int)(random_next(g)%141);gate->hit=0;}
 }
 if(!g->monster_active){g->monster_wait-=20;if(g->monster_wait<=0){g->monster_active=1;g->monster_x=(random_next(g)&1)?35:285;}}
 else{
  g->monster_x+=g->monster_x<g->x?2:-2;
  if(abs_i(g->monster_x-g->x)<15){
   int jump=ll_jump_height(g);
   if(previous>=LL_JUMP_MS/2&&jump<=21){g->kills++;g->effect=4;g->monster_active=0;g->monster_wait=3600;}
   else if(jump<18){damage(g);g->monster_active=0;g->monster_wait=4200;}
  }
 }
 if(g->phase==LL_PLAY&&g->height>=LL_GOAL){g->height=LL_GOAL;g->best=LL_GOAL;g->phase=LL_WON;g->held=0;g->effect=3;}
}
void ll_update(ll_game_t *g,uint32_t now){uint32_t dt=now-g->last_ms;g->last_ms=now;
 if(g->phase!=LL_PLAY)return;
 if(dt>200){ll_cancel(g,now);return;}
 g->accumulator_ms+=dt;
 while(g->accumulator_ms>=20&&g->phase==LL_PLAY){g->accumulator_ms-=20;step(g);}
}
void ll_edge(ll_game_t *g,int key,int down,uint32_t now){
 if(key<0||key>2)return;
 ll_update(g,now);unsigned bit=1u<<key;
 if(!down){g->held&=~bit;return;}
 if(g->held&bit)return;
 g->held|=bit;
 if(g->phase!=LL_PLAY){if(key==2){if(g->phase==LL_PAUSED){g->phase=LL_PLAY;g->held=0;}else ll_start(g);}return;}
 if(key==2&&!g->jump_ms){g->jump_ms=20;g->effect=1;}
}
void ll_cancel(ll_game_t *g,uint32_t now){g->held=0;g->last_ms=now;g->accumulator_ms=0;if(g->phase==LL_PLAY)g->phase=LL_PAUSED;}
uint32_t ll_hash(const ll_game_t *g){uint32_t h=2166136261u;
 const int values[]={g->phase,g->x,g->height,g->jump_ms,g->hp,g->invincible_ms,g->kills,g->monster_x,g->monster_active,g->monster_wait,(int)g->held,(int)g->rng,(int)g->elapsed_ms};
 for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++)h=(h^(uint32_t)values[i])*16777619u;
 for(int i=0;i<3;i++){h=(h^(uint32_t)g->gates[i].altitude)*16777619u;h=(h^(uint32_t)g->gates[i].gap)*16777619u;h=(h^(uint32_t)g->gates[i].hit)*16777619u;}return h;
}
