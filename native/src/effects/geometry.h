#ifndef DAZZLE_NATIVE_GEOMETRY_H
#define DAZZLE_NATIVE_GEOMETRY_H
#include <stdint.h>
/* Immediate geometry only: no scene, allocations, random numbers or clock. */
double dz_native_gain(void);
double dz_native_scale(void); /* largest fitted Original unit, for chord error */
void dz_native_line(double ax, double ay, double bx, double by, unsigned symmetry,
                    uint8_t color, int spread);
void dz_native_line_at(double cx, double cy, double ax, double ay, double bx, double by,
                       unsigned symmetry, uint8_t color);
int dz_native_count(int ax, int ay, int bx, int by);
double dz_native_fraction(int step, int count);
/* Analytic quadrant, parameterized by the midpoint iterator's major-axis work. */
double dz_native_quadrant(double progress, double rx, double ry);
void dz_native_point(unsigned effect, double x, double y, int spread);
void dz_native_point_at(unsigned effect, double cx, double cy, double x, double y);
#endif
