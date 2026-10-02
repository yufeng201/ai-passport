#pragma once
#include <stdint.h>
#include "../common/game_achievements.h"
#define AN_WIDTH 320
#define AN_HEIGHT 240
#define AN_STAGES 5
#define AN_RANGE 58
#define AN_PERFECT_MS 160
#define AN_GUARD_MAX 100
typedef enum { AN_TITLE,AN_PLAY,AN_PAUSED,AN_CLEAR,AN_FAILED } an_phase_t;
typedef enum { AN_ENTRY,AN_WINDUP,AN_RECOVER } an_enemy_phase_t;
typedef struct { int active,x,side,hp,max_hp,kind,timer_ms,warning_ms,vulnerable_ms; an_enemy_phase_t phase; } an_enemy_t;
typedef struct {
    an_phase_t phase;
    uint32_t medals;
    uint32_t rng,last_ms,scene_ms,elapsed_ms,accumulator_ms,pressed_ms[3];
    int stage,unlocked,best,score,battery,muted,health,stamina,guarding,guard_ms,broken_ms;
    int riposte_ms;
    int spawned,kills,goal,spawn_ms,slash_ms,slash_side,cooldown_ms,damage_ms,feedback_ms,feedback,combo,effect;
    unsigned held,blocked,long_sent;
    an_enemy_t enemy;
} an_game_t;
void an_init(an_game_t *g,uint32_t now,uint32_t seed);
void an_start(an_game_t *g);
void an_update(an_game_t *g,uint32_t now);
void an_edge(an_game_t *g,int key,int down,uint32_t now);
void an_cancel(an_game_t *g,uint32_t now);
uint32_t an_hash(const an_game_t *g);
void an_render_strip(const an_game_t *g,uint16_t *pixels,int y,int rows);
void an_device_run(void);
