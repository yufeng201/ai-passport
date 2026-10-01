#pragma once
#include "rr_game.h"
/* Release-based B actions bypass the driver's delayed click/double-click events.
 * Unsigned subtraction handles timer wrap. Long holds emit exactly once. */
typedef struct { uint32_t pressed_ms; int down, long_sent; } rr_action_key_t;
static inline int rr_action_poll(rr_action_key_t *key, uint32_t now)
{
    if (key->down && !key->long_sent && now-key->pressed_ms >= 500) {
        key->long_sent=1; return RR_PAUSE;
    }
    return -1;
}
static inline int rr_action_edge(rr_action_key_t *key, int down, uint32_t now)
{
    if (down) {
        if (!key->down) *key=(rr_action_key_t){.down=1,.pressed_ms=now};
        return -1;
    }
    if (!key->down) return -1;
    int input=rr_action_poll(key,now);
    if (input<0 && !key->long_sent) input=RR_ACTION;
    key->down=0;
    return input;
}
