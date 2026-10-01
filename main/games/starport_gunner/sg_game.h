#pragma once
#include <stdint.h>
#define SG_WIDTH 320
#define SG_HEIGHT 240
#define SG_STAGES 5
#define SG_ENEMIES 6
#define SG_SHOTS 24
#define SG_THREATS 12
#define SG_CHARGE_MAX 1200
#define SG_CHARGE_THRESHOLD 500
#define SG_COOLDOWN 2400
typedef enum { SG_TITLE, SG_PLAY, SG_PAUSED, SG_CLEAR, SG_FAILED } sg_phase_t;
typedef struct { int active,x,y,hp,max_hp,kind,dir,attack_ms; uint32_t id; } sg_enemy_t;
typedef struct { int active,x,y,dx,dy,power,pierce; uint32_t hit_mask; } sg_shot_t;
typedef struct { int active,x,y,warning_ms; } sg_threat_t;
typedef struct {
    sg_phase_t phase;
    uint32_t rng,last_ms,scene_ms,elapsed_ms,accumulator_ms,pressed_ms[3],next_id;
    int stage,unlocked,best,score,battery,muted,health,aim,spawn_ms,spawned,kills,goal;
    int charging,charge_ms,cooldown_ms,shot_ms,damage_ms,flash_ms,flash_x,flash_y,effect;
    unsigned held,blocked,long_sent;
    sg_enemy_t enemies[SG_ENEMIES];
    sg_shot_t shots[SG_SHOTS];
    sg_threat_t threats[SG_THREATS];
} sg_game_t;
void sg_init(sg_game_t *g,uint32_t now,uint32_t seed);
void sg_start(sg_game_t *g);
void sg_update(sg_game_t *g,uint32_t now);
void sg_edge(sg_game_t *g,int key,int down,uint32_t now);
void sg_cancel(sg_game_t *g,uint32_t now);
uint32_t sg_hash(const sg_game_t *g);
int sg_aim_dx(int aim);
int sg_aim_dy(int aim);
/* Swept segment vs an inclusive hit rectangle; prevents fast shots tunneling. */
int sg_segment_hits(int x,int y,int tx,int ty,int left,int top,int right,int bottom);
void sg_render_strip(const sg_game_t *g,uint16_t *pixels,int y,int rows);
void sg_device_run(void);
