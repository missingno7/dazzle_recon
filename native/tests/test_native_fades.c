/* Reuse the actual fade harness and its canaries; only the source picture differs. */
#define main faithful_fade_main
#include "test_dzfd.c"
#undef main
#include "effects/effects.h"
#include "platform/canvas.h"
static SDL_Renderer *renderer;
static struct dz_video saved;
static unsigned effect;
#ifdef DZ_NATIVE_SECOND_FADE_BATCH
static const unsigned effects[] = {5,22,3,34,4,19,6,41,38,
    DZ_EFFECT_BUTTERFLIES,DZ_EFFECT_DIAMONDS,DZ_EFFECT_TILES,DZ_EFFECT_WEDGES};
#else
static const unsigned effects[] = {9,33,13,0,10,31,7,27,
    DZ_EFFECT_BUTTERFLIES,DZ_EFFECT_DIAMONDS,DZ_EFFECT_TILES,DZ_EFFECT_WEDGES};
#endif

static int polygon_effect(unsigned effect)
{
    return effect == DZ_EFFECT_BUTTERFLIES || effect == DZ_EFFECT_DIAMONDS ||
        effect == DZ_EFFECT_TILES || effect == DZ_EFFECT_WEDGES;
}
static void picture(int native)
{
    dz_display.native_effects = native ? UINT64_MAX : 0;
    uint16_t selected = page_paragraph;
    dz_srand(1);
    plot_color = 3; draw_symmetry = 1; corkscrew_color_counter = 0;
    draw_x = draw_y = 0;
    semi_auto_mode = 0; image_work_limit = 1; image_work_count = 0;
    int steps = effect >= 36 ? 3 : effect == 19 || effect == 6 ? 1 : effect == 33 ? 1000 : effect == 13 ? 12 : effect == 0 || effect == 10 ? 2 :
        effect == 9 || effect == 31 ? 1 : 300;
    for (int i = 0; i < steps; ++i) { (effect < 36 ? alt_image_records+effect : ctrl_image_records+effect-36)->draw(i == 0); plot_callback(draw_symmetry); }
    if (native) {
        assert(dz_canvas_has_native());
        dz_canvas_stroke(0, 0, 16, 240, 10, 251);
        dz_canvas_select_page(2000);
        dz_canvas_stroke(0, 0, 16, 240, 10, 251);
        dz_canvas_select_page(selected);
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
    /* Every dispatched fade completes once with each real Native picture.
     * Compare the full logical aperture/registers/palettes plus work/time/RNG. */
    for (int policy = 0; policy < 4; ++policy)
    for (unsigned e = 0; e < sizeof effects/sizeof effects[0]; ++e)
    for (int aa = 0; aa < (effects[e] == 27 ? 2 : 1); ++aa) for (const char *key = keys; *key; ++key) {
        if (polygon_effect(effects[e]) && policy != 0) continue;
        effect = effects[e];
        uint64_t ns = 0, rng = 0; uint32_t random_state = 0;
        for (int native = 0; native < 2; ++native) {
            dz_display.canvas = DZ_CANVAS_SCREEN; dz_display.native_aa = aa;
            dz_display.presentation = (enum dz_presentation)policy;
            setup(0,0,1);
            assert(dz_canvas_configure(renderer,400,240,(enum dz_presentation)policy) == 1);
            picture(native);
            current_graphics_key = (uint16_t)*key;
            dispatch_fade_key();
            if (!native) { saved = dz_video; ns = dz_virtual_ns(); rng = dz_rand_calls(); random_state = dz_random_state(); }
            else {
                assert(!memcmp(&saved,&dz_video,sizeof saved));
                assert(ns == dz_virtual_ns() && rng == dz_rand_calls() && random_state == dz_random_state());
                if (dz_canvas_has_native() && *key != 'j') {
                    struct dz_canvas_view v = dz_canvas_surface();
                    /* Untouched pages and surviving Classic-equivalent ink
                     * remain pictures. Extended no longer crops their margins;
                     * fade_centres and test_canvas verify stores and lifecycle. */
                    assert(v.width == 400 && v.height >= 240);
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
    for (int policy = 0; policy < 4; ++policy)
    for (unsigned e = 0; e < sizeof effects/sizeof effects[0]; ++e)
    for (int native = 0; native < 2; ++native) {
        if (polygon_effect(effects[e]) && policy != 0) continue;
        effect = effects[e];
        dz_display.canvas = DZ_CANVAS_SCREEN; dz_display.native_aa = 1;
        dz_display.presentation = (enum dz_presentation)policy;
        setup(0,0,1);
        assert(dz_canvas_configure(renderer,400,240,(enum dz_presentation)policy) == 1);
        picture(native);
        current_graphics_key = 'p';
        display_boundary_fade_and_pan();
        assert(display_pan_state == 2 && page_paragraph == 2000);
        picture(native);
        current_graphics_key = 'n';
        display_boundary_fade_and_pan();
        if (!native) saved = dz_video;
        else assert(!memcmp(&saved,&dz_video,sizeof saved));
        teardown();
    }
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface); dz_shutdown();
    printf("All 24 fades with %u Native effects: full logical state/time/RNG identical; clear has no stale ink\n",
        (unsigned)(sizeof effects/sizeof effects[0]));
    return 0;
}
