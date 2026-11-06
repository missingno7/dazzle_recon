#ifndef DAZZLE_NATIVE_FADES_H
#define DAZZLE_NATIVE_FADES_H
#include <stdint.h>
int dz_native_fade_enabled(unsigned key);
int dz_native_fades_any(void);
int dz_native_fades_select(const char *keys);
/* Independent of image callbacks: a fade can call an image generator, or chain
 * another fade, without acquiring that image's queued pen. No charged work. */
extern void (*dz_native_fade_plot)(unsigned symmetry, unsigned char color);
void dz_native_fade_begin(unsigned key);
void dz_native_fade_end(void);
void dz_native_fade_line(double ax, double ay, double bx, double by);
void dz_native_fade_real_line(double ax, double ay, double bx, double by);
void dz_native_fade_fixed_line(double ax, double ay, double bx, double by);
void dz_native_fade_color_rule(int rule);
void dz_native_fade_mesh(int spacing, int color_rule);
void dz_native_fade_mesh_finish(void);
void dz_native_fade_hexagon(void);
void dz_native_fade_blister(int x, int y, int stride, int flat, int offset,
                            int bitmap_x, int bitmap_y);
/* Filled convex shape / thick polyline, in software coordinates. XOR is the
 * actual EGA raster operation; spans are half open, including shared cuts. */
void dz_native_fade_shape(const double *x, const double *y, int count,
                          unsigned symmetry, uint8_t color);
void dz_native_fade_polyline(const double *x, const double *y, int count,
                             double width, unsigned symmetry, uint8_t color);
#endif
