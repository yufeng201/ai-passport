#include "sg_game.h"
#include "sg_copy.h"
#include "starport_gunner_noto_sc_12.h"
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
    int left = max(x, 0), right = min(x + w, SG_WIDTH);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * SG_WIDTH + xx] = color;
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
        for (int x = max(cx - rx, 0); x <= min(cx + rx, SG_WIDTH - 1); ++x)
            if ((x-cx)*(x-cx)*ry*ry + (y-cy)*(y-cy)*rx*rx <= rx*rx*ry*ry)
                c->pixels[(y-c->y)*SG_WIDTH+x] = color;
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
        const sg_cjk_glyph_t *glyph = NULL;
        for (unsigned i=0;i<SG_CJK_GLYPH_COUNT;++i)
            if(sg_cjk_glyphs[i].codepoint==codepoint){glyph=&sg_cjk_glyphs[i];break;}
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
static void center(canvas_t *c,int y,const char *s,int scale,uint16_t color){ztext(c,(SG_WIDTH-text_width(s,scale))/2,y,s,scale,color);}
static void background(canvas_t *c,const sg_game_t *g){
    for(int yy=c->y;yy<c->y+c->rows;yy++)box(c,0,yy,320,1,RGB(9+g->stage*2,12+yy/14,31+yy/5));
    for(int i=0;i<60;i++){
        int x=20+(i*73)%280,y=34+(i*47+g->scene_ms/140)%147;
        box(c,x,y,i%9==0?2:1,1,RGB(70+(i%4)*38,125+(i%3)*34,200));
    }
    /* Distant planet and rings, behind every projectile and ship. */
    oval(c,260,88,33,33,RGB(34,47,97));oval(c,254,84,24,28,RGB(41,66,116));
    line(c,212,99,299,68,1,RGB(72,104,155));line(c,213,102,301,71,1,RGB(58,80,132));
    for(int i=0;i<12;i++){
        int x=24+i*25,h=12+(i*17)%26;
        box(c,x,199-h,17,h,RGB(21,36,59));box(c,x+3,199-h+3,3,3,RGB(40,122,151));
    }
    box(c,16,198,288,3,TEAL);box(c,16,201,288,10,RGB(33,54,80));
    for(int x=22;x<300;x+=18)box(c,x,205,10,2,RGB(68,108,130));
}
static void ship(canvas_t *c,int x,int y,int kind,int warning){
    uint16_t metal=kind==2?RGB(168,95,175):kind?RGB(206,146,84):RGB(90,129,185);
    int w=kind==2?26:13;
    line(c,x-w,y+6,x,y-9,3,metal);line(c,x,y-9,x+w,y+6,3,metal);
    box(c,x-w,y+4,w*2,5,RGB(50,62,100));box(c,x-5,y-5,10,13,metal);
    box(c,x-3,y-3,6,6,warning?RED:RGB(236,206,244));
    box(c,x-w+3,y+9,5,3,TEAL);box(c,x+w-6,y+9,5,3,TEAL);
    if(kind==2){box(c,x-18,y-3,5,7,GOLD);box(c,x+14,y-3,5,7,GOLD);}
}
static void world(canvas_t *c,const sg_game_t *g){
    int dx=sg_aim_dx(g->aim),dy=sg_aim_dy(g->aim);
    if(g->phase==SG_TITLE){ship(c,92,108,0,0);ship(c,224,120,1,0);ship(c,162,92,2,0);}
    else{
        for(int i=0;i<SG_ENEMIES;i++){
            const sg_enemy_t *e=&g->enemies[i];if(!e->active)continue;
            ship(c,e->x,e->y,e->kind,e->attack_ms<700);
            if(e->max_hp>1){int w=e->kind==2?48:24;
                box(c,e->x-w/2,e->y-15,w,3,RGB(48,49,83));box(c,e->x-w/2,e->y-15,w*e->hp/e->max_hp,3,GOLD);
            }
        }
        for(int i=0;i<SG_THREATS;i++){
            const sg_threat_t *t=&g->threats[i];if(!t->active)continue;
            if(t->warning_ms>0){
                line(c,t->x-7,t->y-7,t->x+7,t->y+7,1,RED);line(c,t->x+7,t->y-7,t->x-7,t->y+7,1,RED);
                for(int yy=t->y+12;yy<196;yy+=12)box(c,t->x,yy,1,4,RGB(165,65,101));
                if((g->scene_ms/140)%2==0)box(c,t->x-5,193,11,3,RED);
            }else{oval(c,t->x,t->y,4,5,RED);box(c,t->x-1,t->y-9,2,5,GOLD);}
        }
        for(int i=0;i<SG_SHOTS;i++){
            const sg_shot_t *s=&g->shots[i];if(!s->active)continue;
            line(c,s->x-s->dx,s->y-s->dy,s->x,s->y,s->power>1?4:2,s->power>1?GOLD:TEAL);
            box(c,s->x-1,s->y-2,2,3,CREAM);
        }
        if(g->flash_ms>0){int r=4+(180-g->flash_ms)/20;
            line(c,g->flash_x-r,g->flash_y,g->flash_x+r,g->flash_y,2,GOLD);
            line(c,g->flash_x,g->flash_y-r,g->flash_x,g->flash_y+r,2,CREAM);
        }
    }
    /* Dotted aiming rail remains readable after releasing the direction key. */
    if(g->phase==SG_PLAY&&!g->charging)for(int i=3;i<8;i++)box(c,160+dx*i,189+dy*i,1,2,RGB(83,149,175));
    oval(c,160,201,24,8,RGB(43,72,100));box(c,145,192,30,10,RGB(79,120,152));
    line(c,160,196,160+dx*2,196+dy*2,6,RGB(123,159,188));
    line(c,160,193,160+dx*2,193+dy*2,2,g->charging?GOLD:TEAL);
    box(c,150,202,20,3,RGB(112,209,205));
    if(g->damage_ms>0&&(g->scene_ms/100)%2==0)box(c,148,191,24,4,RED);
}
static void hud(canvas_t *c,const sg_game_t *g){
    box(c,32,8,216,25,INK);ztext(c,39,13,SG_STAGE,1,CREAM);number(c,56,15,g->stage,1,GOLD);
    ztext(c,74,13,SG_PROGRESS,1,CREAM);number(c,104,15,g->kills,1,TEAL);text(c,117,15,"/",1,CREAM);number(c,125,15,g->goal,1,CREAM);
    ztext(c,151,13,SG_SCORE,1,CREAM);number(c,182,15,g->score,1,GOLD);
    for(int i=0;i<3;i++){box(c,260+i*9,13,6,9,i<g->health?TEAL:RGB(54,64,86));}
    if(g->phase!=SG_PLAY)return;
    box(c,32,213,256,24,INK);
    if(g->charging){
        box(c,43,215,234,3,RGB(47,63,83));box(c,43,215,234*g->charge_ms/SG_CHARGE_MAX,3,GOLD);
        center(c,220,g->charge_ms>=SG_CHARGE_THRESHOLD&&g->cooldown_ms==0?SG_CHARGING:SG_NORMAL,1,CREAM);
    }else{
        center(c,217,g->cooldown_ms>0?SG_COOLING:SG_READY,1,g->cooldown_ms>0?CREAM:TEAL);
        if(g->cooldown_ms>0)box(c,48,233,224*(SG_COOLDOWN-g->cooldown_ms)/SG_COOLDOWN,2,GOLD);
    }
}
static void overlay(canvas_t *c,const sg_game_t *g){
    if(g->phase==SG_TITLE){
        center(c,39,SG_TITLE_TEXT,2,CREAM);center(c,72,SG_SUBTITLE,1,TEAL);
        box(c,38,142,244,90,INK);center(c,145,sg_stage_names[g->stage-1],1,GOLD);
        center(c,167,SG_START,1,TEAL);center(c,186,SG_CONTROL,1,CREAM);center(c,207,SG_SELECT,1,RGB(151,180,207));
        ztext(c,34,14,g->muted?SG_MUTED:SG_SOUND,1,CREAM);
    }else if(g->phase==SG_PAUSED||g->phase==SG_CLEAR||g->phase==SG_FAILED){
        box(c,39,55,242,137,TEAL);box(c,41,57,238,133,INK);
        center(c,69,g->phase==SG_PAUSED?SG_PAUSED_TEXT:g->phase==SG_FAILED?SG_FAILED_TEXT:g->stage==5?SG_CLEAR_TEXT:SG_WON,2,CREAM);
        ztext(c,92,106,SG_SCORE,1,CREAM);number(c,148,108,g->score,2,GOLD);
        ztext(c,92,128,SG_BEST,1,CREAM);number(c,148,130,g->best,1,TEAL);
        center(c,151,g->phase==SG_PAUSED?SG_RESUME:g->phase==SG_CLEAR&&g->stage<5?SG_NEXT:SG_RETRY,1,CREAM);
        center(c,173,g->phase==SG_PAUSED?SG_EXIT:SG_HOME,1,RGB(151,180,207));
    }
}
void sg_render_strip(const sg_game_t *g,uint16_t *pixels,int y,int rows){
    if(!pixels||y<0||rows<=0||y>SG_HEIGHT||rows>SG_HEIGHT-y)return;
    canvas_t c={pixels,y,rows};background(&c,g);world(&c,g);if(g->phase!=SG_TITLE)hud(&c,g);overlay(&c,g);
    for(int yy=y;yy<y+rows;yy++){
        int dy=yy<30?30-yy:yy>=210?yy-209:0;if(!dy)continue;
        for(int xx=0;xx<30;xx++){int dx=30-xx;if(dx*dx+dy*dy>900){pixels[(yy-y)*320+xx]=0;pixels[(yy-y)*320+319-xx]=0;}}
    }
}
