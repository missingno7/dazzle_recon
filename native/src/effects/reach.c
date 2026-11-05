#include "reach.h"
#include "effects.h"
#include "geometry.h"
#include "pens.h"
#include "dazzle.h"
#include "platform/canvas.h"
#include "dzconst.h"
#include <stdlib.h>

int dz_extended_native(unsigned effect)
{
    /* Some palette-I/O options lower VGA400's extent to 172, inside its
     * vertical middle. That suffix uses the existing side-cell plotter; a
     * full reflected stroke could otherwise restore omitted middle ink. */
    return screen_extent >= screen_half_width && screen_extent >= screen_half_height &&
        dz_native_enabled(effect);
}

static unsigned advance(unsigned color, int *substep)
{
    if (video_adapter != ADAPTER_EGA || !(*substep = (*substep+1)&3)) ++color;
    color &= color_count-1;
    return color < 3 ? 3 : color;
}
/* choose_color's coordinate rules, including DOS word products. Rule 1 is
 * random and is never continued. The original parameters remain read-only. */
static unsigned coordinate_color(int rule, int x, int y, unsigned color)
{
    uint32_t xx = (uint32_t)(int32_t)(int16_t)(x*x);
    uint32_t yy = (uint32_t)(int32_t)(int16_t)(y*y);
    switch (rule) {
    case 2: color = abs(x); break;
    case 3: color = abs(y); break;
    case 4: color = abs(x)+abs(y); break;
    case 5: color = (xx+yy)>>5; break;
    case 6: color = (xx-yy)>>5; break;
    case 7: color = (uint32_t)(int32_t)dz_asr16((int16_t)(y*y),5)-abs(x); break;
    case 8: color = (unsigned)dz_asr16((int16_t)abs((int16_t)(x*y)),5); break;
    }
    color &= color_count-1;
    return color < 3 ? 3 : color;
}
static int rising;
static void diagonal_cell(int x, int y, unsigned symmetry, unsigned char color)
{
    unsigned effect = rising ? DZ_EFFECT_DIAG2 : DZ_EFFECT_DIAG1;
    if (dz_native_enabled(effect)) {
        int before = rising ? y : screen_extent-y;
        int after = rising ? screen_extent-y : y;
        int slope = rising ? 1 : -1;
        if (before > x) before = x;
        if (after > dz_extended_reach()-x) after = dz_extended_reach()-x;
        dz_native_cell_at(0,0,x-before,y-slope*before,x+after,y+slope*after,
            before,before+after,symmetry,color);
    } else dz_extended_stamp(x,y,symmetry,color);
}
static void diagonal(unsigned symmetry, unsigned char color)
{
    int y = draw_y, counter = diag2_color_counter, sub = ega_color_substep;
    int reach = dz_extended_reach();
    for (int x = draw_x+1; x <= reach; ++x) {
        y += rising ? 1 : -1;
        if (y < 0) y = screen_extent;
        if (y > screen_extent) y = 0;
        if (!rising) color = (unsigned char)coordinate_color(effect_parameter,x,y,color);
        else if (effect_parameter && counter++ > diag2_color_period) {
            counter = 0; color = (unsigned char)advance(color,&sub);
        }
        diagonal_cell(x,y,symmetry,color);
    }
}
void dz_extended_diagonal(int up)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED || draw_x < screen_extent) return;
    rising = up; dz_extended_queue(diagonal);
}

static int formula_field;
static void field_cell(int x, int y, unsigned symmetry, unsigned char color)
{
    if (dz_native_enabled(formula_field ? DZ_EFFECT_FORMULA1 : DZ_EFFECT_TUNNEL))
        dz_native_field_cell(formula_field,x,y,symmetry,color);
    else dz_extended_stamp(x,y,symmetry,color);
}
static unsigned formula_color(int x, int y)
{
    int16_t quadratic = (int16_t)((int16_t)(y*y)*formula1_y_square_factor);
    int16_t term = (int16_t)((uint16_t)x*(UINT32_C(1)<<formula1_x_shift));
    return (uint16_t)dz_asr16((int16_t)(quadratic-term),formula1_color_shift)&color_index_mask;
}
static unsigned row_color(unsigned color, int rows)
{
    color += (unsigned)rows;
    return color < color_count ? color : 3+(color-color_count)%(color_count-3);
}
static void field(unsigned symmetry, unsigned char color)
{
    int reach = dz_extended_reach();
    if (formula_field || tunnel_right_edge_ptr == &tunnel_right_limit) {
        if (draw_x == screen_extent) for (int x = screen_extent+1; x <= reach; ++x)
            field_cell(x,draw_y,symmetry,formula_field ? formula_color(x,draw_y) : color);
    }
    /* The last tunnel row is extent-1. Continue each of its columns into the
     * subsequent rows. In the triangular variant x <= y still holds. These
     * rows are visible only in the plotter's original exchanged copies. */
    if (!formula_field && draw_y == screen_extent-1)
        for (int y = screen_extent; y <= reach; ++y)
            field_cell(draw_x,y,symmetry,(unsigned char)row_color(color,y-draw_y));
}
void dz_extended_field(int formula)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    if (draw_x != screen_extent && (formula || draw_y != screen_extent-1)) return;
    formula_field = formula; dz_extended_queue(field);
}

static unsigned segment_effect;
static int seg_ax, seg_ay, seg_bx, seg_by;
static void segment(unsigned symmetry, unsigned char color)
{
    int x = draw_x, y = draw_y, err = error, sub = ega_color_substep;
    int remaining = dz_native_count(x,y,seg_bx,seg_by);
    int count = dz_native_count(seg_ax,seg_ay,seg_bx,seg_by);
    for (int i = 1; i < remaining; ++i) {
        if (dx >= dy) {
            if (err >= 0) { err -= dx; y += sy; }
            x += sx; err += dy;
        } else {
            if (err >= 0) { err -= dy; x += sx; }
            y += sy; err += dx;
        }
        if (segment_effect == DZ_EFFECT_PUFFS) color = (unsigned char)advance(color,&sub);
        if (dz_extended_native(segment_effect))
            dz_native_cell_at(0,0,seg_ax,seg_ay,seg_bx,seg_by,count-remaining+i,count,symmetry,color);
        else dz_extended_stamp(x,y,symmetry,color);
    }
}
void dz_extended_segment(unsigned effect, int ax, int ay, int bx, int by)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    int cut = effect == DZ_EFFECT_PUFFS ? abs(draw_x) >= screen_extent || abs(draw_y) >= screen_extent :
        draw_x > screen_extent || draw_y > screen_extent;
    if (!cut) return;
    segment_effect = effect; seg_ax = ax; seg_ay = ay; seg_bx = bx; seg_by = by;
    dz_extended_queue(segment);
}

/* These two suffixes contain many points/rows. Keep one presentation pen per
 * generator and advance it by one original plot, so an entire side does not
 * appear at once. A rebuild/page switch abandons the old suffix. */
static struct suffix { int active, x, y, left, right, limit, expand, per_row;
    unsigned color; uint16_t page; unsigned symmetry; } triangle, hex;
static void triangle_step(unsigned symmetry, unsigned char color)
{
    (void)color;
    if (!triangle.active || triangle.page != page_paragraph) return;
    triangle.symmetry = symmetry;
    if (dz_native_enabled(DZ_EFFECT_TRIANGLES)) {
        dz_canvas_sides_only(1);
        dz_native_row_cell(triangle.x,triangle.y,triangle.left,triangle.right,
            triangle.expand ? -1 : 0,1,symmetry,(unsigned char)triangle.color);
        dz_canvas_sides_only(0);
    } else dz_extended_stamp(triangle.x,triangle.y,symmetry,(unsigned char)triangle.color);
    if (++triangle.x > triangle.right) {
        if (triangle.right++ >= triangle.limit || ++triangle.y > dz_extended_reach()) triangle.active = 0;
        if (triangle.expand) --triangle.left;
        triangle.x = triangle.left;
        if (triangle.per_row) triangle.color = row_color(triangle.color,1);
    }
}
void dz_extended_triangles(int rebuild)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    if (rebuild || triangle.page != page_paragraph) triangle.active = 0;
    /* Older geometry advances before the next logical plot, so later figures
     * retain the original last-writer order where the two touch in the sides. */
    if (triangle.active) { triangle_step(triangle.symmetry,0); return; }
    if (triangles_active && draw_y == screen_extent && draw_x == triangles_row_right &&
        triangles_row_right < triangles_right_limit) {
        triangle = (struct suffix){1,triangles_row_left-!!triangles_expand_left,draw_y+1,
            triangles_row_left-!!triangles_expand_left,triangles_row_right+1,triangles_right_limit,
            !!triangles_expand_left,!!effect_parameter,row_color(plot_color,!!effect_parameter),page_paragraph,0};
    }
    if (triangle.active) dz_extended_queue(triangle_step);
}
static void hex_step(unsigned symmetry, unsigned char color)
{
    (void)color;
    if (!hex.active || hex.page != page_paragraph) return;
    hex.symmetry = symmetry;
    int px = hex.x, py = hex.y;
    if (hex.x < 0) {
        if (++hex.left > dz_extended_reach()) { hex.active = 0; return; }
        hex.x = hex.left; hex.y = 0;
        int sub = hex.per_row; hex.color = advance(hex.color,&sub); hex.per_row = sub;
        px = hex.x; py = hex.y;
    } else if (++hex.y > hex.left) --hex.x;
    if (dz_extended_native(DZ_EFFECT_HEXAGON)) dz_native_line(px,py,hex.x,hex.y,symmetry,(unsigned char)hex.color);
    else dz_extended_stamp(hex.x,hex.y,symmetry,(unsigned char)hex.color);
}
void dz_extended_hexagon(int rebuild)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    if (rebuild || hex.page != page_paragraph) hex.active = 0;
    if (hex.active) { hex_step(hex.symmetry,0); return; }
    if (draw_x > screen_extent && draw_y == screen_extent && hexagon_start_x == screen_extent+1)
        hex = (struct suffix){1,draw_x,draw_y,hexagon_start_x,0,0,0,ega_color_substep,plot_color,page_paragraph,0};
    if (hex.active) dz_extended_queue(hex_step);
}

/* The deterministic staircase never turns inward. Its extent reset can be
 * removed in a presentation copy without inventing a random run or revisiting
 * the middle. The random bouncing variant has no such continuation. */
static struct {
    int active, x, y, counter, length, direction, phase;
    unsigned color, symmetry;
    uint16_t page;
    struct dz_native_path path;
} stair;
static void stair_step(unsigned symmetry, unsigned char color)
{
    (void)color;
    if (!stair.active || stair.page != page_paragraph) return;
    stair.symmetry = symmetry;
    if (stair.counter++ >= stair.length) {
        stair.counter = 0;
        if (!(++stair.direction&1)) { ++stair.length; stair.direction = 0; }
    }
    stair.x += stair.direction; stair.y += !stair.direction;
    if (!(stair.phase = (stair.phase+1)&7) && ++stair.color >= color_count) stair.color = 3;
    if (dz_extended_native(DZ_EFFECT_ZIGSTREAM)) {
        dz_native_path_prepare(&stair.path,stair.x,stair.y,0);
        dz_native_path_draw(&stair.path,symmetry,(unsigned char)stair.color);
    } else dz_extended_stamp(stair.x,stair.y,symmetry,(unsigned char)stair.color);
    if (stair.x > dz_extended_reach() && stair.y > dz_extended_reach()) stair.active = 0;
}
void dz_extended_zigstream(int rebuild, int variant)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    if (rebuild || stair.page != page_paragraph || variant) stair.active = 0;
    if (stair.active) { stair_step(stair.symmetry,0); return; }
    if (variant || (draw_x <= screen_extent && draw_y <= screen_extent)) return;
    stair.active = 1; stair.x = draw_x; stair.y = draw_y; stair.page = page_paragraph;
    stair.counter = zigstream_run_counter; stair.length = zigstream_run_length;
    stair.direction = zigstream_direction; stair.phase = zigstream_color_phase; stair.color = plot_color;
    stair.path.valid = 0;
    dz_native_path_prepare(&stair.path,draw_x,draw_y,1); stair.path.color = (uint8_t)plot_color;
    dz_extended_queue(stair_step);
}

/* Up to four cut walks run at once; a fifth replaces the oldest. */
static struct circuit { struct dz_extended_circuit c; struct dz_native_path path; uint16_t page; int active, length; } circuits[4];
static unsigned circuit_next;
static void (*circuit_owner)(int16_t);
static void circuit_advance(struct dz_extended_circuit *c)
{
    int spiral = c->effect == DZ_EFFECT_SPIRAL;
    if (c->step++ >= c->length) {
        c->step = 0;
        if (spiral) {
            c->direction = (c->direction+1)&3;
            if (!(c->direction & 1)) ++c->length;
        } else if (++c->direction > 3) {
            ++c->length; c->x += c->shift; c->direction = 0;
            if (++c->color >= (int)color_count) c->color = FIRST_CYCLING_COLOR;
        }
    }
    c->x += c->dx[c->direction]; c->y += c->dy[c->direction];
    if (spiral && !(c->phase = (c->phase+1)&7) && ++c->color >= (int)color_count)
        c->color = FIRST_CYCLING_COLOR;
}
/* The middle cannot show a ring that crosses into it, so a ring is only worth
 * drawing if it stays wholly outside. Otherwise it would end in a hard seam. */
static int circuit_ring_clear(struct dz_extended_circuit c, unsigned symmetry)
{
    int x0 = c.x, x1 = c.x, y0 = c.y, y1 = c.y, length = c.length;
    for (int i = 4*(length+2); i > 0; --i) {
        circuit_advance(&c);
        if (c.x < x0) x0 = c.x;
        if (c.x > x1) x1 = c.x;
        if (c.y < y0) y0 = c.y;
        if (c.y > y1) y1 = c.y;
    }
    if (x0 < screen_half_width) return 0;
    return (symmetry & 3u) != 3u || y0 >= screen_half_width || y1 < screen_half_width;
}
static void circuit_walk(struct circuit *w, unsigned symmetry)
{
    struct dz_extended_circuit *c = &w->c;
    int spiral = c->effect == DZ_EFFECT_SPIRAL;
    int reach = dz_extended_reach();
    if (w->page != page_paragraph || (spiral ? c->x > reach || c->y > reach : c->length >= c->limit)) {
        w->active = 0; return;
    }
    if (w->length != c->length) {
        w->length = c->length;
        if (!circuit_ring_clear(*c,symmetry)) { w->active = 0; return; }
    }
    circuit_advance(c);
    if (dz_extended_native(c->effect)) {
        w->path.corners = 1;
        dz_native_path_prepare(&w->path,c->x,c->y,0);
        dz_native_path_draw(&w->path,symmetry,(unsigned char)c->color);
    } else dz_extended_stamp(c->x,c->y,symmetry,(unsigned char)c->color);
}
static void circuit_after(unsigned symmetry, unsigned char color)
{
    (void)color;
    for (unsigned i = 0; i < sizeof circuits/sizeof circuits[0]; ++i)
        if (circuits[i].active) circuit_walk(&circuits[i],symmetry);
}
void dz_extended_circuit_begin(const struct dz_extended_circuit *c)
{
    if (dz_display.presentation != DZ_PRESENT_EXTENDED) return;
    struct circuit *w = &circuits[circuit_next++ % (sizeof circuits/sizeof circuits[0])];
    w->c = *c; w->page = page_paragraph; w->active = 1; w->length = -1; w->path.valid = 0;
    dz_native_path_prepare(&w->path,c->x,c->y,1);
    w->path.color = (uint8_t)c->color;
    circuit_owner = current_image_draw;
}
void dz_extended_circuit_step(void)
{
    if (circuit_owner != current_image_draw) {
        for (unsigned i = 0; i < sizeof circuits/sizeof circuits[0]; ++i) circuits[i].active = 0;
        return;
    }
    for (unsigned i = 0; i < sizeof circuits/sizeof circuits[0]; ++i)
        if (circuits[i].active) { dz_extended_queue(circuit_after); return; }
}
