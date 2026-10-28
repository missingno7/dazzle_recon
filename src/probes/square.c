/* 066a:026b; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 80 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "066a:026b", "size": 80, "symbols": {"_x": "DS:5df0", "_y": "DS:5dee", "_plot": "DS:5c3f", "_tick": "DS:5d7a", "_limit": "DS:5da6", "_service": "11c1:2515"}} */
extern int x,y;
extern unsigned tick,limit;
extern void (*plot)(int);
extern void service(void);
void square(int n,int color) {
    int i,j;
    for(i=1;i<=n;i++) {
        for(j=1;j<=n;j++) { plot(color); y++; }
        x++; y-=n;
        if(tick>=limit) service();
    }
    x-=n;
}
