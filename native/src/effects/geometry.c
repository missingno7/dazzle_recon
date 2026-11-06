#include "geometry.h"
#include "effects.h"
#include "dazzle.h"
#include "dzconst.h"
#include "platform/canvas.h"
#include <math.h>
#include <stdlib.h>

static struct dz_native_mapping mapping(void)
{ return dz_canvas_native_mapping(screen_half_width,screen_half_height); }
int dz_extended_reach(void)
{
    int w, h;
    dz_canvas_native_size(&w,&h);
    if (!w || dz_display.presentation != DZ_PRESENT_EXTENDED) return 0;
    struct dz_native_mapping m = mapping();
    return m.unit_x ? (int)ceil((w-m.cx)/m.unit_x)+1 : 0;
}
double dz_native_scale(void)
{ struct dz_native_mapping m = mapping(); return fmax(m.unit_x,m.unit_y); }
int dz_native_radius_x(int y)
{
    float aspect = (float)((double)screen_half_width/screen_half_height*0.75);
    double value = y*(double)aspect;
    int x = (int)value;
    return x+(value-x > 0.5);
}
void dz_native_pixel(double cx, double cy, double x, double y, double *px, double *py)
{ struct dz_native_mapping m = mapping(); *px = m.cx+(cx+x)*m.unit_x; *py = m.cy+(cy+y)*m.unit_y; }
void dz_native_line_caps_at(double cx, double cy, double ax, double ay, double bx, double by,
                            unsigned symmetry, uint8_t color, unsigned caps)
{
    struct dz_native_mapping m = mapping();
    if (!m.unit_x || !m.unit_y) return;
    int branches = symmetry & 1u ? 4 : 1;
    int swaps = (symmetry & 3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
        int sx = branch >= 2 ? -1 : 1, sy = branch & 1 ? -1 : 1;
        double xa = swap ? cy+ay : cx+ax, ya = swap ? cx+ax : cy+ay;
        double xb = swap ? cy+by : cx+bx, yb = swap ? cx+bx : cy+by;
        double vx = (swap ? by-ay : bx-ax)*m.unit_x/m.weight_x;
        double vy = (swap ? bx-ax : by-ay)*m.unit_y/m.weight_y;
        double length = hypot(vx,vy), weight = length ? fmax(fabs(vx),fabs(vy))/length : 1;
        dz_canvas_stroke_cell(m.cx+sx*xa*m.unit_x,m.cy+sy*ya*m.unit_y,
            m.cx+sx*xb*m.unit_x,m.cy+sy*yb*m.unit_y,m.weight_x*weight,m.weight_y*weight,color,caps);
    }
}
void dz_native_line_at(double cx, double cy, double ax, double ay, double bx, double by,
                       unsigned symmetry, uint8_t color)
{ dz_native_line_caps_at(cx,cy,ax,ay,bx,by,symmetry,color,3); }
void dz_native_line(double ax, double ay, double bx, double by, unsigned symmetry, uint8_t color)
{ dz_native_line_at(0,0,ax,ay,bx,by,symmetry,color); }
int dz_native_count(int ax, int ay, int bx, int by)
{
    int x = abs(bx-ax), y = abs(by-ay);
    return x > y ? x : y;
}
static void cell_span(double cx, double cy, double ax, double ay, double bx, double by,
                      int step, int count, unsigned symmetry, uint8_t color, double width, int cell_ends)
{
    struct dz_native_mapping m = mapping();
    if (!m.unit_x || !m.unit_y) return;
    double a, b; dz_native_bounds(step,count,&a,&b);
    double dx = bx-ax, dy = by-ay;
    double x0 = ax+dx*a, y0 = ay+dy*a, x1 = ax+dx*b, y1 = ay+dy*b;
    unsigned caps = cell_ends ? 0 : (step == 0 ? 1u : 0u) | (step >= count ? 2u : 0u);
    int branches = symmetry & 1u ? 4 : 1, swaps = (symmetry & 3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
        int sx = branch >= 2 ? -1 : 1, sy = branch & 1 ? -1 : 1;
        double xa = swap ? cy+y0 : cx+x0, ya = swap ? cx+x0 : cy+y0;
        double xb = swap ? cy+y1 : cx+x1, yb = swap ? cx+x1 : cy+y1;
        /* Bresenham chooses one minor-axis cell per major-axis sample.
         * Its continuous band is one cell wide on that minor axis. */
        double vx = (swap ? dy : dx)*m.unit_x/m.weight_x;
        double vy = (swap ? dx : dy)*m.unit_y/m.weight_y;
        double length = hypot(vx,vy), weight = length ? fmax(fabs(vx),fabs(vy))/length : 1;
        dz_canvas_stroke_cell(m.cx+sx*xa*m.unit_x,m.cy+sy*ya*m.unit_y,
            m.cx+sx*xb*m.unit_x,m.cy+sy*yb*m.unit_y,
            m.weight_x*weight*width,m.weight_y*weight*width,color,caps);
    }
}
void dz_native_cell_width_at(double cx, double cy, double ax, double ay, double bx, double by,
                             int step, int count, unsigned symmetry, uint8_t color, double width)
{ cell_span(cx,cy,ax,ay,bx,by,step,count,symmetry,color,width,0); }
void dz_native_cell_at(double cx, double cy, double ax, double ay, double bx, double by,
                       int step, int count, unsigned symmetry, uint8_t color)
{
    /* A line ends in the logical cell its last sample owns, so the ends of
     * neighbouring lines meet flush instead of as a row of round bumps. */
    cell_span(cx,cy,ax,ay,bx,by,step,count,symmetry,color,1,1);
    if (step == 0) dz_native_raster_cell(cx,cy,ax,ay,symmetry,color);
    if (step >= count) dz_native_raster_cell(cx,cy,bx,by,symmetry,color);
}
void dz_native_raster_cell(double cx, double cy, double x, double y,
                           unsigned symmetry, uint8_t color)
{
    const double px[4] = {x-0.5,x+0.5,x+0.5,x-0.5};
    const double py[4] = {y-0.5,y-0.5,y+0.5,y+0.5};
    dz_native_quad_at(cx,cy,px,py,symmetry,color);
}

static int accepted(int x, int y, int swap)
{
    int row = swap ? x : y;
    return video_backend != BACKEND_EGA_640X700 ||
        (row >= -screen_half_height && row <= screen_half_height &&
         !((screen_half_height+row)&1) && !((screen_half_height-row)&1));
}
void dz_native_sample_cell(int x, int y, double radius, unsigned symmetry,
                           dz_native_sample sample, void *context)
{
    struct dz_native_mapping m = mapping();
    int w,h; dz_canvas_native_size(&w,&h);
    if (!m.unit_x || !m.unit_y) return;
    SDL_FRect clip = dz_canvas_content();
    int clip_x0 = dz_display.presentation == DZ_PRESENT_EXTENDED ? 0 : (int)clip.x;
    int clip_x1 = dz_display.presentation == DZ_PRESENT_EXTENDED ? w : (int)(clip.x+clip.w);
    int clip_y0 = (int)clip.y, clip_y1 = (int)(clip.y+clip.h);
    int branches = symmetry&1u ? 4 : 1, swaps = (symmetry&3u) == 3u ? 2 : 1;
    double ry = video_backend == BACKEND_EGA_640X700 ? 2*radius : radius;
    for (int swap = 0; swap < swaps; ++swap) {
        if (!accepted(x,y,swap)) continue;
        for (int b = 0; b < branches; ++b) {
            int sx = b >= 2 ? -1 : 1, sy = b&1 ? -1 : 1;
            double cx = m.cx+sx*(swap ? y : x)*m.unit_x;
            double cy = m.cy+sy*(swap ? x : y)*m.unit_y;
            double rxp = radius*m.unit_x;
            double ryp = ry*m.unit_y;
            int x0 = (int)fmax(0,ceil(cx-rxp-0.5)), x1 = (int)fmin(w,ceil(cx+rxp-0.5));
            int y0 = (int)fmax(0,ceil(cy-ryp-0.5)), y1 = (int)fmin(h,ceil(cy+ryp-0.5));
            if (x0 < clip_x0) x0 = clip_x0;
            if (x1 > clip_x1) x1 = clip_x1;
            if (y0 < clip_y0) y0 = clip_y0;
            if (y1 > clip_y1) y1 = clip_y1;
            for (int py = y0; py < y1; ++py) {
                int start = x0, previous = -1;
                double ty = sy*(py+0.5-m.cy)/m.unit_y;
                for (int px = x0; px < x1; ++px) {
                    double tx = sx*(px+0.5-m.cx)/m.unit_x;
                    int color = sample(swap ? ty : tx,swap ? tx : ty,context);
                    if (color == previous) continue;
                    if (previous >= 0) dz_canvas_native_span(py,start,px,(uint8_t)previous);
                    start = px; previous = color;
                }
                if (previous >= 0) dz_canvas_native_span(py,start,x1,(uint8_t)previous);
            }
        }
    }
}
void dz_native_dot(int x, int y, unsigned symmetry, uint8_t color)
{
    struct dz_native_mapping m = mapping();
    /* Equal area to one actual raster cell, circular in physical pixels. */
    double area = m.unit_x*m.unit_y*(video_backend == BACKEND_EGA_640X700 ? 2 : 1);
    double diameter = 2*sqrt(area/3.14159265358979323846);
    int branches = symmetry&1u ? 4 : 1, swaps = (symmetry&3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) {
        if (!accepted(x,y,swap)) continue;
        for (int b = 0; b < branches; ++b) {
            double px = m.cx+(b >= 2 ? -1 : 1)*(swap ? y : x)*m.unit_x;
            double py = m.cy+(b&1 ? -1 : 1)*(swap ? x : y)*m.unit_y;
            dz_canvas_stroke(px,py,px,py,diameter,color);
        }
    }
}
void dz_native_row_cell_step(int x, int y, int left, int right, double dl, double dr,
                             double row_step, unsigned symmetry, uint8_t color)
{
    int enhanced = video_backend == BACKEND_EGA_640X700;
    int swaps = (symmetry&3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) {
        if (!accepted(x,y,swap)) continue;
        /* Parity is decided AFTER exchange. Its two-row cell lies on the
         * physical Y axis. Tiles additionally samples every second source Y. */
        double hx = enhanced && swap ? 1 : 0.5;
        double hy = row_step ? row_step*0.5 : enhanced && !swap ? 1 : 0.5;
        const double qx[4] = {x-hx-(x==left ? dl*hy : 0),
            x+hx-(x==right ? dr*hy : 0),x+hx+(x==right ? dr*hy : 0),
            x-hx+(x==left ? dl*hy : 0)};
        const double qy[4] = {y-hy,y-hy,y+hy,y+hy};
        dz_native_quad_at(0,0,swap ? qy : qx,swap ? qx : qy,symmetry&1u,color);
    }
}
void dz_native_row_cell(int x, int y, int left, int right, double dl, double dr,
                        unsigned symmetry, uint8_t color)
{ dz_native_row_cell_step(x,y,left,right,dl,dr,0,symmetry,color); }

void dz_native_bounds(int sample, int count, double *start, double *end)
{
    *start = count > 0 ? fmax(0,(sample-0.5)/count) : 0;
    *end = count > 0 ? fmin(1,(sample+0.5)/count) : 0;
}
double dz_native_fraction(int step, int count)
{ return step < 0 ? 0 : count <= 0 || step >= count ? 1 : (double)step/count; }
void dz_native_quad_at(double cx, double cy, const double x[4], const double y[4],
                       unsigned symmetry, uint8_t color)
{
    struct dz_native_mapping m = mapping();
    int branches = symmetry & 1u ? 4 : 1, swaps = (symmetry & 3u) == 3u ? 2 : 1;
    for (int swap = 0; swap < swaps; ++swap) for (int branch = 0; branch < branches; ++branch) {
        int sx = branch >= 2 ? -1 : 1, sy = branch & 1 ? -1 : 1;
        double px[4], py[4];
        for (int i = 0; i < 4; ++i) {
            px[i] = m.cx+sx*(swap ? cy+y[i] : cx+x[i])*m.unit_x;
            py[i] = m.cy+sy*(swap ? cx+x[i] : cy+y[i])*m.unit_y;
        }
        dz_canvas_native_quad(px,py,color);
    }
}
double dz_native_untruncated(double value)
{ return value > 0.5 ? value-0.5 : value < -0.5 ? value+0.5 : 0; }
void dz_native_path_prepare(struct dz_native_path *p, double x, double y, int reset)
{
    p->fresh = reset || !p->valid || p->page != page_paragraph;
    if (p->fresh) p->dx = p->dy = 0;
    p->ax = p->fresh ? x : p->bx; p->ay = p->fresh ? y : p->by;
    p->bx = x; p->by = y; p->page = page_paragraph; p->valid = 1;
}
void dz_native_path_draw(struct dz_native_path *p, unsigned symmetry, uint8_t color)
{
    /* Complete the older sample's half, then the current half. The leading
     * cap stays at the actual pen; no extrapolation or one-step colour lead. */
    cell_span(0,0,p->ax,p->ay,p->bx,p->by,0,1,symmetry,p->fresh ? color : p->color,1,0);
    cell_span(0,0,p->ax,p->ay,p->bx,p->by,1,1,symmetry,color,1,0);
    /* A turn in an integer walk keeps the square corner of its cell. */
    double dx = p->bx-p->ax, dy = p->by-p->ay;
    if (p->corners && !p->fresh && (dx || dy)) {
        if (dx*p->dx+dy*p->dy <= 0) dz_native_raster_cell(0,0,p->ax,p->ay,symmetry,p->color);
    }
    if (dx || dy) { p->dx = dx; p->dy = dy; }
    p->color = color;
}
void dz_native_path_draw_width(struct dz_native_path *p, unsigned symmetry,
                               uint8_t color, double width)
{
    dz_native_cell_width_at(0,0,p->ax,p->ay,p->bx,p->by,0,1,symmetry,
                            p->fresh ? color : p->color,width);
    dz_native_cell_width_at(0,0,p->ax,p->ay,p->bx,p->by,1,1,symmetry,color,width);
    p->color = color;
}
/* Along the stroke, from a band edge's offset to the corner's ray. A ray along
 * the stroke, or no ray at all, cuts nothing: the stroke keeps its cell's end. */
static double ring_cut(double ox, double oy, double ux, double uy, double rx, double ry, double end)
{
    double across = ux*ry-uy*rx;
    return across ? (oy*rx-ox*ry)/across : end;
}
void dz_native_ring_step(int x, int y, int step, int length, int side, int shift,
                         const int16_t *dx, const int16_t *dy,
                         unsigned symmetry, uint8_t color)
{
    double ux = dx[side], uy = dy[side], moves = length+1;
    double ax = x-(step+1)*ux, ay = y-(step+1)*uy;
    double rx[2] = {shift,shift}, ry[2] = {0,0};
    for (int i = 0; i < 4; ++i) {
        if (i <= ((side+3)&3)) { rx[0] += dx[i]; ry[0] += dy[i]; }
        if (i <= side) { rx[1] += dx[i]; ry[1] += dy[i]; }
    }
    /* A stroke's band is one cell across its minor axis in every mapping. */
    double ox = ux ? 0 : 0.5, oy = ux ? 0.5 : 0, qx[4], qy[4];
    for (int edge = 0; edge < 2; ++edge) {
        double sx = edge ? -ox : ox, sy = edge ? -oy : oy;
        double from = ring_cut(sx,sy,ux,uy,rx[0],ry[0],-0.5);
        double to = moves+ring_cut(sx,sy,ux,uy,rx[1],ry[1],0.5);
        /* The first rings are shorter than their cuts are wide. */
        if (to < from) from = to = (from+to)/2;
        double a = from+(to-from)*step/moves, b = from+(to-from)*(step+1)/moves;
        qx[edge ? 3 : 0] = ax+sx+a*ux; qy[edge ? 3 : 0] = ay+sy+a*uy;
        qx[edge ? 2 : 1] = ax+sx+b*ux; qy[edge ? 2 : 1] = ay+sy+b*uy;
    }
    dz_native_quad_at(0,0,qx,qy,symmetry,color);
}
double dz_native_quadrant(double progress, double rx, double ry)
{
    double length = hypot(rx,ry), split = rx*rx/length;
    return progress < split ? asin(fmin(1,progress/rx)) : acos(fmax(0,fmin(1,(length-progress)/ry)));
}
static double point_x, point_y, point_cx, point_cy;
static int point_at;
static void point_draw(unsigned symmetry, unsigned char color)
{
    if (point_at) dz_native_line_at(point_cx,point_cy,point_x,point_y,point_x,point_y,symmetry,color);
    else dz_native_line(point_x,point_y,point_x,point_y,symmetry,color);
}
void dz_native_point(unsigned effect, double x, double y)
{
    if (!dz_native_enabled(effect)) return;
    point_x = x; point_y = y; point_at = 0; dz_native_queue(point_draw);
}
void dz_native_point_at(unsigned effect, double cx, double cy, double x, double y)
{
    if (!dz_native_enabled(effect)) return;
    point_x = x; point_y = y; point_cx = cx; point_cy = cy; point_at = 1; dz_native_queue(point_draw);
}
