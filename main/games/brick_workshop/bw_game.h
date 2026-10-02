#pragma once
#include <stdint.h>
#include "../common/game_achievements.h"
#define BW_WIDTH 320
#define BW_HEIGHT 240
#define BW_STAGES 5
#define BW_BRICKS 24
#define BW_Q 256
#define BW_RADIUS 3
#define BW_PADDLE_Y 206
#define BW_PADDLE_W 56
typedef enum { BW_TITLE,BW_PLAY,BW_PAUSED,BW_CLEAR,BW_FAILED } bw_phase_t;
typedef struct { int x,y,hp,max_hp; } bw_brick_t;
typedef struct {
    bw_phase_t phase;
    uint32_t medals;
    uint32_t rng,last_ms,scene_ms,elapsed_ms,accumulator_ms,pressed_ms[3];
    int stage,unlocked,best,score,battery,muted,lives,paddle_x,ball_x,ball_y,vx,vy,ready;
    int destroyed,supply_ms;
    int remaining,goal,slow_uses,slow_ms,stale_ms,assist_ms,motion,motion_dir,flash_ms,flash_x,flash_y,effect;
    unsigned held,blocked,long_sent;
    bw_brick_t bricks[BW_BRICKS];
} bw_game_t;
void bw_init(bw_game_t *g,uint32_t now,uint32_t seed);
void bw_start(bw_game_t *g);
void bw_update(bw_game_t *g,uint32_t now);
void bw_edge(bw_game_t *g,int key,int down,uint32_t now);
void bw_cancel(bw_game_t *g,uint32_t now);
uint32_t bw_hash(const bw_game_t *g);
int bw_brick_x(const bw_game_t *g,int index);
void bw_render_strip(const bw_game_t *g,uint16_t *pixels,int y,int rows);
void bw_device_run(void);
