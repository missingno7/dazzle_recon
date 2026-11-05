/* Extra plots never enter DOS memory; even colour zero is real side ink. */
#include "dazzle.h"
#include "dvdt.h"
#include "platform/canvas.h"
#include "canvas_capture.h"
#include <stdio.h>

int main(void)
{
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *s = SDL_CreateSurface(960,540,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(s); assert(r);
    dz_display.canvas = DZ_CANVAS_SCREEN; dz_display.native_effects = 0;
    dz_display.presentation = DZ_PRESENT_EXTENDED;
    video_adapter = 8; bios_resolution_mode = 0; configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    assert(dz_canvas_configure(r,960,540,DZ_PRESENT_EXTENDED) == 1);
    dz_canvas_take_restart(); dz_canvas_select_page(0);
    draw_delay_count = 1; draw_x = 180; draw_y = 20; plot_color = 17;
    uint32_t before = dz_framebuffer_hash();
    plot_callback(1);
    assert(before == dz_framebuffer_hash() && symmetry_edge == 1);
    struct dz_canvas_view v = canvas_capture();
    int ink = 0;
    for (int y = 0; y < 540; ++y) for (int x = 0; x < 960; ++x)
        if (v.indices[y*960+x]) { assert(x < 120 || x >= 840); ++ink; }
    assert(ink > 0);
    dz_canvas_clear(0); draw_y = -200; plot_callback(1);
    dz_canvas_start(400); v = canvas_capture();
    assert(v.indices[885] == 17); /* genuine row-H reflection, on the next page */
    dz_canvas_start(0); draw_y = 20;
    /* Wrapped/out-of-range words never become an opposite-side stamp. */
    dz_canvas_clear(0); draw_x = INT16_MIN; plot_callback(3);
    v = canvas_capture();
    for (int i = 0; i < 960*540; ++i) assert(!v.indices[i]);
    /* A coloured index zero is the background, on the sides too. */
    draw_x = 180; plot_color = 0; plot_callback(1);
    dz_video.dac[0][0] = 63; dz_video.dac[0][1] = dz_video.dac[0][2] = 0;
    struct dz_presenter *p = dz_presenter_create(r,DZ_RENDERER_INDEXED); assert(p);
    assert(dz_presenter_draw(p,DZ_PRESENT_EXTENDED,1) == 1);
    SDL_Surface *raw = SDL_RenderReadPixels(r,NULL);
    SDL_Surface *rgb = SDL_ConvertSurface(raw,SDL_PIXELFORMAT_ARGB8888); assert(rgb);
    uint32_t *row = (uint32_t *)((uint8_t *)rgb->pixels+297*rgb->pitch);
    assert(row[0] == UINT32_C(0xffff0000));
    assert(row[480] == UINT32_C(0xffff0000));
    SDL_DestroySurface(raw); SDL_DestroySurface(rgb);
    /* The background word's stripes continue past both 4:3 edges at the same
     * four-column period, and the middle keeps the literal page fill. */
    dz_video.dac[0][0] = 0;
    dz_canvas_clear(0); clear_vram_words(0x2211,0x7d00,0);
    v = canvas_capture();
    const uint8_t *line = v.indices+297*960;
    double cell = 960/426.67;
    for (int c = 2; c < 18; ++c) {
        int right_x = 840+(int)((c+0.5)*cell), left_x = 119-(int)((c+0.5)*cell);
        assert(line[right_x] == (((c/4)&1) ? 0x22 : 0x11));
        assert(line[left_x] == 0x11 || line[left_x] == 0x22);
    }
    uint8_t a = line[840+(int)(0.5*cell)], b = line[840+(int)(4.5*cell)];
    assert(a != b && line[119-(int)(0.5*cell)] == 0x22 && line[119-(int)(4.5*cell)] == 0x11);
    dz_canvas_clear_page(0,0x7d00,0);
    v = canvas_capture();
    assert(!v.indices[297*960+885]);
    dz_presenter_destroy(p); SDL_DestroyRenderer(r); SDL_DestroySurface(s); dz_shutdown();
    puts("Unclipped quadrants, no logical stores/wrap, nonblack zero and side clear passed");
    return 0;
}
