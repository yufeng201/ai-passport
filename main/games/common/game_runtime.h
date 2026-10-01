#pragma once
#include <stdint.h>
typedef struct { uint32_t best,unlocked,muted; } game_progress_t;
typedef struct {
    const char *name;
    void (*init)(uint32_t now);
    void (*update)(uint32_t now);
    void (*edge)(int key,int down,uint32_t now);
    void (*cancel)(uint32_t now);
    void (*render)(uint16_t *pixels,int y,int rows);
    void (*battery)(int percent);
    void (*restore)(const game_progress_t *record);
    void (*progress)(game_progress_t *record);
    int (*effect)(void);
} game_app_t;
/* One display/input owner for the lifetime of a standalone firmware.
 * All callbacks except audio execute on this owner; no LVGL task is started. */
void game_runtime_run(const game_app_t *app);
