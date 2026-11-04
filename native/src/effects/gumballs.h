#ifndef DAZZLE_NATIVE_GUMBALLS_H
#define DAZZLE_NATIVE_GUMBALLS_H
void dz_native_gumballs_begin(int radius, int cx, int cy, unsigned flags);
void dz_native_gumballs_row(void);
void dz_native_gumballs_step(int copy);
void dz_native_gumballs_end(void);
#endif
