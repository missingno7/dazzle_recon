/* Independent geometric checks, plus actual effect/page/restart integration. */
#include "dazzle.h"
#include "dzconst.h"
#include "effects/effects.h"
#include "effects/geometry.h"
#include "effects/cycloids.h"
#include "effects/ellipses.h"
#include "effects/daisies.h"
#include "effects/squirals.h"
#include "effects/corkscrew.h"
#include "effects/segments.h"
#include "effects/gumballs.h"
extern int16_t gumballs_draw_flags;
extern int16_t yarnball_radius_y, yarnball_center_x, yarnball_center_y, yarnball_radius_step, yarnball_full_ring, yarnball_draw_flags;
extern float yarnball_cos30, yarnball_sin30, yarnball_aspect_ratio, yarnball_aspect_scale;
#include "canvas_capture.h"
#include <stdio.h>

static SDL_Renderer *renderer;
static double ux, uy, ox, oy;
static int w, h, end_butt;
static void setup(int width, int height, int mode)
{
    dz_native_plot = NULL;
    current_image_draw = NULL;
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    video_adapter = mode < 2 ? ADAPTER_VGA : ADAPTER_EGA;
    bios_resolution_mode = mode == 1 || mode == 2;
    configure_video_backend(0); initialize_math_tables();
    dz_display.native_effects = UINT64_MAX; dz_display.native_aa = 0;
    dz_video_set_mode(mode < 2 ? (mode == 1 ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400) : DZ_MODE_EGA_350,1);
    assert(dz_canvas_configure(renderer,width,height,DZ_PRESENT_ORIGINAL) == 1);
    dz_canvas_take_restart(); dz_canvas_select_page(0); dz_canvas_clear(0);
    draw_x = draw_y = corkscrew_color_counter = 0;
    plot_color = 17; draw_delay_count = 1; draw_symmetry = 0;
    palette_elapsed_ticks = 0; palette_cycle_ticks = UINT16_MAX;
    input_pending = exit_requested = 0; dz_srand(1); dz_clock_reset();
    semi_auto_mode = 0; image_work_limit = 1; image_work_count = 0;
    w = width; h = height;
    double fitted = fmin(h,w*0.75);
    ux = fitted*4/3/(2*screen_half_width); uy = fitted/(2*screen_half_height);
    ox = w*0.5+0.5*ux; oy = h*0.5+0.5*fitted/dz_video.height;
}
static void exact_lines(double lines[][4], int count)
{
    struct dz_canvas_view v = canvas_capture();
    double wx = fmax(1,ux), wy = fmax(1,uy*2*screen_half_height/dz_video.height);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        int inside = end_butt && hypot((x+0.5-lines[0][0])/wx,(y+0.5-lines[0][1])/wy) <= 0.5;
        for (int i = 0; i < count; ++i) {
            double ax = lines[i][0]/wx, ay = lines[i][1]/wy;
            double dx = (lines[i][2]-lines[i][0])/wx, dy = (lines[i][3]-lines[i][1])/wy;
            double length = dx*dx+dy*dy;
            double t = length ? ((x+0.5)/wx-ax)*dx/length+((y+0.5)/wy-ay)*dy/length : 0;
            if (end_butt && t >= 1) continue;
            t = fmax(0,fmin(1,t));
            double d = hypot((x+0.5)/wx-ax-dx*t,(y+0.5)/wy-ay-dy*t);
            double normal = length ? fmax(fabs(dx),fabs(dy))/sqrt(length) : 1;
            int tie = fabs(d-normal/2) < 1e-12;
            double px = (x+0.5)/wx-ax, py = (y+0.5)/wy-ay;
            if (tie && t > 0 && t < 1) {
                if (fabs(dx) >= fabs(dy) && (py-px*dy/dx)*dy < 0) continue;
                if (fabs(dy) > fabs(dx) && (px-py*dx/dy)*dx < 0) continue;
            }
            inside |= d <= normal/2+1e-12;
        }
        assert((v.indices[y*w+x] != 0) == inside);
    }
}

static void curve_point(struct dz_canvas_view v, double x, double y)
{
    int px = (int)floor(ox+x*ux), py = (int)floor(oy+y*uy), found = 0;
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
        if (px+dx >= 0 && px+dx < w && py+dy >= 0 && py+dy < h)
            found |= v.indices[(py+dy)*w+px+dx] != 0;
    assert(found);
}
static void rays(void)
{
    /* A diagonal sweep must equal a subpixel capsule, including its long
     * clipped continuation. An enlarged Bresenham staircase fails this. */
    effect_parameter = 0; sweep2_line_done = 1; sweep2_end_x = 136;
    for (int i = 0; i <= screen_extent; ++i) { sweep2(0); plot_callback(0); }
    double lines[][4] = {{ox,oy,ox+137*ux,oy+screen_extent*uy}};
    exact_lines(lines,1);

    dz_canvas_clear(0);
    double angle = 0.317, radius = 60;
    double x = radius*(cos(angle)-sin(angle)), y = radius*(cos(angle)+sin(angle));
    int ex = (int)x, ey = (int)y;
    draw_x = (int16_t)ey; draw_y = (int16_t)-ex;
    dz_native_squirals_begin(0,0,2,ex,ey);
    int done = 1;
    do {
        done = line_step((int16_t)ex,(int16_t)ey,(int16_t)done);
        dz_native_squirals_step(); plot_callback(0); plot_callback(0);
    } while (!done);
    double edges[][4] = {{ox+ey*ux,oy-ex*uy,ox+ex*ux,oy+ey*uy},
        {ox-ey*ux,oy+ex*uy,ox-ex*ux,oy-ey*uy}};
    exact_lines(edges,2);

    dz_canvas_clear(0);
    daisies_sample_index = 0; daisies_sample_step = 1; daisies_radius = 60;
    daisies_edge_count = 3; daisies_work_index = 1;
    daisies_edge_points[0].x = 2; daisies_edge_points[0].y = 60;
    daisies_edge_points[1].x = 3; daisies_edge_points[1].y = 60;
    daisies_edge_points[2].x = 4; daisies_edge_points[2].y = 60; daisies_center_x = daisies_center_y = 0;
    dz_native_daisies_prepare(); draw_x = 3; draw_y = 60; dz_native_daisies_begin();
    done = 1;
    do {
        done = line_step(0,0,(int16_t)done); dz_native_daisies_step();
        for (int i = 0; i < 4; ++i) plot_callback(0);
    } while (!done);
    struct dz_canvas_view v = canvas_capture();
    for (int i = 0; i <= 1000; ++i) curve_point(v,3.0*i/1000,60.0*i/1000);

    dz_canvas_clear(0);
    g_long5b42 = 80; g_long5b46 = 60; ellipse_offset_x = 29; ellipse_offset_y = 56;
    ellipse_draw_mode = 2; ellipse_center_x = ellipse_center_y = 0;
    dz_native_ellipse_prepare(1);
    for (int i = 0; i < 29; ++i) dz_native_ellipse_prepare(0);
    draw_x = -29; draw_y = -56; dz_native_ellipse_begin(0);
    done = 1;
    do {
        done = line_step(29,56,(int16_t)done); dz_native_ellipse_step(); plot_callback(0);
    } while (!done);
    v = canvas_capture();
    double fy = 56;
    for (int i = -1000; i <= 1000; ++i) curve_point(v,29.0*i/1000,fy*i/1000);
}
static void curve_geometry(void)
{
    /* Follow the original table/rounded vertices and their straight joins,
     * rather than testing the removed analytic rolling-circle replacement. */
    cycloids_orbit_radius = 60; cycloids_pen_radius = 0; cycloids_spin_step = 3;
    double aspect = (double)screen_half_width/screen_half_height*0.75;
    int px = (int)(aspect*60), py = 0;
    for (int i = 1; i <= 64; ++i) {
        double theta = i*6.283185307179586/64;
        int qx = (int)(aspect*60*cos(theta)), qy = (int)(60*sin(theta));
        draw_x = (int16_t)px; draw_y = (int16_t)py;
        dz_native_cycloids_begin(px,py,qx,qy);
        int done = 1;
        do {
            done = line_step((int16_t)qx,(int16_t)qy,(int16_t)done);
            dz_native_cycloids_step(); plot_callback(0);
        } while (!done);
        struct dz_canvas_view v = canvas_capture();
        for (int j = 0; j <= 16; ++j) curve_point(v,px+(qx-px)*j/16.0,py+(qy-py)*j/16.0);
        px = qx; py = qy;
    }
    struct dz_canvas_view v;
    dz_canvas_clear(0);
    arches_first_radius_x = arches_second_radius_x = 60;
    arches_first_radius_y = arches_second_radius_y = 50;
    dz_native_arches_prepare(1); dz_native_arches_begin(50,60,0,0);
    draw_ellipse(1,50,60,0,0,0,17,SHAPE_OUTLINE); dz_native_arches_end(); dz_native_plot = NULL;
    v = canvas_capture();
    for (int i = 0; i < 4096; ++i) {
        double a = i*6.283185307179586/4096;
        curve_point(v,60*cos(a),50*sin(a));
    }
    dz_canvas_clear(0);
    corkscrew_phase = 0; corkscrew_distance = 10; corkscrew_origin_x = corkscrew_origin_y = 0;
    corkscrew_growth_step = 0.02f; draw_x = draw_y = 7;
    dz_native_corkscrew_prepare(1); plot_callback(3);
    int positions[200][2];
    for (int i = 0; i < 200; ++i) {
        corkscrew(0); plot_callback(3);
        positions[i][0] = draw_x; positions[i][1] = draw_y;
    }
    v = canvas_capture();
    for (int i = 0; i < 200; ++i) {
        curve_point(v,positions[i][0],positions[i][1]);
        curve_point(v,positions[i][1],positions[i][0]);
    }
}
static void segments(void)
{
    /* Real completion stores omit the endpoint in these three generators. */
    const unsigned effects[] = {DZ_EFFECT_BOXES,DZ_EFFECT_PUFFS,DZ_EFFECT_BARS,DZ_EFFECT_WANDERING};
    for (unsigned e = 0; e < sizeof effects/sizeof effects[0]; ++e) {
        dz_canvas_clear(0); draw_x = draw_y = 0;
        unsigned effect = effects[e];
        boxes_restart = 0; boxes_line_done = 1; boxes_origin_x = boxes_origin_y = 0;
        boxes_target_x = 80; boxes_target_y = 50; boxes_width = boxes_height = 40; boxes_edge_state = 1;
        puffs_line_done = 1; puffs_origin_x = puffs_origin_y = 0;
        puffs_end_x = 80; puffs_end_y = 50; puffs_spread = 20; effect_parameter = 1;
        bars_line_done = 1; bars_start_x = bars_start_y = 0;
        bars_end_x = 80; bars_end_y = 50; bars_span_x = 80; bars_span_y = 50;
        wandering_line_done = 1; wandering_segments_left = 8;
        wandering_start_x = wandering_start_y = 0; wandering_end_x = 80; wandering_end_y = 50;
        wandering_start_dx = wandering_start_dy = wandering_end_dx = wandering_end_dy = 0;
        for (int i = 0; i <= 80; ++i) { alt_image_records[effect].draw(0); plot_callback(0); }
        double end = effect == DZ_EFFECT_WANDERING ? 1 : 79.5/80;
        end_butt = effect != DZ_EFFECT_WANDERING;
        double lines[][4] = {{ox,oy,ox+80*end*ux,oy+50*end*uy}};
        exact_lines(lines,1); end_butt = 0;
    }
}
static void raster_walks(void)
{
    const unsigned effects[] = {DZ_EFFECT_HEXAGON,DZ_EFFECT_HILBERT,DZ_EFFECT_SPIRAL,
        DZ_EFFECT_SPIRAL1,DZ_EFFECT_SQUARES,DZ_EFFECT_SWEEP,DZ_EFFECT_ZIGSTREAM};
    for (unsigned i = 0; i < sizeof effects/sizeof effects[0]; ++i) {
        unsigned effect = effects[i];
        setup(641,479,0);
        current_image_draw = alt_image_records[effect].draw;
        dz_srand(1); plot_color = 17; draw_symmetry = 1;
        struct dz_canvas_store_audit audit = {0}; dz_canvas_store_audit = &audit;
        unsigned calls = effect == DZ_EFFECT_HILBERT ? 1u : 256u;
        for (unsigned frame = 0; frame < calls; ++frame) {
            current_image_draw((int16_t)(frame == 0));
            plot_callback(draw_symmetry);
        }
        dz_canvas_store_audit = NULL;
        assert(!audit.faithful && audit.native && dz_canvas_has_native());
    }
}
static void rings(void)
{
    dz_canvas_clear(0);
    /* Derived-radius rings are circular in every DOS mode. Centre width is
     * independent of local radius, and does not stretch the circle. */
    double aspect = dz_native_radius_x(40)/40.0;
    dz_native_ring_begin(DZ_EFFECT_GUMBALLS,40,0,25,0);
    draw_ellipse(1,40,0,25,0,0,17,SHAPE_OUTLINE); dz_native_ring_end(); dz_native_plot = NULL;
    struct dz_canvas_view v = canvas_capture();
    for (int i = 0; i < 4096; ++i) {
        double a = i*6.283185307179586/4096;
        curve_point(v,25+aspect*40*cos(a),40*sin(a));
    }
    dz_canvas_clear(0);
    dz_native_ring_begin(DZ_EFFECT_CONES,40,60,0,0);
    draw_ellipse(1,40,60,0,0,0,17,SHAPE_OUTLINE); dz_native_ring_end(); dz_native_plot = NULL;
    v = canvas_capture();
    for (int i = 0; i < 4096; ++i) {
        double a = i*6.283185307179586/4096;
        curve_point(v,60*cos(a),40*sin(a));
    }
}
static void yarn_layout(void)
{
    dz_canvas_clear(0); draw_symmetry = 0; plot_color = 17;
    yarnball_radius_y = 40; yarnball_center_x = yarnball_center_y = 60;
    yarnball_radius_step = 0; yarnball_full_ring = 1; yarnball_draw_flags = SHAPE_OUTLINE;
    yarnball_cos30 = 0.866f; yarnball_sin30 = 0.5f;
    yarnball_aspect_ratio = (float)screen_half_width/screen_half_height*yarnball_aspect_scale;
    yarnball(0); dz_native_plot = NULL;
    struct dz_canvas_view v = canvas_capture();
    int left = w, right = -1;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (v.indices[y*w+x]) {
        if (x < left) left = x;
        if (x > right) right = x;
    }
    double radius = dz_native_radius_x(40);
    /* Real projected neighbour centres retain the original cluster bounds. */
    double offset = (int)(40*yarnball_aspect_ratio);
    double x0 = ox+(60-offset-radius)*ux-fmax(1,ux)/2;
    double x1 = ox+(60+offset+radius)*ux+fmax(1,ux)/2;
    assert(fabs(left-x0) <= 2 && fabs(right-x1) <= 2);
}
static void filled_balls(void)
{
    const unsigned flags[] = {SHAPE_FILL,SHAPE_FILL|ELLIPSE_FILL_COLOR_PER_PIXEL,
        SHAPE_FILL|ELLIPSE_FILL_MIRRORED_COLORS,SHAPE_FILL|ELLIPSE_FILL_COLOR_PER_ROW};
    double aspect = dz_native_radius_x(40)/40.0;
    for (unsigned f = 0; f < sizeof flags/sizeof flags[0]; ++f) for (int symmetry = 0; symmetry <= 3; symmetry += 3) {
        dz_canvas_clear(0); draw_symmetry = (uint16_t)symmetry;
        dz_native_gumballs_begin(40,0,0,flags[f]);
        draw_ellipse(1,40,0,0,0,17,17,(uint16_t)flags[f]);
        dz_native_gumballs_end(); dz_native_plot = NULL;
        struct dz_canvas_view v = canvas_capture();
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            double px = (x+0.5-ox)/ux, py = (y+0.5-oy)/uy;
            double d = px*px/(40*aspect*40*aspect)+py*py/(40*40);
            double swap = py*py/(40*aspect*40*aspect)+px*px/(40*40);
            int inside = d < 1 || (symmetry == 3 && swap < 1);
            assert((v.indices[y*w+x] != 0) == inside);
            if (!f && inside) assert(v.indices[y*w+x] == (video_adapter == ADAPTER_EGA ? 1 : 17));
        }
    }
}
static void star_colors(void)
{
    /* A horizontal ray gives an unambiguous colour oracle at each sample,
     * independent of capsule enumeration. A one-step colour lead fails this. */
    dz_canvas_clear(0); draw_x = draw_y = 0; plot_color = 11;
    bigstar_restart = bigstar_resize_pending = 0; bigstar_inner_radius = 10;
    bigstar_steep_count = 1; bigstar_shallow_count = 3; bigstar_trace_index = 2;
    bigstar_segment_state = 1; bigstar_line_done = 1; bigstar_color_mode = 1;
    bigstar_base_color = 11; bigstar_diagonal_x = 60; bigstar_diagonal_y = 11;
    bigstar_traces[2].x2 = 7; bigstar_traces[2].y2 = 11;
    for (int i = 0; i <= 53; ++i) { bigstar(0); plot_callback(0); }
    struct dz_canvas_view v = canvas_capture();
    int row = (int)floor(oy+11*uy);
    for (int i = 0; i <= 53; ++i) {
        int col = (int)floor(ox+(7+i)*ux);
        assert(v.indices[row*w+col] == 12+i);
    }
    int beyond = (int)floor(ox+61*ux);
    assert(!v.indices[row*w+beyond]);
}
static void raw_lines(void)
{
    setup(320,400,0);
    assert(dz_canvas_configure(renderer,320,400,DZ_PRESENT_STRETCH) == 1);
    dz_canvas_take_restart();
    const int lengths[] = {7,13,31,47,80,121};
    for (unsigned n = 0; n < sizeof lengths/sizeof lengths[0]; ++n)
        for (int minor = 0; minor <= lengths[n]; ++minor)
            for (int direction = 0; direction < 8; ++direction) for (int coloured = 0; coloured < 2; ++coloured) {
                dz_canvas_clear(0); draw_x = -37; draw_y = -23;
                int vx = direction & 4 ? minor : lengths[n];
                int vy = direction & 4 ? lengths[n] : minor;
                int x = -37+(direction & 1 ? -vx : vx), y = -23+(direction & 2 ? -vy : vy);
                int done = 1, sample = 0;
                dz_native_segment_begin(DZ_EFFECT_BOXES,1,x,y);
                do {
                    plot_color = (uint16_t)(coloured ? 3+sample%253 : 17);
                    done = line_step((int16_t)x,(int16_t)y,(int16_t)done);
                    dz_native_segment_step(0); plot_callback(0); ++sample;
                } while (!done);
                struct dz_canvas_view v = canvas_capture();
                if (memcmp(v.indices,dz_canvas_memory(),320*400)) {
                    fprintf(stderr,"raw line differs: -37,-23 -> %d,%d coloured=%d\n",x,y,coloured);
                    assert(0);
                }
            }
}
static void cache_prediction(void)
{
    /* Compare the pure safety prediction with the actual original iterator,
     * including thin traces that never complete. Never write a malformed cache. */
    setup(320,400,0);
    const int radii[] = {0,1,2,5,17,40,159,174,199,319};
    for (unsigned y = 0; y < sizeof radii/sizeof radii[0]; ++y)
        for (unsigned x = 0; x < sizeof radii/sizeof radii[0]; ++x) {
            int done = 1, count = 0;
            do { done = ellipse_step((int16_t)radii[y],(int16_t)radii[x],(int16_t)done); ++count; }
            while (!done && count < 700);
            assert(dz_native_ellipse_count(radii[y],radii[x]) == (done ? count : 0));
        }
}
static void rotated_copy_order(void)
{
    setup(320,400,0);
    assert(dz_canvas_configure(renderer,320,400,DZ_PRESENT_STRETCH) == 1);
    dz_canvas_take_restart(); dz_canvas_clear(0);
    draw_x = 20; draw_y = -10; plot_color = 17;
    dz_native_squirals_begin(40,50,3,10,20);
    (void)line_step(10,20,1); dz_native_squirals_step();
    /* Original case 3 first plots (cx+saved_y,cy-saved_x), then its opposite.
     * Inspect that first store, before the equal-colour second copy hides order. */
    draw_x = 30; draw_y = 30; plot_callback(1);
    assert(!memcmp(canvas_capture().indices,dz_canvas_memory(),320*400));
    dz_native_plot = NULL;
}

static void production(unsigned effect)
{
    draw_symmetry = 3;
    int n = effect >= 36 ? 3 : effect == 19 || effect == 6 ? 1 : effect == 9 || effect == 31 ? 1 : effect == 0 || effect == 10 ? 2 : effect == 13 ? 12 : 500;
    for (int i = 0; i < n; ++i) { (effect < 36 ? alt_image_records+effect : ctrl_image_records+effect-36)->draw(i == 0); plot_callback(draw_symmetry); }
    assert(dz_native_plot == NULL && dz_canvas_has_native());
}
static void pages_and_restart(unsigned effect)
{
    production(effect);
    size_t bytes = (size_t)w*h;
    uint8_t *first = malloc(bytes); assert(first);
    memcpy(first,canvas_capture().indices,bytes);
    dz_canvas_select_page(2000); dz_srand(1); production(effect);
    dz_canvas_start(400); dz_canvas_split(199);
    struct dz_canvas_view v = canvas_capture();
    /* Below the split the original first page starts again. */
    int split = (int)lround(h*0.5);
    assert(!memcmp(v.indices+(size_t)split*w,first,(size_t)(h-split)*w));
    free(first); dz_canvas_start(0); dz_canvas_split(400);
    int a = w+1, b = h+1;
    assert(dz_canvas_configure(renderer,a,b,DZ_PRESENT_ORIGINAL) == 1);
    assert(dz_canvas_take_restart() && !dz_canvas_take_restart());
    v = canvas_capture();
    for (int i = 0; i < a*b; ++i) assert(!v.indices[i]);
    dz_canvas_select_page(0); dz_srand(1); production(effect);
}
int main(void)
{
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *s = SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    renderer = SDL_CreateSoftwareRenderer(s); assert(renderer); dz_display.canvas = DZ_CANVAS_SCREEN;
    const int sizes[][2] = {{641,479},{1440,1080},{1920,1080},{3840,2160}};
    for (unsigned size = 0; size < sizeof sizes/sizeof sizes[0]; ++size) {
        setup(sizes[size][0],sizes[size][1],0); rays();
        segments();
        if (size == 0) star_colors();
        for (int mode = 0; mode < 4; ++mode) { setup(sizes[size][0],sizes[size][1],mode); curve_geometry(); rings(); yarn_layout();
            if (size == 0) filled_balls(); }
    }
    raster_walks();
    raw_lines();
    cache_prediction();
    rotated_copy_order();
    const unsigned effects[] = {9,33,13,0,10,31,7,5,22,3,34,4,19,6,41,38};
    for (unsigned e = 0; e < sizeof effects/sizeof effects[0]; ++e) {
        setup(641,479,0); pages_and_restart(effects[e]);
    }
    /* Known wrapped variant: an entire accelerating sweep stays Faithful. */
    setup(641,479,0); effect_parameter = 1; sweep2_line_done = 1;
    sweep2_end_x = sweep2_end_x_step = 0;
    for (int i = 0; i < 1000; ++i) { sweep2(0); plot_callback(1); }
    assert(!dz_canvas_has_native());
    /* Same doubled-radii choice: EGA350 fits the cache, EGA700 spills. */
    setup(641,479,2); dz_srand(48); arches(1); plot_callback(1);
    assert(dz_canvas_has_native());
    setup(641,479,3); dz_srand(48); arches(1); plot_callback(1);
    assert(!dz_canvas_has_native());
    /* A same-owner rebuild discards its old endpoint; the new zero ring is
     * a Native point at its own spread centre, including the outer plot. */
    setup(641,479,0); current_image_draw = cones;
    dz_native_ring_begin(DZ_EFFECT_CONES,40,40,0,0); dz_native_ring_end();
    assert(dz_native_plot);
    dz_native_ring_begin(DZ_EFFECT_CONES,0,0,0,0);
    assert(!dz_native_plot);
    struct dz_canvas_store_audit audit = {0}; dz_canvas_store_audit = &audit;
    draw_ellipse(1,0,0,0,0,0,17,SHAPE_OUTLINE); dz_native_ring_end(); plot_callback(0);
    dz_canvas_store_audit = NULL;
    assert(!audit.faithful && audit.native == 5 && dz_canvas_has_native());
    /* EGA XOR is a raster operation, including for pending native plots. */
    setup(641,479,2); dz_canvas_raster_xor(1);
    boxes(1); plot_callback(1);
    assert(!dz_canvas_has_native());
    setup(641,479,2); boxes(1); assert(dz_native_plot);
    dz_canvas_raster_xor(1); plot_callback(1);
    assert(!dz_native_plot && !dz_canvas_has_native());
    /* Semi-Auto can rebuild without plotting, then select another image. */
    setup(641,479,0); current_image_draw = sine; sine(1);
    assert(dz_native_plot);
    current_image_draw = lotsadots; lotsadots(1); plot_callback(0);
    assert(!dz_native_plot && !dz_canvas_has_native());
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(s); dz_shutdown();
    puts("Native effects: original vertices, sample colours, logical-cell bands, rings, pages/split and restart passed");
    return 0;
}
