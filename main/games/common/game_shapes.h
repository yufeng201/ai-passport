#pragma once
#include <stdint.h>
/* Analytic shapes with four coverage samples, clipped to the current strip.
 * All geometry is integer; no supersampled framebuffer or runtime allocation. */
static inline int game_shape_min(int a,int b){return a<b?a:b;}
static inline int game_shape_max(int a,int b){return a>b?a:b;}
static inline void game_shape_blend(uint16_t *p,int alpha,uint16_t color)
{
    if(!alpha)return;
    uint16_t bg=*p;
    int r=(((color>>11)&31)*alpha+((bg>>11)&31)*(4-alpha)+2)/4;
    int g=(((color>>5)&63)*alpha+((bg>>5)&63)*(4-alpha)+2)/4;
    int b=((color&31)*alpha+(bg&31)*(4-alpha)+2)/4;
    *p=(uint16_t)((r<<11)|(g<<5)|b);
}
static inline void game_shape_ellipse(uint16_t *p,int y,int rows,int cx,int cy,int rx,int ry,uint16_t color)
{
    if(rx<=0||ry<=0)return;
    int64_t rx2=(int64_t)rx*rx*16,ry2=(int64_t)ry*ry*16;
    for(int yy=game_shape_max(y,cy-ry-1);yy<game_shape_min(y+rows,cy+ry+1);yy++)
        for(int xx=game_shape_max(0,cx-rx-1);xx<game_shape_min(320,cx+rx+1);xx++){
            int coverage=0;
            for(int sy=1;sy<=3;sy+=2)for(int sx=1;sx<=3;sx+=2){
                int64_t dx=4*(xx-cx)+sx,dy=4*(yy-cy)+sy;
                coverage+=dx*dx*ry2+dy*dy*rx2<=rx2*ry2;
            }
            game_shape_blend(&p[(yy-y)*320+xx],coverage,color);
        }
}
static inline void game_shape_stroke(uint16_t *p,int y,int rows,int ax,int ay,int bx,int by,int width,uint16_t color)
{
    int dx=4*(bx-ax),dy=4*(by-ay),radius=2*width;
    int64_t len=(int64_t)dx*dx+(int64_t)dy*dy;
    int margin=width+2;
    for(int yy=game_shape_max(y,game_shape_min(ay,by)-margin);yy<game_shape_min(y+rows,game_shape_max(ay,by)+margin+1);yy++){
        int left=game_shape_min(ax,bx)-margin,right=game_shape_max(ax,bx)+margin;
        if(dy){
            int center=ax+(yy-ay)*dx/dy;
            int span=margin+(width+2)*(dx<0?-dx:dx)/(dy<0?-dy:dy);
            left=game_shape_max(left,center-span);right=game_shape_min(right,center+span);
        }
        for(int xx=game_shape_max(0,left);xx<=game_shape_min(319,right);xx++){
            int coverage=0;
            for(int sy=1;sy<=3;sy+=2)for(int sx=1;sx<=3;sx+=2){
                int64_t px=4*(xx-ax)+sx,py=4*(yy-ay)+sy;
                int64_t dot=px*dx+py*dy;
                if(!len||dot<=0)coverage+=px*px+py*py<=(int64_t)radius*radius;
                else if(dot>=len){px-=dx;py-=dy;coverage+=px*px+py*py<=(int64_t)radius*radius;}
                else{int64_t cross=px*dy-py*dx;coverage+=cross*cross<=(int64_t)radius*radius*len;}
            }
            game_shape_blend(&p[(yy-y)*320+xx],coverage,color);
        }
    }
}
static inline void game_shape_panel(uint16_t *p,int y,int rows,int x,int top,int w,int h,int radius,uint16_t color)
{
    if(w<=0||h<=0)return;
    radius=game_shape_min(radius,game_shape_min(w/2,h/2));
    for(int yy=game_shape_max(y,top);yy<game_shape_min(y+rows,top+h);yy++)
        for(int xx=game_shape_max(0,x);xx<game_shape_min(320,x+w);xx++){
            int coverage=0;
            for(int sy=1;sy<=3;sy+=2)for(int sx=1;sx<=3;sx+=2){
                int px=4*(xx-x)+sx,py=4*(yy-top)+sy,r=4*radius;
                int dx=game_shape_max(r-px,game_shape_max(px-4*w+r,0));
                int dy=game_shape_max(r-py,game_shape_max(py-4*h+r,0));
                coverage+=dx*dx+dy*dy<=r*r;
            }
            game_shape_blend(&p[(yy-y)*320+xx],coverage,color);
        }
}
