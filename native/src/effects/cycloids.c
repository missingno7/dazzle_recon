/* The original joins its rounded 64-entry table vertices with straight lines.
 * Retain those vertices; a trigonometric re-evaluation is a different figure. */
#include "cycloids.h"
#include "effects.h"
#include "geometry.h"
static int enabled, count, step;
static double x0, y0, x1, y1;
static void draw(unsigned symmetry, unsigned char color)
{ dz_native_cell_at(0,0,x0,y0,x1,y1,step++,count,symmetry,color); }
void dz_native_cycloids_begin(int ax, int ay, int bx, int by)
{
    enabled = dz_native_enabled(DZ_EFFECT_CYCLOIDS);
    count = dz_native_count(ax,ay,bx,by); step = 0;
    x0 = ax; y0 = ay; x1 = bx; y1 = by;
}
void dz_native_cycloids_step(void) { if (enabled) dz_native_queue(draw); }
