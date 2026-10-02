#include "ll_game.h"
#include "../common/game_shapes.h"
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
    int left = max(x, 0), right = min(x + w, 320);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * 320 + xx] = color;
}

static void panel(canvas_t *c,int x,int y,int w,int h,uint16_t color)
{
    game_shape_panel(c->pixels,c->y,c->rows,x,y,w,h,6,color);
}

/* Smooth vector strokes and ellipses, clipped to the strip. */
static void line(canvas_t *c,int x,int y,int tx,int ty,int width,uint16_t color)
{
    game_shape_stroke(c->pixels,c->y,c->rows,x,y,tx,ty,width,color);
}
static void oval(canvas_t *c,int cx,int cy,int rx,int ry,uint16_t color)
{
    game_shape_ellipse(c->pixels,c->y,c->rows,cx,cy,rx,ry,color);
}

/* Native-size 4bpp Noto Sans glyphs: no nearest-neighbor font scaling. */
static void ztext(canvas_t *c,int x,int y,const char *s,int scale,uint16_t color)
{
    if(scale<1||scale>3||y+42<=c->y||y>=c->y+c->rows)return;
    while(*s){
        const unsigned char *p=(const unsigned char *)s;uint16_t cp;
        if(*p<128){cp=*p;s++;}
        else if((p[0]&0xf0)==0xe0&&p[1]&&p[2]){cp=(uint16_t)(((p[0]&15)<<12)|((p[1]&63)<<6)|(p[2]&63));s+=3;}
        else{s++;continue;}
        const cb_cjk_glyph_t *glyph=NULL;
        unsigned lo=0,hi=CB_CJK_GLYPH_COUNT;
        while(lo<hi){unsigned mid=lo+(hi-lo)/2;const cb_cjk_glyph_t *candidate=&cb_cjk_glyphs[mid];
            if(candidate->scale<scale||(candidate->scale==scale&&candidate->codepoint<cp))lo=mid+1;else hi=mid;
        }
        if(lo<CB_CJK_GLYPH_COUNT&&cb_cjk_glyphs[lo].scale==scale&&cb_cjk_glyphs[lo].codepoint==cp)glyph=&cb_cjk_glyphs[lo];
        if(!glyph){box(c,x,y,10*scale,scale,RED);x+=12*scale;continue;}
        for(int yy=max(y,c->y);yy<min(y+glyph->height,c->y+c->rows);yy++){
            for(int xx=max(x,0);xx<min(x+glyph->width,320);xx++){
                unsigned index=(unsigned)((yy-y)*glyph->width+xx-x);
                unsigned packed=cb_font_alpha[glyph->offset+index/2];
                int alpha=(index&1)?(packed&15):(packed>>4);if(!alpha)continue;
                uint16_t *pixel=&c->pixels[(yy-c->y)*320+xx];uint16_t bg=*pixel;
                int r=(((color>>11)&31)*alpha+((bg>>11)&31)*(15-alpha)+7)/15;
                int g=(((color>>5)&63)*alpha+((bg>>5)&63)*(15-alpha)+7)/15;
                int b=((color&31)*alpha+(bg&31)*(15-alpha)+7)/15;
                *pixel=(uint16_t)((r<<11)|(g<<5)|b);
            }
        }
        x+=glyph->advance;
    }
}
static void text(canvas_t *c,int x,int y,const char *s,int scale,uint16_t color)
{
    ztext(c,x,y,s,scale,color);
}

/* Decimal formatting without libc, bounded to six digits. */
static void number(canvas_t *c, int x, int y, int value, int scale, uint16_t color)
{
    char out[8]; int n = 0;
    do { out[n++] = '0' + value % 10; value /= 10; } while (value && n < 6);
    for (int i = 0; i < n/2; ++i) { char t = out[i]; out[i] = out[n-1-i]; out[n-1-i] = t; }
    out[n] = 0; text(c,x,y,out,scale,color);
}


void ll_render_strip(const ll_game_t *g,uint16_t *pixels,int y,int rows){
 if(!pixels||y<0||rows<=0||y+rows>240)return;
 canvas_t c={pixels,y,rows};
 for(int yy=y;yy<y+rows;yy++)box(&c,0,yy,320,1,RGB(24+yy/12,19,35));
 for(int side=0;side<2;side++)for(int j=0;j<8;j++){
  int yy=(j*38+g->height%38)-25;
  panel(&c,side?290:0,yy,30,34,RGB(61,46,61));line(&c,side?305:15,yy+6,side?298:22,yy+24,2,RGB(104,66,61));
 }
 for(int i=0;i<3;i++){int yy=ll_gate_y(g,i),gap=g->gates[i].gap;
  panel(&c,24,yy,gap-58-24,14,RGB(105,88,108));panel(&c,gap+58,yy,296-gap-58,14,RGB(105,88,108));
  line(&c,gap-58,yy+2,gap-58,yy+12,3,GOLD);line(&c,gap+58,yy+2,gap+58,yy+12,3,GOLD);
 }
 int platform=ll_platform_y(g),lava=platform+35;
 for(int yy=max(y,lava);yy<min(y+rows,240);yy++)box(&c,26,yy,268,1,RGB(255,80+(yy-lava)/2,25));
 for(int x=32;x<290;x+=20)oval(&c,x,lava+((x+(int)(g->elapsed_ms/90))%9),12,5,GOLD);
 panel(&c,g->x-32,platform,64,13,TEAL);box(&c,g->x-26,platform+4,52,3,RGB(34,117,117));
 line(&c,g->x-18,platform+14,g->x-10,platform+24,2,GOLD);line(&c,g->x+18,platform+14,g->x+10,platform+24,2,GOLD);
 int feet=platform-ll_jump_height(g);
 if(!g->invincible_ms||(g->invincible_ms/100)%2){
  oval(&c,g->x,feet-19,7,7,CREAM);panel(&c,g->x-7,feet-13,14,10,TEAL);
  box(&c,g->x-8,feet-4,6,4,CREAM);box(&c,g->x+2,feet-4,6,4,CREAM);box(&c,g->x+2,feet-22,3,3,INK);
 }
 if(g->monster_active){int x=g->monster_x;
  oval(&c,x,platform-8,11,8,RED);box(&c,x-6,platform-12,4,3,CREAM);box(&c,x+3,platform-12,4,3,CREAM);
  line(&c,x-8,platform-14,x-12,platform-21,2,GOLD);line(&c,x+8,platform-14,x+12,platform-21,2,GOLD);
 }
 panel(&c,32,8,256,26,INK);number(&c,40,14,g->height,1,GOLD);text(&c,76,14,"/1200",1,CREAM);
 for(int i=0;i<3;i++)oval(&c,160+i*16,21,5,5,i<g->hp?RED:RGB(66,52,63));
 box(&c,258,15,23,10,CREAM);box(&c,260,17,19,6,INK);box(&c,281,18,2,4,CREAM);
 if(g->battery>=0)box(&c,260,17,19*min(g->battery,100)/100,6,TEAL);else box(&c,267,19,5,2,CREAM);
 if(g->phase!=LL_PLAY){
  panel(&c,40,48,240,107,INK);
  text(&c,66,58,g->phase==LL_TITLE?"LAVA LIFT":g->phase==LL_PAUSED?"PAUSED":g->phase==LL_WON?"ESCAPED!":"TRY AGAIN",2,GOLD);
  text(&c,64,96,"A LEFT   B RIGHT",1,CREAM);text(&c,64,114,"C JUMP / START",1,TEAL);
  text(&c,64,134,"BEST",1,CREAM);number(&c,106,134,g->best,1,GOLD);
 }
}
