/* Extent suffixes must reach real side columns without a single middle store. */
#include "dazzle.h"
#include "dzconst.h"
#include "effects/effects.h"
#include "effects/geometry.h"
#include "effects/reach.h"
#include "effects/segments.h"
#include "effects/corkscrew.h"
#include "effects/lines.h"
#include "effects/sweep2.h"
#include "canvas_capture.h"
#include <stdio.h>

static SDL_Renderer *renderer;
static void setup(int native, int width)
{
    dz_native_plot = NULL; dz_extended_after_plot = NULL;
    current_image_draw = NULL;
    dz_display.native_effects = native ? UINT64_MAX : 0;
    dz_display.native_aa = 0;
    dz_display.presentation = DZ_PRESENT_EXTENDED;
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    assert(dz_canvas_configure(renderer,width,540,DZ_PRESENT_EXTENDED) == 1);
    dz_canvas_take_restart(); dz_canvas_select_page(0); dz_canvas_clear(0);
    screen_half_width = 160; screen_half_height = 200; screen_extent = 201;
    color_count = 256; color_index_mask = 255; plot_color = 17;
    ega_color_substep = 0; draw_x = 202; draw_y = 0;
    input_pending = exit_requested = 0; draw_delay_count = 1;
    palette_elapsed_ticks = 0; palette_cycle_ticks = UINT16_MAX;
    dz_srand(1); dz_clock_reset();
}
static void suffix(int times)
{
    if (!dz_extended_after_plot) fprintf(stderr,"missing suffix at %d,%d times=%d\n",draw_x,draw_y,times);
    assert(dz_extended_after_plot);
    void (*after)(unsigned,unsigned char) = dz_extended_after_plot;
    dz_extended_after_plot = NULL;
    /* The continuation cannot mutate even the logical aperture/registers. */
    unsigned char *before = malloc(sizeof dz_video); assert(before);
    memcpy(before,&dz_video,sizeof dz_video);
    int x=draw_x,y=draw_y,c=plot_color,e=error,ddx=dx,ddy=dy,ssx=sx,ssy=sy;
    uint64_t ns=dz_virtual_ns(), rng=dz_rand_calls();
    uint32_t random=dz_random_state();
    for (int i=0;i<times;++i) after(3,(unsigned char)plot_color);
    assert(!memcmp(before,&dz_video,sizeof dz_video)); free(before);
    assert(x==draw_x && y==draw_y && c==plot_color && e==error &&
        ddx==dx && ddy==dy && ssx==sx && ssy==sy);
    assert(ns==dz_virtual_ns() && rng==dz_rand_calls() && random==dz_random_state());
}
static void check(const char *name)
{
    struct dz_canvas_view v = canvas_capture();
    SDL_FRect r = dz_canvas_content();
    int edge=0;
    for (int y=0;y<v.height;++y) {
        edge |= v.indices[y*v.width] || v.indices[(y+1)*v.width-1];
        for (int x=(int)r.x;x<(int)(r.x+r.w);++x) assert(v.indices[y*v.width+x]==0);
    }
    /* A finite tip can end before the 21:9 edge. Every fixture reaches 16:9. */
    if (v.width==960 && !edge) fprintf(stderr,"no edge ink: %s\n",name);
    assert(v.width!=960 || edge);
}
static void cases(int native, int width)
{
    setup(native,width);
    dz_native_sweep(1,202,2,1); suffix(1); check("sweep");
    setup(native,width); effect_parameter=0;
    sweep2_end_x=100; sweep2_end_y=201; sweep2_line_done=1;
    dz_native_sweep2_prepare(1); suffix(1); check("sweep2");
    setup(native,width);
    draw_x=201; draw_y=10; effect_parameter=2;
    dz_extended_diagonal(0); suffix(1); check("diag1");
    /* Descending colour formulas are evaluated at each new X/Y, not smeared. */
    struct dz_canvas_view v=canvas_capture();
    int seen=0;
    for(int y=0;y<v.height;++y) seen |= v.indices[y*v.width+v.width-1]==213;
    if(width==960) assert(seen);
    for(int variant=0;variant<2;++variant) {
        setup(native,width); draw_x=201; draw_y=10; effect_parameter=variant;
        diag2_color_counter=3; diag2_color_period=2;
        dz_extended_diagonal(1); suffix(1); check("diag2");
    }
    setup(native,width); draw_x=201; draw_y=10;
    formula1_x_shift=2; formula1_y_square_factor=3; formula1_color_shift=4;
    dz_extended_field(1); suffix(1); check("formula1");
    for(int triangle=0;triangle<2;++triangle) {
        setup(native,width); draw_x=triangle?20:201; draw_y=triangle?200:20;
        tunnel_right_limit=201; tunnel_right_edge_ptr=triangle?&draw_y:&tunnel_right_limit;
        dz_extended_field(0); suffix(1); check("tunnel");
    }
    setup(native,width); zigstream_run_counter=0; zigstream_run_length=20;
    zigstream_direction=1; zigstream_color_phase=0;
    dz_extended_zigstream(1,0); suffix(4000); check("zigstream stairs");
    setup(native,width); screen_extent=172; draw_x=173;
    zigstream_run_counter=0; zigstream_run_length=20;
    zigstream_direction=1; zigstream_color_phase=0;
    dz_extended_zigstream(1,0); suffix(4000); check("lower-extent side cells");
    for(int puff=0;puff<2;++puff) {
        setup(native,width); draw_x=200; draw_y=0;
        unsigned effect=puff?DZ_EFFECT_PUFFS:DZ_EFFECT_BARS;
        dz_native_segment_begin(effect,1,260,0);
        (void)line_step(260,0,1);
        (void)line_step(260,0,0);
        if (!puff) (void)line_step(260,0,0);
        dz_native_segment_step(0); suffix(1); check(puff?"puffs":"bars");
    }
    setup(native,width); draw_x=202; draw_y=201; hexagon_start_x=202;
    dz_extended_hexagon(1); suffix(90000); check("hexagon");
    setup(native,width); draw_x=20; draw_y=201; triangles_active=1;
    triangles_row_left=0; triangles_row_right=20; triangles_right_limit=160;
    triangles_expand_left=1; effect_parameter=1;
    dz_extended_triangles(1); suffix(60000); check("triangles");
    setup(native,width); draw_x=20; draw_y=0;
    dz_native_little_begin(180,0,140,0);
    for(int i=0;i<=120;++i) {
        dz_native_little_step(); draw_x=(int16_t)(200+i); draw_y=0;
        if(draw_x>=201) dz_extended_little_copy(0);
    }
    check("littleStars");
    setup(native,width); draw_x=draw_y=202;
    corkscrew_origin_x=corkscrew_origin_y=0;
    corkscrew_along=202.5f; corkscrew_across=0;
    corkscrew_phase=(float)(3.141592653589793+0.02);
    corkscrew_distance=162.04f; corkscrew_growth_step=0.04f; corkscrew_color_counter=0;
    dz_extended_corkscrew(1); suffix(1000); check("corkscrew excursion");
    /* A rejected Y sample's analytic interval starts just below the gate. */
    setup(native,width); draw_x=0; draw_y=0;
    dz_native_little_begin(0,100,0,150);
    for(int i=0;i<=101;++i) dz_native_little_step();
    draw_x=0; draw_y=201; dz_extended_little_copy(0);
    v=canvas_capture(); SDL_FRect r=dz_canvas_content();
    for(int y=0;y<v.height;++y) for(int x=(int)r.x;x<(int)(r.x+r.w);++x)
        assert(v.indices[y*v.width+x]==0);
}
int main(void)
{
    dz_options.headless=dz_options.deterministic=1;
    dz_display.canvas=DZ_CANVAS_SCREEN;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *s=SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    renderer=SDL_CreateSoftwareRenderer(s); assert(renderer);
    video_adapter=ADAPTER_VGA; bios_resolution_mode=0; configure_video_backend(0);
    dz_timer_tick_handler=dz_keyboard_irq_handler=dz_mouse_event_handler=NULL;
    for(int native=0;native<2;++native) for(int width=960;width<=1280;width+=320) cases(native,width);
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(s); dz_shutdown();
    puts("Extended: suffixes reach 16:9 edges; both wide middles, memory, RNG and time untouched");
    return 0;
}
