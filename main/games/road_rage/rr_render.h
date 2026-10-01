#pragma once
#include "rr_game.h"
/* Draw tightly packed RGB565 scanlines into caller-owned memory (320 * rows).
 * Native byte order, no allocation, no I/O; y/rows must describe valid screen rows.
 * All geometry and assets are shared between firmware and browser. */
void rr_render_strip(const rr_game_t *g, uint16_t *pixels, int y, int rows);
