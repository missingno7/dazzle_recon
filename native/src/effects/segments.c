/* Ray fans and drifting segments: only the next interval is rasterized.
 * Endpoints select a segment, never a path through the endpoint enumeration. */
#include "segments.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int enabled, count, step;
static double ax, ay, bx, by;
static void draw(unsigned symmetry, unsigned char color)
{
    double a = dz_native_fraction(step-1,count), b = dz_native_fraction(step,count);
    dz_native_line(ax+(bx-ax)*a,ay+(by-ay)*a,ax+(bx-ax)*b,ay+(by-ay)*b,symmetry,color,1);
    ++step;
}
void dz_native_segment_begin(unsigned effect, int initialize, int x, int y)
{
    if (!dz_native_enabled(effect)) { enabled = 0; return; }
    if (!initialize) return;
    enabled = 1;
    ax = draw_x; ay = draw_y; bx = x; by = y;
    count = dz_native_count(draw_x,draw_y,x,y); step = 0;
    /* A wrapped doubled delta is no longer a straight-line iterator. */
    if (fabs(bx-ax) >= 16384 || fabs(by-ay) >= 16384) enabled = 0;
}
void dz_native_segment_step(int reset)
{
    if (!enabled) return;
    /* Completion in boxes/puffs/bars discards the endpoint BEFORE its store.
     * Plot only the reset pen; do not paint the omitted last interval. */
    if (reset) { ax = bx = draw_x; ay = by = draw_y; count = step = 0; }
    dz_native_queue(draw);
}

static int little_enabled, little_count, little_step, copy;
static unsigned previous_copies, current_copies;
static double cx, cy, lx, ly, ex, ey, a, b;
void dz_native_little_begin(int x, int y, int end_x, int end_y)
{
    little_enabled = dz_native_enabled(DZ_EFFECT_LITTLESTARS);
    cx = x; cy = y; lx = draw_x; ly = draw_y; ex = end_x; ey = end_y;
    little_count = dz_native_count(draw_x,draw_y,end_x,end_y); little_step = 0;
    previous_copies = current_copies = 0;
}
void dz_native_little_step(void)
{
    previous_copies = current_copies; current_copies = 0;
    a = dz_native_fraction(little_step-1,little_count);
    b = dz_native_fraction(little_step,little_count); ++little_step;
}
static void little_draw(unsigned symmetry, unsigned char color)
{
    double start = previous_copies >> copy & 1u ? a : b;
    double x0 = lx+(ex-lx)*start, y0 = ly+(ey-ly)*start;
    double x1 = lx+(ex-lx)*b, y1 = ly+(ey-ly)*b;
    if (copy >= 4) { double t = x0; x0 = y0; y0 = t; t = x1; x1 = y1; y1 = t; }
    int sx = copy%4 >= 2 ? -1 : 1, sy = copy & 1 ? -1 : 1;
    /* Both placement and straight star rays spread, with no separate side stars. */
    dz_native_line(cx+sx*x0,cy+sy*y0,cx+sx*x1,cy+sy*y1,symmetry,color,1);
}
void dz_native_little_copy(int reflection)
{
    if (little_enabled) {
        copy = reflection; current_copies |= 1u << copy; dz_native_queue(little_draw);
    }
}
