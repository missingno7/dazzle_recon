#ifndef DAZZLE_NATIVE_SINE_H
#define DAZZLE_NATIVE_SINE_H
#include <stdint.h>
void dz_native_sine_prepare(int rebuild, int x, int y);
int dz_native_sine_pending(void);
void dz_native_sine_draw(unsigned symmetry, unsigned char color);
#endif
