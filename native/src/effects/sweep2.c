/* A ray grows by one continuous interval per original line-iterator call. */
#include "sweep2.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
static int count, step;
static double x, y;
static void draw(unsigned symmetry, unsigned char color)
{
    double a = dz_native_fraction(step-1,count), b = dz_native_fraction(step,count);
    dz_native_line(x*a,y*a,x*b,y*b,symmetry,color,1); ++step;
}
void dz_native_sweep2_prepare(int initialize)
{
    /* The accelerating signed-word variant eventually overflows the doubled
     * Bresenham deltas. Keep that entire image instance Faithful. */
    if (!dz_native_enabled(DZ_EFFECT_SWEEP2) || effect_parameter) return;
    if (initialize) {
        x = sweep2_end_x; y = sweep2_end_y; step = 0;
        count = dz_native_count(0,0,sweep2_end_x,sweep2_end_y);
    }
    dz_native_queue(draw);
}
