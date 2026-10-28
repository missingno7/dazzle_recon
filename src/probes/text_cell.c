/* 158d:0219; EXACT; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 64 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "158d:0219", "size": 64, "symbols": {}} */
#include <dos.h>
void text_cell(unsigned x,unsigned y,char c,char attr) {
    union {unsigned word; char byte[2];} cell;
    if(y<25 && x<80) {
        cell.byte[1]=attr;
        cell.byte[0]=c;
  *(unsigned far *)MK_FP(0xb800,y*160+x*2)=cell.word;
    }
}
