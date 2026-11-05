/* Native polygons reconstruct their edges and colour-distance bands. */
#include "polygons.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "dzconst.h"
#include "platform/canvas.h"
#include <math.h>
#include <stdlib.h>

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
{ dz_native_dot(draw_x,draw_y,symmetry,color); }

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

/* The colour clock samples distance from the real left edge, rather than
 * from a Bresenham row's staircase. Row bands remain horizontal bands. */
static double fill_left(double y)
{
    double lo = HUGE_VAL;
    for (int i = 0; i < polygon.count; ++i) {
        int j = (i+1)%polygon.count;
        double ay = polygon.origin_y+polygon.y[i], by = polygon.origin_y+polygon.y[j];
        if (y < fmin(ay,by) || y >= fmax(ay,by)) continue;
        double x = polygon.origin_x+polygon.x[i]+(polygon.x[j]-polygon.x[i])*(y-ay)/(by-ay);
        if (x < lo) lo = x;
    }
    return lo;
}
struct fill_span { int first,end; double y0,dy,x0,dx; };
static int integer_compare(const void *a, const void *b)
{ return (*(const int *)a > *(const int *)b)-(*(const int *)a < *(const int *)b); }
static void fill_native(void)
{
    if (!(polygon.flags & SHAPE_FILL) || polygon.count < 3) return;
    struct dz_native_mapping m = dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int width,height; dz_canvas_native_size(&width,&height);
    if (!m.unit_x || !m.unit_y) return;
    struct polygon_copy copies[POLYGON_COPIES];
    int count = make_copies(copies), mode = polygon.flags & POLYGON_FILL_COLOR_MASK;
    double low = HUGE_VAL, high = -HUGE_VAL;
    for (int c = 0; c < count; ++c) for (int i = 0; i < polygon.count; ++i) {
        double x,y; transformed(&m,copies[c],i,&x,&y);
        low = fmin(low,y); high = fmax(high,y);
    }
    for (int y = (int)fmax(0,ceil(low-0.5)); y < (int)fmin(height,ceil(high-0.5)); ++y) {
        struct fill_span spans[POLYGON_COPIES];
        /* Edges, vertex changes and pairwise painter-order crossings partition
         * the row. Choose its winner once per interval, not once per pixel. */
        int cuts[256], n = 0;
        double ty = (y+0.5-m.cy)/m.unit_y;
        for (int c = 0; c < count; ++c) {
            struct polygon_copy k = copies[c];
            double l,r;
            spans[c] = (struct fill_span){0};
            if (!span_at_y(&m,k,y+0.5,&l,&r)) continue;
            int a = (int)fmax(0,ceil(l-0.5)), b = (int)fmin(width,ceil(r-0.5));
            if (a >= b) continue;
            spans[c] = (struct fill_span){a,b,k.swap ? -k.sx*m.cx/m.unit_x : k.sy*ty,
                k.swap ? k.sx/m.unit_x : 0,k.swap ? k.sy*ty : -k.sx*m.cx/m.unit_x,
                k.swap ? 0 : k.sx/m.unit_x};
            cuts[n++] = a; cuts[n++] = b;
            if (k.swap) for (int i = 0; i < polygon.count; ++i) {
                int cut = (int)ceil(m.cx+k.sx*(polygon.origin_y+polygon.y[i])*m.unit_x-0.5);
                if (cut > a && cut < b) cuts[n++] = cut;
            }
        }
        for (int a = 0; a < count; ++a) for (int b = a+1; b < count; ++b) {
            double d = spans[a].dy-spans[b].dy;
            double delta = spans[b].y0-spans[a].y0;
            if (!d && !delta) { d = spans[a].dx-spans[b].dx; delta = spans[b].x0-spans[a].x0; }
            if (!d) continue;
            int cut = (int)ceil(delta/d-0.5);
            if (cut > 0 && cut < width) cuts[n++] = cut;
        }
        qsort(cuts,(size_t)n,sizeof cuts[0],integer_compare);
        for (int i = 0; i+1 < n; ++i) {
            int first = cuts[i], end = cuts[i+1], winner = -1;
            if (first >= end) continue;
            double mid = (first+end)*0.5, wy = -HUGE_VAL, wx = -HUGE_VAL;
            for (int c = 0; c < count; ++c) {
                struct fill_span *s = spans+c;
                if (mid < s->first || mid >= s->end) continue;
                double by = s->y0+s->dy*mid, bx = s->x0+s->dx*mid;
                if (by > wy || (by == wy && bx >= wx)) { winner = c; wy = by; wx = bx; }
            }
            if (winner < 0) continue;
            if (!mode) { dz_canvas_native_span(y,first,end,(uint8_t)polygon.fill_color); continue; }
            struct fill_span *s = spans+winner;
            double base_y = s->y0+s->dy*(first+0.5);
            double base_x = s->x0+s->dx*(first+0.5);
            double value, step;
            if (mode == POLYGON_FILL_COLOR_PER_ROW) {
                value = base_y-(polygon.origin_y+polygon_min_y)+0.5; step = s->dy;
            } else {
                double left = fill_left(base_y);
                double next = fill_left(base_y+s->dy*(end-first-1));
                if (!isfinite(left)) continue;
                if (!isfinite(next)) next = left;
                value = base_x-left+0.5;
                step = s->dx-(end-first > 1 ? (next-left)/(end-first-1) : 0);
            }
            int start = first; unsigned previous = advance(polygon.fill_color,(unsigned)fmax(0,floor(value)));
            for (int x = first+1; x < end; ++x) {
                value += step;
                unsigned color = advance(polygon.fill_color,(unsigned)fmax(0,floor(value)));
                if (color == previous) continue;
                dz_canvas_native_span(y,start,x,(uint8_t)previous); start = x; previous = color;
            }
            dz_canvas_native_span(y,start,end,(uint8_t)previous);
        }
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
