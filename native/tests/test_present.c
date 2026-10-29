/* SDL software ctest; --backend NAME opts in to hidden-window GPU verification. */
#include "platform/platform.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures, comparisons;

static void viewport(void)
{
    SDL_FRect r = dz_present_viewport(1920, 1080, DZ_PRESENT_ORIGINAL);
    assert(r.x == 240 && r.y == 0 && r.w == 1440 && r.h == 1080);
    r = dz_present_viewport(1920, 1080, DZ_PRESENT_CROP);
    assert(r.x == 0 && r.y == -180 && r.w == 1920 && r.h == 1440);
    r = dz_present_viewport(1280, 1024, DZ_PRESENT_ORIGINAL);
    assert(r.x == 0 && r.y == 32 && r.w == 1280 && r.h == 960);
    r = dz_present_viewport(1280, 1024, DZ_PRESENT_CROP);
    assert(fabsf(r.x + 42.666667f) < 0.0001f && fabsf(r.w - 1365.333333f) < 0.001f);
    r = dz_present_viewport(700, 300, DZ_PRESENT_STRETCH);
    assert(r.x == 0 && r.y == 0 && r.w == 700 && r.h == 300);
    r = dz_present_viewport(1, 1, DZ_PRESENT_ORIGINAL);
    assert(r.x == 0 && r.y == 0.125f && r.w == 1 && r.h == 0.75f);
    r = dz_present_viewport(0, 100, DZ_PRESENT_CROP);
    assert(r.w == 0 && r.h == 0);
    for (int w = 1; w < 80; ++w) for (int h = 1; h < 80; ++h)
        for (int policy = 0; policy < 3; ++policy) {
            r = dz_present_viewport(w, h, (enum dz_presentation)policy);
            assert(fabsf(2 * r.x + r.w - w) < 0.0001f && fabsf(2 * r.y + r.h - h) < 0.0001f);
            if (policy == DZ_PRESENT_STRETCH) { assert(r.w == w && r.h == h); continue; }
            assert(fabsf(r.w * 3 - r.h * 4) < 0.0001f);
            if (policy == DZ_PRESENT_ORIGINAL) assert(r.w <= w + 0.0001f && r.h <= h + 0.0001f);
            else assert(r.w >= w - 0.0001f && r.h >= h - 0.0001f);
        }
}

static void state(enum dz_video_mode mode)
{
    dz_video_set_mode(mode, 1);
    for (unsigned i = 0; i < sizeof dz_video.vram; ++i)
        dz_video.vram[i] = (uint8_t)((i * 29u + i / 317u + (i >> 13)) & 255u);
    for (unsigned i = 0; i < 256; ++i) {
        dz_video.dac[i][0] = (uint8_t)(i & 63u);
        dz_video.dac[i][1] = (uint8_t)((i / 4u) & 63u);
        dz_video.dac[i][2] = (uint8_t)((i * 11u) & 63u);
    }
    for (unsigned i = 0; i < 16; ++i) dz_video.ega_attr[i] = (uint8_t)((i * 13u) & 63u);
    dz_video.display_start_row = mode == DZ_MODE_VGA_400 ? 817u : mode == DZ_MODE_VGA_200 ? 203u : 818u;
    dz_video.line_compare = mode == DZ_MODE_VGA_400 ? 187u : (unsigned)dz_video.height;
    dz_video.cursor_visible = 1;
    dz_video.text[0][0][0] = 0xdb; dz_video.text[0][0][1] = 0x0f;
    dz_video.text[4][17][0] = 'X'; dz_video.text[4][17][1] = 0x9e;
}

static SDL_Surface *capture(SDL_Renderer *r, struct dz_presenter *p, SDL_Texture *target,
                            enum dz_presentation policy)
{
    assert(SDL_SetRenderTarget(r, target));
    int drawn = dz_presenter_draw(p, policy, 1);
    if (drawn < 0) { fprintf(stderr, "draw: %s\n", SDL_GetError()); return NULL; }
    assert(drawn == 1);
    assert(dz_presenter_draw(p, policy, 0) == 0);
    SDL_Surface *raw = SDL_RenderReadPixels(r, NULL);
    if (!raw) { fprintf(stderr, "read: %s\n", SDL_GetError()); return NULL; }
    SDL_Surface *result = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(raw);
    return result;
}

static void compare(SDL_Renderer *r, struct dz_presenter *indexed, struct dz_presenter *rgba,
                    int w, int h, enum dz_presentation policy, int scenario)
{
    SDL_Texture *a = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_Texture *b = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    assert(a && b);
    SDL_Surface *sa = capture(r, indexed, a, policy), *sb = capture(r, rgba, b, policy);
    assert(sa && sb && sa->w == w && sa->h == h && sb->w == w && sb->h == h);
    unsigned differences = 0;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        uint32_t ca = ((uint32_t *)((uint8_t *)sa->pixels + y * sa->pitch))[x];
        uint32_t cb = ((uint32_t *)((uint8_t *)sb->pixels + y * sb->pitch))[x];
        if (ca != cb) {
            if (!differences) fprintf(stderr, "DIFF state=%d %dx%d policy=%d at=%d,%d indexed=%08x rgba=%08x\n",
                                     scenario, w, h, policy, x, y, ca, cb);
            ++differences;
        }
        if (dz_video.mode == DZ_MODE_TEXT && (dz_display.screensaver || dz_display.preview_parent))
            assert(ca == UINT32_C(0xff000000) && cb == UINT32_C(0xff000000));
    }
    if (differences) { ++failures; fprintf(stderr, "  differing pixels=%u\n", differences); }
    ++comparisons;
    SDL_DestroySurface(sa); SDL_DestroySurface(sb);
    assert(SDL_SetRenderTarget(r, NULL));
    SDL_DestroyTexture(a); SDL_DestroyTexture(b);
}

static void dirty(SDL_Renderer *r, struct dz_presenter *p)
{
    SDL_Texture *target = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
    assert(target && SDL_SetRenderTarget(r, target));
    state(DZ_MODE_VGA_400);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 1) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    dz_video.vram[320 * 500] ^= 1; /* hidden row: no upload/present */
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    dz_video.vram[0] ^= 1; /* split displays row zero */
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0);
    dz_video.dac[0][0] ^= 1;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    ++dz_video.display_start_row;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    ++dz_video.line_compare;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 0) == 1);
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 0) == 0);
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 1) == 1); /* expose */
    assert(SDL_SetRenderTarget(r, NULL)); SDL_DestroyTexture(target);
    target = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 700, 300);
    assert(target && SDL_SetRenderTarget(r, target));
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 0) == 1); /* resize without forced invalidation */
    assert(dz_presenter_draw(p, DZ_PRESENT_CROP, 0) == 0);
    assert(SDL_SetRenderTarget(r, NULL)); SDL_DestroyTexture(target);
}

static void fallback(SDL_Renderer *r)
{
    struct dz_presenter *p = dz_presenter_create(r, DZ_RENDERER_AUTO);
    assert(p && dz_presenter_path(p) == DZ_RENDERER_INDEXED);
    dz_presenter_destroy(p);
    if (!strcmp(SDL_GetRendererName(r), "software")) return;
    /* Simulate a driver that doesn't advertise INDEX8. SDL's real internal format
     * list stays intact, so the RGBA reference can still create its texture. */
    SDL_PropertiesID props = SDL_GetRendererProperties(r);
    const SDL_PixelFormat none[] = {SDL_PIXELFORMAT_UNKNOWN};
    void *formats = SDL_GetPointerProperty(props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, NULL);
    assert(SDL_SetPointerProperty(props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, (void *)none));
    assert(!dz_presenter_create(r, DZ_RENDERER_INDEXED));
    p = dz_presenter_create(r, DZ_RENDERER_AUTO);
    assert(p && dz_presenter_path(p) == DZ_RENDERER_RGBA);
    dirty(r, p);
    dz_presenter_destroy(p);
    assert(SDL_SetPointerProperty(props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, formats));
}

static void text_cache(SDL_Renderer *r, struct dz_presenter *p)
{
    SDL_Texture *target = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 640, 480);
    assert(target && SDL_SetRenderTarget(r, target));
    dz_video_set_mode(DZ_MODE_TEXT, 1);
    dz_video.cursor_visible = 0;
    dz_options.headless = 1;
    dz_clock_reset();
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 1) == 1);
    dz_delay_ms(500);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 0); /* no visible blink */
    dz_video.text[0][0][0] = 'X'; dz_video.text[0][0][1] = 0x87;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    dz_delay_ms(500);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1); /* blinking attribute */
    dz_video.text[0][0][1] = 7; dz_video.cursor_visible = 1;
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1);
    dz_delay_ms(500);
    assert(dz_presenter_draw(p, DZ_PRESENT_ORIGINAL, 0) == 1); /* cursor */
    dz_options.headless = 0;
    assert(SDL_SetRenderTarget(r, NULL)); SDL_DestroyTexture(target);
}

int main(int argc, char **argv)
{
    viewport();
    if (argc == 2 && !strcmp(argv[1], "--static")) {
        assert(dz_initialize());
        dz_video_set_mode(DZ_MODE_VGA_400, 1);
        for (unsigned i = 0; i < 320u * 400u; ++i) dz_video.vram[i] = (uint8_t)i;
        dz_delay_ms(20000);
        dz_shutdown();
        return 0;
    }
    const char *backend = argc == 3 && !strcmp(argv[1], "--backend") ? argv[2] : "software";
    dz_options.deterministic = 1;
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = NULL;
    SDL_Surface *surface = NULL;
    SDL_Renderer *r;
    if (!strcmp(backend, "software")) {
        surface = SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_ARGB8888);
        r = SDL_CreateSoftwareRenderer(surface);
    } else {
        window = SDL_CreateWindow("DAZZLE render test", 64, 64, SDL_WINDOW_HIDDEN);
        r = !window ? NULL : !strcmp(backend, "gpu") ? SDL_CreateGPURenderer(NULL, window) :
            SDL_CreateRenderer(window, !strcmp(backend, "default") ? NULL : backend);
    }
    if (!r) { fprintf(stderr, "UNAVAILABLE %s: %s\n", backend, SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 77; }
    printf("backend=%s actual=%s", backend, SDL_GetRendererName(r));
    SDL_GPUDevice *device = SDL_GetGPURendererDevice(r);
    if (device) printf(" device=%s", SDL_GetGPUDeviceDriver(device));
    puts(""); fflush(stdout);
    struct dz_presenter *a = dz_presenter_create(r, DZ_RENDERER_INDEXED);
    struct dz_presenter *b = dz_presenter_create(r, DZ_RENDERER_RGBA);
    if (!a || !b) {
        fprintf(stderr, "UNAVAILABLE indexed: %s\n", SDL_GetError());
        struct dz_presenter *fallback = dz_presenter_create(r, DZ_RENDERER_AUTO);
        assert(fallback);
        if (!a) assert(dz_presenter_path(fallback) == DZ_RENDERER_RGBA);
        dirty(r, fallback);
        puts("AUTO fallback drew successfully");
        dz_presenter_destroy(fallback);
        dz_presenter_destroy(a); dz_presenter_destroy(b);
        SDL_DestroyRenderer(r); SDL_DestroySurface(surface); SDL_DestroyWindow(window); SDL_Quit(); return 77;
    }
    assert(dz_presenter_path(a) == DZ_RENDERER_INDEXED && dz_presenter_path(b) == DZ_RENDERER_RGBA);
    fallback(r);
    static const int sizes[][2] = {{640,480},{1920,1080},{2560,1080},{1280,1024},{700,300},{1,1},{1,2},{2,1},{641,479}};
    dirty(r, a); dirty(r, b);
    text_cache(r, a); text_cache(r, b);
    for (int scenario = 0; scenario < 8; ++scenario) {
        if (strcmp(backend, "software")) { printf("state=%d\n", scenario); fflush(stdout); }
        if (scenario != 1) state(scenario < 2 ? DZ_MODE_VGA_400 : scenario == 2 ? DZ_MODE_VGA_200 :
                                 scenario < 5 ? DZ_MODE_EGA_350 : DZ_MODE_TEXT);
        else for (unsigned i = 0; i < 256; ++i) dz_video.dac[i][1] ^= 63; /* palette only */
        if (scenario == 4) for (unsigned i = 0; i < 16; ++i) dz_video.ega_attr[i] ^= 63;
        dz_display.screensaver = scenario == 6;
        dz_display.preview_parent = scenario == 7 ? (void *)(uintptr_t)1 : NULL;
        for (unsigned i = 0; i < sizeof sizes / sizeof *sizes; ++i)
            for (int policy = 0; policy < 3; ++policy)
                compare(r, a, b, sizes[i][0], sizes[i][1], (enum dz_presentation)policy, scenario);
    }
    dz_presenter_destroy(a); dz_presenter_destroy(b);
    SDL_DestroyRenderer(r); SDL_DestroySurface(surface); SDL_DestroyWindow(window); SDL_Quit();
    printf("comparisons=%d failures=%d\n", comparisons, failures);
    return failures ? 1 : 0;
}
