/* Follow the real along/across combinations the original truncates before
 * adding its integer origin. */
#include "corkscrew.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "platform/canvas.h"
#include "reach.h"
#include <math.h>
static struct dz_native_path path;
static void draw(unsigned symmetry, unsigned char color)
{ dz_native_path_draw(&path,symmetry,color); }
void dz_native_corkscrew_prepare(int rebuild, double x, double y)
{
    if (!dz_native_enabled(DZ_EFFECT_CORKSCREW)) { path.valid = 0; return; }
    dz_native_path_prepare(&path,corkscrew_origin_x+dz_native_untruncated(x),
                           corkscrew_origin_y+dz_native_untruncated(y),rebuild);
    dz_native_queue(draw);
}

/* The first excursion beyond the restart gate is still the same helix.
 * Advance it alongside the new logical coil, one sample per original plot;
 * stop when it curls back to that gate, before it can revisit the middle. */
static struct {
    int active, ox, oy, extent, counter;
    float phase, distance, growth;
    unsigned color, symmetry;
    uint16_t page;
    struct dz_native_path path;
} tail;
static void extend(unsigned symmetry, unsigned char color)
{
    (void)color;
    if (!tail.active || tail.page != page_paragraph) return;
    tail.symmetry = symmetry;
    float along = (float)(tail.distance*(1-cos(tail.phase)/4));
    float across = (float)(((double)tail.distance*tail.distance/tail.extent)*sin(tail.phase));
    int x = tail.ox+(int16_t)((double)along-across);
    int y = tail.oy+(int16_t)((double)along+across);
    if (x <= tail.extent) { tail.active = 0; return; }
    tail.phase = (float)(tail.phase+0.02);
    tail.distance = (float)((double)tail.distance+tail.growth);
    if (!(tail.counter = (tail.counter+1)&15) && ++tail.color >= color_count) tail.color = 3;
    if (dz_extended_native(DZ_EFFECT_CORKSCREW)) {
        dz_native_path_prepare(&tail.path,tail.ox+dz_native_untruncated((double)along-across),
            tail.oy+dz_native_untruncated((double)along+across),0);
        dz_native_path_draw(&tail.path,symmetry,(unsigned char)tail.color);
    } else dz_extended_stamp(x,y,symmetry,(unsigned char)tail.color);
}
void dz_extended_corkscrew(int rebuild)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    if (rebuild || tail.page != page_paragraph) tail.active = 0;
    if (tail.active) { extend(tail.symmetry,0); return; }
    if (draw_x > screen_extent && !tail.active) {
        tail.active = 1; tail.page = page_paragraph;
        tail.ox = corkscrew_origin_x; tail.oy = corkscrew_origin_y; tail.extent = screen_extent;
        tail.phase = corkscrew_phase; tail.distance = corkscrew_distance; tail.growth = corkscrew_growth_step;
        tail.counter = corkscrew_color_counter; tail.color = plot_color;
        tail.path.valid = 0;
        dz_native_path_prepare(&tail.path,tail.ox+dz_native_untruncated((double)corkscrew_along-corkscrew_across),
            tail.oy+dz_native_untruncated((double)corkscrew_along+corkscrew_across),1);
        tail.path.color = (uint8_t)tail.color;
    }
    if (tail.active) dz_extended_queue(extend);
}
