/* Four ray fans ending on the alternating-radius petal arcs. */
#include "daisies.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int enabled, count, step, branch;
static double start, end, x, y, a, b;
static double phase(int index)
{ return (index <= 1 ? index : index < 9 ? 2*index-1 : 16)*6.283185307179586/64; }
void dz_native_daisies_prepare(void)
{
    enabled = dz_native_enabled(DZ_EFFECT_DAISIES);
    if (!enabled) return;
    start = phase(daisies_sample_index); end = phase(daisies_sample_index+daisies_sample_step);
}
void dz_native_daisies_begin(void)
{
    if (!enabled) return;
    double t = dz_native_fraction(daisies_work_index,daisies_edge_count-1), theta = start+(end-start)*t;
    double aspect = (double)screen_half_width/screen_half_height*0.75;
    x = aspect*daisies_radius*sin(theta); y = daisies_radius*cos(theta);
    count = dz_native_count(draw_x,draw_y,0,0); step = 0;
}
static void draw(unsigned symmetry, unsigned char color)
{
    int sx = branch == 1 || branch == 3 ? -1 : 1, sy = branch == 1 || branch == 2 ? -1 : 1;
    dz_native_line_at(daisies_center_x,daisies_center_y,
        sx*x*(1-a),sy*y*(1-a),sx*x*(1-b),sy*y*(1-b),symmetry,color);
    if (++branch < 4) dz_native_queue(draw);
}
void dz_native_daisies_step(void)
{
    if (!enabled) return;
    a = dz_native_fraction(step-1,count); b = dz_native_fraction(step,count); ++step;
    branch = 0; dz_native_queue(draw);
}
