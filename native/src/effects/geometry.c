#include "geometry.h"
#include "effects.h"
#include "dazzle.h"
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
void dz_native_cell_width_at(double cx, double cy, double ax, double ay, double bx, double by,
                             int step, int count, unsigned symmetry, uint8_t color, double width)
{
    struct dz_native_mapping m = mapping();
    if (!m.unit_x || !m.unit_y) return;
    double a, b; dz_native_bounds(step,count,&a,&b);
    double dx = bx-ax, dy = by-ay;
    double x0 = ax+dx*a, y0 = ay+dy*a, x1 = ax+dx*b, y1 = ay+dy*b;
    unsigned caps = (step == 0 ? 1u : 0u) | (step >= count ? 2u : 0u);
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
void dz_native_cell_at(double cx, double cy, double ax, double ay, double bx, double by,
                       int step, int count, unsigned symmetry, uint8_t color)
{ dz_native_cell_width_at(cx,cy,ax,ay,bx,by,step,count,symmetry,color,1); }
void dz_native_raster_cell(double cx, double cy, double x, double y,
                           unsigned symmetry, uint8_t color)
{
    const double px[4] = {x-0.5,x+0.5,x+0.5,x-0.5};
    const double py[4] = {y-0.5,y-0.5,y+0.5,y+0.5};
    dz_native_quad_at(cx,cy,px,py,symmetry,color);
}
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
    p->ax = p->fresh ? x : p->bx; p->ay = p->fresh ? y : p->by;
    p->bx = x; p->by = y; p->page = page_paragraph; p->valid = 1;
}
void dz_native_path_draw(struct dz_native_path *p, unsigned symmetry, uint8_t color)
{
    /* Complete the older sample's half, then the current half. The leading
     * cap stays at the actual pen; no extrapolation or one-step colour lead. */
    dz_native_cell_at(0,0,p->ax,p->ay,p->bx,p->by,0,1,symmetry,p->fresh ? color : p->color);
    dz_native_cell_at(0,0,p->ax,p->ay,p->bx,p->by,1,1,symmetry,color);
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
