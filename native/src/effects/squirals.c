/* Each rotating, decaying square edge is a true straight segment. */
#include "squirals.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int enabled, count, step, branch, mode;
static double x, y, cx, cy, a, b;
void dz_native_squirals_begin(double radius, double angle, int center_x, int center_y, int reflection, int ex, int ey)
{
    enabled = dz_native_enabled(DZ_EFFECT_SQUIRALS);
    if (!enabled) return;
    x = radius*(cos(angle)-sin(angle)); y = radius*(cos(angle)+sin(angle));
    cx = center_x; cy = center_y; mode = reflection;
    count = dz_native_count(ey,-ex,ex,ey); step = 0;
}
static void draw(unsigned symmetry, unsigned char color)
{
    double ax = y+(x-y)*a, ay = -x+(y+x)*a;
    double bx = y+(x-y)*b, by = -x+(y+x)*b;
    int copy = mode == 3 ? branch+2 : branch;
    if (copy >= 2) { double v = ax; ax = -ay; ay = v; v = bx; bx = -by; by = v; }
    if (copy & 1) { ax = -ax; ay = -ay; bx = -bx; by = -by; }
    dz_native_line_at(cx,cy,ax,ay,bx,by,symmetry,color);
    if (++branch < (mode >= 2 ? 2 : 4)) dz_native_queue(draw);
}
void dz_native_squirals_step(void)
{
    if (!enabled) return;
    a = dz_native_fraction(step-1,count); b = dz_native_fraction(step,count); ++step;
    branch = 0; dz_native_queue(draw);
}
