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
    aspect = (double)screen_half_width/screen_half_height*0.75;
    dz_native_ring_begin(flags & SHAPE_FILL ? DZ_EFFECT_COUNT : DZ_EFFECT_GUMBALLS,r,0,x,y);
}
void dz_native_gumballs_row(void) { if (enabled) extent = ellipse_fill_x; }
static void fill(unsigned symmetry, unsigned char color)
{
    int sx = copy >= 2 ? -1 : 1, sy = copy == 1 || copy == 2 ? -1 : 1;
    double low = extent > 0 ? (sx*ellipse_fill_x-0.5)/(extent+0.5) : -1;
    double high = extent > 0 ? (sx*ellipse_fill_x+0.5)/(extent+0.5) : 1;
    double bottom = sy*ellipse_trace_y-0.5, top = sy*ellipse_trace_y+0.5;
    int swaps = (symmetry & 3u) == 3u ? 2 : 1, branches = symmetry & 1u ? 4 : 1;
    for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
        int xs = branch >= 2 ? -1 : 1, ys = branch & 1 ? -1 : 1;
        double px, py, qx, qy;
        dz_native_pixel(xs*(swap ? cy : cx),ys*(swap ? cx : cy),0,0,&px,&py);
        dz_native_pixel(0,0,swap ? radius : radius*aspect,swap ? radius*aspect : radius,&qx,&qy);
        int w,h; dz_canvas_native_size(&w,&h);
        double rx = qx-w*0.5, ry = qy-h*0.5;
        /* Swap the cell too: exchanged-axis symmetry transposes row bands. */
        if (!swap) {
            int y0 = (int)ceil(py+fmin(ys*bottom,ys*top)*ry/radius-0.5);
            int y1 = (int)ceil(py+fmax(ys*bottom,ys*top)*ry/radius-0.5);
            if (y0 < 0) y0 = 0;
            if (y1 > h) y1 = h;
            for (int y = y0; y < y1; ++y) {
                double t = (y+0.5-py)/ry;
                if (fabs(t) > 1) continue;
                double half = rx*sqrt(1-t*t);
                int first = (int)ceil(px+fmin(xs*low,xs*high)*half-0.5);
                int end = (int)ceil(px+fmax(xs*low,xs*high)*half-0.5);
                dz_canvas_native_span(y,first,end,color);
            }
        } else {
            /* In a transposed cell, solve its colour-band inequalities on
             * each scanline. The result is at most two ordinary spans. */
            double u0 = fmax(-1,fmin(xs*bottom,xs*top)/radius);
            double u1 = fmin(1,fmax(xs*bottom,xs*top)/radius);
            if (u0 > u1) continue;
            double far = fmax(fabs(u0),fabs(u1));
            double near = u0 <= 0 && u1 >= 0 ? 0 : fmin(fabs(u0),fabs(u1));
            double s0 = sqrt(1-far*far), s1 = sqrt(1-near*near);
            double lo = fmin(ys*low,ys*high), hi = fmax(ys*low,ys*high);
            double bottom_y = fmin(fmin(lo*s0,lo*s1),fmin(hi*s0,hi*s1));
            double top_y = fmax(fmax(lo*s0,lo*s1),fmax(hi*s0,hi*s1));
            int y0 = (int)ceil(py+bottom_y*ry-0.5), y1 = (int)ceil(py+top_y*ry-0.5);
            if (y0 < 0) y0 = 0;
            if (y1 > h) y1 = h;
            for (int y = y0; y < y1; ++y) {
                double t = (y+0.5-py)/ry, l = lo, q = hi;
                if (t < 0) { t = -t; l = -hi; q = -lo; }
                if (q <= 0 || (t == 0 && l > 0)) continue;
                double lower = t/q, upper = l > 0 ? fmin(1,t/l) : 1;
                if (lower > upper || lower > 1) continue;
                double inner = sqrt(fmax(0,1-upper*upper));
                double outer = sqrt(fmax(0,1-lower*lower));
                for (int side = 0; side < 2; ++side) {
                    double left = fmax(u0,side ? inner : -outer);
                    double right = fmin(u1,side ? outer : -inner);
                    int first = (int)ceil(px+left*rx-0.5), end = (int)ceil(px+right*rx-0.5);
                    if (left < right) dz_canvas_native_span(y,first,end,color);
                }
            }
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
