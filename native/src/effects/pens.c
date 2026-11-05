#include "pens.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"

void dz_native_plot_cell(int x, int y, unsigned symmetry, unsigned char color)
{
    if (video_backend != BACKEND_EGA_640X700) {
        dz_native_raster_cell(0,0,x,y,symmetry,color);
        return;
    }
    /* The enhanced backend has 700 coordinate rows but 350 raster rows.
     * The exchanged copy has its own parity decision, as in DVDT. */
    int swaps = (symmetry&3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) {
        int px = swap ? y : x, py = swap ? x : y;
        if (py < -screen_half_height || py > screen_half_height ||
            ((screen_half_height+py)&1) || ((screen_half_height-py)&1)) continue;
        const double qx[4] = {px-0.5,px+0.5,px+0.5,px-0.5};
        const double qy[4] = {py-1.0,py-1.0,py+1.0,py+1.0};
        dz_native_quad_at(0,0,qx,qy,symmetry&1u,color);
    }
}

static void cell(unsigned symmetry, unsigned char color)
{ dz_native_plot_cell(draw_x,draw_y,symmetry,color); }

void dz_native_cells(unsigned effect)
{
    dz_native_plot = NULL;
    if (dz_native_enabled(effect)) dz_native_queue(cell);
}

static double pen_x, pen_y, pen_bx, pen_by;
static int pen_step, pen_count;
/* The short pen sweeps a real curve. Join corresponding samples on successive
 * pens, never the end of one pen to the beginning of the next. */
static struct dz_native_path curve_paths[3][16];
static struct dz_native_path *curve_path;
static int curve_dx, curve_dy;
static void curve_strip(unsigned symmetry, unsigned char color)
{
    struct dz_native_path *p = curve_path;
    if (!p->fresh) {
        /* A moving short pen sweeps a surface. Corresponding sample strips
         * share their edges; separate round tracks leave a dotted mesh where
         * the polar tangent crosses the diagonal pen. Split at the same
         * temporal midpoint used by curve strokes for old/new colours. */
        double mx = (p->ax+p->bx)*0.5, my = (p->ay+p->by)*0.5;
        const double a[4] = {p->ax-curve_dx*0.5,p->ax+curve_dx*0.5,
            mx+curve_dx*0.5,mx-curve_dx*0.5};
        const double b[4] = {p->ay-curve_dy*0.5,p->ay+curve_dy*0.5,
            my+curve_dy*0.5,my-curve_dy*0.5};
        const double x[4] = {mx-curve_dx*0.5,mx+curve_dx*0.5,
            p->bx+curve_dx*0.5,p->bx-curve_dx*0.5};
        const double y[4] = {my-curve_dy*0.5,my+curve_dy*0.5,
            p->by+curve_dy*0.5,p->by-curve_dy*0.5};
        dz_native_quad_at(0,0,a,b,symmetry,p->color);
        dz_native_quad_at(0,0,x,y,symmetry,color);
    }
    dz_native_path_draw(p,symmetry,color);
}
static void pen(unsigned symmetry, unsigned char color)
{
    if (curve_path) curve_strip(symmetry,color);
    dz_native_cell_at(0,0,pen_x,pen_y,pen_bx,pen_by,pen_step,pen_count,symmetry,color);
}
void dz_native_pen(unsigned effect, double x, double y, int dx, int dy,
                   int step, int count)
{
    dz_native_plot = NULL;
    curve_path = NULL;
    if (!dz_native_enabled(effect)) return;
    pen_x = x; pen_y = y; pen_bx = x+dx*count; pen_by = y+dy*count;
    pen_step = step; pen_count = count;
    dz_native_queue(pen);
}

void dz_native_curve_pen(unsigned effect, int reset, double x, double y,
                         int dx, int dy, int step, int count)
{
    unsigned slot = effect == DZ_EFFECT_ATOM ? 0 : effect == DZ_EFFECT_CURVES ? 1 : 2;
    if (reset || !dz_native_enabled(effect))
        for (int i = 0; i < 16; ++i) curve_paths[slot][i].valid = 0;
    dz_native_pen(effect,x,y,dx,dy,step,count);
    if (!dz_native_enabled(effect)) return;
    curve_path = &curve_paths[slot][step];
    curve_dx = dx; curve_dy = dy;
    dz_native_path_prepare(curve_path,x+dx*step,y+dy*step,0);
}

void dz_native_diagonal(unsigned effect, int rising)
{
    /* Each wrap starts a separate straight diagonal. Its samples partition
     * a cell-wide band; neighbouring diagonals meet without round ends. */
    int slope = rising ? 1 : -1;
    int before = rising ? draw_y : screen_extent-draw_y;
    int after = rising ? screen_extent-draw_y : draw_y;
    if (before > draw_x) before = draw_x;
    if (after > screen_extent-draw_x) after = screen_extent-draw_x;
    dz_native_pen(effect,draw_x-before,draw_y-slope*before,1,slope,before,before+after);
}
