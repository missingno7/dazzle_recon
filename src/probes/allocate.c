/* 11c1:062b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 65 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "11c1:062b", "size": 65, "symbols": {"_malloc": "0000:2c51"}} */
#include <stdlib.h>
void *allocate(unsigned *amount,unsigned minimum) {
    unsigned n;
    void *p;
    n=*amount;
    while(n>=minimum && (p=malloc(n))==0) n/=2;
 *amount=n;
    return p;
}
