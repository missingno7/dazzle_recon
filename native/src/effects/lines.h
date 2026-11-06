#ifndef DAZZLE_NATIVE_LINES_H
#define DAZZLE_NATIVE_LINES_H
#include <stdint.h>
void dz_native_walk(unsigned effect, int reset, double x, double y);
/* A circuit of drifting rings; arguments as dz_native_ring_step. */
void dz_native_ring_walk(unsigned effect, int x, int y, int step, int length, int side, int shift,
                         const int16_t *dx, const int16_t *dy);
void dz_native_sweep(int reset, int x, int run, int last);
int dz_native_hilbert_begin(int reset);
int dz_native_hilbert_xor_fallback(void);
void dz_native_hilbert_background(int x, int y, unsigned char color);
void dz_native_hilbert_step(int x, int y, int width, unsigned char color);
void dz_native_hilbert_finish(void);
#endif
