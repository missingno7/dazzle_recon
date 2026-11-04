/* Production CLI/presentation, with CPU deltas at exact virtual deadlines.
 * Separate executable: no timing instrumentation in the shipped presenter. */
#define main native_cli_main
#include "../src/main.c"
#undef main
#include "platform/canvas.h"
#include <windows.h>
#include <assert.h>

static SDL_Surface *raster_surface;
static SDL_Renderer *raster_renderer;

static double cpu(void)
{
    FILETIME created, exited, kernel, user;
    ULARGE_INTEGER k, u;
    assert(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user));
    k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
    return (double)(k.QuadPart + u.QuadPart) / 1e7;
}
static void observe(void)
{
    if (getenv("DAZZLE_BENCH_RASTER") && !raster_renderer) {
        assert(SDL_InitSubSystem(SDL_INIT_VIDEO));
        raster_surface = SDL_CreateSurface(16,16,SDL_PIXELFORMAT_ARGB8888);
        raster_renderer = SDL_CreateSoftwareRenderer(raster_surface); assert(raster_renderer);
        assert(dz_canvas_configure(raster_renderer,dz_display.window_width,dz_display.window_height,
            dz_display.presentation) == 1);
    }
    struct dz_upload_stats s = dz_presenter_upload_stats(NULL);
    if (!dz_options.headless) {
        int count, w, h;
        SDL_Window **windows = SDL_GetWindows(&count);
        assert(windows && count == 1);
        SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
        assert(renderer && SDL_GetCurrentRenderOutputSize(renderer,&w,&h));
        assert(w == dz_display.window_width && h == dz_display.window_height);
        printf("OUTPUT %dx%d backend=%s\n",w,h,SDL_GetRendererName(renderer));
        SDL_free(windows);
    }
    printf("BENCH ns=%" PRIu64 " cpu=%.7f wall=%.7f image=%u uploads=%" PRIu64 " bytes=%" PRIu64 " textures=%" PRIu64 "\n",
        dz_virtual_ns(), cpu(), (double)SDL_GetTicksNS()/1e9, current_image_index,s.uploads,s.bytes,s.textures);
    printf("INK native=%d parameter=%d\n",dz_canvas_has_native(),effect_parameter);
    fflush(stdout);
}
int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "--static")) {
        window_size(argv[2]);
        dz_display.canvas = !strcmp(argv[3], "screen") ? DZ_CANVAS_SCREEN : DZ_CANVAS_CLASSIC;
        dz_options.headless = strcmp(argv[4], "window") != 0;
        dz_options.deterministic = !strcmp(argv[4], "deterministic");
        assert(dz_initialize()); dz_video_set_mode(DZ_MODE_VGA_400, 1);
        for (uint32_t i = 0; i < 320u*400u; ++i) dz_canvas_write(i, (uint8_t)i);
        dz_delay_ms(1000); observe(); dz_delay_ms(20000); observe();
        dz_shutdown(); return 0;
    }
    dz_testctl_state_observer = observe;
    int result = native_cli_main(argc, argv);
    SDL_DestroyRenderer(raster_renderer); SDL_DestroySurface(raster_surface);
    return result;
}
