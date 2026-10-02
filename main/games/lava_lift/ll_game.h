#pragma once
#include <stdint.h>
#define LL_GOAL 1200
#define LL_JUMP_MS 720
typedef enum {LL_TITLE, LL_PLAY, LL_PAUSED, LL_WON, LL_FAILED} ll_phase_t;
typedef struct {int altitude,gap,hit;} ll_gate_t;
typedef struct {ll_phase_t phase; uint32_t last_ms,rng,elapsed_ms,accumulator_ms;
 int x,height,jump_ms,hp,invincible_ms,best,battery,effect,kills,monster_x,monster_active,monster_wait;
 unsigned held; ll_gate_t gates[3];} ll_game_t;
void ll_init(ll_game_t *g,uint32_t now,uint32_t seed);
void ll_start(ll_game_t *g);
void ll_update(ll_game_t *g,uint32_t now);
void ll_edge(ll_game_t *g,int key,int down,uint32_t now);
void ll_cancel(ll_game_t *g,uint32_t now);
int ll_jump_height(const ll_game_t *g);
int ll_platform_y(const ll_game_t *g);
int ll_gate_y(const ll_game_t *g,int i);
uint32_t ll_hash(const ll_game_t *g);
void ll_render_strip(const ll_game_t *g,uint16_t *pixels,int y,int rows);
void ll_device_run(void);
