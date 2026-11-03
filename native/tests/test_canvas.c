#include "platform/canvas.h"
#include "dazzle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static void compare(enum dz_presentation policy)
{
    dz_display.canvas = DZ_CANVAS_CLASSIC;
    assert(dz_presenter_draw(classic, policy, 1) == 1);
    SDL_Surface *a = SDL_RenderReadPixels(renderer, NULL);
    dz_display.canvas = DZ_CANVAS_SCREEN;
    assert(dz_presenter_draw(screen, policy, 1) == 1);
    SDL_Surface *b = SDL_RenderReadPixels(renderer, NULL);
    assert(a && b && a->format == b->format);
    for (int y = 0; y < a->h; ++y)
        assert(!memcmp((uint8_t *)a->pixels + y * a->pitch, (uint8_t *)b->pixels + y * b->pitch, (size_t)a->w * 4));
    assert(dz_presenter_draw(screen, policy, 0) == 0);
    assert(dz_presenter_draw(rgba, policy, 1) == 1);
    SDL_Surface *c = SDL_RenderReadPixels(renderer, NULL);
    assert(c && c->format == a->format);
    for (int y = 0; y < a->h; ++y)
        assert(!memcmp((uint8_t *)a->pixels + y * a->pitch, (uint8_t *)c->pixels + y * c->pitch, (size_t)a->w * 4));
    assert(dz_presenter_draw(rgba, policy, 0) == 0);
    SDL_DestroySurface(c);
    SDL_DestroySurface(a); SDL_DestroySurface(b);
}

int main(void)
{
    dz_options.headless = dz_options.deterministic = 1;
    assert(dz_initialize() && SDL_InitSubSystem(SDL_INIT_VIDEO));
    SDL_Surface *surface = SDL_CreateSurface(641, 479, SDL_PIXELFORMAT_ARGB8888);
    renderer = SDL_CreateSoftwareRenderer(surface);
    classic = dz_presenter_create(renderer, DZ_RENDERER_INDEXED);
    screen = dz_presenter_create(renderer, DZ_RENDERER_AUTO);
    rgba = dz_presenter_create(renderer, DZ_RENDERER_RGBA);
    assert(classic && screen && rgba);
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
    struct dz_canvas_view waiting = dz_canvas_visible();
    for (int i = 0; i < waiting.width * waiting.height; ++i) assert(waiting.indices[i] == 0);
    assert(dz_presenter_draw(screen, DZ_PRESENT_ORIGINAL, 0) == 0);
    input_pending = exit_requested = 0;
    current_image_draw = rebuild_image; plot_callback = no_plot;
    dz_slice_step();
    assert(rebuilds == 1 && !dz_canvas_take_restart() && dz_canvas_read(1) == 55);
    dz_presenter_destroy(classic); dz_presenter_destroy(screen); dz_presenter_destroy(rgba);
    SDL_DestroyTexture(target); SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);
    dz_shutdown();
    puts("Canvas stores/reads, XOR, move, palette, wrap, start/split, cache and restart passed");
    return 0;
}
