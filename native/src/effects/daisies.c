/* Fill between the original cached rays. Their radial colour cells retain
 * logical ownership; the continuous fan deliberately fills raster moire holes. */
#include "daisies.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include <math.h>
static int enabled, count, step, branch;
static double lx, ly, rx, ry, pen_x, pen_y;
void dz_native_daisies_prepare(void)
{ enabled = dz_native_enabled(DZ_EFFECT_DAISIES); }
void dz_native_daisies_begin(void)
{
    if (!enabled) return;
    int index = daisies_work_index, last = daisies_edge_count-1;
    pen_x = draw_x; pen_y = draw_y;
    int left = index, right = index;
    /* Repeated truncated endpoints own the same cell; the later ray wins. */
    while (left > 0 && daisies_edge_points[left-1].x == draw_x && daisies_edge_points[left-1].y == draw_y) --left;
    while (right < last && daisies_edge_points[right+1].x == draw_x && daisies_edge_points[right+1].y == draw_y) ++right;
    lx = left > 0 ? (draw_x+daisies_edge_points[left-1].x)*0.5 : draw_x;
    ly = left > 0 ? (draw_y+daisies_edge_points[left-1].y)*0.5 : draw_y;
    rx = right < last ? (draw_x+daisies_edge_points[right+1].x)*0.5 : draw_x;
    ry = right < last ? (draw_y+daisies_edge_points[right+1].y)*0.5 : draw_y;
    count = dz_native_count(draw_x,draw_y,0,0); step = 0;
}
static void draw(unsigned symmetry, unsigned char color)
{
    int sx = branch == 1 || branch == 3 ? -1 : 1, sy = branch == 1 || branch == 2 ? -1 : 1;
    int sample = step-1;
    double a,b; dz_native_bounds(sample,count,&a,&b); a = 1-a; b = 1-b;
    double x[] = {sx*lx*a,sx*rx*a,sx*rx*b,sx*lx*b};
    double y[] = {sy*ly*a,sy*ry*a,sy*ry*b,sy*ly*b};
    dz_native_quad_at(daisies_center_x,daisies_center_y,x,y,symmetry,color);
    /* The current ray still owns its pen, including its original overwrite
     * of nearby rays. The fan fill supplies only the between-ray surface. */
    dz_native_cell_at(daisies_center_x,daisies_center_y,sx*pen_x,sy*pen_y,0,0,
        sample,count,symmetry,color);
    if (++branch < 4) dz_native_queue(draw);
}
void dz_native_daisies_step(void)
{ if (enabled) { ++step; branch = 0; dz_native_queue(draw); } }
