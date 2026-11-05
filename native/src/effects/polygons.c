/* Native fills retain the logical colour cells and follow the original straight edges. */
#include "polygons.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"
#include "platform/canvas.h"
#include <math.h>

extern int16_t screen_half_width, screen_half_height;
extern uint16_t color_count, draw_symmetry;
extern int16_t polygon_min_y, polygon_row_limit;
extern int16_t polygon_row_left[350], polygon_row_right[350];

#define POLYGON_VERTICES 8
#define POLYGON_COPIES 8

struct polygon_copy { int sx, sy, swap; };
struct polygon_state {
    int active, count, origin_x, origin_y, symmetry;
    int x[POLYGON_VERTICES], y[POLYGON_VERTICES];
    unsigned fill_color, edge_color, flags, colors;
};
static struct polygon_state polygon;

static unsigned advance(unsigned color, unsigned count)
{
    if (count == 0) return color;
    if (polygon.colors <= FIRST_CYCLING_COLOR) return color;
    if (color < FIRST_CYCLING_COLOR) {
        unsigned reserved = FIRST_CYCLING_COLOR-color;
        if (count < reserved) return color+count;
        count -= reserved;
        color = FIRST_CYCLING_COLOR;
    }
    return FIRST_CYCLING_COLOR+
        ((color-FIRST_CYCLING_COLOR+count)%(polygon.colors-FIRST_CYCLING_COLOR));
}

static void outer_cell(unsigned symmetry, unsigned char color)
{ dz_native_raster_cell(0,0,draw_x,draw_y,symmetry,color); }

int dz_native_polygon_begin(unsigned effect, int count,
                            const int16_t *vertex_x, const int16_t *vertex_y,
                            uint16_t fill_color, uint16_t edge_color, uint16_t flags)
{
    polygon.active = dz_native_enabled(effect);
    if (!polygon.active) return 0;
    if (count < 3 || count >= POLYGON_VERTICES) {
        polygon.active = 0;
        return 0;
    }
    polygon.count = count;
    polygon.origin_x = vertex_x[0]; polygon.origin_y = vertex_y[0];
    polygon.x[0] = polygon.y[0] = 0;
    for (int i = 1; i < count; ++i) {
        polygon.x[i] = vertex_x[i]; polygon.y[i] = vertex_y[i];
    }
    polygon.symmetry = draw_symmetry;
    polygon.fill_color = fill_color;
    polygon.edge_color = edge_color;
    polygon.flags = flags;
    polygon.colors = color_count;
    dz_native_plot = NULL;
    dz_canvas_suppress(1);
    return 1;
}

static int make_copies(struct polygon_copy copies[POLYGON_COPIES])
{
    int branches = polygon.symmetry & 1 ? 4 : 1;
    int swaps = (polygon.symmetry & 3) == 3 ? 2 : 1;
    int n = 0;
    for (int swap = 0; swap < swaps; ++swap)
        for (int branch = 0; branch < branches; ++branch)
            copies[n++] = (struct polygon_copy){branch >= 2 ? -1 : 1,
                branch & 1 ? -1 : 1,swap};
    return n;
}

static void transformed(const struct dz_native_mapping *m, struct polygon_copy c,
                        int i, double *x, double *y)
{
    double px = polygon.origin_x+polygon.x[i];
    double py = polygon.origin_y+polygon.y[i];
    *x = m->cx+c.sx*(c.swap ? py : px)*m->unit_x;
    *y = m->cy+c.sy*(c.swap ? px : py)*m->unit_y;
}

static int span_at_y(const struct dz_native_mapping *m, struct polygon_copy c,
                     double py, double *left, double *right)
{
    double low = HUGE_VAL, high = -HUGE_VAL;
    for (int i = 0; i < polygon.count; ++i) {
        int j = (i+1)%polygon.count;
        double ax, ay, bx, by;
        transformed(m,c,i,&ax,&ay); transformed(m,c,j,&bx,&by);
        if (py < fmin(ay,by) || py >= fmax(ay,by)) continue;
        double x = ax+(bx-ax)*(py-ay)/(by-ay);
        low = fmin(low,x); high = fmax(high,x);
    }
    if (low == HUGE_VAL) return 0;
    *left = low; *right = high;
    return 1;
}

static unsigned sample_fill_color(int mode, int row, int cell_x)
{
    if (mode == POLYGON_FILL_COLOR_PER_ROW) return advance(polygon.fill_color,(unsigned)row);
    if (mode == POLYGON_FILL_COLOR_PER_PIXEL) {
        int left = polygon_row_left[row]+polygon.origin_x;
        int right = polygon_row_right[row]+polygon.origin_x;
        if (cell_x < left) cell_x = left;
        if (cell_x > right) cell_x = right;
        return advance(polygon.fill_color,(unsigned)(cell_x-left));
    }
    return polygon.fill_color;
}

static void fill_native(void)
{
    if (!(polygon.flags & SHAPE_FILL) || polygon.count < 3) return;
    struct dz_native_mapping m = dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int width, height; dz_canvas_native_size(&width,&height);
    if (!m.unit_x || !m.unit_y || !width || !height) return;
    struct polygon_copy copies[POLYGON_COPIES];
    int count = make_copies(copies);
    double low_y = HUGE_VAL, high_y = -HUGE_VAL;
    for (int c = 0; c < count; ++c) for (int i = 0; i < polygon.count; ++i) {
        double x,y; transformed(&m,copies[c],i,&x,&y);
        low_y = fmin(low_y,y); high_y = fmax(high_y,y);
    }
    int first_y = (int)fmax(0,ceil(low_y-0.5));
    int end_y = (int)fmin(height,ceil(high_y-0.5));
    int first_row = polygon.origin_y+polygon_min_y;
    int fill_mode = polygon.flags & POLYGON_FILL_COLOR_MASK;

    for (int y = first_y; y < end_y; ++y) {
        double py = y+0.5, left[POLYGON_COPIES], right[POLYGON_COPIES];
        int first[POLYGON_COPIES], end[POLYGON_COPIES], any = 0;
        int x0 = width, x1 = 0;
        for (int c = 0; c < count; ++c) {
            if (!span_at_y(&m,copies[c],py,&left[c],&right[c])) {
                first[c] = end[c] = 0; continue;
            }
            first[c] = (int)fmax(0,ceil(left[c]-0.5));
            end[c] = (int)fmin(width,ceil(right[c]-0.5));
            if (first[c] < end[c]) {
                if (first[c] < x0) x0 = first[c];
                if (end[c] > x1) x1 = end[c];
                any = 1;
            }
        }
        if (!any) continue;
        int run_start = x0, run_color = -1;
        for (int x = x0; x < x1; ++x) {
            int winner = -1, win_y = 0, win_x = 0, win_order = -1, color = -1;
            double tx = (x+0.5-m.cx)/m.unit_x, ty = (py-m.cy)/m.unit_y;
            for (int c = 0; c < count; ++c) {
                if (x < first[c] || x >= end[c]) continue;
                double base_x = copies[c].swap ? copies[c].sy*ty : copies[c].sx*tx;
                double base_y = copies[c].swap ? copies[c].sx*tx : copies[c].sy*ty;
                int cell_x = (int)floor(base_x+0.5), cell_y = (int)floor(base_y+0.5);
                int row = cell_y-first_row;
                if (row < 0 || row > polygon_row_limit || row >= 350) continue;
                int lo = polygon_row_left[row], hi = polygon_row_right[row];
                if (lo > hi) continue;
                int abs_lo = lo+polygon.origin_x, abs_hi = hi+polygon.origin_x;
                if (cell_x < abs_lo) cell_x = abs_lo;
                if (cell_x > abs_hi) cell_x = abs_hi;
                if (winner < 0 || cell_y > win_y ||
                    (cell_y == win_y && (cell_x > win_x ||
                     (cell_x == win_x && c > win_order)))) {
                    winner = c; win_y = cell_y; win_x = cell_x; win_order = c;
                    color = (int)sample_fill_color(fill_mode,row,cell_x);
                }
            }
            if (color != run_color) {
                if (run_color >= 0) dz_canvas_native_span(y,run_start,x,(uint8_t)run_color);
                run_start = x; run_color = color;
            }
            if (winner < 0) {
                if (run_color >= 0) dz_canvas_native_span(y,run_start,x,(uint8_t)run_color);
                run_color = -1; run_start = x+1;
            }
        }
        if (run_color >= 0) dz_canvas_native_span(y,run_start,x1,(uint8_t)run_color);
    }
}

static void outline_native(void)
{
    if (!(polygon.flags & SHAPE_OUTLINE)) return;
    int color_mode = (polygon.flags & POLYGON_EDGE_COLOR_PER_POINT) != 0;
    unsigned color_step = 0;
    for (int i = 0; i < polygon.count; ++i) {
        int j = (i+1)%polygon.count;
        int ax = polygon.origin_x+polygon.x[i], ay = polygon.origin_y+polygon.y[i];
        int bx = polygon.origin_x+polygon.x[j], by = polygon.origin_y+polygon.y[j];
        int count = dz_native_count(ax,ay,bx,by);
        for (int step = 0; step <= count; ++step) {
            unsigned color = color_mode ? advance(polygon.edge_color,color_step) : polygon.edge_color;
            dz_native_cell_at(0,0,ax,ay,bx,by,step,count,polygon.symmetry,(uint8_t)color);
            ++color_step;
        }
    }
}

void dz_native_polygon_end(int active)
{
    if (!active) return;
    dz_canvas_suppress(0);
    fill_native();
    outline_native();
    dz_native_queue(outer_cell);
}
