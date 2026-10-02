#pragma once
#include <stdint.h>
#include "../common/game_achievements.h"

#define RR_WIDTH 320
#define RR_HEIGHT 240
#define RR_ENTITIES 6
#define RR_FINISH_METRES 1600
#define RR_STAGES 5
typedef struct { int metres, spawn_ms, max_speed, challenge; } rr_stage_t;

typedef enum { RR_TITLE, RR_RACING, RR_PAUSED, RR_FINISHED, RR_WRECKED } rr_phase_t;
typedef enum { RR_LEFT, RR_RIGHT, RR_ACTION, RR_PAUSE, RR_HOME } rr_input_t;
typedef struct {
    int32_t depth;
    int8_t lane;
    uint8_t active, car, color, passed;
} rr_entity_t;
typedef struct {
    rr_phase_t phase;
    uint32_t medals;
    uint32_t rng, elapsed_ms, accumulator_ms, scenery_ms;
    int32_t metres_mm, lane_q8;
    int16_t health, speed, score, overtakes, knockouts;
    int16_t chain,boost_ms;
    int16_t attack_ms, cooldown_ms, hurt_ms, spawn_ms, battery;
    int8_t lane, attack_side;
    uint8_t stage, unlocked,muted;
    rr_entity_t entities[RR_ENTITIES];
} rr_game_t;

/* Reset caller-owned state with a reproducible nonzero PRNG seed; no allocation. */
void rr_init(rr_game_t *g, uint32_t seed);
/* Apply one normalized three-key event. No callbacks, I/O or blocking. */
void rr_input(rr_game_t *g, rr_input_t input);
/* Advance elapsed milliseconds in fixed 20 ms steps, capped at 250 ms per call. */
void rr_tick(rr_game_t *g, uint32_t elapsed_ms);
/* Current 1..8 race position derived from overtakes and knockouts. */
int rr_rank(const rr_game_t *g);
/* Hash logical fields (not struct padding) for host/Wasm replay verification. */
uint32_t rr_state_hash(const rr_game_t *g);

/* Immutable stage parameters and optional bonus completion, no allocation. */
const rr_stage_t *rr_stage(const rr_game_t *g);
int rr_bonus(const rr_game_t *g);
int rr_stars(const rr_game_t *g);

/* Shared world-to-screen geometry for sprites, contact and attack cues. */
typedef struct { int x, y, w, h; } rr_rect_t;
int rr_road_center(const rr_game_t *g, int y);
rr_rect_t rr_player_rect(const rr_game_t *g);
rr_rect_t rr_entity_rect(const rr_game_t *g, const rr_entity_t *e);
int rr_contact(const rr_game_t *g, const rr_entity_t *e);
int rr_attackable(const rr_game_t *g, const rr_entity_t *e);
