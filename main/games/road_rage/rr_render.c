#include "rr_render.h"
#include "../common/game_visual.h"
#include "rr_copy.h"
#include "road_rage_noto_sc_12.h"
#include <stddef.h>

#define RGB(r,g,b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define INK RGB(15,19,35)
#define CREAM RGB(255,237,191)
#define GOLD RGB(255,183,77)
#define RED RGB(248,79,86)
#define TEAL RGB(75,228,204)

typedef struct { uint16_t *pixels; int y, rows; } canvas_t;
static int min(int a, int b) { return a < b ? a : b; }
static int max(int a, int b) { return a > b ? a : b; }

/* Clipped rectangle primitive; strip-local writes only. */
static void box(canvas_t *c, int x, int y, int w, int h, uint16_t color)
{
    int left = max(x, 0), right = min(x + w, RR_WIDTH);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * RR_WIDTH + xx] = color;
}

/* Integer Bresenham line, including both endpoints; clipped by box(). */
static void line(canvas_t *c, int x, int y, int tx, int ty, int width, uint16_t color)
{
    if (max(y,ty)+width <= c->y || min(y,ty) >= c->y+c->rows) return;
    int dx = tx > x ? tx - x : x - tx, sx = x < tx ? 1 : -1;
    int dy = ty > y ? y - ty : ty - y, sy = y < ty ? 1 : -1, err = dx + dy;
    for (;;) {
        box(c, x, y, width, width, color);
        if (x == tx && y == ty) break;
        int e = 2 * err;
        if (e >= dy) { err += dy; x += sx; }
        if (e <= dx) { err += dx; y += sy; }
    }
}

/* Filled ellipse using squared integer distances; no floating-point library. */
static void oval(canvas_t *c, int cx, int cy, int rx, int ry, uint16_t color)
{
    for (int y = max(cy - ry, c->y); y <= min(cy + ry, c->y + c->rows - 1); ++y)
        for (int x = max(cx - rx, 0); x <= min(cx + rx, RR_WIDTH - 1); ++x)
            if ((x-cx)*(x-cx)*ry*ry + (y-cy)*(y-cy)*rx*rx <= rx*rx*ry*ry)
                c->pixels[(y-c->y)*RR_WIDTH+x] = color;
}

/* Original compact 5x7 uppercase bitmap alphabet, resident in read-only Flash. */
static const uint8_t font[][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14}
};

/* Text uses a fixed Flash alphabet, not runtime fonts or transient allocations. */
static void text(canvas_t *c, int x, int y, const char *s, int scale, uint16_t color)
{
    if (y+7*scale <= c->y || y >= c->y+c->rows) return;
    for (; *s; ++s, x += 6 * scale) {
        int glyph = *s >= 'A' && *s <= 'Z' ? *s - 'A' : *s >= '0' && *s <= '9' ? *s - '0' + 26 : -1;
        if (glyph >= 0) {
            for (int yy = 0; yy < 7; ++yy)
                for (int xx = 0; xx < 5; ++xx)
                    if (font[glyph][yy] & (16 >> xx)) box(c, x+xx*scale, y+yy*scale, scale, scale, color);
        } else if (*s == '.') box(c, x+2*scale, y+6*scale, scale, scale, color);
        else if (*s == '+') { box(c,x,y+3*scale,5*scale,scale,color); box(c,x+2*scale,y+scale,scale,5*scale,color); }
        else if (*s == '-') box(c, x, y+3*scale, 5*scale, scale, color);
        else if (*s == '/') line(c, x+4*scale, y, x, y+6*scale, scale, color);
        else if (*s == ':') { box(c,x+2*scale,y+2*scale,scale,scale,color); box(c,x+2*scale,y+5*scale,scale,scale,color); }
    }
}

/* Decode only the validated UTF-8 copy. Missing glyphs remain visible as boxes. */
static void ztext(canvas_t *c, int x, int y, const char *s, int scale, uint16_t color)
{
    if (y+14*scale <= c->y || y >= c->y+c->rows) return;
    while (*s) {
        const unsigned char *p = (const unsigned char *)s;
        if (*p < 128) {
            char ascii[] = { *s++,0 };
            text(c,x,y+2*scale,ascii,scale,color); x += 6*scale;
            continue;
        }
        uint16_t codepoint = 0;
        if ((p[0]&0xf0)==0xe0 && p[1] && p[2]) {
            codepoint = (uint16_t)(((p[0]&15)<<12)|((p[1]&63)<<6)|(p[2]&63)); s += 3;
        } else { ++s; }
        const rr_cjk_glyph_t *glyph = NULL;
        for (unsigned i=0;i<RR_CJK_GLYPH_COUNT;++i)
            if(rr_cjk_glyphs[i].codepoint==codepoint){glyph=&rr_cjk_glyphs[i];break;}
        if(glyph) {
            for(int yy=0;yy<14;++yy)for(int xx=0;xx<12;++xx)
                if(glyph->rows[yy]&(1<<(11-xx)))box(c,x+xx*scale,y+yy*scale,scale,scale,color);
        } else {
            box(c,x,y,10*scale,scale,RED); box(c,x,y+11*scale,10*scale,scale,RED);
            box(c,x,y,scale,12*scale,RED); box(c,x+9*scale,y,scale,12*scale,RED);
        }
        x += 12*scale;
    }
}

/* Decimal formatting without libc, bounded to six digits. */
static void number(canvas_t *c, int x, int y, int value, int scale, uint16_t color)
{
    char out[8]; int n = 0;
    do { out[n++] = '0' + value % 10; value /= 10; } while (value && n < 6);
    for (int i = 0; i < n/2; ++i) { char t = out[i]; out[i] = out[n-1-i]; out[n-1-i] = t; }
    out[n] = 0; text(c,x,y,out,scale,color);
}

/* Back-facing motorcycle and rider, 16x28 pixel design with palette recoloring. */
static const char *const bike[] = {
 "......0000......", ".....011110.....", "....01222210....", "....01222210....",
 "....01133110....", ".....044440.....", ".....055550.....", "....05566550....",
 "...0556666550...", "..077566665770..", ".07775666657770.", ".07705666650770.",
 "..00.056650.00..", ".....055550.....", "....00888800....", "...0098888900...",
 "...0998888990...", "...099AAAA990...", "...099AAAA990...", "....09999990....",
 "....0B999B0.....", "....0B999B0.....", ".....0CCC0......", ".....0CCC0......",
 ".....0CDC0......", ".....0CCC0......", ".....0CCC0......", "......000......."
};

/* Nearest-neighbor sprite scaling directly into strips; no decoded image cache. */
static void motorcycle(canvas_t *c, int x, int bottom, int height, int color, int lean, int attack)
{
    if (bottom+max(height/12,1)+2 <= c->y || bottom-height-5 >= c->y+c->rows) return;
    const uint16_t jackets[] = { TEAL, RED, GOLD };
    uint16_t palette[] = { INK, RGB(240,224,187), RGB(48,69,95), RGB(140,208,213),
        RGB(35,36,51), jackets[color%3], RGB(34,83,91), RGB(211,170,140),
        RGB(183,194,207), RGB(48,53,75), RED, RGB(210,205,177), RGB(25,26,36), RGB(109,111,122) };
    int width = height * 16 / 28, top = bottom - height;
    oval(c,x,bottom+1,max(width*3/4,2),max(height/12,1),RGB(22,20,35));
    for (int yy = max(top,c->y); yy < min(bottom,c->y+c->rows); ++yy) {
        int source_y = (yy-top)*28/height;
        int shift = lean*(bottom-yy)/height;
        for (int xx = 0; xx < width; ++xx) {
            char p = bike[source_y][xx*16/width];
            int index = p >= 'A' ? p-'A'+10 : p-'0';
            if (p != '.') box(c,x-width/2+xx+shift,yy,1,1,palette[index]);
        }
    }
    if (attack) {
        int ax = x + attack * (width/2+height/5);
        line(c,x+attack*width/3,top+height/3,ax,top+height/3-3,3,RGB(211,170,140));
        line(c,ax,top+height/3-3,ax+attack*height/5,top+height/4,3,CREAM);
        line(c,ax+attack*height/5,top+height/4-5,ax+attack*height/5,top+height/4+6,2,GOLD);
    }
}

/* Stage copy and visual theme stay in Flash; every stage reuses the same pool. */
static const char *stage_name(const rr_game_t *g)
{
    static const char *const names[]={RR_TEXT_STAGE1,RR_TEXT_STAGE2,RR_TEXT_STAGE3,RR_TEXT_STAGE4,RR_TEXT_STAGE5};
    return names[g->stage>=1 && g->stage<=RR_STAGES ? g->stage-1 : 0];
}
static const char *challenge_name(const rr_game_t *g)
{
    static const char *const names[]={RR_TEXT_CHALLENGE1,RR_TEXT_CHALLENGE2,RR_TEXT_CHALLENGE3,RR_TEXT_CHALLENGE4,RR_TEXT_CHALLENGE5};
    return names[g->stage>=1 && g->stage<=RR_STAGES ? g->stage-1 : 0];
}

/* Fixed layers: sunset, distant ridges, shoulder, road and moving lane marks. */
static void landscape(canvas_t *c, const rr_game_t *g)
{
    int stage=g->stage>=1 && g->stage<=RR_STAGES ? g->stage : 1;
    static const int sky[5][3]={{76,48,99},{33,28,69},{112,73,58},{15,23,40},{46,22,70}};
    for (int y = c->y; y < c->y+c->rows; ++y) {
        if (y < 83) {
            int gradient=stage==4 ? y/4 : y;
            box(c,0,y,320,1,RGB(sky[stage-1][0]+gradient,sky[stage-1][1]+gradient,sky[stage-1][2]+gradient/2));
        } else {
            int d = y-82, center = rr_road_center(g,y), half = 22 + d*9/10;
            box(c,0,y,320,1,stage==3 ? RGB(151+d/5,104+d/9,69+d/6) : stage==4 ? RGB(26+d/9,38+d/9,46+d/9) : RGB(83+d/5,64+d/9,78+d/6));
            /* Left coast: tiny bands and glints instead of a decoded scenery image. */
            if(stage==1)box(c,0,y,max(0,center-half-18),1,RGB(56+d/5,83+d/8,102+d/10));
            if (stage==1 && d%11 == 0) {
                int water_edge = max(0,center-half-24);
                box(c,(d*17)%max(water_edge,1),y,min(13,water_edge),1,RGB(142,148,151));
            }
            box(c,center-half-9,y,2*half+18,1,RGB(214,125,108));
            box(c,center-half-5,y,2*half+10,1,((d+g->scenery_ms/24)/12)%2 ? CREAM : RED);
            box(c,center-half,y,2*half,1,stage==4 ? RGB(24+d/10,35+d/8,46+d/6) : RGB(37+d/8,37+d/9,52+d/7));
            box(c,center-half+3,y,1,1,RGB(117,108,125));
            box(c,center+half-4,y,1,1,RGB(117,108,125));
            if (((18000/(d+20)+g->scenery_ms/28)/45)%2 == 0) {
                int w = max(1,d/45);
                box(c,center-half/3,y,w,1,CREAM);
                box(c,center+half/3,y,w,1,CREAM);
            }
        }
    }
    if(stage==4) {
        oval(c,244,51,15,15,RGB(186,208,220));
        oval(c,250,47,13,13,RGB(25,34,53));
    } else {
        oval(c,238,55,25,25,stage==2 ? RGB(248,156,193) : RGB(255,207,130));
        for (int i = 0; i < 4; ++i) box(c,209,62+i*5,60,2,stage==2 ? RGB(107,65,131) : RGB(217,131,128));
    }
    for (int x = 0; c->y < 88 && x < 320; ++x) {
        int p = x%91, mountain = p < 43 ? 80-p/2 : 58+(p-43)/3;
        box(c,x,mountain,1,83-mountain,RGB(103,71,111));
        int q = (x+21)%67, near = q < 33 ? 88-q/3 : 77+(q-33)/4;
        box(c,x,near,1,87-near,RGB(59,49,78));
    }
    if(stage==2 && c->y<88) {
        for(int i=0;i<14;++i) {
            int x=i*25-7,h=15+(i*19)%35;
            box(c,x,83-h,19,h,RGB(28,24,48));
            box(c,x+2,82-h,15,2,i%2 ? TEAL : RED);
            for(int yy=86-h;yy<80;yy+=7)for(int xx=x+3;xx<x+16;xx+=6)
                box(c,xx,yy,2,3,(xx+yy)%3 ? GOLD : RGB(106,78,123));
        }
    }
    /* Roadside silhouettes reuse the same bounded perspective positions. */
    for (int i = 0; i < 5; ++i) {
        int z = ((int)g->scenery_ms/16+i*215)%1100;
        int y = 88+z*z*148/1210000;
        int h = 5+z*z*58/1210000;
        int half = 22+(y-82)*9/10;
        int x = rr_road_center(g,y)+(i%2 ? -1 : 1)*(half+10+h/3);
        if(stage==3) {
            box(c,x-h/12,y-h,max(2,h/6),h,RGB(61,74,47));
            line(c,x,y-h/2,x-h/3,y-h/2,max(1,h/10),RGB(61,74,47));
            line(c,x-h/3,y-h/2,x-h/3,y-h*3/4,max(1,h/10),RGB(61,74,47));
            continue;
        }
        if(stage==2 || stage==5) {
            box(c,x,y-h,max(1,h/15),h,INK);
            box(c,x-h/3,y-h,h*2/3,max(2,h/5),stage==2 ? TEAL : RED);
            continue;
        }
        line(c,x,y,x+h/6,y-h,max(1,h/12),INK);
        line(c,x+h/6,y-h,x-h/2,y-h+h/5,max(1,h/15),INK);
        line(c,x+h/6,y-h,x+h/2,y-h+h/4,max(1,h/15),INK);
        line(c,x+h/6,y-h,x-h/3,y-h-h/6,max(1,h/15),INK);
        line(c,x+h/6,y-h,x+h/2,y-h-h/8,max(1,h/15),INK);
    }
}

static void player(canvas_t *c, const rr_game_t *g)
{
    int player_x = rr_road_center(g,211)+g->lane_q8*85/256;
    if (!g->hurt_ms || (g->hurt_ms/80)%2 == 0) {
        motorcycle(c,player_x,221,56,0,(g->lane*256-g->lane_q8)/28,
                   g->attack_ms ? g->attack_side : 0);
    }
}

/* Game objects are painter-sorted far to near without a dynamic sort buffer. */
static void traffic(canvas_t *c, const rr_game_t *g)
{
    int order[RR_ENTITIES];
    for (int i = 0; i < RR_ENTITIES; ++i) order[i] = i;
    for (int i = 1; i < RR_ENTITIES; ++i) {
        int key = order[i], j = i-1;
        while (j >= 0 && g->entities[order[j]].depth > g->entities[key].depth) {
            order[j+1] = order[j]; --j;
        }
        order[j+1] = key;
    }
    int player_drawn=0;
    for (int i = 0; i < RR_ENTITIES; ++i) {
        const rr_entity_t *e = &g->entities[order[i]];
        if (!e->active) continue;
        rr_rect_t rect=rr_entity_rect(g,e);
        int y=rect.y+rect.h,h=rect.h,x=rect.x+rect.w/2;
        rr_rect_t player_rect=rr_player_rect(g);
        if (!player_drawn && y>=player_rect.y+player_rect.h) {
            player(c,g); player_drawn=1;
        }
        if (e->active == 2) {
            line(c,x-h/3,y-h/3,x+h/3,y-h/5,4,INK);
            line(c,x-h/4,y-h/3,x+h/4,y-h/5,2,RED);
            for (int j=0;j<4;++j) box(c,x-h/2+j*h/3,y-h+(j%2)*5,2,2,GOLD);
            ztext(c,x-12,y-h-14,RR_TEXT_KO,1,GOLD);
        } else if (e->car) {
            int w = h*4/5;
            oval(c,x,y+1,w*2/3,max(1,h/10),INK);
            box(c,x-w/2,y-h,w,h,INK);
            box(c,x-w/2+2,y-h+2,w-4,h-4,RGB(116,138,172));
            box(c,x-w/3,y-h+3,w*2/3,h/3,RGB(37,61,84));
            box(c,x-w/2+2,y-h/4,w/5,max(1,h/12),RED);
            box(c,x+w/4,y-h/4,w/5,max(1,h/12),RED);
        } else {
            motorcycle(c,x,y,h,e->color,0,0);
            if (rr_attackable(g,e)) {
                box(c,x-8,y-h-16,17,14,GOLD);
                ztext(c,x-5,y-h-15,RR_TEXT_HIT,1,INK);
            }
        }
    }
    if (!player_drawn) player(c,g);
    int player_x=rr_road_center(g,211)+g->lane_q8*85/256;
    if (g->hurt_ms > 900) {
        ztext(c,136,140,RR_TEXT_CRASH,2,CREAM);
        for (int i = 0; i < 5; ++i) box(c,player_x-20+i*9,215-(i%3)*5,3,3,GOLD);
    }
}

/* Opaque HUD avoids expensive alpha layers. Keep all content away from corners. */
static void hud(canvas_t *c, const rr_game_t *g)
{
    box(c,22,7,276,27,INK);
    ztext(c,27,11,RR_TEXT_POS,1,RGB(161,167,190));
    number(c,53,12,rr_rank(g),2,CREAM); text(c,66,19,"/8",1,RGB(161,167,190));
    ztext(c,89,10,RR_TEXT_ARMOR,1,RGB(161,167,190));
    box(c,89,26,60,4,RGB(58,52,70)); box(c,89,26,g->health*60/100,4,g->health>30 ? TEAL : RED);
    number(c,165,12,g->speed,2,GOLD); text(c,205,20,"KM/H",1,CREAM);
    ztext(c,252,9,RR_TEXT_BATTERY,1,RGB(161,167,190));
    if (g->battery < 0) text(c,276,22,"--",1,CREAM);
    else number(c,276,22,g->battery,1,CREAM);
    box(c,34,231,252,3,RGB(31,29,46));
    box(c,34,231,g->metres_mm/1000*252/rr_stage(g)->metres,3,GOLD);
    for(int i=0;i<3;i++)box(c,27+i*10,63,6,3,i<g->chain?TEAL:RGB(65,77,99));
    box(c,250,38,48,23,INK);
    ztext(c,255,40,g->cooldown_ms ? RR_TEXT_COOLDOWN : RR_TEXT_READY,1,g->cooldown_ms ? RGB(161,167,190) : TEAL);
    box(c,254,57,40,2,RGB(58,52,70));
    box(c,254,57,(450-g->cooldown_ms)*40/450,2,TEAL);
    if(g->boost_ms>0){box(c,96,77,128,19,INK);ztext(c,130,79,RR_BOOST,1,GOLD);box(c,102,94,116*g->boost_ms/3000,2,TEAL);}
    box(c,22,38,50,20,INK);
    number(c,28,41,g->stage,2,GOLD); text(c,42,48,"/5",1,CREAM);
    if (g->elapsed_ms < 4000 && g->phase == RR_RACING) {
        box(c,68,38,184,36,INK);
        ztext(c,136,40,stage_name(g),1,GOLD);
        ztext(c,91,57,challenge_name(g),1,CREAM);
    } else if (g->phase==RR_RACING) {
        box(c,77,38,166,17,INK);
        ztext(c,85,39,challenge_name(g),1,rr_bonus(g) ? TEAL : CREAM);
    }
}

/* Title, pause and results are distinct persistent pages over the shared world. */
static void overlay(canvas_t *c, const rr_game_t *g)
{
    if (g->phase == RR_RACING) return;
    if (g->phase == RR_TITLE) {
        ztext(c,40,9,g->muted?RR_MUTED:RR_SOUND,1,CREAM);
        box(c,38,27,244,111,INK);
        box(c,42,31,236,1,GOLD); box(c,42,132,236,1,GOLD);
        ztext(c,88,37,RR_TEXT_TITLE,3,CREAM);
        text(c,106,86,"ROAD RAGE",2,GOLD);
        number(c,117,117,g->stage,1,GOLD);
        text(c,124,117,"/5",1,CREAM);
        ztext(c,145,112,stage_name(g),1,RGB(176,156,170));
        box(c,96,144,128,21,GOLD); ztext(c,121,148,RR_TEXT_START,1,INK);
        box(c,43,177,104,20,INK); ztext(c,66,181,RR_TEXT_LANES,1,CREAM);
        box(c,176,177,99,20,INK); ztext(c,207,181,RR_TEXT_ATTACK,1,CREAM);
        ztext(c,97,209,RR_TEXT_STAGE_SELECT,1,CREAM);
        ztext(c,106,223,RR_TEXT_PAUSE_HINT,1,RGB(176,156,170));
    } else {
        box(c,48,60,224,122,INK); box(c,52,64,216,2,GOLD);
        const char *title = g->phase == RR_PAUSED ? RR_TEXT_PAUSED : g->phase == RR_FINISHED ? (g->stage==RR_STAGES ? RR_TEXT_ALL_CLEAR : RR_TEXT_FINISH) : RR_TEXT_WRECKED;
        int title_x = g->phase == RR_PAUSED ? 124 : 112;
        ztext(c,title_x,76,title,2,g->phase == RR_WRECKED ? RED : GOLD);
        if (g->phase == RR_PAUSED) {
            ztext(c,115,112,RR_TEXT_BREATH,1,CREAM);
            ztext(c,121,138,RR_TEXT_RESUME,1,TEAL);
            ztext(c,109,158,RR_TEXT_HOME,1,CREAM);
        } else {
            ztext(c,76,108,RR_TEXT_SCORE,1,RGB(161,167,190)); number(c,118,105,g->score,2,CREAM);
            ztext(c,187,108,RR_TEXT_POS,1,RGB(161,167,190)); number(c,220,105,rr_rank(g),2,GOLD);
            ztext(c,76,126,RR_TEXT_KNOCKOUTS,1,RGB(161,167,190)); number(c,140,129,g->knockouts,1,TEAL);
            if(g->phase==RR_FINISHED) {
                int stars=rr_stars(g);
                static const unsigned char star[7]={4,4,31,14,14,10,17};
                for(int i=0;i<3;++i)for(int yy=0;yy<7;++yy)for(int xx=0;xx<5;++xx)
                    if(star[yy]&(16>>xx))box(c,181+i*17+xx*2,125+yy*2,2,2,i<stars ? GOLD : RGB(95,88,105));
            }
            if(g->phase==RR_FINISHED) {
                ztext(c,76,144,RR_TEXT_BONUS,1,CREAM);
                ztext(c,134,144,rr_bonus(g) ? RR_TEXT_COMPLETE : RR_TEXT_NOT_COMPLETE,1,rr_bonus(g) ? TEAL : RGB(161,167,190));
            } else ztext(c,76,144,RR_TEXT_RETRY,1,CREAM);
            box(c,76,161,168,16,GOLD);
            ztext(c,121,163,g->phase==RR_FINISHED && g->stage<RR_STAGES ? RR_TEXT_NEXT : RR_TEXT_REPLAY,1,INK);
        }
    }
}

void rr_render_strip(const rr_game_t *g, uint16_t *pixels, int y, int rows)
{
    if (!pixels || y < 0 || rows <= 0 || y+rows > RR_HEIGHT) return;
    canvas_t c = { pixels,y,rows };
    landscape(&c,g); traffic(&c,g);
    if(g->stage==4)for(int i=0;i<18;++i) {
        int x=(i*47+(int)g->scenery_ms/19)%320;
        int y=(i*31+(int)g->scenery_ms/8)%240;
        line(&c,x,y,x-2,y+7,1,RGB(102,123,145));
    }
    if (g->phase != RR_TITLE) hud(&c,g);
    overlay(&c,g);
    if(g->phase==RR_TITLE)game_visual_medals(pixels,y,rows,102,game_medal_count(g->medals,g->stage),GOLD,RGB(70,83,103));
    /* Match the physical 30px rounded corners in landscape coordinates. */
    for (int yy = y; yy < y+rows; ++yy) {
        int dy = yy < 30 ? 30-yy : yy >= 210 ? yy-209 : 0;
        if (!dy) continue;
        for (int xx = 0; xx < 30; ++xx) {
            int dx = 30-xx;
            if (dx*dx+dy*dy > 30*30) {
                pixels[(yy-y)*320+xx] = 0;
                pixels[(yy-y)*320+319-xx] = 0;
            }
        }
    }
}
