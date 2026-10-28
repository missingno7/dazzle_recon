/* 066a:000e; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 269 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "066a:000e", "size": 269, "symbols": {"_x": "DS:5df0", "_y": "DS:5dee", "_dx": "DS:482c", "_dy": "DS:482e", "_sx": "DS:4830", "_sy": "DS:4832", "_error": "DS:4834"}} */
extern int x,y,dx,dy,sx,sy,error;
#include <stdlib.h>
int line_step(int endx,int endy,int initialize) {
    if(initialize) {
        dx=endx-x; dy=endy-y;
        sx=dx>0?1:dx==0?0:-1;
        sy=dy>0?1:dy==0?0:-1;
        dx=abs(dx)<<1; dy=abs(dy)<<1;
        if(dx>=dy) error=dy-(dx>>1); else error=dx-(dy>>1);
        if(dx==0 && dy==0) return 1; else return 0;
    } else {
        if(dx>=dy) {
            if(error>=0) {error-=dx; y+=sy;}
            x+=sx; error+=dy;
            if(x==endx) return 1; else return 0;
        } else {
            if(error>=0) {error-=dy; x+=sx;}
            y+=sy; error+=dx;
            if(y==endy) return 1;
        }
    }
    return 0;
}
