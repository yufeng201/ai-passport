#pragma once
#include <stdint.h>
#include "../common/game_achievements.h"
#define CB_WIDTH 320
#define CB_HEIGHT 240
#define CB_STAGES 5
#define CB_GOAL 12
#define CB_FLIGHT_MS 600
#define CB_CHARGE_MAX 1200
typedef enum { CB_TITLE, CB_PLAY, CB_PAUSED, CB_CLEAR, CB_FAILED } cb_phase_t;
typedef struct { int x,y,w,h; } cb_platform_t;
typedef struct {
    cb_phase_t phase;
    uint32_t medals;
    uint32_t rng,last_ms,scene_ms,elapsed_ms,accumulator_ms,pressed_ms[3];
    int stage,unlocked,best,score,combo,landings,battery,muted;
    unsigned held,blocked,long_sent;
    int rescued;
    int charging,charge_ms,flying,flight_ms,jump_dx,jump_rise;
    int x,y,camera,scroll_ms,scroll_dx,feedback_ms,effect;
    cb_platform_t current,target;
} cb_game_t;
void cb_init(cb_game_t *g,uint32_t now,uint32_t seed);
void cb_start(cb_game_t *g);
void cb_update(cb_game_t *g,uint32_t now);
/* Physical order: A=0, B=1, C=2. Input edges are timestamped before queuing. */
void cb_edge(cb_game_t *g,int key,int down,uint32_t now);
void cb_cancel(cb_game_t *g,uint32_t now);
int cb_jump_distance(int ms);
int cb_jump_y(const cb_game_t *g,int flight_ms);
int cb_landing_x(const cb_game_t *g,int charge_ms);
int cb_can_land(const cb_game_t *g,int charge_ms);
uint32_t cb_hash(const cb_game_t *g);
void cb_render_strip(const cb_game_t *g,uint16_t *pixels,int y,int rows);
void cb_device_run(void);
