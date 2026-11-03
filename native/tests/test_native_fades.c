/* Reuse the actual fade harness and its canaries; only the source picture differs. */
#define main faithful_fade_main
#include "test_dzfd.c"
#undef main
#include "platform/canvas.h"
static SDL_Renderer *renderer;
static struct dz_video saved;
static void picture(int native)
{
    dz_display.native_sine = native;
    dz_canvas_select_page(0);
    for (int i = 0; i < 300; ++i) { sine(i == 0); plot_callback(3); }
    if (native) {
        dz_canvas_stroke(0, 0, 16, 240, 10, 251);
        dz_canvas_select_page(2000);
        dz_canvas_stroke(0, 0, 16, 240, 10, 251);
        dz_canvas_select_page(0);
    }
}
int main(void)
{
    dz_options = (struct dz_options){1,1,1,1.0,NULL};
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *surface = SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    renderer = SDL_CreateSoftwareRenderer(surface); assert(renderer);
    memcpy(original_bounds, countdown_segment_bounds, sizeof original_bounds);
    const char *keys = "abcdefgijklmoqrstuvwxyz";
    /* Every dispatched fade completes once with a real Native sine picture.
     * Compare the full logical aperture/registers/palettes plus work/time/RNG. */
    for (int aa = 0; aa < 2; ++aa) for (const char *key = keys; *key; ++key) {
        uint64_t ns = 0, rng = 0; uint32_t random_state = 0;
        for (int native = 0; native < 2; ++native) {
            dz_display.canvas = DZ_CANVAS_SCREEN; dz_display.native_aa = aa;
            setup(0,0,1);
            assert(dz_canvas_configure(renderer,400,240,DZ_PRESENT_ORIGINAL) == 1);
            picture(native);
            current_graphics_key = (uint16_t)*key;
            dispatch_fade_key();
            if (!native) { saved = dz_video; ns = dz_virtual_ns(); rng = dz_rand_calls(); random_state = dz_random_state(); }
            else {
                assert(!memcmp(&saved,&dz_video,sizeof saved));
                assert(ns == dz_virtual_ns() && rng == dz_rand_calls() && random_state == dz_random_state());
                if (dz_canvas_has_native() && *key != 'j') {
                    struct dz_canvas_view v = dz_canvas_surface();
                    if (display_cleared && strchr("vwyz", *key))
                        for (int i = 0; i < v.width*v.height; ++i) assert(!v.indices[i]);
                    if (!strchr("vwz", *key))
                        for (int y = 0; y < v.height; ++y) assert(!v.indices[y*v.width+5]);
                    /* A whole-canvas clear also removes all AA metadata. */
                    dz_canvas_clear(0); v = dz_canvas_surface();
                    for (int i = 0; i < v.width*v.height; ++i) {
                        assert(!v.indices[i]); if (v.coverage) assert(v.coverage[i] == 255);
                    }
                }
            }
            teardown();
        }
    }
    /* Exercise Pan's actual owner boundary with visible and hidden native ink. */
    for (int native = 0; native < 2; ++native) {
        dz_display.canvas = DZ_CANVAS_SCREEN; dz_display.native_aa = 1;
        setup(0,0,1);
        assert(dz_canvas_configure(renderer,400,240,DZ_PRESENT_ORIGINAL) == 1);
        picture(native);
        current_graphics_key = 'p';
        display_boundary_fade_and_pan();
        assert(display_pan_state == 2 && page_paragraph == 2000);
        for (int i = 0; i < 300; ++i) { sine(i == 0); plot_callback(3); }
        current_graphics_key = 'n';
        display_boundary_fade_and_pan();
        if (!native) saved = dz_video;
        else assert(!memcmp(&saved,&dz_video,sizeof saved));
        teardown();
    }
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface); dz_shutdown();
    puts("All 24 fades with Native sine: full logical state/time/RNG identical; clear has no stale ink");
    return 0;
}
