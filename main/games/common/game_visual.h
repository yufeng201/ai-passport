#pragma once
#include <stdint.h>
#include "game_shapes.h"
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
    for(int i=0;i<8;i++)
        game_shape_ellipse(p,y,rows,cx+dx[i]*r/3,cy+dy[i]*r/3+age*age/30000,1,1,color);
}
static inline void game_visual_medals(uint16_t *p,int y,int rows,int yy,int count,uint16_t gold,uint16_t dim)
{
    for(int i=0;i<3;i++){
        int cx=142+i*18;
        game_shape_ellipse(p,y,rows,cx,yy+3,4,4,i<count?gold:dim);
        game_shape_stroke(p,y,rows,cx-2,yy+3,cx,yy+5,1,i<count?dim:gold);
        game_shape_stroke(p,y,rows,cx,yy+5,cx+3,yy+1,1,i<count?dim:gold);
    }
}
