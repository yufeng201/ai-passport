#include "rr_game.h"

static const rr_stage_t stages[RR_STAGES] = {
    {1600,1400,155,5}, {1800,1250,165,3}, {2000,1100,175,50},
    {2200,950,185,80}, {2400,800,195,3}
};
const rr_stage_t *rr_stage(const rr_game_t *g)
{
    int stage = g->stage < 1 || g->stage > RR_STAGES ? 1 : g->stage;
    return &stages[stage-1];
}
int rr_bonus(const rr_game_t *g)
{
    switch(g->stage) {
    case 1: return g->overtakes >= 5;
    case 2: return g->knockouts >= 3;
    case 3: return g->health >= 50;
    case 4: return g->elapsed_ms <= 80000;
    case 5: return rr_rank(g) <= 3;
    default: return 0;
    }
}
int rr_stars(const rr_game_t *g)
{
    return g->phase == RR_FINISHED ? 1+rr_bonus(g)+(g->health>=80) : 0;
}

static int absolute(int x) { return x < 0 ? -x : x; }

int rr_road_center(const rr_game_t *g, int y)
{
    int phase=(int)(g->scenery_ms/80)%240;
    int wave=phase<120 ? phase-60 : 180-phase;
    return 160+wave*(240-y)*(240-y)/100000;
}
rr_rect_t rr_player_rect(const rr_game_t *g)
{
    int lean=(g->lane*256-g->lane_q8)/28;
    int x=rr_road_center(g,211)+g->lane_q8*85/256;
    return (rr_rect_t){x-16+(lean<0 ? lean : 0),165,32+absolute(lean),56};
}
rr_rect_t rr_entity_rect(const rr_game_t *g, const rr_entity_t *e)
{
    int z=e->depth, bottom=88+z*z*128/1000000;
    int h=7+z*z*48/1000000, half=22+(bottom-82)*9/10;
    int x=rr_road_center(g,bottom)+e->lane*half*2/3;
    int w=e->car ? h*4/5 : h*16/28;
    return (rr_rect_t){x-w/2,bottom-h,w,h};
}
int rr_contact(const rr_game_t *g, const rr_entity_t *e)
{
    rr_rect_t p=rr_player_rect(g),q=rr_entity_rect(g,e);
    return e->active==1 && p.x<q.x+q.w && q.x<p.x+p.w &&
        p.y<q.y+q.h && q.y<p.y+p.h;
}
int rr_attackable(const rr_game_t *g, const rr_entity_t *e)
{
    int gap=absolute(e->lane*256-g->lane_q8);
    rr_rect_t p=rr_player_rect(g),q=rr_entity_rect(g,e);
    return g->phase==RR_RACING && g->cooldown_ms==0 && e->active==1 && !e->car &&
        gap>=160 && gap<=330 && e->depth<=1030 &&
        q.y+q.h>=p.y+12 && q.y<=p.y+p.h*2/3;
}

/* Deterministic xorshift stream, owned by the game and used only at spawns. */
static uint32_t random_next(rr_game_t *g)
{
    uint32_t x = g->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return g->rng = x;
}

void rr_init(rr_game_t *g, uint32_t seed)
{
    *g = (rr_game_t){ .phase = RR_TITLE, .rng = seed ? seed : 0xD057u,
        .health = 100, .speed = 100, .spawn_ms = 900, .battery = -1, .stage=1, .unlocked=1 };
}

int rr_rank(const rr_game_t *g)
{
    int rank = 8 - (g->overtakes + g->knockouts * 2) / 3;
    return rank < 1 ? 1 : rank;
}

void rr_input(rr_game_t *g, rr_input_t input)
{
    if (input == RR_HOME) {
        int battery = g->battery, stage=g->stage, unlocked=g->unlocked;
        rr_init(g, 0xD057u);
        g->battery = battery; g->stage=stage; g->unlocked=unlocked;
        return;
    }
    if (input == RR_PAUSE) {
        if (g->phase == RR_RACING) g->phase = RR_PAUSED;
        else if (g->phase == RR_PAUSED) g->phase = RR_RACING;
        return;
    }
    if (g->phase == RR_TITLE && input == RR_LEFT && g->stage > 1) { --g->stage; return; }
    if (g->phase == RR_TITLE && input == RR_RIGHT && g->stage < g->unlocked) { ++g->stage; return; }
    if (input == RR_ACTION && g->phase != RR_RACING) {
        if (g->phase == RR_PAUSED) g->phase = RR_RACING;
        else {
            int battery=g->battery,stage=g->stage,unlocked=g->unlocked;
            if(g->phase==RR_FINISHED && stage<RR_STAGES)++stage;
            rr_init(g, 0xD057u+(uint32_t)(stage-1)*7919u);
            g->battery=battery;g->stage=stage;g->unlocked=unlocked;
            g->phase=RR_RACING;
        }
        return;
    }
    if (g->phase != RR_RACING) return;
    if (input == RR_LEFT && g->lane > -1) --g->lane;
    if (input == RR_RIGHT && g->lane < 1) ++g->lane;
    if (input == RR_ACTION && g->cooldown_ms == 0) {
        int best = -1, best_depth = -1;
        for (int i = 0; i < RR_ENTITIES; ++i) {
            const rr_entity_t *e = &g->entities[i];
            if (rr_attackable(g,e) && e->depth > best_depth) {
                best = i; best_depth = e->depth;
            }
        }
        g->attack_side = g->lane == 1 ? -1 : 1;
        g->attack_ms = 240;
        g->cooldown_ms = 450;
        if (best >= 0) {
            g->attack_side = g->entities[best].lane * 256 < g->lane_q8 ? -1 : 1;
            g->entities[best].active = 2;
            ++g->knockouts;
            g->score += 120;
        }
    }
}

/* Spawn at the horizon into a fixed pool; no allocation or simultaneous wall. */
static void spawn(rr_game_t *g)
{
    for (int i = 0; i < RR_ENTITIES; ++i) {
        if (g->entities[i].active) continue;
        uint32_t r = random_next(g);
        g->entities[i] = (rr_entity_t){ .active = 1, .lane = (int8_t)(r % 3) - 1,
            .car = (r >> 4) % 10 < (uint32_t)(2+(g->stage-1)/2), .color = (r >> 8) % 3 };
        return;
    }
}

/* One bounded simulation step. All time-dependent gameplay freezes when paused. */
static void step(rr_game_t *g)
{
    if (g->phase == RR_TITLE) { g->scenery_ms += 20; return; }
    if (g->phase != RR_RACING) return;
    g->elapsed_ms += 20;
    g->scenery_ms += 20;
    int target = 100 + (int)(g->elapsed_ms / 350);
    if (target > rr_stage(g)->max_speed) target = rr_stage(g)->max_speed;
    if (g->hurt_ms > 0) target = 75;
    if (g->speed < target) ++g->speed;
    if (g->speed > target) --g->speed;
    g->metres_mm += g->speed * 50 / 9; /* km/h -> mm per 20 ms */
    int diff = g->lane * 256 - g->lane_q8;
    g->lane_q8 += diff > 32 ? 32 : diff < -32 ? -32 : diff;
    if (g->attack_ms > 0) g->attack_ms = g->attack_ms > 20 ? g->attack_ms - 20 : 0;
    if (g->cooldown_ms > 0) g->cooldown_ms = g->cooldown_ms > 20 ? g->cooldown_ms - 20 : 0;
    if (g->hurt_ms > 0) g->hurt_ms = g->hurt_ms > 20 ? g->hurt_ms - 20 : 0;
    g->spawn_ms -= 20;
    if (g->spawn_ms <= 0) {
        spawn(g);
        g->spawn_ms = (int16_t)(rr_stage(g)->spawn_ms + random_next(g) % 400);
    }
    for (int i = 0; i < RR_ENTITIES; ++i) {
        rr_entity_t *e = &g->entities[i];
        if (!e->active) continue;
        e->depth += e->active == 2 ? 18 : 4 + g->speed / 35;
        if (e->active == 2) {
            if (e->depth > 1120) e->active = 0;
            continue;
        }
        if (rr_contact(g,e) && g->hurt_ms == 0) {
            g->health -= e->car ? 30 : 20;
            g->hurt_ms = 1200;
            e->active = 0;
            if (g->health <= 0) { g->health = 0; g->phase = RR_WRECKED; }
        } else if (e->depth > 1120) {
            e->active = 0;
            ++g->overtakes;
            g->score += 40;
        }
    }
    if (g->phase == RR_RACING && g->metres_mm >= rr_stage(g)->metres * 1000) {
        g->metres_mm = rr_stage(g)->metres * 1000;
        g->phase = RR_FINISHED;
        g->score += g->health * 5;
        if(g->stage<RR_STAGES && g->unlocked<=g->stage)g->unlocked=g->stage+1;
    }
}

void rr_tick(rr_game_t *g, uint32_t elapsed_ms)
{
    if (elapsed_ms > 250) elapsed_ms = 250;
    g->accumulator_ms += elapsed_ms;
    while (g->accumulator_ms >= 20) { g->accumulator_ms -= 20; step(g); }
}

/* Explicit field hashing makes parity independent of ABI padding/endianness. */
static void mix(uint32_t *h, uint32_t v)
{
    for (int i = 0; i < 4; ++i) { *h = (*h ^ (v & 255)) * 16777619u; v >>= 8; }
}
uint32_t rr_state_hash(const rr_game_t *g)
{
    uint32_t h = 2166136261u;
    const int32_t fields[] = { g->phase, (int32_t)g->rng, (int32_t)g->elapsed_ms,
        (int32_t)g->accumulator_ms, (int32_t)g->scenery_ms, g->metres_mm, g->lane_q8,
        g->health, g->speed, g->score, g->overtakes, g->knockouts, g->attack_ms,
        g->cooldown_ms, g->hurt_ms, g->spawn_ms, g->battery, g->lane, g->attack_side, g->stage, g->unlocked };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) mix(&h, (uint32_t)fields[i]);
    for (int i = 0; i < RR_ENTITIES; ++i) {
        const rr_entity_t *e = &g->entities[i];
        mix(&h, (uint32_t)e->depth); mix(&h, (uint32_t)e->lane);
        mix(&h, e->active); mix(&h, e->car); mix(&h, e->color); mix(&h, e->passed);
    }
    return h;
}
