#ifndef DAZZLE_NATIVE_SEGMENTS_H
#define DAZZLE_NATIVE_SEGMENTS_H
void dz_native_segment_begin(unsigned effect, int initialize, int x, int y);
void dz_native_segment_step(int reset);
void dz_native_little_begin(int cx, int cy, int x, int y);
void dz_native_little_step(void);
void dz_native_little_copy(int reflection);
void dz_extended_little_copy(int reflection);
#endif
