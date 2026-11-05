#include "pens.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"
#include "platform/canvas.h"
#include <math.h>
#include <stdlib.h>

extern uint8_t grid_patterns[256];
static unsigned cell_effect;

static int formula_sample(double x, double y, void *context)
{
    (void)context;
    /* The triangular scan and exchanged copies sample one symmetric field.
     * Its extra inclusive row overlaps the diagonal. Selecting orientation by
     * the last logical cell leaves a staircase there even with real samples. */
    x = fabs(x); y = fabs(y);
    if (y > x) { double swap = x; x = y; y = swap; }
    /* Each intermediate DOS operation is modulo 65536. Both the final
     * power-of-two shift and low palette mask divide that modulus, so the
     * discarded wraps cancel exactly, also for fractional coordinates. */
    double value = y*y*formula1_y_square_factor-x*(1u<<formula1_x_shift);
    return (int64_t)floor(value/(1u<<formula1_color_shift)) & color_index_mask;
}
static int bitmap(int x, int y)
{
    return x < 0 || y < 0 || x >= ball_bitmap_width || y >= ball_bitmap_height ? 0 :
        ball_bitmap_pixels[x*ball_bitmap_height+y];
}
/* The "bitmap" is initialize_palette_mapping's circle, shaded by distance from
 * an off-centre point. Evaluate that at real bitmap coordinates; its integer
 * samples are the table (mapping axes are exchanged: width holds the height). */
static struct { int half_x, half_y, radius, step; double limit_x, limit_y; } ball;
static void ball_setup(void)
{
    ball.half_x = ball_bitmap_width/2; ball.half_y = ball_bitmap_height/2;
    ball.limit_x = ball.half_x+ball.half_x/4; ball.limit_y = ball.half_y+ball.half_y/4;
    ball.radius = (ball.half_x < ball.half_y ? ball.half_x : ball.half_y)-1;
    ball.step = video_backend == BACKEND_EGA_640X700 ? 2 : 1;
}
static int ball_ring(double x, double y)
{
    double dx = x-ball.limit_x, dy = y-ball.limit_y;
    int color = (int)sqrt(dx*dx+dy*dy)%253;
    return (color_count < 0x80 ? color>>1 : color)+FIRST_CYCLING_COLOR;
}
int dz_native_ball_field(double x, double y)
{
    ball_setup();
    double dx = x-ball.half_x, dy = y-ball.half_y;
    return dx*dx+dy*dy > (double)ball.radius*ball.radius ? 0 : ball_ring(x,y);
}
static int ball_plotted(int nx, int ny)
{
    return bitmap(balls_sample_x+nx*effect_parameter,
                  balls_sample_y+ny*ball.step*effect_parameter) != 0;
}
/* Original traversal skips samples outside the circle, so the disc between
 * the last plotted sample and the real outline goes to its nearest plotted cell. */
static int ball_outline(void)
{
    for (int ny = -1; ny <= 1; ++ny) for (int nx = -1; nx <= 1; ++nx)
        if (!ball_plotted(nx,ny)) return 1;
    return 0;
}
static int ball_inner(double x, double y, void *context)
{
    (void)context;
    return ball_ring(balls_sample_x+(x-draw_x)*effect_parameter,
                     balls_sample_y+(y-draw_y)*effect_parameter) & color_index_mask;
}
static int ball_edge(double x, double y, void *context)
{
    (void)context;
    double dx = x-draw_x, dy = y-draw_y;
    int nx = (int)floor(dx/ball.step+0.5), ny = (int)floor(dy/ball.step+0.5);
    if ((nx || ny) && ball_plotted(nx*ball.step,ny)) return -1;
    int color = dz_native_ball_field(balls_sample_x+dx*effect_parameter,
                                     balls_sample_y+dy*effect_parameter);
    return color ? color & color_index_mask : -1;
}
static int mask_at(int x, int y)
{
    /* Neighbouring tiles use the same original layout, including crossings. */
    int tx = (int)floor(x/8.0)*8, ty = (int)floor(y/8.0)*8;
    int pattern = ((tx+ty)&0x18)<<3;
    return grid_patterns[pattern+(y-ty)*8+x-tx] == 0;
}
/* A bilinear mask at fixed Y is linear in X. Intersect its threshold and
 * nearest-plotted-sample half planes analytically, then fill one output span.
 * This is the same surface as the sampler trial, without per-pixel mask fetches. */
static void grid_draw(unsigned symmetry, unsigned char color)
{
    struct dz_native_mapping m = dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int w,h; dz_canvas_native_size(&w,&h);
    if (!m.unit_x || !m.unit_y) return;
    int doubled = video_backend == BACKEND_EGA_640X700;
    int gx = draw_x, gy = doubled ? draw_y/2 : draw_y;
    int branches = symmetry&1u ? 4 : 1, swaps = (symmetry&3u) == 3u ? 2 : 1;
    for (int yy = gy-1; yy <= gy; ++yy) for (int xx = gx-1; xx <= gx; ++xx) {
        int c[4] = {mask_at(xx,yy),mask_at(xx+1,yy),mask_at(xx,yy+1),mask_at(xx+1,yy+1)};
        for (int swap = 0; swap < swaps; ++swap) {
            if (doubled && swap && ((screen_half_height+draw_x)&1)) continue;
            int v[4] = {c[0],c[swap ? 2 : 1],c[swap ? 1 : 2],c[3]};
            double base_x = swap ? yy : xx, base_y = swap ? xx : yy;
            int own_x = swap ? gy-yy : gx-xx, own_y = swap ? gx-xx : gy-yy;
            double ux = m.unit_x*(doubled && swap ? 2 : 1);
            double uy = m.unit_y*(doubled && !swap ? 2 : 1);
            for (int branch = 0; branch < branches; ++branch) {
                int sx = branch >= 2 ? -1 : 1, sy = branch&1 ? -1 : 1;
                /* Adjacent patches must compute their shared cut from the
                 * same anchor, including exact physical pixel-centre ties. */
                double ay = m.cy+sy*base_y*uy, by = m.cy+sy*(base_y+1)*uy;
                int first = (int)fmax(0,ceil(fmin(ay,by)-0.5));
                int end = (int)fmin(h,ceil(fmax(ay,by)-0.5));
                for (int y = first; y < end; ++y) {
                    double fy = (y+0.5-m.cy)/(sy*uy)-base_y, lo = 0, hi = 1;
                    double a = v[0]+(v[2]-v[0])*fy;
                    double slope = v[1]-v[0]+(v[3]-v[2]-v[1]+v[0])*fy;
                    if (slope > 0) lo = fmax(lo,(0.5-a)/slope);
                    else if (slope < 0) hi = fmin(hi,(0.5-a)/slope);
                    else if (a < 0.5) continue;
                    for (int i = 0; i < 4; ++i) if (v[i]) {
                        int cx = i&1, cy = i>>1;
                        if (doubled && swap && ((xx+cy)&1)) continue;
                        double k = 2*(cx-own_x);
                        double rhs = cx+cy-own_x-own_y-2*fy*(cy-own_y);
                        if (k > 0) hi = fmin(hi,rhs/k);
                        else if (k < 0) lo = fmax(lo,rhs/k);
                        else if (rhs < 0) { hi = lo; break; }
                    }
                    if (lo >= hi) continue;
                    double ax = m.cx+sx*(base_x+lo)*ux, bx = m.cx+sx*(base_x+hi)*ux;
                    dz_canvas_native_span(y,(int)ceil(fmin(ax,bx)-0.5),
                        (int)ceil(fmax(ax,bx)-0.5),color);
                }
            }
        }
    }
}

void dz_native_field_cell(int formula, int x, int y, unsigned symmetry, unsigned char color)
{
    if (formula) dz_native_sample_cell(x,y,0.5,symmetry,formula_sample,NULL);
    else dz_native_row_cell(x,y,0,tunnel_right_edge_ptr == &draw_y ? y : tunnel_right_limit,
        0,tunnel_right_edge_ptr == &draw_y ? 1 : 0,symmetry,color);
}

static void cell(unsigned symmetry, unsigned char color)
{
    switch (cell_effect) {
    case DZ_EFFECT_FORMULA1: dz_native_sample_cell(draw_x,draw_y,0.5,symmetry,formula_sample,NULL); break;
    case DZ_EFFECT_BALLS:
        /* Once the walk leaves the bitmap, DOS re-plots the last cell unchanged. */
        if (balls_sample_x >= ball_bitmap_width || balls_sample_y >= ball_bitmap_height) break;
        ball_setup();
        if (ball_outline()) dz_native_sample_cell(draw_x,draw_y,1.5,symmetry,ball_edge,NULL);
        else dz_native_sample_cell(draw_x,draw_y,0.5,symmetry,ball_inner,NULL);
        break;
    case DZ_EFFECT_GRID: grid_draw(symmetry,color); break;
    default: dz_native_dot(draw_x,draw_y,symmetry,color); break;
    }
}

void dz_native_samples(unsigned effect)
{
    dz_native_plot = NULL;
    cell_effect = effect;
    if (dz_native_enabled(effect)) dz_native_queue(cell);
}

static int row_left, row_right;
static double row_dl, row_dr;
static void row(unsigned symmetry, unsigned char color)
{ dz_native_row_cell(draw_x,draw_y,row_left,row_right,row_dl,row_dr,symmetry,color); }
void dz_native_rows(unsigned effect, int left, int right, double dl, double dr)
{
    dz_native_plot = NULL;
    if (!dz_native_enabled(effect)) return;
    row_left = left; row_right = right; row_dl = dl; row_dr = dr;
    dz_native_queue(row);
}

static int line_valid, line_x, line_y;
static uint16_t line_page;
static double line_slope;
static void line_pen(unsigned symmetry, unsigned char color)
{
    /* A vertical pen translated by (1,k) sweeps a parallelogram. Each of
     * its four scheduled samples reveals its own strip, including at bounces. */
    /* The four scheduled software samples describe one continuous pen.
     * Enhanced EGA's alternating accepted rows shift its quantized footprint;
     * carrying that parity into Native makes a sawtooth edge. The logical
     * plotter still rejects those rows; geometric ink uses the real pen. */
    const double x[4] = {draw_x-0.5,draw_x+0.5,draw_x+0.5,draw_x-0.5};
    const double y[4] = {draw_y-0.5-line_slope*0.5,draw_y-0.5+line_slope*0.5,
        draw_y+0.5+line_slope*0.5,draw_y+0.5-line_slope*0.5};
    dz_native_quad_at(0,0,x,y,symmetry,color);
}
void dz_native_lines(int reset)
{
    dz_native_plot = NULL;
    if (reset || line_page != page_paragraph) line_valid = 0;
    if (!dz_native_enabled(DZ_EFFECT_LINES)) { line_valid = 0; return; }
    if (!lines_stroke_index || !line_valid) {
        line_slope = line_valid && abs(draw_x-line_x) == 1 ?
            (double)(lines_base_y-line_y)/(draw_x-line_x) : 0;
        line_x = draw_x; line_y = lines_base_y; line_page = page_paragraph; line_valid = 1;
    }
    dz_native_queue(line_pen);
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
