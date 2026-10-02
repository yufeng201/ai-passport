#pragma once
#include <stdint.h>
/* Five stages, two bits each. A replay never lowers the earned rating. */
static inline unsigned game_medal_count(uint32_t ledger,int stage)
{
    return stage>=1&&stage<=5?(ledger>>((stage-1)*2))&3u:0;
}
static inline uint32_t game_medal_record(uint32_t ledger,int stage,unsigned rating)
{
    if(stage<1||stage>5||rating>3)return ledger;
    unsigned shift=(unsigned)(stage-1)*2;
    if(rating>game_medal_count(ledger,stage))ledger=(ledger&~(3u<<shift))|(rating<<shift);
    return ledger;
}
