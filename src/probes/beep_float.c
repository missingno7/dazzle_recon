/* 11c1:1fbf; RELOCATED; profile dazzle (BC++ 3.0 -ml -O1).
 * Complete extent: 48 bytes. Bindings recovered from the original image.
 */
/* objcmp: {"address": "11c1:1fbf", "size": 48, "symbols": {"_sound": "0000:3d88", "_delay": "0000:29a6", "_nosound": "0000:3db4", "F_FTOL@": "0000:0fbe"}, "segments": {"_DATA": "DS:201c"}, "absolute_symbols": {"FIDRQQ": "0x5c32"}} */
void sound(unsigned); void delay(unsigned); void nosound(void);
void beep_float(float hz,unsigned ms) {
    int f;
    f=hz+0.5; sound(f); delay(ms); nosound();
}
