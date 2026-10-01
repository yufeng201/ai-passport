#include "sg_game.h"
static int clamp(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static uint32_t random_next(sg_game_t *g){g->rng^=g->rng<<13;g->rng^=g->rng>>17;g->rng^=g->rng<<5;return g->rng;}
int sg_aim_dx(int aim){static const int values[]={-8,-6,-4,-2,0,2,4,6,8};return values[clamp(aim,0,8)];}
int sg_aim_dy(int aim){static const int values[]={-8,-10,-11,-12,-12,-12,-11,-10,-8};return values[clamp(aim,0,8)];}
static int abs_i(int x){return x<0?-x:x;}
int sg_segment_hits(int x,int y,int tx,int ty,int left,int top,int right,int bottom){
    int dx=abs_i(tx-x),sx=x<tx?1:-1,dy=-abs_i(ty-y),sy=y<ty?1:-1,err=dx+dy;
    for(;;){
        if(x>=left&&x<=right&&y>=top&&y<=bottom)return 1;
        if(x==tx&&y==ty)return 0;
        int e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}
    }
}
static void clear_input(sg_game_t *g){g->blocked|=g->held;g->held=0;g->long_sent=0;g->charging=0;g->charge_ms=0;}
static void phase(sg_game_t *g,sg_phase_t p){g->phase=p;clear_input(g);g->accumulator_ms=0;}
static void finish(sg_game_t *g,int won){
    if(g->score>g->best)g->best=g->score;
    if(won&&g->unlocked<=g->stage&&g->stage<SG_STAGES)g->unlocked=g->stage+1;
    phase(g,won?SG_CLEAR:SG_FAILED);g->effect=won?3:2;
}
void sg_init(sg_game_t *g,uint32_t now,uint32_t seed){*g=(sg_game_t){.phase=SG_TITLE,.rng=seed?seed:1,.last_ms=now,.stage=1,.unlocked=1,.battery=-1};}
void sg_start(sg_game_t *g){
    int stage=clamp(g->stage,1,5),unlocked=clamp(g->unlocked,1,5),best=g->best,battery=g->battery,muted=g->muted;
    uint32_t rng=g->rng,now=g->last_ms;unsigned blocked=g->held|g->blocked;
    *g=(sg_game_t){.phase=SG_PLAY,.rng=rng,.last_ms=now,.stage=stage,.unlocked=unlocked,.best=best,
        .battery=battery,.muted=muted,.blocked=blocked,.health=3,.aim=4,.goal=stage==5?5:stage+5,.spawn_ms=600,.effect=3};
}
static void spawn(sg_game_t *g){
    int active=0;for(int i=0;i<SG_ENEMIES;i++)active+=g->enemies[i].active!=0;
    if(g->spawned>=g->goal||active>=3)return;
    for(int i=0;i<SG_ENEMIES;i++)if(!g->enemies[i].active){
        int boss=g->stage==5&&g->spawned==g->goal-1;
        int armor=g->stage>=3&&g->spawned%2==0;
        int hp=boss?16:armor?(g->stage>=4?3:2):1;
        int x=112+(int)(random_next(g)%97);
        g->enemies[i]=(sg_enemy_t){1,x,42,hp,hp,boss?2:armor?1:0,(g->spawned%2)?1:-1,
            2600-g->stage*140,++g->next_id};
        /* Slot reuse must not retain a previous occupant's hit bit. */
        for(int j=0;j<SG_SHOTS;j++)g->shots[j].hit_mask&=~(1u<<i);
        g->spawned++;return;
    }
}
static int fire(sg_game_t *g,int charge){
    if(g->shot_ms>0)return 0;
    int charged=charge>=SG_CHARGE_THRESHOLD&&g->cooldown_ms==0;
    for(int i=0;i<SG_SHOTS;i++)if(!g->shots[i].active){
        g->shots[i]=(sg_shot_t){1,160,189,sg_aim_dx(g->aim),sg_aim_dy(g->aim),charged?3:1,charged?3:1,0};
        g->shot_ms=180;if(charged)g->cooldown_ms=SG_COOLDOWN;g->effect=1;return 1;
    }
    return 0;
}
static void blast(sg_game_t *g,int x,int y){g->flash_ms=180;g->flash_x=x;g->flash_y=y;}
static void step(sg_game_t *g){
    g->elapsed_ms+=20;
    if(g->cooldown_ms>0)g->cooldown_ms-=20;
    if(g->shot_ms>0)g->shot_ms-=20;
    if(g->damage_ms>0)g->damage_ms-=20;
    if(g->flash_ms>0)g->flash_ms-=20;
    g->spawn_ms-=20;
    if(g->spawn_ms<=0){spawn(g);g->spawn_ms=2200-g->stage*180;}
    for(int i=0;i<SG_ENEMIES;i++){
        sg_enemy_t *e=&g->enemies[i];if(!e->active)continue;
        if(e->y<76+(i%3)*12)e->y++;
        else if(g->stage>=2&&g->elapsed_ms%((g->stage>=4?40:80))==0){
            e->x+=e->dir;if(e->x<=82)e->dir=1;if(e->x>=238)e->dir=-1;
        }
        e->attack_ms-=20;
        if(e->attack_ms<=0){
            for(int j=0;j<SG_THREATS;j++)if(!g->threats[j].active){
                g->threats[j]=(sg_threat_t){1,e->x,e->y+12,800};break;
            }
            e->attack_ms=e->kind==2?2200:4200-g->stage*220;
        }
    }
    /* Threats stay still during a visible warning, then descend at 100px/s. */
    for(int i=0;i<SG_THREATS;i++){
        sg_threat_t *t=&g->threats[i];if(!t->active)continue;
        if(t->warning_ms>0){t->warning_ms-=20;continue;}
        t->y+=2;
        if(t->y>=198){
            t->active=0;if(g->damage_ms==0){g->health--;g->damage_ms=700;g->effect=2;blast(g,t->x,198);}
        }
    }
    for(int i=0;i<SG_SHOTS;i++){
        sg_shot_t *s=&g->shots[i];if(!s->active)continue;
        int tx=s->x+s->dx,ty=s->y+s->dy,r=s->power>1?4:2;
        for(int j=0;j<SG_THREATS&&s->active;j++){
            sg_threat_t *t=&g->threats[j];if(!t->active)continue;
            if(sg_segment_hits(s->x,s->y,tx,ty,t->x-6-r,t->y-6-r,t->x+6+r,t->y+6+r)){
                t->active=0;g->score+=25;blast(g,t->x,t->y);if(--s->pierce<=0)s->active=0;
            }
        }
        for(int j=0;j<SG_ENEMIES&&s->active;j++){
            sg_enemy_t *e=&g->enemies[j];if(!e->active||(s->hit_mask&(1u<<j)))continue;
            int w=e->kind==2?26:13;
            if(sg_segment_hits(s->x,s->y,tx,ty,e->x-w-r,e->y-9-r,e->x+w+r,e->y+9+r)){
                s->hit_mask|=1u<<j;e->hp-=s->power;blast(g,e->x,e->y);
                if(e->hp<=0){e->active=0;g->kills++;g->score+=e->kind==2?1200:e->kind==1?200:100;}
                if(--s->pierce<=0)s->active=0;
            }
        }
        s->x=tx;s->y=ty;if(tx<8||tx>312||ty<28)s->active=0;
    }
    if(g->health<=0)finish(g,0);
    else if(g->kills>=g->goal)finish(g,1);
}
void sg_update(sg_game_t *g,uint32_t now){
    uint32_t key_now=now,dt=now-g->last_ms;
    if((int32_t)dt<0)dt=0;else g->last_ms=now;
    if(dt>250)dt=250;
    if(g->phase==SG_TITLE||g->phase==SG_PLAY)g->scene_ms+=dt;
    if(g->phase==SG_PLAY){
        g->accumulator_ms+=dt;
        while(g->accumulator_ms>=20&&g->phase==SG_PLAY){g->accumulator_ms-=20;step(g);}
        if(g->charging){uint32_t d=key_now-g->pressed_ms[1];g->charge_ms=(int)(d>SG_CHARGE_MAX?SG_CHARGE_MAX:d);}
    }
    for(int key=0;key<3;key++){
        unsigned bit=1u<<key;if(!(g->held&bit)||(g->long_sent&bit)||key_now-g->pressed_ms[key]<800)continue;
        if(g->phase==SG_PLAY&&key==2){phase(g,SG_PAUSED);return;}
        if(g->phase==SG_PAUSED&&key==0){phase(g,SG_TITLE);return;}
        if((g->phase==SG_CLEAR||g->phase==SG_FAILED)&&key==1){phase(g,SG_TITLE);return;}
        if(g->phase==SG_TITLE&&key==0){g->long_sent|=bit;return;}
    }
}
void sg_edge(sg_game_t *g,int key,int down,uint32_t now){
    if(key<0||key>2)return;
    sg_update(g,now);unsigned bit=1u<<key;
    if(g->blocked&bit){if(!down)g->blocked&=~bit;return;}
    if(down){
        if(g->held)return;
        g->held|=bit;g->long_sent&=~bit;g->pressed_ms[key]=now;
        if(g->phase==SG_PLAY&&key==1){g->charging=1;g->charge_ms=0;}return;
    }
    if(!(g->held&bit))return;
    unsigned was_long=g->long_sent&bit;uint32_t duration=now-g->pressed_ms[key];g->held&=~bit;g->long_sent&=~bit;
    if(was_long){if(g->phase==SG_TITLE&&key==0)g->muted=!g->muted;return;}
    if(g->phase==SG_TITLE){if(key==1)sg_start(g);else if(key==0&&g->stage>1)g->stage--;else if(key==2&&g->stage<g->unlocked)g->stage++;}
    else if(g->phase==SG_PAUSED&&key==1)phase(g,SG_PLAY);
    else if((g->phase==SG_CLEAR||g->phase==SG_FAILED)&&key==1){if(g->phase==SG_CLEAR&&g->stage<5)g->stage++;sg_start(g);}
    else if(g->phase==SG_PLAY){
        if(key==0)g->aim=clamp(g->aim-1,0,8);
        else if(key==2)g->aim=clamp(g->aim+1,0,8);
        else if(key==1&&g->charging){g->charging=0;g->charge_ms=0;fire(g,(int)(duration>SG_CHARGE_MAX?SG_CHARGE_MAX:duration));}
    }
}
void sg_cancel(sg_game_t *g,uint32_t now){sg_update(g,now);if(g->phase==SG_PLAY)phase(g,SG_PAUSED);else clear_input(g);}
uint32_t sg_hash(const sg_game_t *g){
    uint32_t h=2166136261u;
#define HASH(v) do{uint32_t q=(uint32_t)(v);for(int b=0;b<4;b++){h=(h^(q&255))*16777619u;q>>=8;}}while(0)
    HASH(g->phase);HASH(g->rng);HASH(g->last_ms);HASH(g->scene_ms);HASH(g->elapsed_ms);HASH(g->accumulator_ms);HASH(g->next_id);
    for(int i=0;i<3;i++)HASH(g->pressed_ms[i]);
    HASH(g->stage);HASH(g->unlocked);HASH(g->best);HASH(g->score);HASH(g->battery);HASH(g->muted);HASH(g->health);HASH(g->aim);
    HASH(g->spawn_ms);HASH(g->spawned);HASH(g->kills);HASH(g->goal);HASH(g->charging);HASH(g->charge_ms);HASH(g->cooldown_ms);
    HASH(g->shot_ms);HASH(g->damage_ms);HASH(g->flash_ms);HASH(g->flash_x);HASH(g->flash_y);HASH(g->effect);HASH(g->held);HASH(g->blocked);HASH(g->long_sent);
    for(int i=0;i<SG_ENEMIES;i++){const sg_enemy_t *e=&g->enemies[i];HASH(e->active);HASH(e->x);HASH(e->y);HASH(e->hp);HASH(e->max_hp);HASH(e->kind);HASH(e->dir);HASH(e->attack_ms);HASH(e->id);}
    for(int i=0;i<SG_SHOTS;i++){const sg_shot_t *s=&g->shots[i];HASH(s->active);HASH(s->x);HASH(s->y);HASH(s->dx);HASH(s->dy);HASH(s->power);HASH(s->pierce);HASH(s->hit_mask);}
    for(int i=0;i<SG_THREATS;i++){const sg_threat_t *t=&g->threats[i];HASH(t->active);HASH(t->x);HASH(t->y);HASH(t->warning_ms);}
#undef HASH
    return h;
}
