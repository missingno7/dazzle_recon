/* Ellipse fans and the shared growing/shrinking ring outline schedule. */
#include "ellipses.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"
#include <math.h>
#include <stdlib.h>
extern int16_t yarnball_radius_y;
static int arches_enabled, arches_active, branch, ring_active, arches_fallback;
static unsigned ring_effect;
static double rx, ry, cx, cy, angle0, angle1;
static double arc_x0, arc_y0, arc_x1, arc_y1;
static int arc_steps;
static unsigned arc_caps;
static void outline_point(unsigned symmetry, unsigned char color)
{
    if (arches_active) dz_native_line(cx,cy,cx,cy,symmetry,color);
    else dz_native_line_at(cx,cy,0,0,0,0,symmetry,color);
    if (++branch < 4) dz_native_queue(outline_point);
}
static void outline_draw(unsigned symmetry, unsigned char color)
{
    int sx = branch >= 2 ? -1 : 1, sy = branch == 1 || branch == 2 ? -1 : 1;
    double ax = cx+sx*arc_x0, ay = cy+sy*arc_y0;
    for (int i = 1; i <= arc_steps; ++i) {
        double t = angle0+(angle1-angle0)*i/arc_steps;
        double bx = cx+sx*(i == arc_steps ? arc_x1 : rx*sin(t));
        double by = cy+sy*(i == arc_steps ? arc_y1 : ry*cos(t));
        unsigned caps = (i == 1 ? arc_caps & 1u : 1u) |
                        (i == arc_steps ? arc_caps & 2u : 2u);
        dz_native_line_caps_at(0,0,ax,ay,bx,by,symmetry,color,caps);
        ax = bx; ay = by;
    }
    if (++branch < 4) dz_native_queue(outline_draw);
}
/* Count the original midpoint cache, including its first-region zero-Y bug.
 * This is a pure bounded prediction: no logical state, RNG or work charges.
 * Radii below the cache bound keep all products within the DOS long range. */
int dz_native_ellipse_count(int y, int x)
{
    if (y < 0) return 0;
    if (!x) {
        float aspect = (float)((double)screen_half_width/screen_half_height*0.75);
        double value = y*(double)aspect;
        x = (int)value; if (value-x > 0.5) ++x;
    }
    x = abs(x);
    if (ceil(hypot(x,y))+2 >= 700) return 0;
    if (!y) return 1;
    int64_t xx = x*x, yy = y*y, gx = 0, gy = 2*xx*y, error = xx/4-xx*y;
    int first = 1;
    for (int count = 2; count < 700; ++count) {
        if (first) {
            error += gx+yy; gx += 2*yy;
            if (error > 0) { --y; gy -= 2*xx; error -= gy; }
            if (gy <= gx) {
                int64_t delta = yy-xx;
                error += (gx+gy)/2+(delta < 0 ? -((-delta+3)/4) : delta/4);
                first = 0;
            }
            /* Original completion is checked only in the second region. */
            if (!y) return 0;
        } else {
            error += -gy+xx; gy -= 2*xx; --y;
            if (error < 0) { gx += 2*yy; error += gx; }
            if (!y) return count;
        }
    }
    return 0;
}
void dz_native_arches_prepare(int rebuild)
{
    if (!rebuild) return;
    arches_enabled = 0; arches_fallback = 0;
    if (!dz_native_enabled(DZ_EFFECT_ARCHES)) return;
    if (!dz_native_ellipse_count(arches_first_radius_y,arches_first_radius_x) ||
        !dz_native_ellipse_count(arches_second_radius_y,arches_second_radius_x)) {
        arches_fallback = DZ_NATIVE_CACHE_SPILL; return;
    }
    /* Semi-Auto has no work limit: a later F10 can reach a malformed cache.
     * Keep that complete instance Faithful, rather than changing layouts later. */
    if (semi_auto_mode) { arches_fallback = DZ_NATIVE_UNBOUNDED; return; }
    uint32_t work = (uint16_t)image_work_count;
    for (int i = 0; ; ++i) {
        /* Auto always draws the initializer and one continuation before its
         * next work-limit comparison, which post-increments the word count. */
        if (i >= 2) { if (work > image_work_limit) break; ++work; }
        int a = dz_native_ellipse_count(arches_first_radius_y-8*i,arches_first_radius_x);
        int b = dz_native_ellipse_count(arches_second_radius_y,arches_second_radius_x-8*i);
        if (!a || !b) { arches_fallback = DZ_NATIVE_CACHE_SPILL; return; }
        work += (unsigned)(a+b);
    }
    arches_enabled = 1;
}
void dz_native_arches_begin(int y, int x, int a, int b)
{
    dz_native_plot = NULL; ry = y;
    rx = x ? abs(x) : dz_native_radius_x(y);
    cx = a; cy = b; arches_active = 1;
}
int dz_native_arches_fallback(void) { return arches_fallback; }
void dz_native_arches_end(void)
{
    arches_active = 0;
    if (arches_enabled && ellipse_trace_limit < 700)
        dz_native_point(DZ_EFFECT_ARCHES,cx-(ry > 0 ? rx : 0),cy);
}
void dz_native_outline_step(void)
{
    if ((!arches_active || !arches_enabled) && !ring_active) return;
    if (ellipse_trace_limit >= 700) return;
    if (rx <= 0 || ry <= 0) { branch = 0; dz_native_queue(outline_point); return; }
    double length = hypot(rx,ry);
    /* Keep the integrator's sample position (index-1), not the preceding
     * interval. Explicit midpoint cuts centre its colour independently of caps. */
    int sample = ellipse_trace_index-1, count = ellipse_trace_limit-1;
    double a,b; dz_native_bounds(sample,count,&a,&b);
    angle0 = dz_native_quadrant(a*length,rx,ry);
    angle1 = dz_native_quadrant(b*length,rx,ry);
    arc_caps = (sample == 0 ? 1u : 0u) | (sample == count ? 2u : 0u);
    double error = dz_native_scale()*fmax(rx,ry)*(angle1-angle0)*(angle1-angle0)/8;
    arc_steps = (int)ceil(sqrt(error/0.125));
    if (arc_steps < 1) arc_steps = 1;
    /* Four imminent copies share endpoints and subdivision count. No arc list. */
    arc_x0 = rx*sin(angle0); arc_y0 = ry*cos(angle0);
    double t = angle0+(angle1-angle0)*arc_steps/arc_steps;
    arc_x1 = rx*sin(t); arc_y1 = ry*cos(t);
    branch = 0; dz_native_queue(outline_draw);
}

static int fan_step, fan_count, fan_enabled;
static double fan_x, fan_y, ax, ay, bx, by;
static void fan_draw(unsigned symmetry, unsigned char color)
{
    dz_native_cell_at(ellipse_center_x,ellipse_center_y,ax,ay,bx,by,
        fan_step,fan_count,symmetry,color);
    ++fan_step;
}
void dz_native_ellipse_prepare(int rebuild)
{
    fan_enabled = dz_native_enabled(DZ_EFFECT_ELLIPSE);
    if (!fan_enabled) return;
    (void)rebuild;
    fan_x = ellipse_offset_x; fan_y = ellipse_offset_y;
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

/* Cones, yarnball and gumballs share the existing quadrant schedule, but not
 * its rounded directions. Keep the original centres and local ring metric. */
void dz_native_ring_begin(unsigned effect, int y, int x, double a, double b)
{
    dz_native_plot = NULL;
    ring_effect = effect; arches_active = 0;
    ry = y; rx = x ? abs(x) : dz_native_radius_x(y);
    cx = a; cy = b;
    ring_active = dz_native_enabled(effect) && ry >= 0 &&
        ceil(hypot(rx,ry))+2 < 700;
}
void dz_native_ring_end(void)
{
    if (ring_active) dz_native_point_at(ring_effect,cx,cy,ry > 0 ? -rx : 0,0);
    ring_active = 0;
}
void dz_native_yarnball_begin(int x, int y)
{ dz_native_ring_begin(DZ_EFFECT_YARNBALL,yarnball_radius_y,0,x,y); }
