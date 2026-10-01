#include "cb_game.h"
#include "cb_copy.h"
#include "cloudbound_noto_sc_12.h"
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
    int left = max(x, 0), right = min(x + w, CB_WIDTH);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * CB_WIDTH + xx] = color;
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
        for (int x = max(cx - rx, 0); x <= min(cx + rx, CB_WIDTH - 1); ++x)
            if ((x-cx)*(x-cx)*ry*ry + (y-cy)*(y-cy)*rx*rx <= rx*rx*ry*ry)
                c->pixels[(y-c->y)*CB_WIDTH+x] = color;
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
        const cb_cjk_glyph_t *glyph = NULL;
        for (unsigned i=0;i<CB_CJK_GLYPH_COUNT;++i)
            if(cb_cjk_glyphs[i].codepoint==codepoint){glyph=&cb_cjk_glyphs[i];break;}
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
static void center(canvas_t *c,int y,const char *s,int scale,uint16_t color){ztext(c,(CB_WIDTH-text_width(s,scale))/2,y,s,scale,color);}
static void cloud(canvas_t *c,int x,int y,int w,uint16_t shade){
    oval(c,x,y,w/2,8,shade);oval(c,x-w/4,y-5,w/4,10,shade);oval(c,x+w/6,y-8,w/4,13,shade);
}
static void background(canvas_t *c,const cb_game_t *g){
    static const int top[5][3]={{54,104,161},{41,119,192},{102,56,115},{18,28,70},{14,45,76}};
    static const int bottom[5][3]={{221,235,216},{196,240,234},{255,186,126},{87,80,134},{93,191,154}};
    int stage=g->stage-1;
    for(int yy=c->y;yy<c->y+c->rows;yy++){
        int r=top[stage][0]+(bottom[stage][0]-top[stage][0])*yy/240;
        int gr=top[stage][1]+(bottom[stage][1]-top[stage][1])*yy/240;
        int b=top[stage][2]+(bottom[stage][2]-top[stage][2])*yy/240;
        box(c,0,yy,320,1,RGB(r,gr,b));
    }
    if(g->stage>=4){
        for(int i=0;i<38;i++){
            int x=34+(i*73)%252,y=32+(i*43)%130;
            box(c,x,y,(i%7==0)?2:1,1,RGB(170+(i%4)*20,191,225));
        }
    }
    oval(c,254,65,g->stage>=4?15:25,g->stage>=4?15:25,RGB(255,223,173));
    if(g->stage==4)oval(c,260,59,14,14,RGB(27,36,79));
    if(g->stage==5){
        for(int x=0;x<320;x+=4){int wave=(x/12+g->scene_ms/150)%30;int yy=35+(wave<15?wave:30-wave);
            box(c,x,yy,4,7,RGB(74,194,164));box(c,x,yy+8,4,3,RGB(104,155,177));}
    }
    int drift=(int)((g->scene_ms/80)%400);
    for(int i=0;i<5;i++){
        int x=((i*97-drift+800)%400)-40;
        cloud(c,x,98+(i%3)*27,70,RGB(155,189,205));
    }
    for(int i=0;i<5;i++){
        int x=((i*91-drift/2+600)%400)-40;
        cloud(c,x,210+(i%2)*12,110,RGB(219,226,220));
    }
}
static void island(canvas_t *c,cb_platform_t p,int camera,int stage){
    int x=p.x-camera;
    for(int yy=0;yy<31;yy++){
        int inset=yy*p.w/70;
        box(c,x+inset,p.y+7+yy,p.w-inset*2,1,yy%7==0?RGB(100,111,129):RGB(66,85,107));
    }
    box(c,x,p.y,p.w,p.h/2,RGB(180,224,218));
    box(c,x+2,p.y+2,p.w-4,3,CREAM);
    box(c,x,p.y+8,p.w,3,stage>=4?RGB(113,126,185):RGB(77,178,153));
    for(int i=7;i<p.w;i+=13){box(c,x+i,p.y+14,3,4,RGB(127,159,172));}
}
static void traveler(canvas_t *c,int x,int feet,int crouch,int flying){
    int y=feet-25+crouch;
    oval(c,x,feet+3,10,2,RGB(54,81,113));
    box(c,x-6,y+10,12,11-crouch,RGB(32,55,83));
    box(c,x-5,y+11,10,7,TEAL);box(c,x-5,y+17,3,5,GOLD);
    box(c,x-5,y+2,10,9,RGB(255,220,181));
    box(c,x-7,y,14,5,RGB(37,58,83));box(c,x-8,y+4,16,2,RGB(37,58,83));
    box(c,x+3,y+6,2,2,INK);
    box(c,x-6,feet-4,5,4,INK);box(c,x+2,feet-4,5,4,INK);
    line(c,x-5,y+12,x-12,y+(flying?10:18),2,TEAL);
    line(c,x+5,y+12,x+11,y+(flying?8:18),2,TEAL);
    line(c,x-5,y+10,x-13-(flying?6:0),y+12,2,GOLD);
}
static void world(canvas_t *c,const cb_game_t *g){
    cb_platform_t current=g->current,target=g->target;int x=g->x,y=g->y;
    if(g->phase==CB_TITLE){current=(cb_platform_t){40,164,70,16};target=(cb_platform_t){174,148,62,16};x=75;y=164;}
    island(c,current,g->camera,g->stage);island(c,target,g->camera,g->stage);
    if(g->phase==CB_PLAY&&g->stage==1&&g->charging){
        int landing=70+cb_jump_distance(g->charge_ms),rise=g->current.y-g->target.y;
        for(int t=60;t<=600;t+=60){int xx=70+cb_jump_distance(g->charge_ms)*t/600;
            int yy=g->current.y-rise*t/600-4*58*t*(600-t)/(600*600);
            box(c,xx-g->camera,yy-4,2,2,CREAM);}
        int ok=cb_can_land(g,g->charge_ms);
        line(c,landing-4,g->target.y-8,landing,g->target.y-3,1,ok?TEAL:RED);
        line(c,landing,g->target.y-3,landing+4,g->target.y-8,1,ok?TEAL:RED);
    }
    if(g->flying){for(int i=1;i<=4;i++)box(c,x-g->camera-i*7,y-12+i*2,2,2,GOLD);}
    traveler(c,x-g->camera,y,g->charging?g->charge_ms/250:0,g->flying);
    if(g->feedback_ms>0&&g->combo>0){ztext(c,104,54,CB_PERFECT,1,GOLD);number(c,210,57,g->combo,1,GOLD);}
}
static void battery(canvas_t *c,const cb_game_t *g){
    box(c,261,12,22,10,INK);box(c,283,15,2,4,CREAM);
    box(c,263,14,18,6,RGB(56,77,94));
    if(g->battery>=0)box(c,263,14,18*min(g->battery,100)/100,6,TEAL);
    else line(c,268,17,275,17,1,CREAM);
}
static void hud(canvas_t *c,const cb_game_t *g){
    box(c,32,8,217,27,INK);ztext(c,39,14,CB_STAGE,1,CREAM);number(c,69,16,g->stage,1,GOLD);
    ztext(c,89,14,CB_PROGRESS,1,CREAM);number(c,121,16,g->landings,1,TEAL);text(c,135,16,"/12",1,CREAM);
    ztext(c,165,14,CB_SCORE,1,CREAM);number(c,194,16,g->score,1,GOLD);
    if(g->phase!=CB_PLAY)return;
    box(c,32,196,256,37,INK);
    if(g->charging){
        box(c,45,202,230,6,RGB(46,66,87));box(c,45,202,230*g->charge_ms/CB_CHARGE_MAX,6,g->charge_ms>=CB_CHARGE_MAX?GOLD:TEAL);
        center(c,214,g->charge_ms>=CB_CHARGE_MAX?CB_CAPPED:CB_CONTROL,1,CREAM);
    }else center(c,200,g->flying?CB_FLYING:g->scroll_ms?CB_SCROLL:CB_READY,1,CREAM);
    if(!g->charging)center(c,217,CB_PAUSE_HINT,1,RGB(160,186,199));
}
static void overlay(canvas_t *c,const cb_game_t *g){
    if(g->phase==CB_TITLE){
        center(c,37,CB_TITLE_TEXT,2,CREAM);center(c,69,CB_SUBTITLE,1,GOLD);
        center(c,91,cb_stage_names[g->stage-1],1,CREAM);
        box(c,38,177,244,58,INK);center(c,181,CB_START,1,TEAL);center(c,198,CB_CONTROL,1,CREAM);
        center(c,216,CB_SELECT,1,RGB(177,199,212));
        ztext(c,35,13,g->muted?CB_MUTED:CB_SOUND,1,CREAM);
    }else if(g->phase==CB_PAUSED||g->phase==CB_CLEAR||g->phase==CB_FAILED){
        box(c,39,58,242,133,TEAL);box(c,41,60,238,129,INK);
        const char *title=g->phase==CB_PAUSED?CB_PAUSED_TEXT:g->phase==CB_FAILED?CB_FAILED_TEXT:g->stage==5?CB_CLEAR_TEXT:CB_WON;
        center(c,72,title,2,CREAM);
        ztext(c,91,105,CB_SCORE,1,CREAM);number(c,147,107,g->score,2,GOLD);
        ztext(c,91,127,CB_BEST,1,CREAM);number(c,147,129,g->best,1,TEAL);
        center(c,151,g->phase==CB_PAUSED?CB_RESUME:g->phase==CB_CLEAR&&g->stage<5?CB_NEXT:CB_RETRY,1,CREAM);
        center(c,172,g->phase==CB_PAUSED?CB_EXIT:CB_HOME,1,RGB(172,196,210));
    }
}
void cb_render_strip(const cb_game_t *g,uint16_t *pixels,int y,int rows){
    if(!pixels||y<0||rows<=0||y+rows>CB_HEIGHT)return;
    canvas_t c={pixels,y,rows};background(&c,g);world(&c,g);
    if(g->phase!=CB_TITLE)hud(&c,g);
    overlay(&c,g);battery(&c,g);
    for(int yy=y;yy<y+rows;yy++){
        int dy=yy<30?30-yy:yy>=210?yy-209:0;
        if(!dy)continue;
        for(int xx=0;xx<30;xx++){int dx=30-xx;if(dx*dx+dy*dy>900){pixels[(yy-y)*320+xx]=0;pixels[(yy-y)*320+319-xx]=0;}}
    }
}
