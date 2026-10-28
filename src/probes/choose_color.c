/* 066a:011b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 336 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "066a:011b", "size": 336, "symbols": {"_x": "DS:5df0", "_y": "DS:5dee", "_mode": "DS:7484", "_phase": "DS:5daa", "_colors": "DS:5db6", "_color": "DS:7488", "_rand": "0000:1694", "F_LXMUL@": "0000:11e3", "F_LDIV@": "0000:12e7", "F_LXURSH@": "0000:13d6"}} */
#include <stdlib.h>
extern int x,y,mode,phase;
extern unsigned colors,color;
void choose_color(int kind) {
    unsigned long a,b;
    switch(kind) {
    default: if(mode!=4) color++; else {phase++; if((phase&=3)==0) color++;} break;
    case 1: color=random(colors); break;
    case 2: color=abs(x); break;
    case 3: color=abs(y); break;
    case 4: color=abs(x)+abs(y); break;
    case 5: a=x*x; b=y*y; a+=b; color=(a>>5)&(colors-1); break;
    case 6: a=x*x; b=y*y; a-=b; color=(a>>5)&(colors-1); break;
    case 7: b=(y*y)>>5; color=b-abs(x); break;
    case 8: a=abs(x*y)>>5; color=a; break;
    }
    color&=colors-1;
    if(color<3) color=3;
}
