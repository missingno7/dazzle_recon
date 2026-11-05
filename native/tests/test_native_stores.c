/* Physical-store audit of complete generators, including their outer plot.
 * --long uses 1080p/20 virtual seconds; CTest uses a smaller wide canvas. */
#include "dazzle.h"
#include "dzconst.h"
#include "effects/effects.h"
#include "platform/canvas.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern int16_t gumballs_draw_flags, yarnball_full_ring, yarnball_draw_flags;
static SDL_Renderer *renderer;
static enum dz_presentation policy;
static struct dz_canvas_store_audit stores;
static uint64_t leaked, variants[DZ_EFFECT_COUNT];
static unsigned reported[DZ_EFFECT_COUNT];
static unsigned failures;
static uint64_t native_leaks, fallback_leaks, frames;
static int report;
static const unsigned effects[] = {27,9,33,13,0,10,31,7,5,22,3,34,4,19,6,41,38,
    16,17,28,29,30,32,35,DZ_EFFECT_BUTTERFLIES,DZ_EFFECT_DIAMONDS,
    DZ_EFFECT_TILES,DZ_EFFECT_WEDGES};
static ImageConfigRow *record(unsigned e)
{ return e < 36 ? alt_image_records+e : ctrl_image_records+e-36; }
static void mode(int kind, int width, int height)
{
    dz_canvas_store_audit = NULL; dz_native_plot = NULL;
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    video_adapter = kind < 2 ? ADAPTER_VGA : ADAPTER_EGA;
    bios_resolution_mode = kind == 1 || kind == 2;
    configure_video_backend(0); initialize_math_tables();
    dz_display.native_effects = UINT64_MAX; dz_display.native_aa = 0;
    dz_video_set_mode(kind < 2 ? (kind == 1 ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400) : DZ_MODE_EGA_350,1);
    dz_display.presentation = policy;
    assert(dz_canvas_configure(renderer,width,height,policy) == 1);
    dz_canvas_take_restart();
}
static void reset(unsigned effect, unsigned seed)
{
    dz_canvas_store_audit = NULL; dz_native_plot = NULL;
    dz_canvas_clear(0); dz_canvas_select_page(0); dz_canvas_start(0);
    dz_canvas_split((unsigned)dz_video.height); dz_canvas_raster_xor(0);
    current_image_draw = record(effect)->draw;
    draw_x = draw_y = corkscrew_color_counter = 0;
    plot_color = 17; draw_delay_count = 1; draw_symmetry = seed & 1 ? 1 : 3;
    palette_elapsed_ticks = 0; palette_cycle_ticks = UINT16_MAX;
    input_pending = exit_requested = 0; image_work_count = 0;
    semi_auto_mode = 0; image_work_limit = 1;
    dz_srand((uint16_t)seed); dz_clock_reset();
}
/* Shadow only the original random selection, without touching its stream.
 * Local colour/reflection/direction choices otherwise disappear on return. */
static uint32_t random_state;
static unsigned random_choice(unsigned n)
{
    random_state = random_state*UINT32_C(22695477)+1;
    return (unsigned)(((random_state >> 16 & 32767u)*n) >> 15);
}
static unsigned local_variant(unsigned effect)
{
    random_state = dz_random_state();
    if (effect == DZ_EFFECT_SQUIRALS) {
        for (int i = 0; i < 5; ++i) random_choice(1);
        return random_choice(4);
    }
    if (effect == DZ_EFFECT_CONES) {
        for (int i = 0; i < 3; ++i) random_choice(1);
        unsigned corner = random_choice(4), direction = random_choice(3);
        return corner*5+(direction ? 1+2*(direction-1)+random_choice(2) : 0);
    }
    if (effect == DZ_EFFECT_LITTLESTARS) {
        unsigned r = random_choice(3*screen_extent/8)+screen_extent/8;
        if (!random_choice(2)) random_choice(3);
        unsigned spacing = ((random_choice(4)+2)*r/20)&0xfffeu;
        random_choice(screen_extent); random_choice(screen_extent);
        if (random_choice(2)) random_choice(spacing);
        return video_adapter == ADAPTER_VGA ? random_choice(3) : 3;
    }
    return 0;
}
static void variant(unsigned e, unsigned local)
{
    unsigned v = 0;
    switch (e) {
    case DZ_EFFECT_CONES: case DZ_EFFECT_SQUIRALS: case DZ_EFFECT_LITTLESTARS: v = local; break;
    case DZ_EFFECT_BIGSTAR: v = (unsigned)bigstar_color_mode; break;
    case DZ_EFFECT_ELLIPSE: v = (unsigned)(ellipse_draw_mode*2+effect_parameter); break;
    case DZ_EFFECT_SWEEP2: case DZ_EFFECT_PUFFS: v = (unsigned)effect_parameter; break;
    case DZ_EFFECT_DAISIES: v = (unsigned)daisies_color_per_ray; break;
    case DZ_EFFECT_CYCLOIDS: v = (unsigned)cycloids_color_per_point; break;
    case DZ_EFFECT_ARCHES: v = arches_first_radius_y+8 == 2*(screen_half_height-1); break;
    case DZ_EFFECT_YARNBALL:
        v = yarnball_full_ring ? (unsigned)((yarnball_full_ring-1)*2+!!(yarnball_draw_flags&32)) : 4; break;
    case DZ_EFFECT_GUMBALLS:
        v = gumballs_draw_flags & SHAPE_FILL ?
            (gumballs_draw_flags & ELLIPSE_FILL_COLOR_PER_ROW ? 3u : (unsigned)((gumballs_draw_flags&ELLIPSE_FILL_COLOR_MASK)>>2))
            : 4u+!!(gumballs_draw_flags&ELLIPSE_EDGE_COLOR_PER_POINT); break;
    }
    if (v < 64) variants[e] |= UINT64_C(1) << v;
}
static enum dz_native_instance frame(unsigned e, int rebuild, unsigned seed, int kind, const char *phase, int outer)
{
    unsigned local = local_variant(e);
    stores = (struct dz_canvas_store_audit){0}; dz_canvas_store_audit = &stores;
    record(e)->draw((int16_t)rebuild);
    if (outer) plot_callback(draw_symmetry);
    dz_canvas_store_audit = NULL;
    enum dz_native_instance state = dz_native_instance(e);
    ++frames;
    if (state == DZ_NATIVE_DRAWING) native_leaks += stores.faithful;
    else fallback_leaks += stores.native;
    variant(e,local);
    if ((state == DZ_NATIVE_DRAWING && (stores.faithful || (outer && !stores.native))) || state == DZ_NATIVE_UNSELECTED ||
        (state != DZ_NATIVE_DRAWING && stores.native)) {
        leaked |= UINT64_C(1) << e; ++failures;
        if (report && reported[e]++ < 8)
            printf("LEAK effect=%s mode=%d seed=%u phase=%s stores=%llu native=%llu instance=%d\n",
                record(e)->name,kind,seed,phase,(unsigned long long)stores.faithful,
                (unsigned long long)stores.native,state);
        if (report && e == DZ_EFFECT_ARCHES && reported[e] <= 8)
            printf("ARCHES radii=%d,%d,%d,%d trace=%d\n",arches_first_radius_y+8,
                arches_first_radius_x,arches_second_radius_y,arches_second_radius_x+8,ellipse_trace_limit);
    }
    return state;
}
int main(int argc, char **argv)
{
    int extended = argc > 1 && !strcmp(argv[1],"--long");
    report = (argc > 1 && !strcmp(argv[argc-1],"--report"));
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *surface = SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
    renderer = SDL_CreateSoftwareRenderer(surface); assert(renderer);
    dz_display.canvas = DZ_CANVAS_SCREEN;
    for (int presentation = 0; presentation < 4; ++presentation)
    for (int kind = 0; kind < 2; ++kind) {
        policy = (enum dz_presentation)presentation;
        mode(kind,extended ? 1920 : 960,extended ? 1080 : 540);
        for (unsigned i = 0; i < sizeof effects/sizeof effects[0]; ++i) {
            unsigned e = effects[i];
            if (argc > 1 && !strcmp(argv[1],"--arches") && e != DZ_EFFECT_ARCHES) continue;
            for (unsigned seed = 1; seed <= (extended ? 64u : 32u); ++seed) {
                reset(e,seed);
                frame(e,1,seed,kind,"first",1);
                frame(e,1,seed,kind,"unplotted",0);
                frame(e,1,seed,kind,"restart",1);
            }
            const unsigned seeds[] = {1,32,64,48};
            for (unsigned run = 0; run < (extended ? 4u : 2u); ++run) {
                unsigned seed = seeds[run];
                reset(e,seed);
                if (e == DZ_EFFECT_ARCHES) image_work_limit = UINT16_MAX;
                enum dz_native_instance state = frame(e,1,seed,kind,"long-first",1);
                uint64_t end = dz_virtual_ns()+(extended ? UINT64_C(20000000000) : UINT64_C(2000000000));
                while (dz_virtual_ns() < end) {
                    /* Indefinite F10 eventually gives arches negative radii
                     * and aborts even in Classic. Rebuild before that DOS limit. */
                    int rebuild = e == DZ_EFFECT_ARCHES &&
                        (arches_first_radius_y < 8 || arches_second_radius_x < 8);
                    if (rebuild) image_work_count = 0;
                    enum dz_native_instance next = frame(e,rebuild,seed,kind,"long",1);
                    assert(next == state);
                }
            }
            if (e == DZ_EFFECT_ARCHES) {
                reset(e,1); image_work_limit = 1600;
                unsigned calls = 0;
                uint64_t end = extended ? UINT64_C(20000000000) : UINT64_C(2000000000);
                while (dz_virtual_ns() < end) {
                    int rebuild = !calls;
                    if (calls >= 2) {
                        unsigned previous = (uint16_t)image_work_count;
                        image_work_count = (int16_t)(previous+1);
                        if (previous > image_work_limit) { rebuild = 1; image_work_count = 0; calls = 0; }
                    }
                    assert(frame(e,rebuild,1,kind,"bounded-auto",1) == DZ_NATIVE_DRAWING);
                    ++calls;
                }
                reset(e,1); semi_auto_mode = 1;
                assert(frame(e,1,1,kind,"semi-auto",1) == DZ_NATIVE_UNBOUNDED);
            }
            if (e == DZ_EFFECT_YARNBALL) for (unsigned seed = 32; seed <= 64; seed += 32) {
                reset(e,seed);
                frame(e,1,seed,kind,"full-cluster-init",1);
                frame(e,0,seed,kind,"full-cluster-edge",1);
                frame(e,0,seed,kind,"full-cluster-solid",1);
            }
        }
    }
    /* The old sweep adapter left an unplotted cyclic callback alive when a
     * same-owner rebuild selected its accelerating Faithful variant. */
    mode(0,960,540); reset(DZ_EFFECT_SWEEP2,1);
    assert(frame(DZ_EFFECT_SWEEP2,1,1,0,"cyclic-unplotted",0) == DZ_NATIVE_DRAWING);
    dz_srand(48);
    assert(frame(DZ_EFFECT_SWEEP2,1,48,0,"accelerating-restart",1) == DZ_NATIVE_ACCELERATING);
    reset(DZ_EFFECT_SWEEP2,48); targa64_mode = 1;
    assert(frame(DZ_EFFECT_SWEEP2,1,48,0,"forced-cyclic",1) == DZ_NATIVE_DRAWING);
    targa64_mode = 0;
    /* Prove that the physical-store counter detects a deliberately missing
     * adapter, rather than relying on Native ink or a visual difference. */
    reset(DZ_EFFECT_SINE,1); sine(1); dz_native_plot = NULL;
    stores = (struct dz_canvas_store_audit){0}; dz_canvas_store_audit = &stores;
    plot_callback(1); dz_canvas_store_audit = NULL;
    assert(stores.faithful && !stores.native);
    /* Explicit whole-instance cases: no Native ink at all is allowed. */
    mode(3,960,540); reset(DZ_EFFECT_ARCHES,48);
    assert(frame(DZ_EFFECT_ARCHES,1,48,3,"cache-spill",1) == DZ_NATIVE_CACHE_SPILL);
    mode(2,960,540);
    reset(DZ_EFFECT_BIGSTAR,1);
    assert(frame(DZ_EFFECT_BIGSTAR,1,1,2,"ega-colour",1) == DZ_NATIVE_DRAWING);
    for (unsigned i = 0; i < sizeof effects/sizeof effects[0]; ++i) {
        unsigned e = effects[i]; reset(e,1); dz_canvas_raster_xor(1);
        assert(frame(e,1,1,2,"xor",1) == DZ_NATIVE_XOR);
    }
    for (unsigned i = 0; i < sizeof effects/sizeof effects[0]; ++i) {
        unsigned e = effects[i];
        printf("AUDIT effect=%s leaked=%u variants=%llx\n",record(e)->name,
            (unsigned)(leaked >> e & 1u),(unsigned long long)variants[e]);
    }
    if (!report) {
        assert((variants[DZ_EFFECT_CONES]&0xfffffu) == 0xfffffu);
        assert((variants[DZ_EFFECT_ELLIPSE]&0x3fu) == 0x3fu);
        assert((variants[DZ_EFFECT_GUMBALLS]&0x3fu) == 0x3fu);
        assert((variants[DZ_EFFECT_YARNBALL]&0x1fu) == 0x1fu);
        assert((variants[DZ_EFFECT_LITTLESTARS]&0xfu) == 0xfu);
        assert((variants[DZ_EFFECT_SQUIRALS]&0xfu) == 0xfu);
        assert((variants[DZ_EFFECT_BIGSTAR]&7u) == 7u);
        assert((variants[DZ_EFFECT_ARCHES]&3u) == 3u);
        assert((variants[DZ_EFFECT_CYCLOIDS]&3u) == 3u);
        assert((variants[DZ_EFFECT_DAISIES]&3u) == 3u);
        assert((variants[DZ_EFFECT_PUFFS]&3u) == 3u);
        assert((variants[DZ_EFFECT_SWEEP2]&3u) == 3u);
    }
    printf("PHYSICAL frames=%llu faithful_in_native=%llu native_in_fallback=%llu\n",
        (unsigned long long)frames,(unsigned long long)native_leaks,(unsigned long long)fallback_leaks);
    dz_canvas_store_audit = NULL;
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface); dz_shutdown();
    return failures && !report ? 1 : 0;
}
