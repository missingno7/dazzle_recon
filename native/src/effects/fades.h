#ifndef DAZZLE_NATIVE_FADES_H
#define DAZZLE_NATIVE_FADES_H
#include <stdint.h>
/* Key-indexed (a..z), including the palette and page-only choices. */
#define DZ_NATIVE_FADE_MASK UINT32_C(0x03ffdf7f)
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
/* Real-position circular particles, or an explicit square of the given size.
 * A square is one figure; its unchanged logical subplots only charge work. */
void dz_native_fade_pen(double x,double y,int square);
int dz_native_fade_point_pen(void);
/* Scalar height patches, called at the original plasma recursion's leaves. */
void dz_native_fade_plasma(double x0,double y0,double x1,double y1,
                           double tl,double tr,double br,double bl);
void dz_native_fade_plasma_begin(void);
void dz_native_fade_plasma_sample(int x,int y,unsigned height,int forced);
unsigned dz_native_fade_plasma_read(int x,int y,unsigned fallback);
void dz_native_fade_drip(int column,const uint32_t *positions,const uint32_t *speed);
void dz_native_fade_quilt(int ax,int ay,int bx,int by,int width,int height,unsigned flip);
void dz_native_fade_quilt_end(void);
/* Filled convex shape / thick polyline, in software coordinates. XOR is the
 * actual EGA raster operation; spans are half open, including shared cuts. */
void dz_native_fade_shape(const double *x, const double *y, int count,
                          unsigned symmetry, uint8_t color);
void dz_native_fade_polyline(const double *x, const double *y, int count,
                             double width, unsigned symmetry, uint8_t color);
#endif
