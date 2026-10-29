/* Production observer/compositor invariants, independent of golden images. */
#include "dazzle.h"
#include "dzconst.h"
#include "platform/hybrid.h"
#include <assert.h>
#include <math.h>
#include <string.h>

static SDL_Color colors[256];
static int width, height;
static enum dz_presentation policy;

static void begin(int mode, int w, int h, enum dz_presentation p)
{
    free(palette_buffer); palette_buffer = NULL;
    free(linewipe_history); linewipe_history = NULL;
    free(ball_bitmap_pixels); ball_bitmap_pixels = NULL;
    dz_hybrid_shutdown();
    dz_display.quality = DZ_QUALITY_HYBRID;
    video_adapter = mode < 2 ? ADAPTER_VGA : ADAPTER_EGA;
    bios_resolution_mode = mode == 1 || mode == 2;
    configure_video_backend(1);
    dz_video_set_mode(mode < 2 ? (mode ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400) : DZ_MODE_EGA_350, 1);
    screen_half_width = dz_video.width / 2;
    screen_half_height = mode == 3 ? 350 : dz_video.height / 2;
    page_paragraph = 0; draw_delay_count = 1; plot_color = 1;
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    dz_clock_reset();
    width = w; height = h; policy = p;
    assert(dz_hybrid_resize(w, h, p));
    for (int i = 0; i < 256; ++i) colors[i] = (SDL_Color){0, 0, 0, 255};
    colors[1].r = 255; colors[2].g = 255; colors[3].b = 255;
}

static void point(dz_stroke_id id, double x, double y, int sx, int sy, unsigned symmetry)
{
    draw_x = (int16_t)sx; draw_y = (int16_t)sy;
    if (id) dz_stroke_sample(id, x, y, sx, sy);
    plot_callback((uint16_t)symmetry);
}

static void line(dz_stroke_id id, int x0, int y0, int x1, int y1)
{
    draw_x = (int16_t)x0; draw_y = (int16_t)y0;
    int done = 1;
    do {
        done = line_step((int16_t)x1, (int16_t)y1, (int16_t)done);
        dz_stroke_line(id, x0, y0, x1, y1, draw_x, draw_y);
        plot_callback(0);
    } while (!done);
}

static uint32_t *compose(void) { return dz_hybrid_compose(colors); }

static uint32_t at(double x, double y)
{
    SDL_FRect v = dz_present_viewport(width, height, policy);
    int px = (int)floor(v.x + x * v.w / dz_video.width);
    int py = (int)floor(v.y + y * v.h / dz_video.height);
    assert(px >= 0 && px < width && py >= 0 && py < height);
    return compose()[py * width + px];
}

static uint32_t rgb(uint8_t index)
{
    SDL_Color c = colors[index];
    return 0xff000000u | (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
}

static void canonical(void)
{
    uint32_t *pixels = compose();
    SDL_FRect v = dz_present_viewport(width, height, policy);
    uint32_t cap = DZ_VRAM_PLANE_BYTES * (unsigned)dz_video.pixels_per_byte;
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        double lx = (x + 0.5 - v.x) * dz_video.width / v.w;
        double ly = (y + 0.5 - v.y) * dz_video.height / v.h;
        uint32_t expected = 0xff000000u;
        if (lx >= 0 && lx < dz_video.width && ly >= 0 && ly < dz_video.height) {
            unsigned sy = (unsigned)ly;
            unsigned row = sy > dz_video.line_compare ? sy - dz_video.line_compare - 1 : dz_video.display_start_row + sy;
            expected = rgb(dz_video.vram[(row * (unsigned)dz_video.width + (unsigned)lx) % cap]);
        }
        assert(pixels[y * width + x] == expected);
    }
}

static void footprint(void)
{
    for (int mode = 0; mode < 4; ++mode) for (int p = 0; p < 3; ++p) {
        begin(mode, 701, 479, (enum dz_presentation)p);
        dz_stroke_id id = dz_stroke_begin(DZ_STROKE_CYCLOIDS);
        /* Doubled EGA deliberately rejects every other software row. */
        line(id, -30, 0, 30, 0);
        double cy = dz_video.height / 2.0 + 0.5;
        for (int x = -29; x <= 29; ++x)
            assert((at(screen_half_width + x + 0.5, cy) & 0xff0000u) != 0);
        assert(at(screen_half_width + 0.5, cy + 2) == 0xff000000u);
        assert(at(screen_half_width + 0.5, cy - 2) == 0xff000000u);
        line(id, 30, 0, 30, 30);
        double factor = mode == 3 ? 0.5 : 1;
        for (int y = 0; y < 30; y += mode == 3 ? 2 : 1)
            if (!(at(screen_half_width + 30.5, dz_video.height / 2.0 + y * factor + 0.5) & 0xff0000u)) {
                fprintf(stderr, "vertical gap mode=%d policy=%d y=%d\n", mode, p, y); abort();
            }
        dz_hybrid_retire();
        id = dz_stroke_begin(DZ_STROKE_CYCLOIDS);
        line(id, -25, -24, 25, 24);
        uint32_t *pixels = compose();
        unsigned partial = 0;
        for (int i = 0; i < width * height; ++i)
            if ((pixels[i] & 0xffffffu) && pixels[i] != 0xffff0000u) ++partial;
        assert(partial > 10);
        /* Joins have coverage at every committed sample, including EGA parity. */
        for (int x = -23; x <= 23; ++x) {
            double y = x * 48.0 / 50.0;
            assert((at(screen_half_width + x + 0.5, dz_video.height / 2.0 + y * factor + 0.5) & 0xff0000u) != 0);
        }
    }
}

static void ownership(void)
{
    begin(0, 960, 720, DZ_PRESENT_ORIGINAL);
    plot_color = 2; point(0, 0, 0, 0, 0, 0);
    plot_color = 1;
    dz_stroke_id id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, -1.3, -0.7, -1, 0, 0);
    point(id, 0.3, 0.7, 0, 0, 0);
    /* The original green dot is the presentation-only saved under. */
    uint32_t *pixels = compose(); unsigned mixed = 0;
    for (int i = 0; i < width * height; ++i)
        if ((pixels[i] & 0xff0000u) && (pixels[i] & 0xff00u)) ++mixed;
    assert(mixed);
    /* A later dot occludes ink throughout its whole logical cell. */
    plot_color = 3; point(0, 0, 0, 0, 0, 0);
    assert(at(160.5, 200.5) == 0xff0000ffu);
    /* Same-color ordinary stores also invalidate ownership. */
    plot_color = 1; point(0, -1, 0, -1, 0, 0);
    canonical();
    /* Duplicate owned stores keep first underlay and transfer generation. */
    id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, 10.3, 10.3, 10, 10, 0);
    uint32_t *copy = malloc((size_t)width * height * 4);
    memcpy(copy, compose(), (size_t)width * height * 4);
    point(id, 10.3, 10.3, 10, 10, 0);
    assert(!memcmp(copy, compose(), (size_t)width * height * 4)); free(copy);
    /* Retagging must not resurrect an invalid halo before the next present. */
    id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, 20.8, 20.3, 20, 20, 0);
    plot_color = 3; point(0, 21, 20, 21, 20, 0);
    dz_stroke_break(id); plot_color = 1;
    point(id, 19.7, 20.3, 20, 20, 0);
    assert(at(181.2, 220.5) == 0xff0000ffu);
    /* Remove the fixture before the whole-output canonical check below. */
    plot_color = 0; point(0, 20, 20, 20, 20, 0);
    /* A competing primitive never reveals a nested underlay. */
    plot_color = 3; id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, 10.3, 10.3, 10, 10, 0);
    plot_color = 0; point(0, 10, 10, 10, 10, 0);
    canonical();
    /* Partial clear, then alternating full-aperture clear. */
    plot_color = 1; id = dz_stroke_begin(DZ_STROKE_CYCLOIDS);
    line(id, -30, -200, 30, -190);
    clear_vram_words(0, 80, 0);
    clear_vram_words(0x0302, 32768, 0);
    canonical();
    /* A retired generation cannot come back after the per-address limit. */
    id = dz_stroke_begin(DZ_STROKE_SINE); plot_color = 1;
    point(id, 0.3, 0.3, 0, 0, 0);
    for (int i = 0; i < 8200; ++i) point(0, 0, 0, 0, 0, 0);
    canonical();
}

static void palette_and_mapping(void)
{
    begin(0, 960, 720, DZ_PRESENT_ORIGINAL);
    dz_stroke_id id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, -10.3, -10.3, -10, -10, 0);
    point(id, -9.3, -9.6, -9, -9, 0);
    uint32_t *copy = malloc((size_t)width * height * 4);
    memcpy(copy, compose(), (size_t)width * height * 4);
    colors[1] = (SDL_Color){63, 127, 251, 255};
    uint32_t *pixels = compose();
    for (int i = 0; i < width * height; ++i) {
        unsigned a = (copy[i] >> 16) & 255u;
        assert(pixels[i] == (0xff000000u | ((63u*a+127)/255) << 16 |
            ((127u*a+127)/255) << 8 | (251u*a+127)/255));
    }
    free(copy);
    /* Hidden page ink follows display addressing, not window coordinates. */
    dz_hybrid_retire(); colors[1] = (SDL_Color){255, 0, 0, 255};
    page_paragraph = 2000;
    id = dz_stroke_begin(DZ_STROKE_CYCLOIDS);
    line(id, -10, -200, 10, -200);
    assert(at(160.5, 0.5) == 0xff000000u);
    dz_video.display_start_row = 400;
    assert(at(160.5, 0.5) == 0xffff0000u);
    dz_video.display_start_row = 0; page_paragraph = 0;
    id = dz_stroke_begin(DZ_STROKE_CYCLOIDS); line(id, -10, -200, 10, -200);
    dz_video.display_start_row = 100; dz_video.line_compare = 100;
    assert(at(160.5, 100.5) == 0xff000000u);
    assert(at(160.5, 101.5) == 0xffff0000u);
    /* Partial final row/wrap; after retirement exact Classic addressing. */
    dz_hybrid_retire();
    dz_video.display_start_row = 817; dz_video.line_compare = 187; canonical();
    dz_video.display_start_row = 0; dz_video.line_compare = 400;
    id = dz_stroke_begin(DZ_STROKE_SINE); point(id, 0.3, 0.3, 0, 0, 0);
    assert(dz_hybrid_resize(641, 479, DZ_PRESENT_CROP));
    width = 641; height = 479; policy = DZ_PRESENT_CROP; canonical();
    assert(!dz_hybrid_resize(32768, 32768, DZ_PRESENT_ORIGINAL));
    assert(dz_hybrid_bytes() == 0);
    /* Live geometry on both sides of the partial final VRAM row. */
    begin(0, 960, 720, DZ_PRESENT_ORIGINAL);
    id = dz_stroke_begin(DZ_STROKE_CYCLOIDS); line(id, -160, -200, -140, -200);
    page_paragraph = 4000; plot_color = 3;
    id = dz_stroke_begin(DZ_STROKE_CYCLOIDS); line(id, -150, -181, -130, -181);
    dz_video.display_start_row = 819;
    assert(at(20.5, 0.5) == 0xff0000ffu);
    assert(at(74.5, 0.5) == 0xffff0000u);
    begin(2, 960, 720, DZ_PRESENT_ORIGINAL);
    page_paragraph = 1750;
    id = dz_stroke_begin(DZ_STROKE_CYCLOIDS); line(id, -10, -175, 10, -175);
    dz_video.display_start_row = 350;
    assert(at(320.5, 0.5) == 0xffff0000u);
}

static uint32_t *yield_copy;
static void during_yield(void)
{
    assert(!memcmp(yield_copy, compose(), (size_t)width * height * 4));
}

static void progress_and_fallback(void)
{
    begin(0, 640, 480, DZ_PRESENT_ORIGINAL);
    dz_stroke_id id = dz_stroke_begin(DZ_STROKE_SWEEP2);
    point(id, 0, 0, 0, 0, 0);
    yield_copy = malloc((size_t)width * height * 4);
    memcpy(yield_copy, compose(), (size_t)width * height * 4);
    dz_stroke_line(id, 0, 0, 0, 100, 0, 1);
    dz_timer_tick_handler = during_yield;
    dz_charge(DZ_WORK_UNITS_PER_SECOND / 10);
    dz_timer_tick_handler = NULL; free(yield_copy);
    draw_x = 0; draw_y = 1; plot_callback(0);
    assert(at(160.5, 210.5) == 0xff000000u);
    dz_hybrid_retire();
    id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, 0.3, 0.3, 0, 0, 0);
    point(id, NAN, INFINITY, 0, 0, 0); canonical();
    begin(3, 640, 480, DZ_PRESENT_ORIGINAL);
    id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, 10.3, 10.3, 10, 10, 3);
    set_ega_raster_xor(1);
    point(0, 10, 10, 10, 10, 3); point(0, 10, 10, 10, 10, 3);
    canonical();
    set_ega_raster_xor(0); canonical();
    id = dz_stroke_begin(DZ_STROKE_SINE); point(id, 0.3, 0.3, 0, 0, 3);
    dz_video_set_mode(DZ_MODE_EGA_350, 0); canonical();
}

static void emitter_resets(void)
{
    unsigned seen = 0;
    for (unsigned seed = 1; seed <= 8; ++seed) {
        begin(0, 640, 480, DZ_PRESENT_ORIGINAL); dz_srand(seed * 50);
        unsigned symmetry = seed % 3 == 2 ? 3 : seed % 3;
        sweep2(1); seen |= 1u << effect_parameter; plot_callback((uint16_t)symmetry);
        for (int j = 0; j < 700; ++j) { sweep2(0); plot_callback((uint16_t)symmetry); }
        /* More than three endpoint resets, without painting a future ray. */
        assert(sweep2_end_x != 0 && dz_hybrid_compose(colors));
    }
    assert(seen == 3); /* Both original endpoint modes, plus 0/1/3 symmetry. */
    begin(0, 960, 720, DZ_PRESENT_ORIGINAL);
    dz_stroke_id id = dz_stroke_begin(DZ_STROKE_CYCLOIDS);
    point(id, -1, -1, -1, -1, 0);
    plot_color = 2; point(id, 0, 0, 0, 0, 0);
    uint32_t pixel = at(159.8, 200.2);
    assert(((pixel >> 16) & 255u) + ((pixel >> 8) & 255u) == 255u);
}

static void stamps_and_symmetry(void)
{
    begin(0, 960, 720, DZ_PRESENT_ORIGINAL);
    dz_srand(1);
    for (int i = 0; i < 100; ++i) { lotsadots(i == 0); plot_callback(3); }
    canonical(); /* Stamps alone remain precisely the canonical raster. */
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    plot_color = 1;
    dz_stroke_id id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, -10.3, -30.3, -10, -30, 3);
    point(id, -9.3, -29.3, -9, -29, 3);
    assert(at(160.5, 200.5) == 0xff000000u); /* No connector between branches. */
    /* Every reflected/swapped branch reaches its own accepted store. */
    for (int swap = 0; swap < 2; ++swap) for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2)
            assert((at(160.5 + sx * (swap ? 29 : 9), 200.5 + sy * (swap ? 9 : 29)) & 0xffffffu) != 0);
    uint8_t row[320] = {0};
    /* Planar row stores retire owned geometry, including unchanged bits. */
    begin(2, 960, 720, DZ_PRESENT_ORIGINAL);
    id = dz_stroke_begin(DZ_STROKE_SINE); point(id, 0.3, 0.3, 0, 0, 0);
    ega_init_row_table_and_mode();
    id = dz_stroke_begin(DZ_STROKE_SINE); point(id, 0.3, 0.3, 0, 0, 0);
    ega_write_planar_row(row, 80, 175); canonical();
}

static void presenter_cache(void)
{
    assert(SDL_InitSubSystem(SDL_INIT_VIDEO));
    begin(0, 640, 480, DZ_PRESENT_ORIGINAL);
    SDL_Surface *surface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    struct dz_presenter *p = dz_presenter_create(renderer, DZ_RENDERER_AUTO);
    assert(p && dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    dz_stroke_id id = dz_stroke_begin(DZ_STROKE_SINE);
    point(id, 0.3, 0.3, 0, 0, 0);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    dz_video.dac[1][0] ^= 63u;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    dz_video.display_start_row = 400; dz_video.line_compare = 100;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 1) == 1);
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 701, 479);
    assert(target && SDL_SetRenderTarget(renderer, target));
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 0) == 0);
    assert(SDL_SetRenderTarget(renderer, NULL)); SDL_DestroyTexture(target);
    dz_presenter_destroy(p); SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);
}

int main(void)
{
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize());
    assert(dz_display.quality == DZ_QUALITY_CLASSIC && dz_hybrid_bytes() == 0);
    assert(dz_stroke_begin(DZ_STROKE_SINE) == 0);
    footprint(); ownership(); palette_and_mapping(); progress_and_fallback(); stamps_and_symmetry();
    emitter_resets(); presenter_cache();
    free(palette_buffer); free(linewipe_history); free(ball_bitmap_pixels);
    dz_shutdown();
    puts("Hybrid footprint, joins, ownership, generations, palette, mapping, prefix and fallback passed");
    return 0;
}
