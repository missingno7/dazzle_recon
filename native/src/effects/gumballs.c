/* Each original fill sample owns a short output-resolution span in its
 * original colour band. Only the ellipse boundary is evaluated continuously. */
#include "gumballs.h"
#include "ellipses.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"
#include "platform/canvas.h"
#include <math.h>
static int enabled, extent, copy;
static double radius, cx, cy, aspect;
void dz_native_gumballs_begin(int r, int x, int y, unsigned flags)
{
    enabled = dz_native_enabled(DZ_EFFECT_GUMBALLS) && (flags & SHAPE_FILL);
    radius = r; cx = x; cy = y;
    aspect = r > 0 ? (double)dz_native_radius_x(r)/r : 1;
    dz_native_ring_begin(flags & SHAPE_FILL ? DZ_EFFECT_COUNT : DZ_EFFECT_GUMBALLS,r,0,x,y);
}
void dz_native_gumballs_row(void) { if (enabled) extent = ellipse_fill_x; }
static void fill(unsigned symmetry, unsigned char color)
{
    int sx = copy >= 2 ? -1 : 1, sy = copy == 1 || copy == 2 ? -1 : 1;
    double low = ellipse_fill_x-0.5, high = ellipse_fill_x+0.5;
    /* Preserve logical colour cells. Only the outermost cell continues to the
     * real boundary; scaling every cell by the cached span displaced colours. */
    if (ellipse_fill_x == extent) high = HUGE_VAL;
    if (ellipse_fill_x == -extent) low = -HUGE_VAL;
    if (extent <= 0) { low = -HUGE_VAL; high = HUGE_VAL; }
    double x0 = fmin(sx*low,sx*high), x1 = fmax(sx*low,sx*high);
    double y0 = sy*ellipse_trace_y-0.5, y1 = sy*ellipse_trace_y+0.5;
    struct dz_native_mapping m = dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int swaps = (symmetry & 3u) == 3u ? 2 : 1, branches = symmetry & 1u ? 4 : 1;
    int w,h; dz_canvas_native_size(&w,&h);
    for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
        int xs = branch >= 2 ? -1 : 1, ys = branch & 1 ? -1 : 1;
        double px, py;
        dz_native_pixel(xs*(swap ? cy : cx),ys*(swap ? cx : cy),0,0,&px,&py);
        double rx = radius*(swap ? 1 : aspect)*m.unit_x;
        double ry = radius*(swap ? aspect : 1)*m.unit_y;
        double left = fmin(xs*(swap ? y0 : x0),xs*(swap ? y1 : x1))*m.unit_x;
        double right = fmax(xs*(swap ? y0 : x0),xs*(swap ? y1 : x1))*m.unit_x;
        double bottom = fmin(ys*(swap ? x0 : y0),ys*(swap ? x1 : y1))*m.unit_y;
        double top = fmax(ys*(swap ? x0 : y0),ys*(swap ? x1 : y1))*m.unit_y;
        double first_y = fmax(0,ceil(py+fmax(-ry,bottom)-0.5));
        double end_y = fmin(h,ceil(py+fmin(ry,top)-0.5));
        for (int y = (int)first_y; y < (int)end_y; ++y) {
            double t = (y+0.5-py)/ry;
            if (fabs(t) > 1) continue;
            double half = rx*sqrt(1-t*t);
            int first = (int)ceil(px+fmax(-half,left)-0.5);
            int end = (int)ceil(px+fmin(half,right)-0.5);
            dz_canvas_native_span(y,first,end,color);
        }
    }
}

void dz_native_gumballs_step(int branch)
{ if (enabled) { copy = branch; dz_native_queue(fill); } }
void dz_native_gumballs_end(void)
{
    if (enabled) dz_native_point_at(DZ_EFFECT_GUMBALLS,cx,cy,draw_x-cx,draw_y-cy);
    else dz_native_ring_end();
    enabled = 0;
}
