/* 11c1:0501; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 46 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "11c1:0501", "size": 46, "symbols": {"_mouse_regs": "DS:743e", "_intr": "0000:32d8"}} */
#include <dos.h>
extern struct REGPACK mouse_regs;
void mouse_position(int buttons,int x,int y) {
    mouse_regs.r_cx=buttons;
    mouse_regs.r_es=y;
    mouse_regs.r_dx=x;
    mouse_regs.r_ax=12;
    intr(0x33,&mouse_regs);
}
