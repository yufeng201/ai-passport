#include "bw_game.h"
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static int ab(int n){return n<0?-n:n;}
static void clear_input(bw_game_t *g){g->blocked|=g->held;g->held=0;g->long_sent=0;}
static void phase(bw_game_t *g,bw_phase_t p){g->phase=p;g->accumulator_ms=0;clear_input(g);}
static void finish(bw_game_t *g,int won){
    if(g->score>g->best)g->best=g->score;
    if(won&&g->stage<5&&g->unlocked<=g->stage)g->unlocked=g->stage+1;
    if(won)g->medals=game_medal_record(g->medals,g->stage,1+(g->lives==3)+(g->slow_uses>0));
    phase(g,won?BW_CLEAR:BW_FAILED);g->effect=won?3:2;
}
int bw_brick_x(const bw_game_t *g,int i){return g->bricks[i].x+(g->stage>=4?(i/6%2?-g->motion:g->motion):0);}
static int speed(const bw_game_t *g){return 3+g->stage/2;}
static void reset_ball(bw_game_t *g){g->ready=1;g->ball_x=g->paddle_x*BW_Q;g->ball_y=(BW_PADDLE_Y-BW_RADIUS-1)*BW_Q;g->vx=0;g->vy=0;g->slow_ms=0;g->stale_ms=0;}
void bw_init(bw_game_t *g,uint32_t now,uint32_t seed){*g=(bw_game_t){.phase=BW_TITLE,.rng=seed?seed:1,.last_ms=now,.stage=1,.unlocked=1,.battery=-1};}
void bw_start(bw_game_t *g){
    uint32_t medals=g->medals;
    int stage=clamp(g->stage,1,5),unlocked=clamp(g->unlocked,1,5),best=g->best,battery=g->battery,muted=g->muted;
    uint32_t rng=g->rng,now=g->last_ms;unsigned blocked=g->held|g->blocked;
    *g=(bw_game_t){.medals=medals,.phase=BW_PLAY,.rng=rng,.last_ms=now,.stage=stage,.unlocked=unlocked,.best=best,.battery=battery,
        .muted=muted,.blocked=blocked,.lives=3,.paddle_x=160,.slow_uses=2,.motion_dir=1};
    int rows=stage==1?2:stage<5?3:4;
    for(int i=0;i<BW_BRICKS;i++){
        int hp=i/6<rows?(stage>=2&&i%3==0?2:1):0;if(stage==5&&i%4==0)hp=3;
        g->bricks[i]=(bw_brick_t){36+(i%6)*42,50+(i/6)*16,hp,hp};g->remaining+=hp>0;
    }
    g->goal=g->remaining;reset_ball(g);
}
static void launch(bw_game_t *g){g->ready=0;g->vx=((g->rng&1)?2:-2)*BW_Q;g->rng=g->rng*1664525u+1013904223u;g->vy=-speed(g)*BW_Q;g->effect=1;}
/* A nudge only changes an outgoing wall/ceiling/paddle bounce, never an
 * in-flight position or the obstacle's blocking surface. */
static void assist(bw_game_t *g){
    if(g->stale_ms<6000)return;
    int best=-1,dist=10000,x=g->ball_x/BW_Q,y=g->ball_y/BW_Q;
    for(int i=0;i<BW_BRICKS;i++)if(g->bricks[i].hp){
        int d=ab(bw_brick_x(g,i)+18-x)+ab(g->bricks[i].y+5-y);
        if(d<dist){dist=d;best=i;}
    }
    if(best<0)return;
    int tx=bw_brick_x(g,best)+18,ty=g->bricks[best].y+5;
    int dy=ty-y;if(!dy)dy=-1;
    int dx=clamp((tx-x)*speed(g)/(ab(dy)>8?ab(dy):8),-6,6);
    if(!dx)dx=tx<x?-1:1;
    g->vx=dx*BW_Q;g->vy=(dy<0?-speed(g):speed(g))*BW_Q;
    g->stale_ms=0;g->assist_ms=900;
}
static void reflect_box(bw_game_t *g,int left,int top,int right,int bottom,int old_x,int old_y){
    const int r=BW_RADIUS*BW_Q;
    if(old_y+r<=top){g->ball_y=top-r;g->vy=-ab(g->vy);}
    else if(old_y-r>=bottom){g->ball_y=bottom+r;g->vy=ab(g->vy);}
    else if(old_x+r<=left){g->ball_x=left-r;g->vx=-ab(g->vx);}
    else if(old_x-r>=right){g->ball_x=right+r;g->vx=ab(g->vx);}
    else{
        /* Moving bricks can approach an already touching ball. Eject it on
         * the nearest face rather than reflecting repeatedly from inside. */
        int a=g->ball_x+r-left,b=right-g->ball_x+r,c=g->ball_y+r-top,d=bottom-g->ball_y+r;
        if(c<=a&&c<=b&&c<=d){g->ball_y=top-r;g->vy=-ab(g->vy);}
        else if(d<=a&&d<=b){g->ball_y=bottom+r;g->vy=ab(g->vy);}
        else if(a<=b){g->ball_x=left-r;g->vx=-ab(g->vx);}
        else{g->ball_x=right+r;g->vx=ab(g->vx);}
    }
}
static int overlap(const bw_game_t *g,int l,int t,int r,int b){int radius=BW_RADIUS*BW_Q;return g->ball_x+radius>l&&g->ball_x-radius<r&&g->ball_y+radius>t&&g->ball_y-radius<b;}
static void step(bw_game_t *g){
    g->elapsed_ms+=20;
    if(g->supply_ms>0)g->supply_ms-=20;
    if(g->flash_ms>0)g->flash_ms-=20;
    if(g->assist_ms>0)g->assist_ms-=20;
    if(g->slow_ms>0)g->slow_ms-=20;
    if(g->held&1u)g->paddle_x-=8;else if(g->held&4u)g->paddle_x+=8;
    g->paddle_x=clamp(g->paddle_x,16+BW_PADDLE_W/2,304-BW_PADDLE_W/2);
    if(g->stage>=4&&g->elapsed_ms%60==0){g->motion+=g->motion_dir;if(g->motion>=12)g->motion_dir=-1;else if(g->motion<=-12)g->motion_dir=1;}
    if(g->ready){g->ball_x=g->paddle_x*BW_Q;return;}
    g->stale_ms+=20;unsigned hit_mask=0;
    /* Eight Q8 microsteps keep each axis below one pixel at the 6/7px
     * velocity caps. Paddle contact also checks the exact swept top crossing. */
    for(int n=0;n<8&&g->phase==BW_PLAY;n++){
        int old_x=g->ball_x,old_y=g->ball_y,divider=g->slow_ms>0?16:8;
        g->ball_x+=g->vx/divider;g->ball_y+=g->vy/divider;
        int bounce=0,r=BW_RADIUS*BW_Q;
        if(g->ball_x<(16+BW_RADIUS)*BW_Q){g->ball_x=(16+BW_RADIUS)*BW_Q;g->vx=ab(g->vx);bounce=1;}
        if(g->ball_x>(304-BW_RADIUS)*BW_Q){g->ball_x=(304-BW_RADIUS)*BW_Q;g->vx=-ab(g->vx);bounce=1;}
        if(g->ball_y<(34+BW_RADIUS)*BW_Q){g->ball_y=(34+BW_RADIUS)*BW_Q;g->vy=ab(g->vy);bounce=1;}
        int paddle_top=BW_PADDLE_Y*BW_Q;
        if(g->vy>0&&old_y+r<=paddle_top&&g->ball_y+r>=paddle_top&&
           g->ball_x+r>(g->paddle_x-BW_PADDLE_W/2)*BW_Q&&g->ball_x-r<(g->paddle_x+BW_PADDLE_W/2)*BW_Q){
            int offset=g->ball_x/BW_Q-g->paddle_x;
            int dx=clamp(offset*6/(BW_PADDLE_W/2),-6,6);if(!dx)dx=g->vx<0?-1:1;
            g->vx=dx*BW_Q;g->vy=-speed(g)*BW_Q;g->ball_y=paddle_top-r;g->effect=1;bounce=1;
        }
        if(bounce)assist(g);
        if(g->stage==3||g->stage==5){
            if(overlap(g,126*BW_Q,138*BW_Q,194*BW_Q,146*BW_Q))reflect_box(g,126*BW_Q,138*BW_Q,194*BW_Q,146*BW_Q,old_x,old_y);
        }
        for(int i=0;i<BW_BRICKS;i++){
            bw_brick_t *b=&g->bricks[i];if(b->hp<=0)continue;
            int left=bw_brick_x(g,i)*BW_Q,top=b->y*BW_Q;
            if(!overlap(g,left,top,left+36*BW_Q,top+10*BW_Q))continue;
            reflect_box(g,left,top,left+36*BW_Q,top+10*BW_Q,old_x,old_y);
            if(!(hit_mask&(1u<<i))){
                hit_mask|=1u<<i;b->hp--;g->score+=50;g->flash_ms=120;g->flash_x=left/BW_Q+18;g->flash_y=b->y+5;g->effect=1;g->stale_ms=0;
                if(!b->hp){
                    g->remaining--;g->destroyed++;
                    if(g->destroyed%6==0&&g->slow_uses<3){g->slow_uses++;g->supply_ms=1000;g->effect=7;}
                }
                if(!g->remaining){finish(g,1);return;}
            }
            break;
        }
        if(g->ball_y>214*BW_Q){g->lives--;if(g->lives<=0)finish(g,0);else reset_ball(g);return;}
    }
}
void bw_update(bw_game_t *g,uint32_t now){
    uint32_t dt=now-g->last_ms;if((int32_t)dt<0)dt=0;else g->last_ms=now;if(dt>250)dt=250;uint32_t key_now=g->last_ms;
    if(g->phase==BW_TITLE||g->phase==BW_PLAY)g->scene_ms+=dt;
    if(g->phase==BW_PLAY){g->accumulator_ms+=dt;while(g->accumulator_ms>=20&&g->phase==BW_PLAY){g->accumulator_ms-=20;step(g);}}
    for(int key=0;key<3;key++){
        unsigned bit=1u<<key;if(!(g->held&bit)||(g->long_sent&bit)||key_now-g->pressed_ms[key]<800)continue;
        if(g->phase==BW_PLAY&&key==1){phase(g,BW_PAUSED);return;}
        if(g->phase==BW_PAUSED&&key==0){phase(g,BW_TITLE);return;}
        if((g->phase==BW_CLEAR||g->phase==BW_FAILED)&&key==1){phase(g,BW_TITLE);return;}
        if(g->phase==BW_TITLE&&key==0){g->long_sent|=bit;return;}
    }
}
void bw_edge(bw_game_t *g,int key,int down,uint32_t now){
    if(key<0||key>2)return;
    bw_update(g,now);unsigned bit=1u<<key;if(g->blocked&bit){if(!down)g->blocked&=~bit;return;}
    if(down){if(g->held)return;g->held|=bit;g->long_sent&=~bit;g->pressed_ms[key]=g->last_ms;return;}
    if(!(g->held&bit))return;
    int was_long=(g->long_sent&bit)!=0;g->held&=~bit;g->long_sent&=~bit;
    if(was_long){if(g->phase==BW_TITLE&&key==0)g->muted=!g->muted;return;}
    if(g->phase==BW_TITLE){if(key==1)bw_start(g);else if(key==0&&g->stage>1)g->stage--;else if(key==2&&g->stage<g->unlocked)g->stage++;}
    else if(g->phase==BW_PAUSED&&key==1)phase(g,BW_PLAY);
    else if((g->phase==BW_CLEAR||g->phase==BW_FAILED)&&key==1){if(g->phase==BW_CLEAR&&g->stage<5)g->stage++;bw_start(g);}
    else if(g->phase==BW_PLAY&&key==1){if(g->ready)launch(g);else if(g->slow_uses>0&&g->slow_ms==0){g->slow_uses--;g->slow_ms=3000;g->effect=3;}}
}
void bw_cancel(bw_game_t *g,uint32_t now){bw_update(g,now);if(g->phase==BW_PLAY)phase(g,BW_PAUSED);else clear_input(g);}
uint32_t bw_hash(const bw_game_t *g){
    uint32_t h=2166136261u;
#define HASH(v) do{uint32_t q=(uint32_t)(v);for(int b=0;b<4;b++){h=(h^(q&255))*16777619u;q>>=8;}}while(0)
    HASH(g->medals);HASH(g->phase);HASH(g->rng);HASH(g->last_ms);HASH(g->scene_ms);HASH(g->elapsed_ms);HASH(g->accumulator_ms);for(int i=0;i<3;i++)HASH(g->pressed_ms[i]);
    HASH(g->stage);HASH(g->unlocked);HASH(g->best);HASH(g->score);HASH(g->battery);HASH(g->muted);HASH(g->lives);HASH(g->paddle_x);HASH(g->ball_x);HASH(g->ball_y);HASH(g->vx);HASH(g->vy);HASH(g->ready);
    HASH(g->destroyed);HASH(g->supply_ms);HASH(g->remaining);HASH(g->goal);HASH(g->slow_uses);HASH(g->slow_ms);HASH(g->stale_ms);HASH(g->assist_ms);HASH(g->motion);HASH(g->motion_dir);HASH(g->flash_ms);HASH(g->flash_x);HASH(g->flash_y);HASH(g->effect);HASH(g->held);HASH(g->blocked);HASH(g->long_sent);
    for(int i=0;i<BW_BRICKS;i++){HASH(g->bricks[i].x);HASH(g->bricks[i].y);HASH(g->bricks[i].hp);HASH(g->bricks[i].max_hp);}
#undef HASH
    return h;
}
