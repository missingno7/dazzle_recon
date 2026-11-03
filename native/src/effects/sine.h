#ifndef DAZZLE_NATIVE_SINE_H
#define DAZZLE_NATIVE_SINE_H
#include <stdint.h>
void dz_native_sine_prepare(int rebuild, double ax, double ay, double phase);
int dz_native_sine_pending(void);
void dz_native_sine_draw(uint16_t symmetry, uint8_t color);
#endif
