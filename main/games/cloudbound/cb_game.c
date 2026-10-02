#include "cb_game.h"
static int clamp(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static int ab(int x){return x<0?-x:x;}
static uint32_t random_next(cb_game_t *g){g->rng^=g->rng<<13;g->rng^=g->rng>>17;g->rng^=g->rng<<5;return g->rng;}
static void clear_input(cb_game_t *g){g->blocked|=g->held;g->held=0;g->long_sent=0;g->charging=0;g->charge_ms=0;}
static void phase(cb_game_t *g,cb_phase_t p){g->phase=p;clear_input(g);g->accumulator_ms=0;}
static void finish(cb_game_t *g,int won){
    if(g->score>g->best)g->best=g->score;
    if(won&&g->unlocked<=g->stage&&g->stage<CB_STAGES)g->unlocked=g->stage+1;
    if(won)g->medals=game_medal_record(g->medals,g->stage,1+(!g->rescued)+(g->combo>=3));
    phase(g,won?CB_CLEAR:CB_FAILED);g->effect=won?3:2;
}
int cb_jump_distance(int ms){return 32+clamp(ms,100,CB_CHARGE_MAX)*128/CB_CHARGE_MAX;}
int cb_landing_x(const cb_game_t *g,int ms){return g->x+cb_jump_distance(ms);}
int cb_can_land(const cb_game_t *g,int ms){int x=cb_landing_x(g,ms);return x>=g->target.x+4&&x<g->target.x+g->target.w-4;}
int cb_jump_y(const cb_game_t *g,int ms){
    int t=clamp(ms,0,CB_FLIGHT_MS);
    return g->current.y-g->jump_rise*t/CB_FLIGHT_MS-4*58*t*(CB_FLIGHT_MS-t)/(CB_FLIGHT_MS*CB_FLIGHT_MS);
}
static void next_platform(cb_game_t *g){
    int hold=g->stage==1?700+(int)(random_next(g)%201):560+(int)(random_next(g)%521);
    int w=84-g->stage*8,center=g->x+cb_jump_distance(hold);
    int rise=g->stage>=4?((int)(random_next(g)%3)-1)*18:0;
    g->target=(cb_platform_t){center-w/2,clamp(g->current.y-rise,135,185),w,16};
}
void cb_init(cb_game_t *g,uint32_t now,uint32_t seed){
    *g=(cb_game_t){.phase=CB_TITLE,.rng=seed?seed:1,.last_ms=now,.stage=1,.unlocked=1,.battery=-1};
}
void cb_start(cb_game_t *g){
    uint32_t medals=g->medals;
    int stage=clamp(g->stage,1,CB_STAGES),unlocked=clamp(g->unlocked,1,CB_STAGES);
    uint32_t seed=g->rng,now=g->last_ms;unsigned blocked=g->blocked|g->held;
    int best=g->best,battery=g->battery,muted=g->muted;
    *g=(cb_game_t){.medals=medals,.phase=CB_PLAY,.rng=seed,.last_ms=now,.stage=stage,.unlocked=unlocked,
        .best=best,.battery=battery,.muted=muted,.blocked=blocked,.x=70,.y=170,
        .current={30,170,80,16},.effect=3};
    next_platform(g);
}
static void step(cb_game_t *g){
    g->elapsed_ms+=20;
    if(g->feedback_ms>0)g->feedback_ms-=20;
    if(g->scroll_ms>0){
        g->scroll_ms-=20;g->camera=g->scroll_dx*(300-g->scroll_ms)/300;
        if(g->scroll_ms<=0){
            g->current=g->target;g->current.x-=g->scroll_dx;g->x-=g->scroll_dx;
            g->camera=0;next_platform(g);
        }
        return;
    }
    if(!g->flying)return;
    g->flight_ms+=20;g->x=70+g->jump_dx*g->flight_ms/CB_FLIGHT_MS;g->y=cb_jump_y(g,g->flight_ms);
    if(g->flight_ms<CB_FLIGHT_MS)return;
    g->flying=0;
    if(g->x<g->target.x+4||g->x>=g->target.x+g->target.w-4){finish(g,0);return;}
    int precise=ab(g->x-(g->target.x+g->target.w/2))<=8;
    g->landings++;g->combo=precise?g->combo+1:0;g->score+=100+g->combo*20;
    g->feedback_ms=600;g->effect=precise?5:4;
    if(g->landings>=CB_GOAL){finish(g,1);return;}
    g->scroll_dx=g->x-70;g->scroll_ms=300;g->camera=0;
}
void cb_update(cb_game_t *g,uint32_t now){
    uint32_t key_now=now,dt=now-g->last_ms;
    /* Events queued just before a frame's clock sample may be slightly older.
     * Do not interpret these as a full timer wrap and advance 250 ms. */
    if((int32_t)dt<0)now=g->last_ms;
    else g->last_ms=now;
    dt=(int32_t)dt<0?0:dt>250?250:dt;
    if(g->phase==CB_TITLE||g->phase==CB_PLAY)g->scene_ms+=dt;
    if(g->phase==CB_PLAY){
        g->accumulator_ms+=dt;
        while(g->accumulator_ms>=20&&g->phase==CB_PLAY){g->accumulator_ms-=20;step(g);}
        if(g->charging){uint32_t held_ms=key_now-g->pressed_ms[1];g->charge_ms=(int)(held_ms>CB_CHARGE_MAX?CB_CHARGE_MAX:held_ms);}
    }
    for(int key=0;key<3;++key){unsigned bit=1u<<key;
        if(!(g->held&bit)||(g->long_sent&bit)||key_now-g->pressed_ms[key]<800)continue;
        if(g->phase==CB_PLAY&&key==2){phase(g,CB_PAUSED);return;}
        if(g->phase==CB_PAUSED&&key==0){phase(g,CB_TITLE);return;}
        if((g->phase==CB_CLEAR||g->phase==CB_FAILED)&&key==1){phase(g,CB_TITLE);return;}
        if(g->phase==CB_TITLE&&key==0){g->long_sent|=bit;return;}
    }
}
void cb_edge(cb_game_t *g,int key,int down,uint32_t now){
    if(key<0||key>2)return;
    cb_update(g,now);unsigned bit=1u<<key;
    if(g->blocked&bit){if(!down)g->blocked&=~bit;return;}
    if(down){
        if(g->held)return;
        g->held|=bit;g->long_sent&=~bit;g->pressed_ms[key]=now;
        if(g->phase==CB_PLAY&&key==1&&!g->flying&&!g->scroll_ms){g->charging=1;g->charge_ms=0;}
        return;
    }
    if(!(g->held&bit))return;
    int was_long=(g->long_sent&bit)!=0;uint32_t duration=now-g->pressed_ms[key];
    g->held&=~bit;g->long_sent&=~bit;
    if(was_long){if(g->phase==CB_TITLE&&key==0)g->muted=!g->muted;return;}
    if(g->phase==CB_TITLE){
        if(key==1)cb_start(g);
        else if(key==0&&g->stage>1)g->stage--;
        else if(key==2&&g->stage<g->unlocked)g->stage++;
    }else if(g->phase==CB_FAILED&&key==0&&!g->rescued){
        phase(g,CB_PLAY);g->rescued=1;g->x=70;g->y=g->current.y;
        g->flying=0;g->flight_ms=0;g->combo=0;g->feedback_ms=600;g->effect=7;
    }else if(g->phase==CB_PAUSED&&key==1){phase(g,CB_PLAY);}
    else if((g->phase==CB_CLEAR||g->phase==CB_FAILED)&&key==1){
        if(g->phase==CB_CLEAR&&g->stage<CB_STAGES)g->stage++;
        cb_start(g);
    }else if(g->phase==CB_PLAY&&key==1&&g->charging){
        g->charging=0;g->charge_ms=0;g->flying=1;g->flight_ms=0;
        g->jump_dx=cb_jump_distance((int)(duration>CB_CHARGE_MAX?CB_CHARGE_MAX:duration));
        g->jump_rise=g->current.y-g->target.y;g->effect=1;
    }
}
void cb_cancel(cb_game_t *g,uint32_t now){
    cb_update(g,now);if(g->phase==CB_PLAY)phase(g,CB_PAUSED);else clear_input(g);
}
uint32_t cb_hash(const cb_game_t *g){
    uint32_t h=2166136261u;
#define HASH(v) do{uint32_t q=(uint32_t)(v);for(int b=0;b<4;++b){h=(h^(q&255))*16777619u;q>>=8;}}while(0)
    HASH(g->medals);HASH(g->phase);HASH(g->rng);HASH(g->last_ms);HASH(g->scene_ms);HASH(g->elapsed_ms);HASH(g->accumulator_ms);
    for(int i=0;i<3;++i)HASH(g->pressed_ms[i]);
    HASH(g->stage);HASH(g->unlocked);HASH(g->best);HASH(g->score);HASH(g->combo);HASH(g->landings);HASH(g->battery);HASH(g->muted);
    HASH(g->held);HASH(g->blocked);HASH(g->long_sent);HASH(g->rescued);HASH(g->charging);HASH(g->charge_ms);HASH(g->flying);HASH(g->flight_ms);
    HASH(g->jump_dx);HASH(g->jump_rise);HASH(g->x);HASH(g->y);HASH(g->camera);HASH(g->scroll_ms);HASH(g->scroll_dx);HASH(g->feedback_ms);HASH(g->effect);
    HASH(g->current.x);HASH(g->current.y);HASH(g->current.w);HASH(g->current.h);HASH(g->target.x);HASH(g->target.y);HASH(g->target.w);HASH(g->target.h);
#undef HASH
    return h;
}
