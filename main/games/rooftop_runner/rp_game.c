#include "rp_game.h"
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static void clear_input(rp_game_t *g){g->blocked|=g->held;g->held=0;g->long_sent=0;g->last_dir=0;g->jump_dir=0;g->jump_buffer_ms=0;g->coyote_ms=0;}
static void phase(rp_game_t *g,rp_phase_t p){g->phase=p;g->accumulator_ms=0;clear_input(g);}
/* Twelve deterministic roofs. Every gap has margin beneath the 120px jump
 * range, including a 16px upward step. Hazards stay near the receiving edge. */
static void layout(rp_game_t *g){
    int x=0;
    for(int i=0;i<RP_PLATFORMS;i++){
        int w=i==0?160:124-g->stage*4;
        int y=196;if(g->stage>=2)y-=((i+g->stage)%3)*8;
        int hazard=g->stage>=2&&i%3==2?1:0;if(hazard&&g->stage>=4)hazard=2;
        g->platforms[i]=(rp_platform_t){x,y,w,hazard};
        x+=w+24+g->stage*6+(i%2)*6;
    }
}
static void place(rp_game_t *g){
    rp_platform_t *p=&g->platforms[g->checkpoint];g->x=(p->x+18)*RP_Q;g->y=p->y*RP_Q;
    g->vx=g->vy=0;g->grounded=1;g->platform=g->checkpoint;g->camera=clamp(p->x-44,0,g->platforms[11].x+g->platforms[11].w-260);
    clear_input(g);g->facing=1;
}
void rp_init(rp_game_t *g,uint32_t now,uint32_t seed){
    *g=(rp_game_t){.phase=RP_TITLE,.rng=seed,.last_ms=now,.stage=1,.unlocked=1,.battery=-1,.platform=0,.grounded=1};
    layout(g);place(g);
}
void rp_start(rp_game_t *g){
    uint32_t medals=g->medals;
    int stage=clamp(g->stage,1,5),unlocked=clamp(g->unlocked,1,5),best=g->best,battery=g->battery,muted=g->muted;
    unsigned blocked=g->held|g->blocked;uint32_t now=g->last_ms,seed=g->rng;
    *g=(rp_game_t){.medals=medals,.phase=RP_PLAY,.rng=seed,.last_ms=now,.stage=stage,.unlocked=unlocked,.best=best,.battery=battery,.muted=muted,.blocked=blocked};
    layout(g);place(g);
}
int rp_hazard_x(const rp_game_t *g,int index){
    int move=0;if(g->platforms[index].hazard==2){int t=(int)(g->elapsed_ms/20%80);move=(t<40?t:80-t)/2-10;}
    return g->platforms[index].x+24+move;
}
static void fail(rp_game_t *g){g->falls++;g->respawn_ms=600;phase(g,RP_FAILED);g->effect=2;}
static void finish(rp_game_t *g){
    g->score+=500;if((g->collected&0xffeu)==0xffeu)g->score+=300;if(g->score>g->best)g->best=g->score;
    if(g->stage<5&&g->unlocked<=g->stage)g->unlocked=g->stage+1;
    g->medals=game_medal_record(g->medals,g->stage,1+(g->falls==0)+((g->collected&0xffeu)==0xffeu));
    phase(g,RP_CLEAR);g->effect=3;
}
static void jump(rp_game_t *g){
    if(!g->grounded&&g->coyote_ms<=0){g->jump_buffer_ms=RP_BUFFER_MS;return;}
    g->jump_buffer_ms=0;g->coyote_ms=0;
    g->grounded=0;g->platform=-1;g->vx=g->jump_dir*RP_SPEED;g->vy=-10*RP_Q;g->effect=1;
}
static void step(rp_game_t *g){
    g->elapsed_ms+=20;if(g->coyote_ms>0)g->coyote_ms-=20;
    if(g->feedback_ms>0)g->feedback_ms-=20;
    int dir=g->held&1u?1:g->held&2u?-1:0;
    if(dir)g->facing=dir;
    if(g->grounded)g->vx=dir*RP_SPEED;else if(dir)g->vx=dir*RP_SPEED;
    int old_x=g->x,old_y=g->y;
    if(!g->grounded)g->vy+=RP_Q/2;
    /* Four microsteps resolve side walls as well as landings. Feet
     * crossing a roof from above is the only landing path. */
    for(int n=0;n<4;n++){
        old_x=g->x;old_y=g->y;g->x+=g->vx/4;g->y+=g->vy/4;
        if(g->x<RP_HALF*RP_Q){g->x=RP_HALF*RP_Q;g->vx=0;}
        int right=(g->platforms[11].x+g->platforms[11].w-RP_HALF)*RP_Q;
        if(g->x>right){g->x=right;g->vx=0;}
        for(int i=0;i<RP_PLATFORMS;i++){
            rp_platform_t *p=&g->platforms[i];int left=p->x*RP_Q,top=p->y*RP_Q,r=(p->x+p->w)*RP_Q;
            if(g->x+RP_HALF*RP_Q<=left||g->x-RP_HALF*RP_Q>=r)continue;
            if(g->vy>=0&&old_y<=top&&g->y>=top){
                g->y=top;g->vy=0;g->grounded=1;g->platform=i;g->coyote_ms=0;
                if(i>g->furthest)g->furthest=i;
                if(i%3==0&&i>g->checkpoint){g->checkpoint=i;g->feedback_ms=1200;g->effect=7;}
                if(!dir)g->vx=0;
            }else if(g->y>top&&g->y-RP_BODY*RP_Q<240*RP_Q){
                if(old_x+RP_HALF*RP_Q<=left){g->x=left-RP_HALF*RP_Q;g->vx=0;}
                else if(old_x-RP_HALF*RP_Q>=r){g->x=r+RP_HALF*RP_Q;g->vx=0;}
                /* Roofs are solid down to the bottom of the screen. There is
                 * no underside entry or snap onto a roof after falling. */
            }
        }
    }
    if(g->grounded){
        rp_platform_t *p=&g->platforms[g->platform];
        if(g->x+RP_HALF*RP_Q<=p->x*RP_Q||g->x-RP_HALF*RP_Q>=(p->x+p->w)*RP_Q){g->grounded=0;g->platform=-1;g->vy=0;g->coyote_ms=RP_COYOTE_MS;}
    }
    for(int i=1;i<RP_PLATFORMS;i++){
        rp_platform_t *p=&g->platforms[i];int cx=(p->x+p->w/2)*RP_Q,cy=(p->y-30)*RP_Q;
        if(!(g->collected&(1u<<i))&&g->x+10*RP_Q>cx&&g->x-10*RP_Q<cx&&g->y>cy-4*RP_Q&&g->y-RP_BODY*RP_Q<cy+4*RP_Q){g->collected|=1u<<i;g->score+=100;g->effect=4;}
        if(p->hazard){int hx=rp_hazard_x(g,i)*RP_Q;
            if(g->x+RP_HALF*RP_Q>hx-7*RP_Q&&g->x-RP_HALF*RP_Q<hx+7*RP_Q&&g->y>(p->y-10)*RP_Q&&g->y-RP_BODY*RP_Q<p->y*RP_Q){fail(g);return;}
        }
    }
    /* A released jump shortly before touchdown is consumed once. Pause,
     * respawn and leaving a scene cancel pending input. No mid-air jump. */
    if(g->grounded&&g->jump_buffer_ms>0)jump(g);
    if(g->jump_buffer_ms>0)g->jump_buffer_ms-=20;
    if(g->y>260*RP_Q){fail(g);return;}
    rp_platform_t *end=&g->platforms[11];if(g->grounded&&g->platform==11&&g->x>=(end->x+end->w-24)*RP_Q){finish(g);return;}
    int target=clamp(g->x/RP_Q-112,0,end->x+end->w-260);
    g->camera+=clamp(target-g->camera,-8,8);
}
void rp_update(rp_game_t *g,uint32_t now){
    uint32_t dt=now-g->last_ms;if((int32_t)dt<0)dt=0;else g->last_ms=now;if(dt>250)dt=250;
    if(g->phase==RP_TITLE||g->phase==RP_PLAY)g->scene_ms+=dt;
    if(g->phase==RP_FAILED){g->respawn_ms-=(int)dt;if(g->respawn_ms<=0){phase(g,RP_PLAY);place(g);}return;}
    if(g->phase==RP_PLAY){g->accumulator_ms+=dt;while(g->accumulator_ms>=20&&g->phase==RP_PLAY){g->accumulator_ms-=20;step(g);}}
    for(int k=0;k<3;k++){
        unsigned bit=1u<<k;if(!(g->held&bit)||(g->long_sent&bit)||g->last_ms-g->pressed_ms[k]<800)continue;
        if(g->phase==RP_PLAY&&k==2){phase(g,RP_PAUSED);return;}
        if(g->phase==RP_PAUSED&&k==0){phase(g,RP_TITLE);return;}
        if(g->phase==RP_CLEAR&&k==1){phase(g,RP_TITLE);return;}
        if(g->phase==RP_TITLE&&k==0){g->long_sent|=bit;return;}
    }
}
void rp_edge(rp_game_t *g,int key,int down,uint32_t now){
    if(key<0||key>2)return;
    rp_update(g,now);unsigned bit=1u<<key;if(g->blocked&bit){if(!down)g->blocked&=~bit;return;}
    if(down){
        if(g->held||g->phase==RP_FAILED)return;
        g->held=bit;g->pressed_ms[key]=g->last_ms;g->long_sent&=~bit;
        if(g->phase==RP_PLAY&&key==2)g->jump_dir=!g->grounded?(g->vx>0?1:g->vx<0?-1:0):g->last_ms-g->released_ms<=RP_SWITCH_MS?g->last_dir:0;
        return;
    }
    if(!(g->held&bit))return;
    int long_press=(g->long_sent&bit)!=0;g->held=0;g->long_sent&=~bit;
    if(long_press){if(g->phase==RP_TITLE&&key==0)g->muted=!g->muted;return;}
    if(g->phase==RP_TITLE){if(key==1)rp_start(g);else if(key==0&&g->stage>1){g->stage--;layout(g);}else if(key==2&&g->stage<g->unlocked){g->stage++;layout(g);}}
    else if(g->phase==RP_PAUSED&&key==1)phase(g,RP_PLAY);
    else if(g->phase==RP_CLEAR&&key==1){if(g->stage<5)g->stage++;rp_start(g);}
    else if(g->phase==RP_PLAY){
        if(key==2)jump(g);
        else{g->last_dir=key==0?1:-1;g->released_ms=g->last_ms;if(g->grounded)g->vx=0;}
    }
}
void rp_cancel(rp_game_t *g,uint32_t now){rp_update(g,now);if(g->phase==RP_PLAY)phase(g,RP_PAUSED);else clear_input(g);}
uint32_t rp_hash(const rp_game_t *g){
    uint32_t h=2166136261u;
#define HASH(v) do{uint32_t q=(uint32_t)(v);for(int b=0;b<4;b++){h=(h^(q&255))*16777619u;q>>=8;}}while(0)
    HASH(g->medals);HASH(g->phase);HASH(g->rng);HASH(g->last_ms);HASH(g->scene_ms);HASH(g->elapsed_ms);HASH(g->accumulator_ms);for(int i=0;i<3;i++)HASH(g->pressed_ms[i]);HASH(g->released_ms);
    HASH(g->stage);HASH(g->unlocked);HASH(g->best);HASH(g->score);HASH(g->battery);HASH(g->muted);HASH(g->held);HASH(g->blocked);HASH(g->long_sent);HASH(g->collected);
    HASH(g->x);HASH(g->y);HASH(g->vx);HASH(g->vy);HASH(g->grounded);HASH(g->platform);HASH(g->camera);HASH(g->checkpoint);HASH(g->furthest);HASH(g->last_dir);HASH(g->jump_dir);HASH(g->respawn_ms);HASH(g->falls);HASH(g->feedback_ms);HASH(g->effect);HASH(g->coyote_ms);HASH(g->jump_buffer_ms);HASH(g->facing);
    for(int i=0;i<RP_PLATFORMS;i++){HASH(g->platforms[i].x);HASH(g->platforms[i].y);HASH(g->platforms[i].w);HASH(g->platforms[i].hazard);}
#undef HASH
    return h;
}
