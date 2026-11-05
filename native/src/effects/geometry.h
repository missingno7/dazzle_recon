#ifndef DAZZLE_NATIVE_GEOMETRY_H
#define DAZZLE_NATIVE_GEOMETRY_H
#include <stdint.h>
/* Immediate geometry only: no scene, allocations, random numbers or clock. */
double dz_native_scale(void); /* largest current coordinate unit, for chord error */
int dz_extended_reach(void); /* logical X of the output edge in Extended, else 0 */
int dz_native_radius_x(int radius_y);
void dz_native_pixel(double cx, double cy, double x, double y, double *px, double *py);
void dz_native_line(double ax, double ay, double bx, double by, unsigned symmetry,
                    uint8_t color);
void dz_native_line_at(double cx, double cy, double ax, double ay, double bx, double by,
                       unsigned symmetry, uint8_t color);
void dz_native_line_caps_at(double cx, double cy, double ax, double ay, double bx, double by,
                            unsigned symmetry, uint8_t color, unsigned caps);
void dz_native_bounds(int sample, int count, double *start, double *end);
int dz_native_count(int ax, int ay, int bx, int by);
/* A sample owns the interval between neighbouring midpoints. Only the path's
 * two ends have round caps. No interval runs beyond the current pen cell. */
void dz_native_cell_at(double cx, double cy, double ax, double ay, double bx, double by,
                       int step, int count, unsigned symmetry, uint8_t color);
void dz_native_cell_width_at(double cx, double cy, double ax, double ay, double bx, double by,
                             int step, int count, unsigned symmetry, uint8_t color, double width);
void dz_native_raster_cell(double cx, double cy, double x, double y,
                           unsigned symmetry, uint8_t color);
/* Sample the current reveal rectangle at physical pixel centres. A negative
 * result leaves the previous ink alone. Coordinates are before symmetry. */
typedef int (*dz_native_sample)(double x, double y, void *context);
void dz_native_sample_cell(int x, int y, double radius, unsigned symmetry,
                           dz_native_sample sample, void *context);
void dz_native_dot(int x, int y, unsigned symmetry, uint8_t color);
/* One revealed piece of a row with walking left/right ends. */
void dz_native_row_cell(int x, int y, int left, int right, double dl, double dr,
                        unsigned symmetry, uint8_t color);
/* Explicit source-row spacing, for the enhanced tile rectangle scan. */
void dz_native_row_cell_step(int x, int y, int left, int right, double dl, double dr,
                             double row_step, unsigned symmetry, uint8_t color);
struct dz_native_path { double ax, ay, bx, by, dx, dy; uint16_t page; uint8_t color; int valid, fresh, corners; };
/* Truncation sends v to the cell centred about half a cell nearer zero. This is
 * the smooth curve those cells quantize, and stays within half a cell of each. */
double dz_native_untruncated(double value);
void dz_native_path_prepare(struct dz_native_path *path, double x, double y, int reset);
void dz_native_path_draw(struct dz_native_path *path, unsigned symmetry, uint8_t color);
void dz_native_path_draw_width(struct dz_native_path *path, unsigned symmetry,
                               uint8_t color, double width);
void dz_native_quad_at(double cx, double cy, const double x[4], const double y[4],
                       unsigned symmetry, uint8_t color);
double dz_native_fraction(int step, int count);
/* Analytic quadrant, parameterized by the midpoint iterator's major-axis work. */
double dz_native_quadrant(double progress, double rx, double ry);
void dz_native_point(unsigned effect, double x, double y);
void dz_native_point_at(unsigned effect, double cx, double cy, double x, double y);
#endif
