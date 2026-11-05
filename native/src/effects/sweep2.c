/* A ray grows by one continuous interval per original line-iterator call. */
#include "sweep2.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int count, step, native;
static double x, y;
static void draw(unsigned symmetry, unsigned char color)
{
    dz_native_cell_at(0,0,0,0,x,y,step++,count,symmetry,color);
}
/* The logic stops each ray at the extent, short of a wide output's edge. The
 * same straight ray carries on; the coordinates exceed the original screen on
 * the major axis, so only the Extended sides can change. */
static void extend(unsigned symmetry, unsigned char color)
{
    int reach = dz_extended_reach();
    if (reach <= y) return;
    if (native) dz_native_line(x,y,x*reach/y,reach,symmetry,color);
    else for (int i = (int)y+1; i <= reach; ++i)
        dz_extended_stamp((int)lround(x*i/y),i,symmetry,color);
}
void dz_native_sweep2_prepare(int initialize)
{
    dz_native_plot = NULL;
    if (initialize) {
        x = sweep2_end_x; y = sweep2_end_y; step = 0;
        count = dz_native_count(0,0,sweep2_end_x,sweep2_end_y);
    }
    /* The accelerating signed-word variant eventually overflows the doubled
     * Bresenham deltas. Keep that entire image instance Faithful. */
    native = dz_native_enabled(DZ_EFFECT_SWEEP2) && !effect_parameter;
    if (native) dz_native_queue(draw);
    if (sweep2_line_done) dz_extended_queue(extend);
}
