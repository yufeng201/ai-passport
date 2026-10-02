#include "bw_game.h"
#include "../common/game_visual.h"
#include "../common/game_shapes.h"
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
        const bw_cjk_glyph_t *glyph=NULL;
        unsigned lo=0,hi=BW_CJK_GLYPH_COUNT;
        while(lo<hi){unsigned mid=lo+(hi-lo)/2;const bw_cjk_glyph_t *candidate=&bw_cjk_glyphs[mid];
            if(candidate->scale<scale||(candidate->scale==scale&&candidate->codepoint<cp))lo=mid+1;else hi=mid;
        }
        if(lo<BW_CJK_GLYPH_COUNT&&bw_cjk_glyphs[lo].scale==scale&&bw_cjk_glyphs[lo].codepoint==cp)glyph=&bw_cjk_glyphs[lo];
        if(!glyph){box(c,x,y,10*scale,scale,RED);x+=12*scale;continue;}
        for(int yy=max(y,c->y);yy<min(y+glyph->height,c->y+c->rows);yy++){
            for(int xx=max(x,0);xx<min(x+glyph->width,BW_WIDTH);xx++){
                unsigned index=(unsigned)((yy-y)*glyph->width+xx-x);
                unsigned packed=bw_font_alpha[glyph->offset+index/2];
                int alpha=(index&1)?(packed&15):(packed>>4);if(!alpha)continue;
                uint16_t *pixel=&c->pixels[(yy-c->y)*BW_WIDTH+xx];uint16_t bg=*pixel;
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

static int text_width(const char *s,int scale){
    int width=0;
    while(*s){const unsigned char *p=(const unsigned char *)s;uint16_t cp;
        if(*p<128){cp=*p;s++;}
        else if((p[0]&0xf0)==0xe0&&p[1]&&p[2]){cp=(uint16_t)(((p[0]&15)<<12)|((p[1]&63)<<6)|(p[2]&63));s+=3;}
        else{s++;continue;}
        for(unsigned i=0;i<BW_CJK_GLYPH_COUNT;i++)if(bw_cjk_glyphs[i].codepoint==cp&&bw_cjk_glyphs[i].scale==scale){width+=bw_cjk_glyphs[i].advance;break;}
    }return width;
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
    game_shape_panel(c->pixels,c->y,c->rows,x,y,36,10,3,RGB(42,49,76));game_shape_panel(c->pixels,c->y,c->rows,x+1,y+1,34,7,2,flash?CREAM:shade);
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
    game_shape_panel(c->pixels,c->y,c->rows,px-BW_PADDLE_W/2,BW_PADDLE_Y,BW_PADDLE_W,7,3,RGB(90,113,146));
    box(c,px-BW_PADDLE_W/2+2,BW_PADDLE_Y+1,BW_PADDLE_W-4,2,TEAL);
    for(int i=-20;i<=20;i+=10)box(c,px+i,BW_PADDLE_Y+5,4,2,GOLD);
    int bx=g->ball_x/BW_Q,by=g->ball_y/BW_Q;
    if(g->phase==BW_TITLE){bx=159;by=158;}
    if(!g->ready&&g->phase!=BW_TITLE){
        for(int i=1;i<=3;i++)oval(c,bx-i*g->vx/BW_Q/2,by-i*g->vy/BW_Q/2,2,2,RGB(81,142,169));
    }
    oval(c,bx,by,3,3,CREAM);box(c,bx-1,by-1,2,2,TEAL);
    if(g->flash_ms>0){
        int r=2+(120-g->flash_ms)/20;
        line(c,g->flash_x-r,g->flash_y,g->flash_x+r,g->flash_y,1,GOLD);
    }
}
static void hud(canvas_t *c,const bw_game_t *g){
    panel(c,32,8,216,25,INK);number(c,40,15,g->stage,1,GOLD);
    ztext(c,64,13,BW_PROGRESS,1,CREAM);number(c,94,15,g->remaining,1,TEAL);
    ztext(c,142,13,BW_SCORE,1,CREAM);number(c,173,15,g->score,1,GOLD);
    for(int i=0;i<3;i++)panel(c,260+i*9,13,6,9,i<g->lives?TEAL:RGB(54,64,86));
    if(g->phase!=BW_PLAY)return;
    ztext(c,20,183,BW_USES,1,RGB(146,177,192));number(c,50,185,g->slow_uses,1,GOLD);
    if(g->supply_ms>0)center(c,150,BW_SUPPLY,1,TEAL);
    else if(g->assist_ms>0)center(c,150,BW_ASSIST,1,GOLD);
    panel(c,245,185,48,3,INK);panel(c,245,185,48*(g->destroyed%6)/6,3,GOLD);
    panel(c,32,217,256,20,INK);
    center(c,219,g->ready?BW_READY:g->slow_ms>0?BW_SLOW:g->slow_uses>0?BW_ACTION:BW_EMPTY,1,CREAM);
    if(g->slow_ms>0)panel(c,74,236,172*g->slow_ms/3000,1,TEAL);
}
static void overlay(canvas_t *c,const bw_game_t *g){
    if(g->phase==BW_TITLE){
        panel(c,35,44,250,47,INK);center(c,47,BW_TITLE_TEXT,2,CREAM);center(c,78,BW_SUBTITLE,1,TEAL);
        panel(c,41,111,238,80,INK);center(c,115,bw_stage_names[g->stage-1],1,GOLD);
        center(c,136,BW_START,1,TEAL);center(c,157,BW_CONTROL,1,CREAM);center(c,177,BW_SELECT,1,RGB(152,176,203));
        ztext(c,34,14,g->muted?BW_MUTED:BW_SOUND,1,CREAM);
    }else if(g->phase==BW_PAUSED||g->phase==BW_CLEAR||g->phase==BW_FAILED){
        panel(c,39,55,242,137,TEAL);panel(c,41,57,238,133,INK);
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
