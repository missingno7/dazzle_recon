#ifndef DAZZLE_NATIVE_ELLIPSES_H
#define DAZZLE_NATIVE_ELLIPSES_H
void dz_native_arches_prepare(int rebuild);
int dz_native_ellipse_count(int radius_y, int radius_x);
int dz_native_arches_fallback(void);
void dz_native_arches_begin(int ry, int rx, int cx, int cy);
void dz_native_outline_step(void);
void dz_native_arches_end(void);
void dz_native_ellipse_prepare(int rebuild);
void dz_native_ellipse_begin(int crossing);
void dz_native_ellipse_step(void);
void dz_native_ellipse_end(void);
void dz_native_ring_begin(unsigned effect, int ry, int rx, double cx, double cy);
void dz_native_ring_end(void);
void dz_native_yarnball_begin(int x, int y);
#endif
