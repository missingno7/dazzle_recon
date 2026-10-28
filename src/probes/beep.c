/* 11c1:0e07; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 35 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "11c1:0e07", "size": 35, "symbols": {"_muted": "DS:5dbe", "_sound": "0000:3d88", "_delay": "0000:29a6", "_nosound": "0000:3db4"}} */
extern int muted;
void sound(unsigned); void delay(unsigned); void nosound(void);
void beep(unsigned hz,unsigned ms) {
    if(!muted) {sound(hz);delay(ms);nosound();}
}
