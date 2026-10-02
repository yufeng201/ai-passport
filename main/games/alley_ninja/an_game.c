#include "an_game.h"
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static int ab(int n){return n<0?-n:n;}
static uint32_t random_next(an_game_t *g){g->rng^=g->rng<<13;g->rng^=g->rng>>17;g->rng^=g->rng<<5;return g->rng;}
static void clear_input(an_game_t *g){g->blocked|=g->held;g->held=0;g->long_sent=0;g->guarding=0;g->guard_ms=0;}
static void phase(an_game_t *g,an_phase_t p){g->phase=p;g->accumulator_ms=0;clear_input(g);}
static void feedback(an_game_t *g,int kind){
    /* Let the perfect-block label survive the immediate follow-up slash. */
    if(kind==5&&g->feedback==1&&g->feedback_ms>0)return;
    g->feedback=kind;g->feedback_ms=650;
}
static void finish(an_game_t *g,int won){
    if(g->score>g->best)g->best=g->score;
    if(won&&g->stage<5&&g->unlocked<=g->stage)g->unlocked=g->stage+1;
    if(won)g->medals=game_medal_record(g->medals,g->stage,1+(g->health==3)+(g->combo>=3));
    phase(g,won?AN_CLEAR:AN_FAILED);g->effect=won?3:2;
}
void an_init(an_game_t *g,uint32_t now,uint32_t seed){*g=(an_game_t){.phase=AN_TITLE,.rng=seed?seed:1,.last_ms=now,.stage=1,.unlocked=1,.battery=-1};}
void an_start(an_game_t *g){
    uint32_t medals=g->medals;
    int stage=clamp(g->stage,1,5),unlocked=clamp(g->unlocked,1,5),best=g->best,battery=g->battery,muted=g->muted;
    uint32_t rng=g->rng,now=g->last_ms;unsigned blocked=g->blocked|g->held;
    *g=(an_game_t){.medals=medals,.phase=AN_PLAY,.rng=rng,.last_ms=now,.stage=stage,.unlocked=unlocked,.best=best,.battery=battery,
        .muted=muted,.blocked=blocked,.health=3,.stamina=100,.goal=stage==5?5:stage+4,.spawn_ms=600};
}
static int warning(an_game_t *g){return g->stage>=4?800+(int)(random_next(g)%3)*200:1300-g->stage*100;}
static void spawn(an_game_t *g){
    int side=g->stage==1?1:g->spawned%2?-1:1;
    int boss=g->stage==5&&g->spawned==g->goal-1;
    int armor=g->stage>=3&&(g->spawned%2==0||boss);
    int hp=boss?5:armor?2:1;
    g->enemy=(an_enemy_t){.active=1,.x=160+side*130,.side=side,.hp=hp,.max_hp=hp,.kind=boss?2:armor?1:0,.phase=AN_ENTRY};
    g->spawned++;
}
static void add_score(an_game_t *g,int value){g->score=clamp(g->score+value,0,1000000);}
static void slash(an_game_t *g,int side){
    if(g->cooldown_ms>0||g->broken_ms>0)return;
    g->slash_side=side;g->slash_ms=160;g->cooldown_ms=240;g->effect=1;
    an_enemy_t *e=&g->enemy;
    if(!e->active||e->side!=side||ab(e->x-160)>AN_RANGE){feedback(g,4);return;}
    if(e->kind&&e->vulnerable_ms<=0){feedback(g,3);return;}
    e->hp-=g->riposte_ms>0?2:1;g->riposte_ms=0;feedback(g,5);
    if(e->hp<=0){e->active=0;g->kills++;add_score(g,e->kind==2?1000:e->kind==1?200:100);g->spawn_ms=1100-g->stage*80;g->effect=3;}
}
static void strike(an_game_t *g){
    an_enemy_t *e=&g->enemy;
    if(g->guarding&&g->stamina>=24&&g->broken_ms==0){
        int perfect=g->guard_ms<=AN_PERFECT_MS;
        g->stamina-=perfect?8:24;e->vulnerable_ms=1000;
        feedback(g,perfect?1:2);g->effect=perfect?5:6;
        if(perfect){g->combo=clamp(g->combo+1,0,99);add_score(g,20*g->combo);if(g->combo>=2)g->riposte_ms=1000;}else g->combo=0;
    }else{
        if(g->damage_ms==0){g->health--;g->damage_ms=700;g->effect=2;}
        g->combo=0;g->riposte_ms=0;feedback(g,6);
        if(g->guarding){g->stamina=0;g->guarding=0;g->broken_ms=800;feedback(g,7);}
    }
    e->phase=AN_RECOVER;e->timer_ms=1100;
}
static void step(an_game_t *g){
    g->elapsed_ms+=20;
    if(g->riposte_ms>0)g->riposte_ms-=20;
    if(g->slash_ms>0)g->slash_ms-=20;
    if(g->cooldown_ms>0)g->cooldown_ms-=20;
    if(g->damage_ms>0)g->damage_ms-=20;
    if(g->feedback_ms>0)g->feedback_ms-=20;
    if(g->broken_ms>0)g->broken_ms-=20;
    if(g->guarding){
        g->guard_ms+=20;
        if(g->guard_ms%40==0&&g->stamina>0)g->stamina--;
        if(g->stamina==0){g->guarding=0;g->broken_ms=800;feedback(g,7);}
    }else if(g->broken_ms==0&&g->stamina<100)g->stamina++;
    an_enemy_t *e=&g->enemy;
    if(!e->active){g->spawn_ms-=20;if(g->spawned<g->goal&&g->spawn_ms<=0)spawn(g);}
    else{
        if(e->vulnerable_ms>0)e->vulnerable_ms-=20;
        if(e->phase==AN_ENTRY){
            e->x-=e->side*(g->stage>=4?3:2);
            if(ab(e->x-160)<=36){e->x=160+e->side*36;e->phase=AN_WINDUP;e->timer_ms=warning(g);e->warning_ms=e->timer_ms;}
        }else if(e->phase==AN_WINDUP){e->timer_ms-=20;if(e->timer_ms<=0)strike(g);}
        else{
            e->timer_ms-=20;
            if(e->timer_ms<=0){
                if(e->kind==2){e->side=-e->side;e->x=160+e->side*130;e->phase=AN_ENTRY;}
                else{e->phase=AN_WINDUP;e->timer_ms=warning(g);e->warning_ms=e->timer_ms;}
            }
        }
    }
    if(g->health<=0)finish(g,0);else if(g->kills>=g->goal)finish(g,1);
}
void an_update(an_game_t *g,uint32_t now){
    uint32_t key_now=now,dt=now-g->last_ms;
    if((int32_t)dt<0)dt=0;else g->last_ms=now;
    if(dt>250)dt=250;
    if(g->phase==AN_TITLE||g->phase==AN_PLAY)g->scene_ms+=dt;
    if(g->phase==AN_PLAY){g->accumulator_ms+=dt;while(g->accumulator_ms>=20&&g->phase==AN_PLAY){g->accumulator_ms-=20;step(g);}}
    for(int key=0;key<3;key++){
        unsigned bit=1u<<key;if(!(g->held&bit)||(g->long_sent&bit)||key_now-g->pressed_ms[key]<800)continue;
        if(g->phase==AN_PLAY&&key==2){phase(g,AN_PAUSED);return;}
        if(g->phase==AN_PAUSED&&key==0){phase(g,AN_TITLE);return;}
        if((g->phase==AN_CLEAR||g->phase==AN_FAILED)&&key==1){phase(g,AN_TITLE);return;}
        if(g->phase==AN_TITLE&&key==0){g->long_sent|=bit;return;}
    }
}
void an_edge(an_game_t *g,int key,int down,uint32_t now){
    if(key<0||key>2)return;
    an_update(g,now);unsigned bit=1u<<key;
    if(g->blocked&bit){if(!down)g->blocked&=~bit;return;}
    if(down){
        if(g->held)return;
        g->held|=bit;g->long_sent&=~bit;g->pressed_ms[key]=now;
        if(g->phase==AN_PLAY&&key==1&&g->broken_ms==0&&g->stamina>=24){g->guarding=1;g->guard_ms=0;}return;
    }
    if(!(g->held&bit))return;
    int was_long=(g->long_sent&bit)!=0;g->held&=~bit;g->long_sent&=~bit;
    if(was_long){if(g->phase==AN_TITLE&&key==0)g->muted=!g->muted;return;}
    if(g->phase==AN_TITLE){if(key==1)an_start(g);else if(key==0&&g->stage>1)g->stage--;else if(key==2&&g->stage<g->unlocked)g->stage++;}
    else if(g->phase==AN_PAUSED&&key==1)phase(g,AN_PLAY);
    else if((g->phase==AN_CLEAR||g->phase==AN_FAILED)&&key==1){if(g->phase==AN_CLEAR&&g->stage<5)g->stage++;an_start(g);}
    else if(g->phase==AN_PLAY){if(key==1){g->guarding=0;g->guard_ms=0;}else slash(g,key==0?-1:1);}
}
void an_cancel(an_game_t *g,uint32_t now){an_update(g,now);if(g->phase==AN_PLAY)phase(g,AN_PAUSED);else clear_input(g);}
uint32_t an_hash(const an_game_t *g){
    uint32_t h=2166136261u;
#define HASH(v) do{uint32_t q=(uint32_t)(v);for(int b=0;b<4;b++){h=(h^(q&255))*16777619u;q>>=8;}}while(0)
    HASH(g->medals);HASH(g->phase);HASH(g->rng);HASH(g->last_ms);HASH(g->scene_ms);HASH(g->elapsed_ms);HASH(g->accumulator_ms);for(int i=0;i<3;i++)HASH(g->pressed_ms[i]);
    HASH(g->stage);HASH(g->unlocked);HASH(g->best);HASH(g->score);HASH(g->battery);HASH(g->muted);HASH(g->health);HASH(g->stamina);HASH(g->guarding);HASH(g->guard_ms);HASH(g->broken_ms);
    HASH(g->riposte_ms);HASH(g->spawned);HASH(g->kills);HASH(g->goal);HASH(g->spawn_ms);HASH(g->slash_ms);HASH(g->slash_side);HASH(g->cooldown_ms);HASH(g->damage_ms);HASH(g->feedback_ms);HASH(g->feedback);HASH(g->combo);HASH(g->effect);HASH(g->held);HASH(g->blocked);HASH(g->long_sent);
    const an_enemy_t *e=&g->enemy;HASH(e->active);HASH(e->x);HASH(e->side);HASH(e->hp);HASH(e->max_hp);HASH(e->kind);HASH(e->timer_ms);HASH(e->warning_ms);HASH(e->vulnerable_ms);HASH(e->phase);
#undef HASH
    return h;
}
