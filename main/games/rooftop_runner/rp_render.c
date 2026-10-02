#include "rp_game.h"
#include "../common/game_visual.h"
#include "rp_copy.h"
#include "rooftop_runner_noto_sc_12.h"
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
    int left = max(x, 0), right = min(x + w, RP_WIDTH);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * RP_WIDTH + xx] = color;
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
        for (int x = max(cx - rx, 0); x <= min(cx + rx, RP_WIDTH - 1); ++x)
            if ((x-cx)*(x-cx)*ry*ry + (y-cy)*(y-cy)*rx*rx <= rx*rx*ry*ry)
                c->pixels[(y-c->y)*RP_WIDTH+x] = color;
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
        const rp_cjk_glyph_t *glyph = NULL;
        for (unsigned i=0;i<RP_CJK_GLYPH_COUNT;++i)
            if(rp_cjk_glyphs[i].codepoint==codepoint){glyph=&rp_cjk_glyphs[i];break;}
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

static int text_width(const char *s,int scale){
    int n=0;while(*s){if((unsigned char)*s<128){s++;n+=6*scale;}else{s+=3;n+=12*scale;}}return n;
}
static void center(canvas_t *c,int y,const char *s,int scale,uint16_t color){ztext(c,(RP_WIDTH-text_width(s,scale))/2,y,s,scale,color);}
/* All world positions remain in landscape pixels. The camera is used only for
 * drawing, so scrolling cannot change collision geometry. */
static void background(canvas_t *c,const rp_game_t *g){
    static const int skies[5][3]={{35,74,107},{47,63,114},{88,54,96},{21,36,69},{96,63,89}};
    int st=g->stage-1;
    for(int y=c->y;y<c->y+c->rows;y++)box(c,0,y,320,1,RGB(skies[st][0]+y/5,skies[st][1]+y/6,skies[st][2]+y/8));
    oval(c,254-g->camera/16%40,59,20,20,RGB(255,203,132));
    for(int i=0;i<10;i++){
        int x=i*44-(g->camera/5%44),height=25+(i*17%42);
        box(c,x,159-height,34,height+65,RGB(35,51,74));
        for(int y=165-height;y<156;y+=12)for(int xx=x+6;xx<x+28;xx+=9)box(c,xx,y,3,5,RGB(139,154,144));
    }
    for(int i=0;i<8;i++){
        int x=i*57-(g->camera/3%57),height=29+(i*23%37);
        box(c,x,190-height,46,height+50,RGB(24,34,52));box(c,x+8,185-height,28,5,RGB(34,48,67));
        for(int y=196-height;y<183;y+=14)for(int xx=x+7;xx<x+40;xx+=10)box(c,xx,y,3,5,RGB(205,158,99));
    }
    for(int i=0;i<3;i++){int x=(i*111+g->scene_ms/180)%350-15;line(c,x,72+i*9,x+5,69+i*9,1,INK);line(c,x+5,69+i*9,x+10,72+i*9,1,INK);}
}
static void roof(canvas_t *c,const rp_game_t *g,int i){
    const rp_platform_t *p=&g->platforms[i];int x=p->x-g->camera,y=p->y;
    if(x+p->w<0||x>320)return;
    box(c,x,y,p->w,240-y,RGB(36,44,59));box(c,x,y,p->w,4,CREAM);box(c,x,y+4,p->w,3,RGB(84,103,116));
    for(int yy=y+12;yy<240;yy+=13){line(c,x,yy,x+p->w,yy,1,RGB(49,58,71));for(int xx=x+(yy/13%2)*10;xx<x+p->w;xx+=24)line(c,xx,yy-10,xx,yy,1,RGB(49,58,71));}
    if(g->stage<=2&&i<11)line(c,x+p->w-16,y,x+p->w-1,y,2,TEAL);
    if(i%3==0){line(c,x+10,y-25,x+10,y,2,TEAL);box(c,x+12,y-25,14,8,i<=g->checkpoint?TEAL:RGB(101,120,136));}
    if(i==11){line(c,x+p->w-18,y-42,x+p->w-18,y,2,GOLD);box(c,x+p->w-16,y-42,16,12,GOLD);box(c,x+p->w-16,y-42,7,5,CREAM);}
    if(i>0&&!(g->collected&(1u<<i))){int cx=x+p->w/2,cy=y-30;oval(c,cx,cy,4,5,GOLD);box(c,cx-1,cy-3,2,5,CREAM);}
    if(p->hazard){int hx=rp_hazard_x(g,i)-g->camera;
        if(p->hazard==1){for(int n=-6;n<7;n+=6){line(c,hx+n,y-1,hx+n+3,y-10,2,RED);line(c,hx+n+3,y-10,hx+n+6,y-1,2,RED);}}
        else{oval(c,hx,y-5,7,7,RGB(163,183,200));oval(c,hx,y-5,3,3,RED);for(int k=-1;k<=1;k+=2){line(c,hx+k*9,y-5,hx+k*5,y-9,1,CREAM);line(c,hx+k*8,y-2,hx+k*6,y+2,1,CREAM);}}
    }
}
static void runner(canvas_t *c,const rp_game_t *g){
    int x=g->x/RP_Q-g->camera,y=g->y/RP_Q,dir=g->facing<0?-1:1;
    if(g->grounded)oval(c,x,y+2,9,2,RGB(16,24,40));
    int stride=g->grounded&&g->vx?(int)(g->elapsed_ms/100%2)*4-2:0;
    line(c,x-2,y-7,x-4-stride,y-1,3,INK);line(c,x+2,y-7,x+4+stride,y-1,3,INK);
    box(c,x-4-stride,y-1,4,2,CREAM);box(c,x+3+stride,y-1,4,2,CREAM);
    box(c,x-5,y-15,11,10,INK);box(c,x-4,y-14,9,8,TEAL);box(c,x-4,y-14,3,6,RGB(27,125,139));
    box(c,x-3,y-21,7,7,CREAM);box(c,x-4,y-23,8,4,INK);box(c,x+(dir>0?3:-3),y-18,2,2,INK);
    line(c,x-dir*3,y-14,x-dir*12,y-12+stride,2,GOLD);
    line(c,x+dir*3,y-12,x+dir*7,y-10,2,CREAM);
}
static void world(canvas_t *c,const rp_game_t *g){for(int i=0;i<RP_PLATFORMS;i++)roof(c,g,i);runner(c,g);}
static void hud(canvas_t *c,const rp_game_t *g){
    box(c,32,8,256,25,INK);number(c,40,15,g->stage,1,GOLD);ztext(c,60,13,RP_PROGRESS,1,CREAM);number(c,89,15,g->furthest+1,1,TEAL);text(c,103,15,"/12",1,CREAM);
    ztext(c,150,13,RP_SCORE,1,CREAM);number(c,183,15,g->score,1,GOLD);
    box(c,36,30,246,2,RGB(68,83,106));box(c,36,30,246*g->furthest/11,2,TEAL);
    if(g->phase!=RP_PLAY)return;
    if(g->feedback_ms>0){box(c,77,53,166,21,INK);center(c,57,RP_SAVED,1,TEAL);}
    else if(g->stage<=2&&g->grounded&&g->platform<11&&g->x/RP_Q>g->platforms[g->platform].x+g->platforms[g->platform].w-38){box(c,87,53,146,21,INK);center(c,57,RP_JUMP_HINT,1,GOLD);}
    box(c,32,211,256,29,INK);center(c,212,RP_ACTION,1,CREAM);center(c,225,RP_PAUSE,1,RGB(157,184,202));
}
static void overlay(canvas_t *c,const rp_game_t *g){
    if(g->phase==RP_TITLE){
        box(c,32,41,256,51,INK);center(c,44,RP_TITLE_TEXT,2,CREAM);center(c,77,RP_SUBTITLE,1,GOLD);
        box(c,36,108,248,93,INK);center(c,110,rp_stage_names[g->stage-1],1,GOLD);center(c,131,RP_START,1,TEAL);
        center(c,151,RP_CONTROL,1,CREAM);center(c,168,RP_LINK,1,RGB(168,193,213));center(c,185,RP_SELECT,1,RGB(157,184,202));
        ztext(c,34,14,g->muted?RP_MUTED:RP_SOUND,1,CREAM);
    }else if(g->phase==RP_PAUSED||g->phase==RP_CLEAR){
        box(c,39,55,242,140,TEAL);box(c,41,57,238,136,INK);
        center(c,68,g->phase==RP_PAUSED?RP_PAUSED_TEXT:g->stage==5?RP_CLEAR_TEXT:RP_WON,2,CREAM);
        ztext(c,92,106,RP_SCORE,1,CREAM);number(c,148,108,g->score,2,GOLD);ztext(c,92,128,RP_BEST,1,CREAM);number(c,148,130,g->best,1,TEAL);
        center(c,151,g->phase==RP_PAUSED?RP_RESUME:g->stage<5?RP_NEXT:RP_RETRY,1,CREAM);center(c,174,g->phase==RP_PAUSED?RP_EXIT:RP_HOME,1,RGB(161,185,207));
    }else if(g->phase==RP_FAILED){box(c,51,81,218,74,INK);center(c,89,RP_FAILED_TEXT,2,CREAM);center(c,130,RP_RESPAWN,1,GOLD);}
}
void rp_render_strip(const rp_game_t *g,uint16_t *pixels,int y,int rows){
    if(!pixels||y<0||rows<=0||y>RP_HEIGHT||rows>RP_HEIGHT-y)return;
    canvas_t c={pixels,y,rows};background(&c,g);world(&c,g);if(g->phase!=RP_TITLE)hud(&c,g);overlay(&c,g);
    if(g->phase==RP_CLEAR)center(&c,201,RP_MEDAL_GOAL,1,CREAM);
    if(g->phase==RP_CLEAR)game_visual_medals(pixels,y,rows,143,1+(g->falls==0)+((g->collected&0xffeu)==0xffeu),GOLD,RGB(70,83,103));
    if(g->phase==RP_TITLE)game_visual_medals(pixels,y,rows,34,game_medal_count(g->medals,g->stage),GOLD,RGB(70,83,103));
    if(g->phase==RP_TITLE)for(int i=0;i<5;i++){box(&c,121+i*17,204,11,3,i+1==g->stage?GOLD:i<g->unlocked?TEAL:RGB(65,77,99));}
    if(g->phase==RP_PLAY&&g->feedback_ms>0)game_visual_burst(pixels,y,rows,g->x/RP_Q-g->camera,g->y/RP_Q-20,1200-g->feedback_ms,TEAL);
    for(int yy=y;yy<y+rows;yy++){int dy=yy<30?30-yy:yy>=210?yy-209:0;if(!dy)continue;
        for(int xx=0;xx<30;xx++){int dx=30-xx;if(dx*dx+dy*dy>900){pixels[(yy-y)*320+xx]=0;pixels[(yy-y)*320+319-xx]=0;}}
    }
}
