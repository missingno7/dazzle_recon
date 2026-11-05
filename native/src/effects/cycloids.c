/* The original joins 64-entry table positions with straight lines, so a rolling
 * curve shows corners. Pass a centripetal Catmull-Rom spline through the same
 * positions; line_step still decides how many samples each span takes. */
#include "cycloids.h"
#include "effects.h"
#include "geometry.h"
#include <math.h>
static struct dz_native_path path;
static double px[4], py[4], mx[2], my[2];
static int enabled, reset, count, step, parts;
static void point(double t, double *x, double *y)
{
    double u = 1-t, a = u*u*(1+2*t), b = t*t*(3-2*t), c = t*u*u, d = -t*t*u;
    *x = a*px[1]+b*px[2]+c*mx[0]+d*mx[1];
    *y = a*py[1]+b*py[2]+c*my[0]+d*my[1];
}
/* Centripetal tangent at b, from outer neighbour a and span b-c, whose square
 * root chord lengths are outer and d. Equal to the chord at a repeated point. */
static double tangent(const double *v, int a, int b, int c, double outer, double d)
{
    double chord = v[2]-v[1];
    if (outer <= 0 || d <= 0) return chord;
    double side = a < b ? v[b]-v[a] : v[a]-v[b];
    double across = a < c ? v[c]-v[a] : v[a]-v[c];
    return chord+d*(side/outer-across/(outer+d));
}
static void draw(unsigned symmetry, unsigned char color)
{
    double x, y;
    for (int i = step ? 1 : parts; i <= parts; ++i) {
        point(step ? (step-1+(double)i/parts)/count : 0,&x,&y);
        dz_native_path_prepare(&path,x,y,!step && reset);
        dz_native_path_draw(&path,symmetry,color);
    }
    ++step;
}
void dz_native_cycloids_begin(int first, const double x[4], const double y[4], int cells)
{
    enabled = dz_native_enabled(DZ_EFFECT_CYCLOIDS);
    if (!enabled) { path.valid = 0; return; }
    double d[3];
    for (int i = 0; i < 4; ++i) { px[i] = x[i]; py[i] = y[i]; }
    for (int i = 0; i < 3; ++i) d[i] = sqrt(hypot(x[i+1]-x[i],y[i+1]-y[i]));
    mx[0] = tangent(px,0,1,2,d[0],d[1]); my[0] = tangent(py,0,1,2,d[0],d[1]);
    mx[1] = tangent(px,3,2,1,d[2],d[1]); my[1] = tangent(py,3,2,1,d[2],d[1]);
    reset = first; count = cells; step = 0; parts = 1;
    /* The spline can be much longer than its chord; keep steps near one cell. */
    double length = 0, ax = x[1], ay = y[1], bx, by;
    for (int i = 1; i <= 8; ++i) {
        point(i/8.0,&bx,&by); length += hypot(bx-ax,by-ay); ax = bx; ay = by;
    }
    if (cells > 0) parts = (int)ceil(length/cells);
    if (parts < 1) parts = 1;
}
void dz_native_cycloids_step(void) { if (enabled) dz_native_queue(draw); }
