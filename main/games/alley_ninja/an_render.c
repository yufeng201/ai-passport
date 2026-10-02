#include "an_game.h"
#include "../common/game_visual.h"
#include "../common/game_shapes.h"
#include "an_copy.h"
#include "alley_ninja_noto_sc_12.h"
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
    int left = max(x, 0), right = min(x + w, AN_WIDTH);
    int top = max(y, c->y), bottom = min(y + h, c->y + c->rows);
    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx) c->pixels[(yy - c->y) * AN_WIDTH + xx] = color;
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
        const an_cjk_glyph_t *glyph=NULL;
        unsigned lo=0,hi=AN_CJK_GLYPH_COUNT;
        while(lo<hi){unsigned mid=lo+(hi-lo)/2;const an_cjk_glyph_t *candidate=&an_cjk_glyphs[mid];
            if(candidate->scale<scale||(candidate->scale==scale&&candidate->codepoint<cp))lo=mid+1;else hi=mid;
        }
        if(lo<AN_CJK_GLYPH_COUNT&&an_cjk_glyphs[lo].scale==scale&&an_cjk_glyphs[lo].codepoint==cp)glyph=&an_cjk_glyphs[lo];
        if(!glyph){box(c,x,y,10*scale,scale,RED);x+=12*scale;continue;}
        for(int yy=max(y,c->y);yy<min(y+glyph->height,c->y+c->rows);yy++){
            for(int xx=max(x,0);xx<min(x+glyph->width,AN_WIDTH);xx++){
                unsigned index=(unsigned)((yy-y)*glyph->width+xx-x);
                unsigned packed=an_font_alpha[glyph->offset+index/2];
                int alpha=(index&1)?(packed&15):(packed>>4);if(!alpha)continue;
                uint16_t *pixel=&c->pixels[(yy-c->y)*AN_WIDTH+xx];uint16_t bg=*pixel;
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
        for(unsigned i=0;i<AN_CJK_GLYPH_COUNT;i++)if(an_cjk_glyphs[i].codepoint==cp&&an_cjk_glyphs[i].scale==scale){width+=an_cjk_glyphs[i].advance;break;}
    }return width;
}
static void center(canvas_t *c,int y,const char *s,int scale,uint16_t color){ztext(c,(AN_WIDTH-text_width(s,scale))/2,y,s,scale,color);}
static void background(canvas_t *c,const an_game_t *g){
    static const int tops[5][3]={{10,10,31},{21,9,32},{23,15,30},{16,9,40},{29,19,44}};
    static const int lows[5][3]={{18,28,54},{38,24,59},{40,34,48},{30,24,64},{71,48,60}};
    int stage=g->stage-1;
    for(int yy=c->y;yy<c->y+c->rows;yy++){
        int r=tops[stage][0]+(lows[stage][0]-tops[stage][0])*yy/240;
        int gr=tops[stage][1]+(lows[stage][1]-tops[stage][1])*yy/240;
        int b=tops[stage][2]+(lows[stage][2]-tops[stage][2])*yy/240;
        box(c,0,yy,320,1,RGB(r,gr,b));
    }
    oval(c,241,51,18,18,RGB(150,173,198));oval(c,248,44,18,18,RGB(12,12,35));
    for(int i=0;i<10;i++){
        int x=i*36-7,h=48+(i*29)%43;
        box(c,x,152-h,29,h,RGB(21+i%3*3,24,48));
        for(int yy=156-h;yy<148;yy+=13)for(int xx=x+4;xx<x+26;xx+=10)box(c,xx,yy,3,5,RGB(47,74,100));
    }
    box(c,0,140,64,57,RGB(30,34,61));box(c,256,139,64,58,RGB(28,32,57));
    box(c,12,141,40,3,RGB(174,57,139));box(c,267,142,40,3,TEAL);
    box(c,22,149,18,27,RGB(54,40,82));box(c,26,154,10,3,GOLD);box(c,26,161,10,3,RGB(209,78,156));
    box(c,279,149,16,25,RGB(29,64,80));box(c,284,153,4,17,TEAL);
    box(c,0,194,320,20,RGB(26,34,53));line(c,0,196,319,196,1,RGB(88,98,115));
    for(int x=0;x<320;x+=26){line(c,x,204,x+18,204,1,RGB(36,57,74));line(c,x+7,209,x+22,209,1,RGB(84,51,86));}
    for(int i=0;i<17;i++){int x=(i*47+g->scene_ms/70)%320,y=45+(i*29+g->scene_ms/30)%145;line(c,x,y,x-2,y+5,1,RGB(35,61,85));}
}
static void fighter(canvas_t *c,int x,int side,int enemy,int kind,int crouch){
    int y=160+crouch;
    oval(c,x,194,12,3,RGB(13,21,36));
    oval(c,x,y+20,7,9-crouch/2,enemy?RGB(94,52,98):RGB(35,76,103));
    oval(c,x,y+7,7,7,enemy?RGB(102,67,111):RGB(53,91,123));
    box(c,x-7,y+2,14,4,enemy?RED:TEAL);box(c,x+side*3,y+7,3,2,CREAM);
    line(c,x-3,184,x-5,192,4,INK);line(c,x+3,184,x+5,192,4,INK);
    line(c,x+side*5,y+15,x+side*13,y+18,3,enemy?RGB(117,63,111):TEAL);
    line(c,x+side*13,y+18,x+side*20,y+6,1,CREAM);
    line(c,x-side*6,y+5,x-side*17,y+8,2,enemy?RED:TEAL);
    if(kind){box(c,x-5,y+13,10,8,kind==2?GOLD:RGB(166,117,140));box(c,x-8,y-3,16,3,kind==2?GOLD:RGB(174,91,118));}
}
static void world(canvas_t *c,const an_game_t *g){
    const an_enemy_t *e=&g->enemy;
    int side=g->slash_side?g->slash_side:1;
    fighter(c,160,side,0,0,g->guarding?3:0);
    if(g->phase==AN_TITLE){fighter(c,92,1,1,1,0);fighter(c,228,-1,1,0,0);}
    else if(e->active){
        fighter(c,e->x,-e->side,1,e->kind,e->phase==AN_WINDUP?3:0);
        if(e->max_hp>1){box(c,e->x-14,148,28,3,RGB(57,40,63));box(c,e->x-14,148,28*e->hp/e->max_hp,3,GOLD);}
        if(e->phase==AN_WINDUP){
            int remaining=e->warning_ms?e->timer_ms*26/e->warning_ms:0;
            box(c,e->x-13,133,26,4,RGB(61,38,67));box(c,e->x-13,133,remaining,4,RED);
            box(c,e->x-1,120,3,7,CREAM);box(c,e->x-1,129,3,2,RED);
            if(g->phase==AN_PLAY)center(c,82,AN_WARNING,1,RED);
        }else if(e->vulnerable_ms>0&&g->phase==AN_PLAY){center(c,82,AN_COUNTER,1,TEAL);}
    }
    if(g->guarding){
        line(c,139,154,133,178,2,TEAL);line(c,133,178,143,194,2,TEAL);
        line(c,181,154,187,178,2,TEAL);line(c,187,178,177,194,2,TEAL);
    }
    if(g->slash_ms>0){
        int s=g->slash_side;line(c,160+s*10,155,160+s*43,163,3,CREAM);
        line(c,160+s*43,163,160+s*AN_RANGE,182,2,TEAL);line(c,160+s*AN_RANGE,182,160+s*26,190,2,CREAM);
    }
    if(g->damage_ms>0&&(g->scene_ms/100)%2==0)box(c,153,160,14,4,RED);
}
static void hud(canvas_t *c,const an_game_t *g){
    panel(c,32,8,216,25,INK);number(c,40,15,g->stage,1,GOLD);
    ztext(c,64,13,AN_PROGRESS,1,CREAM);number(c,94,15,g->kills,1,TEAL);text(c,106,15,"/",1,CREAM);number(c,113,15,g->goal,1,CREAM);
    ztext(c,142,13,AN_SCORE,1,CREAM);number(c,173,15,g->score,1,GOLD);
    for(int i=0;i<3;i++)panel(c,260+i*9,13,6,9,i<g->health?TEAL:RGB(54,64,86));
    if(g->phase!=AN_PLAY)return;
    panel(c,75,43,170,19,INK);ztext(c,81,46,AN_STAMINA,1,CREAM);
    panel(c,116,50,118,5,RGB(45,54,78));panel(c,116,50,118*g->stamina/100,5,g->stamina>=24?TEAL:RED);
    if(g->riposte_ms>0){center(c,65,AN_RIPOSTE,1,GOLD);number(c,244,68,g->combo,1,GOLD);}
    if(g->feedback_ms>0)center(c,102,an_feedback[g->feedback],1,g->feedback==1?GOLD:g->feedback>=6?RED:CREAM);
    panel(c,32,215,256,21,INK);center(c,218,g->guarding?AN_GUARD:g->broken_ms>0?AN_BROKEN:AN_READY,1,CREAM);
}
static void overlay(canvas_t *c,const an_game_t *g){
    if(g->phase==AN_TITLE){
        center(c,37,AN_TITLE_TEXT,2,CREAM);center(c,72,AN_SUBTITLE,1,TEAL);
        panel(c,38,93,244,43,INK);center(c,100,an_stage_names[g->stage-1],1,GOLD);center(c,119,AN_START,1,TEAL);
        panel(c,32,211,256,26,INK);center(c,213,AN_CONTROL,1,CREAM);
        center(c,139,AN_SELECT,1,RGB(152,176,203));ztext(c,34,14,g->muted?AN_MUTED:AN_SOUND,1,CREAM);
    }else if(g->phase==AN_PAUSED||g->phase==AN_CLEAR||g->phase==AN_FAILED){
        panel(c,39,55,242,137,TEAL);panel(c,41,57,238,133,INK);
        center(c,69,g->phase==AN_PAUSED?AN_PAUSED_TEXT:g->phase==AN_FAILED?AN_FAILED_TEXT:g->stage==5?AN_CLEAR_TEXT:AN_WON,2,CREAM);
        ztext(c,92,106,AN_SCORE,1,CREAM);number(c,148,108,g->score,2,GOLD);
        ztext(c,92,128,AN_BEST,1,CREAM);number(c,148,130,g->best,1,TEAL);
        center(c,151,g->phase==AN_PAUSED?AN_RESUME:g->phase==AN_CLEAR&&g->stage<5?AN_NEXT:AN_RETRY,1,CREAM);
        center(c,173,g->phase==AN_PAUSED?AN_EXIT:AN_HOME,1,RGB(151,180,207));
    }
}
void an_render_strip(const an_game_t *g,uint16_t *pixels,int y,int rows){
    if(!pixels||y<0||rows<=0||y>AN_HEIGHT||rows>AN_HEIGHT-y)return;
    canvas_t c={pixels,y,rows};background(&c,g);world(&c,g);if(g->phase!=AN_TITLE)hud(&c,g);overlay(&c,g);
    if(g->phase==AN_CLEAR)center(&c,201,AN_MEDAL_GOAL,1,CREAM);
    if(g->phase==AN_CLEAR)game_visual_medals(pixels,y,rows,143,1+(g->health==3)+(g->combo>=3),GOLD,RGB(70,83,103));
    if(g->phase==AN_TITLE)game_visual_medals(pixels,y,rows,156,game_medal_count(g->medals,g->stage),GOLD,RGB(70,83,103));
    if(g->phase==AN_TITLE)for(int i=0;i<5;i++){box(&c,121+i*17,204,11,3,i+1==g->stage?GOLD:i<g->unlocked?TEAL:RGB(65,77,99));}
    if(g->phase==AN_PLAY&&g->feedback_ms>0&&g->feedback==1)game_visual_burst(pixels,y,rows,160,167,650-g->feedback_ms,GOLD);
    for(int yy=y;yy<y+rows;yy++){
        int dy=yy<30?30-yy:yy>=210?yy-209:0;if(!dy)continue;
        for(int xx=0;xx<30;xx++){int dx=30-xx;if(dx*dx+dy*dy>900){pixels[(yy-y)*320+xx]=0;pixels[(yy-y)*320+319-xx]=0;}}
    }
}
