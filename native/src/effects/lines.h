#ifndef DAZZLE_NATIVE_LINES_H
#define DAZZLE_NATIVE_LINES_H
void dz_native_walk(unsigned effect, int reset, double x, double y);
void dz_native_sweep(int reset, int x, int run, int last);
int dz_native_hilbert_begin(int reset);
int dz_native_hilbert_xor_fallback(void);
void dz_native_hilbert_background(int x, int y, unsigned char color);
void dz_native_hilbert_step(int x, int y, int width, unsigned char color);
void dz_native_hilbert_finish(void);
#endif
