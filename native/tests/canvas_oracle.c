/* Whole original program, production canvas and presenter, exact-time observer.
 * Upload only at observation times so the 4K/software matrix stays practical.
 * Every original plot, read, work charge and clock/input service still runs. */
#define main native_cli_main
#include "../src/main.c"
#undef main
#include "platform/canvas.h"
#include <assert.h>
#include "canvas_capture.h"

static SDL_Surface *surface;
static SDL_Window *oracle_window;
static SDL_Renderer *oracle_renderer;
static SDL_Texture *target;
static struct dz_presenter *classic, *screen;
static unsigned comparisons;

static void initialize_renderer(void)
{
    if (oracle_renderer) return;
    assert(SDL_InitSubSystem(SDL_INIT_VIDEO));
    int w = dz_display.window_width ? dz_display.window_width : 960;
    int h = dz_display.window_height ? dz_display.window_height : 720;
    const char *backend = getenv("DAZZLE_CANVAS_ORACLE_BACKEND");
    if (backend && strcmp(backend, "software")) {
        oracle_window = SDL_CreateWindow("canvas oracle", 64, 64, SDL_WINDOW_HIDDEN);
        assert(oracle_window);
        oracle_renderer = !strcmp(backend,"gpu") ? SDL_CreateGPURenderer(NULL,oracle_window) :
            SDL_CreateRenderer(oracle_window, !strcmp(backend, "default") ? NULL : backend);
    } else {
        surface = SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_ARGB8888);
        oracle_renderer = SDL_CreateSoftwareRenderer(surface);
    }
    assert(oracle_renderer);
    target = SDL_CreateTexture(oracle_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    assert(target && SDL_SetRenderTarget(oracle_renderer, target));
    classic = dz_presenter_create(oracle_renderer, DZ_RENDERER_INDEXED);
    screen = dz_presenter_create(oracle_renderer, DZ_RENDERER_INDEXED);
    assert(classic && screen);
}

void dz_present(void)
{
    if (dz_display.canvas == DZ_CANVAS_SCREEN && dz_video.mode != DZ_MODE_TEXT) {
        initialize_renderer();
        assert(dz_canvas_configure(oracle_renderer, dz_display.window_width ? dz_display.window_width : 960,
            dz_display.window_height ? dz_display.window_height : 720, dz_display.presentation) == 1);
    }
}

static void observe(void)
{
    if (dz_display.canvas != DZ_CANVAS_SCREEN || dz_video.mode == DZ_MODE_TEXT) return;
    initialize_renderer();
    dz_display.canvas = DZ_CANVAS_CLASSIC;
    assert(dz_presenter_draw(classic, dz_display.presentation, 1) == 1);
    SDL_Surface *a = SDL_RenderReadPixels(oracle_renderer, NULL);
    dz_display.canvas = DZ_CANVAS_SCREEN;
    assert(dz_presenter_draw(screen, dz_display.presentation, 1) == 1);
    SDL_Surface *b = SDL_RenderReadPixels(oracle_renderer, NULL);
    assert(a && b && a->format == b->format && a->w == b->w && a->h == b->h);
    canvas_assert_pixels(b);
    /* Uniform raster shifts change nearest block phase during pan/split.
     * Unpanned Faithful retains the exact Classic nearest oracle. */
    if (!dz_video.display_start_row && dz_video.line_compare >= (unsigned)dz_video.height) {
        for (int y = 0; y < a->h; ++y)
            if (memcmp((uint8_t *)a->pixels + (size_t)y * a->pitch,
                       (uint8_t *)b->pixels + (size_t)y * b->pitch, (size_t)a->w * 4)) {
                fprintf(stderr, "Canvas differs at ns=%" PRIu64 " row=%d start=%u split=%u image=%u\n",
                    dz_virtual_ns(), y, dz_video.display_start_row, dz_video.line_compare, current_image_index);
                abort();
            }
    }
    SDL_DestroySurface(a); SDL_DestroySurface(b); ++comparisons;
    printf("canvas oracle ns=%" PRIu64 " image=%u bytes=%zu start=%u split=%u checked\n",
        dz_virtual_ns(), current_image_index, dz_canvas_bytes(), dz_video.display_start_row, dz_video.line_compare);
}

int main(int argc, char **argv)
{
    dz_testctl_state_observer = observe;
    int result = native_cli_main(argc, argv);
    dz_testctl_state_observer = NULL;
    dz_presenter_destroy(classic); dz_presenter_destroy(screen);
    SDL_DestroyTexture(target); SDL_DestroyRenderer(oracle_renderer);
    SDL_DestroySurface(surface); SDL_DestroyWindow(oracle_window);
    printf("canvas comparisons=%u\n", comparisons);
    return result;
}
