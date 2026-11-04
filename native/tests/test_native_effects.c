/* Independent geometric checks, plus actual effect/page/restart integration. */
#include "dazzle.h"
#include "dzconst.h"
#include "effects/effects.h"
#include "effects/cycloids.h"
#include "effects/ellipses.h"
#include "effects/daisies.h"
#include "effects/squirals.h"
#include "effects/corkscrew.h"
#include "canvas_capture.h"
#include <stdio.h>

static SDL_Renderer *renderer;
static double ux, uy, weight, gain;
static int w, h;
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
    w = width; h = height;
    double fitted = fmin(h,w*0.75);
    ux = fitted*4/3/(2*screen_half_width); uy = fitted/(2*screen_half_height);
    gain = w/(fitted*4/3); weight = fmax(1,sqrt(fitted*fitted*4/3/(dz_video.width*dz_video.height)));
}
static double distance(double x, double y, const double *line)
{
    double dx = line[2]-line[0], dy = line[3]-line[1], length = dx*dx+dy*dy;
    double t = length ? ((x-line[0])*dx+(y-line[1])*dy)/length : 0;
    t = fmax(0,fmin(1,t));
    return hypot(x-line[0]-dx*t,y-line[1]-dy*t);
}
static void exact_lines(double lines[][4], int count)
{
    struct dz_canvas_view v = canvas_capture();
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        double d = HUGE_VAL;
        for (int i = 0; i < count; ++i) d = fmin(d,distance(x+0.5,y+0.5,lines[i]));
        assert((v.indices[y*w+x] != 0) == (d <= weight/2));
    }
}
static void curve_point(struct dz_canvas_view v, double x, double y)
{
    int px = (int)floor(w*0.5+x*ux), py = (int)floor(h*0.5+y*uy), found = 0;
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
    double lines[][4] = {{w*0.5,h*0.5,w*0.5+137*ux*gain,h*0.5+screen_extent*uy}};
    exact_lines(lines,1);

    dz_canvas_clear(0);
    double angle = 0.317, radius = 60;
    double x = radius*(cos(angle)-sin(angle)), y = radius*(cos(angle)+sin(angle));
    int ex = (int)x, ey = (int)y;
    draw_x = (int16_t)ey; draw_y = (int16_t)-ex;
    dz_native_squirals_begin(radius,angle,0,0,2,ex,ey);
    int done = 1;
    do {
        done = line_step((int16_t)ex,(int16_t)ey,(int16_t)done);
        dz_native_squirals_step(); plot_callback(0); plot_callback(0);
    } while (!done);
    double edges[][4] = {{w*0.5+y*ux,h*0.5-x*uy,w*0.5+x*ux,h*0.5+y*uy},
        {w*0.5-y*ux,h*0.5+x*uy,w*0.5-x*ux,h*0.5-y*uy}};
    exact_lines(edges,2);

    dz_canvas_clear(0);
    daisies_sample_index = 0; daisies_sample_step = 1; daisies_radius = 60;
    daisies_edge_count = 7; daisies_work_index = 3; daisies_center_x = daisies_center_y = 0;
    dz_native_daisies_prepare(); draw_x = 3; draw_y = 60; dz_native_daisies_begin();
    done = 1;
    do {
        done = line_step(0,0,(int16_t)done); dz_native_daisies_step();
        for (int i = 0; i < 4; ++i) plot_callback(0);
    } while (!done);
    double a = 3.141592653589793/64;
    double aspect = (double)screen_half_width/screen_half_height*0.75;
    struct dz_canvas_view v = canvas_capture();
    for (int i = 0; i <= 1000; ++i) curve_point(v,aspect*60*sin(a)*i/1000,60*cos(a)*i/1000);

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
    double fy = 60*sqrt(1-(29.0/80)*(29.0/80));
    for (int i = -1000; i <= 1000; ++i) curve_point(v,29.0*i/1000,fy*i/1000);
}
static void curve_geometry(void)
{
    /* A rolling curve with the pen on its rolling centre is a physical circle
     * in every DOS mode, and remains circular on widescreen. */
    cycloids_orbit_radius = 60; cycloids_pen_radius = 0; cycloids_spin_step = 3;
    double aspect = (double)screen_half_width/screen_half_height*0.75;
    int px = (int)(aspect*60), py = 0;
    for (int i = 1; i <= 64; ++i) {
        double theta = i*6.283185307179586/64;
        int qx = (int)(aspect*60*cos(theta)), qy = (int)(60*sin(theta));
        cycloids_orbit_phase = (int16_t)(i%64); cycloids_spin_phase = (int16_t)((i*3)%64);
        draw_x = (int16_t)px; draw_y = (int16_t)py;
        dz_native_cycloids_begin(px,py,qx,qy);
        int done = 1;
        do {
            done = line_step((int16_t)qx,(int16_t)qy,(int16_t)done);
            dz_native_cycloids_step(); plot_callback(0);
        } while (!done);
        px = qx; py = qy;
    }
    struct dz_canvas_view v = canvas_capture();
    int x0 = w, x1 = 0, y0 = h, y1 = 0;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (v.indices[y*w+x]) {
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }
    assert(abs((x1-x0)-(y1-y0)) <= 2);
    for (int i = 0; i < 4096; ++i) {
        double a = i*6.283185307179586/4096;
        curve_point(v,aspect*60*cos(a),60*sin(a));
    }
    dz_canvas_clear(0);
    arches_first_radius_x = arches_second_radius_x = 60;
    arches_first_radius_y = arches_second_radius_y = 50;
    dz_native_arches_prepare(1); dz_native_arches_begin(50,60,0,0);
    draw_ellipse(1,50,60,0,0,0,17,SHAPE_OUTLINE); dz_native_arches_end(); dz_native_plot = NULL;
    v = canvas_capture();
    for (int i = 0; i < 4096; ++i) {
        double a = i*6.283185307179586/4096;
        curve_point(v,60*cos(a)*gain,50*sin(a));
    }
    dz_canvas_clear(0);
    corkscrew_phase = 0; corkscrew_distance = 10; corkscrew_origin_x = corkscrew_origin_y = 0;
    corkscrew_growth_step = 0.02f; draw_x = 0;
    dz_native_corkscrew_prepare(1); plot_callback(3);
    for (int i = 0; i < 200; ++i) { corkscrew(0); plot_callback(3); }
    v = canvas_capture();
    for (int i = 0; i <= 2000; ++i) {
        double a = 3.96*i/2000, d = 10+a;
        double along = d*(1-cos(a)/4), across = d*d/screen_extent*sin(a);
        curve_point(v,along*gain-across,along+across);
        curve_point(v,along*gain+across,along-across);
    }
}
static void production(unsigned effect)
{
    draw_symmetry = 3;
    int n = effect == 9 || effect == 31 ? 1 : effect == 0 || effect == 10 ? 2 : effect == 13 ? 12 : 500;
    for (int i = 0; i < n; ++i) { alt_image_records[effect].draw(i == 0); plot_callback(draw_symmetry); }
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
        for (int mode = 0; mode < 4; ++mode) { setup(sizes[size][0],sizes[size][1],mode); curve_geometry(); }
    }
    const unsigned effects[] = {9,33,13,0,10,31,7};
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
    /* Semi-Auto can rebuild without plotting, then select another image. */
    setup(641,479,0); current_image_draw = sine; sine(1);
    assert(dz_native_plot);
    current_image_draw = lotsadots; lotsadots(1); plot_callback(0);
    assert(!dz_native_plot && !dz_canvas_has_native());
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(s); dz_shutdown();
    puts("Native effects: continuous real geometry, Original aspect, circular rolling curve, wide layouts, pages/split and restart passed");
    return 0;
}
