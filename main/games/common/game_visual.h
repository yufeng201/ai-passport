#pragma once
#include <stdint.h>
/* Strip-local decorations: immutable geometry, no framebuffer or particle heap. */
static inline void game_visual_dot(uint16_t *p,int y,int rows,int x,int yy,uint16_t color)
{
    if(x>=0&&x<320&&yy>=y&&yy<y+rows)p[(yy-y)*320+x]=color;
}
static inline void game_visual_burst(uint16_t *p,int y,int rows,int cx,int cy,int age,uint16_t color)
{
    if(age<0||age>600)return;
    static const int8_t dx[8]={-3,-2,0,2,3,2,0,-2},dy[8]={0,-2,-3,-2,0,2,3,2};
    int r=2+age/40;
    for(int i=0;i<8;i++)for(int n=0;n<2;n++)
        game_visual_dot(p,y,rows,cx+dx[i]*r/3+n,cy+dy[i]*r/3+age*age/30000,color);
}
static inline void game_visual_medals(uint16_t *p,int y,int rows,int yy,int count,uint16_t gold,uint16_t dim)
{
    static const uint8_t star[7]={8,8,62,28,62,20,34};
    for(int i=0;i<3;i++)for(int r=0;r<7;r++)for(int x=0;x<6;x++)if(star[r]&(1<<x))
        game_visual_dot(p,y,rows,139+i*18+x,yy+r,i<count?gold:dim);
}
