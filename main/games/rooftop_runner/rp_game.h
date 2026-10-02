#pragma once
#include <stdint.h>
#include "../common/game_achievements.h"
#define RP_WIDTH 320
#define RP_HEIGHT 240
#define RP_STAGES 5
#define RP_PLATFORMS 12
#define RP_Q 256
#define RP_SPEED (3*RP_Q)
#define RP_HALF 6
#define RP_BODY 23
#define RP_COYOTE_MS 100
#define RP_BUFFER_MS 100
#define RP_SWITCH_MS 350
typedef enum { RP_TITLE,RP_PLAY,RP_PAUSED,RP_CLEAR,RP_FAILED } rp_phase_t;
typedef struct { int x,y,w,hazard; } rp_platform_t;
typedef struct {
    rp_phase_t phase;
    uint32_t medals;
    uint32_t rng,last_ms,scene_ms,elapsed_ms,accumulator_ms,pressed_ms[3],released_ms;
    int stage,unlocked,best,score,battery,muted;
    unsigned held,blocked,long_sent,collected;
    int x,y,vx,vy,grounded,platform,camera,checkpoint,furthest;
    int last_dir,jump_dir,respawn_ms,falls,feedback_ms,effect;
    int coyote_ms,jump_buffer_ms,facing;
    rp_platform_t platforms[RP_PLATFORMS];
} rp_game_t;
void rp_init(rp_game_t *g,uint32_t now,uint32_t seed);
void rp_start(rp_game_t *g);
void rp_update(rp_game_t *g,uint32_t now);
void rp_edge(rp_game_t *g,int key,int down,uint32_t now);
void rp_cancel(rp_game_t *g,uint32_t now);
uint32_t rp_hash(const rp_game_t *g);
int rp_hazard_x(const rp_game_t *g,int index);
void rp_render_strip(const rp_game_t *g,uint16_t *pixels,int y,int rows);
void rp_device_run(void);
