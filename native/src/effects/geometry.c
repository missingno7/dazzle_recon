#include "geometry.h"
#include "effects.h"
#include "dazzle.h"
#include "platform/canvas.h"
#include <math.h>
#include <stdlib.h>

static double ux, uy, weight, gain;
static int w, h, hw, hh, vw, vh;
static void mapping(void)
{
    int a, b;
    dz_canvas_native_size(&a,&b);
    if (w == a && h == b && hw == screen_half_width && hh == screen_half_height &&
        vw == dz_video.width && vh == dz_video.height) return;
    w = a; h = b; hw = screen_half_width; hh = screen_half_height;
    vw = dz_video.width; vh = dz_video.height;
    double fitted = fmin(h,w*0.75);
    ux = hw > 0 ? fitted*4/3/(2*hw) : 0;
    uy = hh > 0 ? fitted/(2*hh) : 0;
    gain = fitted > 0 ? w/(fitted*4/3) : 1;
    weight = vw > 0 && vh > 0 ? fmax(1,sqrt(fitted*fitted*4/3/(vw*vh))) : 1;
}
double dz_native_gain(void) { mapping(); return gain; }
double dz_native_scale(void) { mapping(); return fmax(ux,uy); }
static void line(double cx, double cy, double ax, double ay, double bx, double by,
                 unsigned symmetry, uint8_t color, int spread)
{
    mapping();
    if (!w || !h) return;
    double xunit = ux*(spread ? gain : 1);
    int branches = symmetry & 1u ? 4 : 1;
    int swaps = (symmetry & 3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
        int sx = branch >= 2 ? -1 : 1, sy = branch & 1 ? -1 : 1;
        double ox = (swap ? cy : cx)*ux*gain, oy = (swap ? cx : cy)*uy;
        dz_canvas_stroke(w*0.5+sx*(ox+(swap ? ay : ax)*xunit), h*0.5+sy*(oy+(swap ? ax : ay)*uy),
                         w*0.5+sx*(ox+(swap ? by : bx)*xunit), h*0.5+sy*(oy+(swap ? bx : by)*uy),weight,color);
    }
}
void dz_native_line(double ax, double ay, double bx, double by, unsigned symmetry,
                    uint8_t color, int spread)
{ line(0,0,ax,ay,bx,by,symmetry,color,spread); }
void dz_native_line_at(double cx, double cy, double ax, double ay, double bx, double by,
                       unsigned symmetry, uint8_t color)
{ line(cx,cy,ax,ay,bx,by,symmetry,color,0); }
int dz_native_count(int ax, int ay, int bx, int by)
{
    int x = abs(bx-ax), y = abs(by-ay);
    return x > y ? x : y;
}
double dz_native_fraction(int step, int count)
{ return step < 0 ? 0 : count <= 0 || step >= count ? 1 : (double)step/count; }
double dz_native_quadrant(double progress, double rx, double ry)
{
    double length = hypot(rx,ry), split = rx*rx/length;
    return progress < split ? asin(fmin(1,progress/rx)) : acos(fmax(0,fmin(1,(length-progress)/ry)));
}
static double point_x, point_y, point_cx, point_cy;
static int point_spread, point_at;
static void point_draw(unsigned symmetry, unsigned char color)
{
    if (point_at) dz_native_line_at(point_cx,point_cy,point_x,point_y,point_x,point_y,symmetry,color);
    else dz_native_line(point_x,point_y,point_x,point_y,symmetry,color,point_spread);
}
void dz_native_point(unsigned effect, double x, double y, int spread)
{
    if (!dz_native_enabled(effect)) return;
    point_x = x; point_y = y; point_spread = spread; point_at = 0; dz_native_queue(point_draw);
}
void dz_native_point_at(unsigned effect, double cx, double cy, double x, double y)
{
    if (!dz_native_enabled(effect)) return;
    point_x = x; point_y = y; point_cx = cx; point_cy = cy; point_at = 1; dz_native_queue(point_draw);
}
