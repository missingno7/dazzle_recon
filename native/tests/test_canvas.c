#include "platform/canvas.h"
#include "dazzle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "canvas_capture.h"

static SDL_Renderer *renderer;
static struct dz_presenter *classic, *screen, *rgba;
static unsigned rebuilds;
static void rebuild_image(int16_t rebuild)
{
    assert(rebuild == 1);
    ++rebuilds;
    dz_canvas_write(1, 55);
}
static void no_plot(uint16_t symmetry) { (void)symmetry; }

static void fade_fold(void)
{
    dz_display.canvas = DZ_CANVAS_SCREEN;
    dz_display.native_aa = 0;
    const int widths[] = {960,1260,800,961,3000};
    const int heights[] = {540,540,600,540,300};
    for (unsigned size = 0; size < sizeof widths/sizeof *widths; ++size) {
        int w = widths[size], h = heights[size];
        dz_video_set_mode(DZ_MODE_EGA_350,1);
        assert(dz_canvas_configure(renderer,w,h,DZ_PRESENT_EXTENDED) == 1);
        dz_canvas_take_restart(); dz_canvas_select_page(0);
        for (int y = 0; y < h; ++y) dz_canvas_native_span(y,0,w,6);
        struct dz_canvas_view v = dz_canvas_surface();
        size_t bytes = (size_t)v.width*v.height;
        uint8_t *before = malloc(bytes); assert(before); memcpy(before,v.indices,bytes);
        dz_canvas_fade_begin('e');
        assert(!memcmp(before,dz_canvas_surface().indices,bytes)); /* old entry clear */
        SDL_FRect r = dz_canvas_content();
        int left = (int)r.x, right = left+(int)r.w, span = right-left;
        for (int pass = 0; pass < 2; ++pass) {
            for (unsigned x = 0; x < 640; ++x) dz_canvas_store(x,3,0);
            v = dz_canvas_surface();
            for (int x = 0; x < w; ++x) assert(v.indices[x] == 3);
            dz_canvas_raster_xor(1);
            for (unsigned x = 0; x < 640; ++x) dz_canvas_store(x,5,1);
            v = dz_canvas_surface();
            for (int x = 0; x < w; ++x) assert(v.indices[x] == (3^5));
            for (unsigned x = 0; x < 640; ++x) dz_canvas_store(x,5,1);
            v = dz_canvas_surface();
            for (int x = 0; x < w; ++x) assert(v.indices[x] == 3); /* once per fold, no doubled edge XOR */
            dz_canvas_raster_xor(0);
        }
        /* A nonuniform centre proves the fold repeats with reversed parity. */
        for (unsigned x = 0; x < 640; ++x) dz_canvas_store(x,(uint8_t)(x%13+1),0);
        v = dz_canvas_surface();
        for (int x = 0; x < w; ++x) if (x < left || x >= right) {
            int q = (x-left)%(2*span); if (q < 0) q += 2*span;
            if (q >= span) q = 2*span-1-q;
            assert(v.indices[x] == v.indices[left+q]);
        }
        if (left) {
            /* Read the reflected copy before a later store changes its source.
             * The centre/control colour must never replace this side sample. */
            dz_canvas_native_span(0,0,left,7);
            dz_canvas_control_pixel_read(0,0,8);
            int lanes = dz_canvas_sample_lanes();
            uint8_t samples[lanes+1]; dz_canvas_sample_sides(samples);
            assert(samples[lanes/2-1] == 7);
            dz_canvas_native_span(0,0,left,9);
            dz_canvas_override_sides(samples); dz_canvas_store(1,11,0);
            dz_canvas_override_sides(NULL);
            v = dz_canvas_surface();
            for (int x = 0; x < left; ++x)
                if (dz_canvas_fold_address(x,0) == 1) assert(v.indices[x] == 7);
            /* A downsampled-away source cell has no side copy to sample. */
            uint8_t seen[640] = {0};
            for (int x = left; x < right; ++x) seen[dz_canvas_fold_address(x,0)] = 1;
            for (unsigned x = 0; x < 640; ++x) if (!seen[x]) {
                dz_canvas_control_pixel_read((uint16_t)x,0,8);
                uint8_t scalar = dz_canvas_sample();
                dz_canvas_sample_sides(samples);
                for (int lane = 0; lane < lanes; ++lane) assert(samples[lane] == scalar);
                break;
            }
            /* Real reach owns the pixel across separate plot calls. Its first
             * XOR discards earlier fallback XOR; later fallback cannot cancel it. */
            dz_canvas_fade_begin('e');
            dz_canvas_raster_xor(1);
            dz_canvas_store(0,5,1);
            assert(dz_canvas_surface().indices[left-1] == (9^5));
            dz_canvas_side_plot(-1,0,5);
            assert(dz_canvas_surface().indices[left-1] == (9^5));
            dz_canvas_store(0,5,1);
            assert(dz_canvas_surface().indices[left-1] == (9^5));
            dz_canvas_side_plot(-1,0,5); /* two genuine XOR stores retain original parity */
            assert(dz_canvas_surface().indices[left-1] == 9);
            dz_canvas_raster_xor(0);
            dz_canvas_side_plot(-1,0,7);
            dz_canvas_store(0,0,0); /* a real head must not block its folded eraser */
            assert(!dz_canvas_surface().indices[left-1]);
            dz_canvas_store(0,3,0);
            assert(!dz_canvas_surface().indices[left-1]); /* no later fallback repaint */
        }
        memcpy(before,v.indices,bytes); display_cleared = 1;
        dz_canvas_fade_end();
        assert(!memcmp(before,dz_canvas_surface().indices,bytes)); /* no end pop */
        dz_canvas_fade_begin('o');
        dz_video_set_mode(DZ_MODE_EGA_350,1); /* Fireworks' mode reset */
        assert(dz_canvas_surface().indices);
        dz_canvas_store(0,9,0);
        v = dz_canvas_surface();
        if (left) assert(v.indices[left-1] == 9);
        /* Every zero store participates even if the logical cell was zero. */
        for (unsigned a = 0; a < 640u*350u; ++a) dz_canvas_store(a,0,0);
        v = dz_canvas_surface();
        for (int i = 0; i < w*h; ++i) assert(!v.indices[i]);
        dz_canvas_fade_end();
        free(before);
    }
}

static void uploads(void)
{
    dz_display.canvas = DZ_CANVAS_SCREEN;
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,1) == 1);
    struct dz_upload_stats before = dz_presenter_upload_stats(screen);
    uint32_t address = 160u+200u*320u;
    dz_canvas_write(address,19);
    struct dz_canvas_view v = dz_canvas_surface();
    assert(v.dirty_count == 1 && v.dirty[0].w > 0 && v.dirty[0].h > 0 && v.dirty[0].w < 4 && v.dirty[0].h < 4);
    uint64_t bytes = (uint64_t)v.dirty[0].w*v.dirty[0].h;
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 1);
    struct dz_upload_stats after = dz_presenter_upload_stats(screen);
    assert(after.textures == before.textures && after.uploads == before.uploads+1);
    assert(after.bytes-before.bytes == bytes);
    assert(!dz_canvas_surface().dirty_count);
    before = after;
    dz_canvas_write(20u*320u+10u,21); dz_canvas_write(380u*320u+300u,22);
    v = dz_canvas_surface();
    assert(v.dirty_count == 2);
    bytes = 0;
    for (int i = 0; i < v.dirty_count; ++i) bytes += (uint64_t)v.dirty[i].w*v.dirty[i].h;
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 1);
    after = dz_presenter_upload_stats(screen);
    assert(after.uploads-before.uploads == 2 && after.bytes-before.bytes == bytes);
    before = after;
    dz_video.dac[19][0] ^= 63;
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 1);
    dz_canvas_start(817); dz_canvas_split(187);
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 1);
    SDL_Surface *pixels = SDL_RenderReadPixels(renderer,NULL);
    assert(pixels); canvas_assert_pixels(pixels); SDL_DestroySurface(pixels);
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,1) == 1);
    after = dz_presenter_upload_stats(screen);
    assert(after.uploads == before.uploads && after.bytes == before.bytes && after.textures == before.textures);
    struct dz_canvas_quad quads[3];
    assert(dz_canvas_quads(quads) == 3);
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 0);
}

static void compare(enum dz_presentation policy)
{
    dz_display.canvas = DZ_CANVAS_CLASSIC;
    assert(dz_presenter_draw(classic, policy, 1) == 1);
    SDL_Surface *a = SDL_RenderReadPixels(renderer, NULL);
    dz_display.canvas = DZ_CANVAS_SCREEN;
    assert(dz_presenter_draw(screen, policy, 1) == 1);
    SDL_Surface *b = SDL_RenderReadPixels(renderer, NULL);
    assert(a && b && a->format == b->format);
    canvas_assert_pixels(b);
    if (!dz_video.display_start_row && dz_video.line_compare >= (unsigned)dz_video.height)
        for (int y = 0; y < a->h; ++y)
            assert(!memcmp((uint8_t *)a->pixels + y * a->pitch, (uint8_t *)b->pixels + y * b->pitch, (size_t)a->w * 4));
    assert(dz_presenter_draw(screen, policy, 0) == 0);
    assert(dz_presenter_draw(rgba, policy, 1) == 1);
    SDL_Surface *c = SDL_RenderReadPixels(renderer, NULL);
    assert(c && c->format == a->format);
    for (int y = 0; y < b->h; ++y)
        assert(!memcmp((uint8_t *)b->pixels + y * b->pitch, (uint8_t *)c->pixels + y * c->pitch, (size_t)b->w * 4));
    assert(dz_presenter_draw(rgba, policy, 0) == 0);
    SDL_DestroySurface(c);
    SDL_DestroySurface(a); SDL_DestroySurface(b);
}

static void native_uploads(void)
{
    dz_display.canvas = DZ_CANVAS_SCREEN;
    dz_video_set_mode(DZ_MODE_VGA_400,1);
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,1) == 1);
    const double strokes[][5] = {{1,1,20,35,2},{253.4,250.8,270.1,265.5,5},
        {630,467,644,482,7},{-8,-10,17,19,3},{130,270,132,275,4}};
    for (unsigned i = 0; i < sizeof strokes/sizeof strokes[0]; ++i) {
        const double *s = strokes[i];
        dz_canvas_stroke(s[0],s[1],s[2],s[3],s[4],(uint8_t)(i+1));
        assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 1);
        SDL_Surface *pixels = SDL_RenderReadPixels(renderer,NULL);
        assert(pixels); canvas_assert_pixels(pixels); SDL_DestroySurface(pixels);
    }
    dz_canvas_select_page(4090); /* stroke crosses the physical aperture wrap */
    dz_canvas_stroke(300,0,305,30,4,9);
    dz_canvas_start(818);
    assert(dz_presenter_draw(screen,DZ_PRESENT_ORIGINAL,0) == 1);
    SDL_Surface *pixels = SDL_RenderReadPixels(renderer,NULL);
    assert(pixels); canvas_assert_pixels(pixels); SDL_DestroySurface(pixels);
    dz_canvas_select_page(0);
}

int main(int argc, char **argv)
{
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Window *window = NULL;
    SDL_Surface *surface = NULL;
    if (argc == 3 && !strcmp(argv[1],"--backend")) {
        window = SDL_CreateWindow("canvas test",641,479,SDL_WINDOW_HIDDEN);
        assert(window);
        renderer = !strcmp(argv[2],"gpu") ? SDL_CreateGPURenderer(NULL,window) : SDL_CreateRenderer(window,argv[2]);
    } else {
        surface = SDL_CreateSurface(641, 479, SDL_PIXELFORMAT_ARGB8888);
        renderer = SDL_CreateSoftwareRenderer(surface);
    }
    assert(renderer);
    classic = dz_presenter_create(renderer, DZ_RENDERER_INDEXED);
    screen = dz_presenter_create(renderer, DZ_RENDERER_AUTO);
    rgba = dz_presenter_create(renderer, DZ_RENDERER_RGBA);
    assert(classic && screen && rgba);
    uploads();
    native_uploads();
    fade_fold();
    dz_display.canvas = DZ_CANVAS_SCREEN;
    for (int mode = DZ_MODE_VGA_400; mode <= DZ_MODE_EGA_350; ++mode) {
        dz_video_set_mode((enum dz_video_mode)mode, 1);
        for (int policy = 0; policy < 3; ++policy) {
            assert(dz_canvas_configure(renderer, 641, 479, (enum dz_presentation)policy) == 1);
            dz_canvas_take_restart();
            for (uint32_t i = 0; i < DZ_VRAM_PIXELS; ++i) dz_canvas_write(i, (uint8_t)(i * 29u + i / 317u));
            dz_canvas_start(mode == DZ_MODE_VGA_200 ? 203 : 817);
            dz_canvas_split(187);
            compare((enum dz_presentation)policy); /* including a mid-row aperture wrap */
            for (unsigned i = 0; i < 256; ++i) dz_video.dac[i][0] ^= 63;
            for (unsigned i = 0; i < 16; ++i) dz_video.ega_attr[i] ^= 63;
            compare((enum dz_presentation)policy);
            dz_canvas_start(0); dz_canvas_split((unsigned)dz_video.height);
            dz_canvas_rect(7, 12, 71, 29, 9);
            dz_canvas_move(13, 16, 7, 12, 71, 29);
            assert(dz_canvas_read(16u * dz_video.width + 13) == 9);
            dz_video.ega_xor = 1;
            dz_canvas_store(16u * dz_video.width + 13, 3, 1);
            assert(dz_canvas_read(16u * dz_video.width + 13) == 10);
            dz_canvas_buffer_store(dz_canvas_destination() + 3000, 42);
            assert(dz_canvas_read(3000) == 42);
            compare((enum dz_presentation)policy);
            uint64_t ns = dz_virtual_ns(), rng = dz_rand_calls();
            assert(dz_canvas_configure(renderer, 641, 479, (enum dz_presentation)((policy + 1) % 3)) == 1);
            assert(dz_canvas_take_restart() && !dz_canvas_take_restart());
            assert(dz_virtual_ns() == ns && dz_rand_calls() == rng);
            for (uint32_t i = 0; i < DZ_VRAM_PIXELS; ++i) assert(dz_canvas_read(i) == 0);
        }
        /* Text-mode clears must invalidate a previous mode's scaled mirror. */
        dz_video_set_mode(DZ_MODE_TEXT, 1);
        dz_video_set_mode((enum dz_video_mode)mode, 0);
        assert(dz_canvas_configure(renderer, 641, 479, DZ_PRESENT_ORIGINAL) == 1);
        assert(!dz_canvas_take_restart());
        for (uint32_t i = 0; i < DZ_VRAM_PIXELS; ++i) assert(dz_canvas_read(i) == 0);
    }
    /* Output resize is also a clear + restart, never old-content resampling. */
    dz_canvas_write(1, 7);
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 701, 501);
    assert(target && SDL_SetRenderTarget(renderer, target));
    assert(dz_presenter_draw(screen, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_canvas_read(1) == 0);
    dz_canvas_write(1, 9); /* remainder of a generator that was on the stack */
    struct dz_canvas_view waiting = dz_canvas_surface();
    for (int i = 0; i < waiting.width * waiting.height; ++i) assert(waiting.indices[i] == 0);
    assert(dz_presenter_draw(screen, DZ_PRESENT_ORIGINAL, 0) == 0);
    input_pending = exit_requested = 0;
    current_image_draw = rebuild_image; plot_callback = no_plot;
    dz_slice_step();
    assert(rebuilds == 1 && !dz_canvas_take_restart() && dz_canvas_read(1) == 55);
    dz_presenter_destroy(classic); dz_presenter_destroy(screen); dz_presenter_destroy(rgba);
    SDL_DestroyTexture(target); SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface); SDL_DestroyWindow(window);
    dz_shutdown();
    puts("Canvas stores/reads, XOR, move, palette, wrap, start/split, cache and restart passed");
    return 0;
}
