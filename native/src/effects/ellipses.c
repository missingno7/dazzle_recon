/* Arches are shrinking outlines; ellipse is a growing ray/diameter fan. */
#include "ellipses.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int arches_enabled, arches_active, branch;
static double rx, ry, cx, cy, angle0, angle1;
static void arches_draw(unsigned symmetry, unsigned char color)
{
    int sx = branch >= 2 ? -1 : 1, sy = branch == 1 || branch == 2 ? -1 : 1;
    double error = dz_native_scale()*dz_native_gain()*fmax(rx,ry)*(angle1-angle0)*(angle1-angle0)/8;
    int n = (int)ceil(sqrt(error/0.125));
    if (n < 1) n = 1;
    double ax = cx+sx*rx*sin(angle0), ay = cy+sy*ry*cos(angle0);
    for (int i = 1; i <= n; ++i) {
        double t = angle0+(angle1-angle0)*i/n;
        double bx = cx+sx*rx*sin(t), by = cy+sy*ry*cos(t);
        dz_native_line(ax,ay,bx,by,symmetry,color,1); ax = bx; ay = by;
    }
    if (++branch < 4) dz_native_queue(arches_draw);
}
void dz_native_arches_prepare(int rebuild)
{
    if (!rebuild) return;
    /* Conservative bound on the quadrant cache. The doubled large EGA image
     * aliases its own completion word; never draw a complete ideal ring there. */
    arches_enabled = dz_native_enabled(DZ_EFFECT_ARCHES) &&
        ceil(hypot(arches_first_radius_x,arches_first_radius_y))+2 < 700 &&
        ceil(hypot(arches_second_radius_x,arches_second_radius_y))+2 < 700;
}
void dz_native_arches_begin(int y, int x, int a, int b)
{ ry = y; rx = x; cx = a; cy = b; arches_active = 1; }
void dz_native_arches_end(void)
{
    arches_active = 0;
    if (arches_enabled && rx > 0 && ry > 0 && ellipse_trace_limit < 700)
        dz_native_point(DZ_EFFECT_ARCHES,cx-rx,cy,1);
}
void dz_native_arches_step(void)
{
    if (!arches_active || !arches_enabled || rx <= 0 || ry <= 0 || ellipse_trace_limit >= 700) return;
    double length = hypot(rx,ry);
    angle0 = dz_native_quadrant(fmax(0,ellipse_trace_index-2)*length/ellipse_trace_limit,rx,ry);
    angle1 = dz_native_quadrant((ellipse_trace_index-1)*length/ellipse_trace_limit,rx,ry);
    /* The inclusive final sample owns the last arc to the axis. */
    if (ellipse_trace_index == ellipse_trace_limit) angle1 = 1.5707963267948966;
    branch = 0; dz_native_queue(arches_draw);
}

static int fan_step, fan_count, fan_index, fan_enabled;
static double fan_rx, fan_ry, fan_x, fan_y, ax, ay, bx, by;
static void fan_draw(unsigned symmetry, unsigned char color)
{
    double a = dz_native_fraction(fan_step-1,fan_count), b = dz_native_fraction(fan_step,fan_count);
    dz_native_line_at(ellipse_center_x,ellipse_center_y,
        ax+(bx-ax)*a,ay+(by-ay)*a,ax+(bx-ax)*b,ay+(by-ay)*b,symmetry,color);
    ++fan_step;
}
void dz_native_ellipse_prepare(int rebuild)
{
    fan_enabled = dz_native_enabled(DZ_EFFECT_ELLIPSE);
    if (!fan_enabled) return;
    if (rebuild) fan_index = 0;
    fan_rx = g_long5b42; fan_ry = g_long5b46;
    double theta = dz_native_quadrant(fmin(hypot(fan_rx,fan_ry),fan_index++),fan_rx,fan_ry);
    fan_x = fan_rx*sin(theta); fan_y = fan_ry*cos(theta);
}
void dz_native_ellipse_begin(int crossing)
{
    if (!fan_enabled) return;
    ax = ay = 0;
    bx = fan_x; by = fan_y;
    if (ellipse_draw_mode == 2) {
        ax -= fan_x; ay -= fan_y;
        if (crossing) { ay = fan_y; by = -fan_y; }
    }
    fan_step = 0;
    fan_count = dz_native_count(draw_x,draw_y,
        ellipse_center_x+ellipse_offset_x,ellipse_center_y+(crossing ? -ellipse_offset_y : ellipse_offset_y));
}
void dz_native_ellipse_step(void) { if (fan_enabled) dz_native_queue(fan_draw); }
void dz_native_ellipse_end(void)
{ if (fan_enabled) dz_native_point_at(DZ_EFFECT_ELLIPSE,ellipse_center_x,ellipse_center_y,bx,by); }
