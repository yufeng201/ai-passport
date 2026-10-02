#include "bw_game.h"
#include "../common/game_visual.h"
#include "bw_copy.h"
#include "brick_workshop_noto_sc_12.h"
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
    int left = max(x, 0), right = min(x + w, BW_WIDTH);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * BW_WIDTH + xx] = color;
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
        for (int x = max(cx - rx, 0); x <= min(cx + rx, BW_WIDTH - 1); ++x)
            if ((x-cx)*(x-cx)*ry*ry + (y-cy)*(y-cy)*rx*rx <= rx*rx*ry*ry)
                c->pixels[(y-c->y)*BW_WIDTH+x] = color;
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
        const bw_cjk_glyph_t *glyph = NULL;
        for (unsigned i=0;i<BW_CJK_GLYPH_COUNT;++i)
            if(bw_cjk_glyphs[i].codepoint==codepoint){glyph=&bw_cjk_glyphs[i];break;}
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
static void center(canvas_t *c,int y,const char *s,int scale,uint16_t color){ztext(c,(BW_WIDTH-text_width(s,scale))/2,y,s,scale,color);}
static void background(canvas_t *c,const bw_game_t *g){
    for(int yy=c->y;yy<c->y+c->rows;yy++)box(c,0,yy,320,1,RGB(12+yy/24,17+yy/18,36+yy/9));
    for(int x=24;x<300;x+=20)line(c,x,35,x,210,1,RGB(27,39,62));
    for(int y=39;y<211;y+=20)line(c,16,y,304,y,1,RGB(27,39,62));
    line(c,15,34,15,211,2,RGB(52,115,144));line(c,303,34,303,211,2,RGB(52,115,144));
    line(c,16,34,304,34,1,TEAL);
    for(int i=0;i<10;i++){int x=24+i*29;box(c,x,211,16,2,RGB(51,69,88));}
    if(g->slow_ms>0&&g->phase!=BW_TITLE){
        line(c,19,120,300,120,1,RGB(37,122,128));
        for(int x=24;x<301;x+=16)box(c,x,119,2,3,TEAL);
    }
}
static void brick(canvas_t *c,int x,int y,int hp,int row,int flash){
    static const uint16_t shades[]={RGB(215,89,140),RGB(230,162,76),RGB(83,208,182),RGB(100,129,219)};
    uint16_t shade=shades[row%4];
    box(c,x,y,36,10,RGB(42,49,76));box(c,x+1,y+1,34,7,flash?CREAM:shade);
    line(c,x+2,y+1,x+31,y+1,1,CREAM);box(c,x+2,y+3,5,2,RGB(209,226,224));
    box(c,x+2,y+8,32,1,RGB(60,76,100));
    for(int i=1;i<hp;i++)box(c,x+16+(i-1)*5,y+4,3,2,INK);
}
static void world(canvas_t *c,const bw_game_t *g){
    if(g->phase==BW_TITLE)for(int i=0;i<6;i++)brick(c,36+i*42,98,1,i%4,0);
    for(int i=0;i<BW_BRICKS;i++)if(g->bricks[i].hp){
        int x=bw_brick_x(g,i),y=g->bricks[i].y;
        brick(c,x,y,g->bricks[i].hp,i/6,g->flash_ms>0&&g->flash_x==x+18&&g->flash_y==y+5);
    }
    if(g->stage==3||g->stage==5){
        box(c,126,138,68,8,RGB(80,104,127));line(c,129,139,189,139,1,CREAM);
        for(int x=130;x<192;x+=10)line(c,x,142,x+4,144,1,GOLD);
    }
    int px=g->phase==BW_TITLE?160:g->paddle_x;
    box(c,px-BW_PADDLE_W/2,BW_PADDLE_Y,BW_PADDLE_W,7,RGB(90,113,146));
    box(c,px-BW_PADDLE_W/2+2,BW_PADDLE_Y+1,BW_PADDLE_W-4,2,TEAL);
    for(int i=-20;i<=20;i+=10)box(c,px+i,BW_PADDLE_Y+5,4,2,GOLD);
    int bx=g->ball_x/BW_Q,by=g->ball_y/BW_Q;
    if(g->phase==BW_TITLE){bx=159;by=158;}
    if(!g->ready&&g->phase!=BW_TITLE){
        for(int i=1;i<=3;i++)box(c,bx-i*g->vx/BW_Q/2,by-i*g->vy/BW_Q/2,2,2,RGB(81,142,169));
    }
    oval(c,bx,by,3,3,CREAM);box(c,bx-1,by-1,2,2,TEAL);
    if(g->flash_ms>0){
        int r=2+(120-g->flash_ms)/20;
        line(c,g->flash_x-r,g->flash_y,g->flash_x+r,g->flash_y,1,GOLD);
    }
}
static void hud(canvas_t *c,const bw_game_t *g){
    box(c,32,8,216,25,INK);number(c,40,15,g->stage,1,GOLD);
    ztext(c,64,13,BW_PROGRESS,1,CREAM);number(c,94,15,g->remaining,1,TEAL);
    ztext(c,142,13,BW_SCORE,1,CREAM);number(c,173,15,g->score,1,GOLD);
    for(int i=0;i<3;i++)box(c,260+i*9,13,6,9,i<g->lives?TEAL:RGB(54,64,86));
    if(g->phase!=BW_PLAY)return;
    ztext(c,20,183,BW_USES,1,RGB(146,177,192));number(c,50,185,g->slow_uses,1,GOLD);
    if(g->supply_ms>0)center(c,150,BW_SUPPLY,1,TEAL);
    else if(g->assist_ms>0)center(c,150,BW_ASSIST,1,GOLD);
    box(c,245,185,48,3,INK);box(c,245,185,48*(g->destroyed%6)/6,3,GOLD);
    box(c,32,217,256,20,INK);
    center(c,219,g->ready?BW_READY:g->slow_ms>0?BW_SLOW:g->slow_uses>0?BW_ACTION:BW_EMPTY,1,CREAM);
    if(g->slow_ms>0)box(c,74,236,172*g->slow_ms/3000,1,TEAL);
}
static void overlay(canvas_t *c,const bw_game_t *g){
    if(g->phase==BW_TITLE){
        box(c,35,44,250,47,INK);center(c,47,BW_TITLE_TEXT,2,CREAM);center(c,78,BW_SUBTITLE,1,TEAL);
        box(c,41,111,238,80,INK);center(c,115,bw_stage_names[g->stage-1],1,GOLD);
        center(c,136,BW_START,1,TEAL);center(c,157,BW_CONTROL,1,CREAM);center(c,177,BW_SELECT,1,RGB(152,176,203));
        ztext(c,34,14,g->muted?BW_MUTED:BW_SOUND,1,CREAM);
    }else if(g->phase==BW_PAUSED||g->phase==BW_CLEAR||g->phase==BW_FAILED){
        box(c,39,55,242,137,TEAL);box(c,41,57,238,133,INK);
        center(c,69,g->phase==BW_PAUSED?BW_PAUSED_TEXT:g->phase==BW_FAILED?BW_FAILED_TEXT:g->stage==5?BW_CLEAR_TEXT:BW_WON,2,CREAM);
        ztext(c,92,106,BW_SCORE,1,CREAM);number(c,148,108,g->score,2,GOLD);
        ztext(c,92,128,BW_BEST,1,CREAM);number(c,148,130,g->best,1,TEAL);
        center(c,151,g->phase==BW_PAUSED?BW_RESUME:g->phase==BW_CLEAR&&g->stage<5?BW_NEXT:BW_RETRY,1,CREAM);
        center(c,173,g->phase==BW_PAUSED?BW_EXIT:BW_HOME,1,RGB(151,180,207));
    }
}
void bw_render_strip(const bw_game_t *g,uint16_t *pixels,int y,int rows){
    if(!pixels||y<0||rows<=0||y>BW_HEIGHT||rows>BW_HEIGHT-y)return;
    canvas_t c={pixels,y,rows};background(&c,g);world(&c,g);if(g->phase!=BW_TITLE)hud(&c,g);overlay(&c,g);
    if(g->phase==BW_CLEAR)center(&c,201,BW_MEDAL_GOAL,1,CREAM);
    if(g->phase==BW_CLEAR)game_visual_medals(pixels,y,rows,143,1+(g->lives==3)+(g->slow_uses>0),GOLD,RGB(70,83,103));
    if(g->phase==BW_TITLE)game_visual_medals(pixels,y,rows,34,game_medal_count(g->medals,g->stage),GOLD,RGB(70,83,103));
    if(g->phase==BW_TITLE)for(int i=0;i<5;i++){box(&c,121+i*17,204,11,3,i+1==g->stage?GOLD:i<g->unlocked?TEAL:RGB(65,77,99));}
    if(g->phase==BW_PLAY&&g->flash_ms>0)game_visual_burst(pixels,y,rows,g->flash_x,g->flash_y,120-g->flash_ms,GOLD);
    for(int yy=y;yy<y+rows;yy++){
        int dy=yy<30?30-yy:yy>=210?yy-209:0;if(!dy)continue;
        for(int xx=0;xx<30;xx++){int dx=30-xx;if(dx*dx+dy*dy>900){pixels[(yy-y)*320+xx]=0;pixels[(yy-y)*320+319-xx]=0;}}
    }
}
