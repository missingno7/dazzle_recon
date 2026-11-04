/* Original CLI/program/clock; present only at observation deadlines. */
#define main native_cli_main
#include "../src/main.c"
#undef main
#include "platform/canvas.h"
#include <assert.h>

static SDL_Surface *surface;
static SDL_Renderer *renderer;
static SDL_Texture *target;
static struct dz_presenter *presenter;
static unsigned ordinal;
extern int16_t gumballs_draw_flags, yarnball_full_ring;

void dz_present(void)
{
    if (dz_display.canvas != DZ_CANVAS_SCREEN || dz_video.mode == DZ_MODE_TEXT) return;
    if (!renderer) {
        assert(SDL_InitSubSystem(SDL_INIT_VIDEO));
        surface = SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_ARGB8888);
        renderer = SDL_CreateSoftwareRenderer(surface);
        assert(renderer);
        target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
            dz_display.window_width, dz_display.window_height);
        assert(target && SDL_SetRenderTarget(renderer, target));
        presenter = dz_presenter_create(renderer, DZ_RENDERER_INDEXED);
        assert(presenter);
    }
    assert(dz_canvas_configure(renderer, dz_display.window_width, dz_display.window_height, dz_display.presentation) == 1);
}
static void observe(void)
{
    printf("NATIVE image=%u ink=%d parameter=%d\n",current_image_index,dz_canvas_has_native(),effect_parameter);
    printf("VARIANT gumballs=%d yarnball=%d bigstar=%d\n",gumballs_draw_flags,yarnball_full_ring,bigstar_color_mode);
    const char *prefix = getenv("DAZZLE_NATIVE_STATE");
    if (prefix) {
        char path[4096];
        assert(snprintf(path, sizeof path, "%s-%u.globals", prefix, ordinal++) < (int)sizeof path);
        FILE *file = fopen(path, "wb"); assert(file);
#define FIELD(name) assert(fwrite((const void *)&name, 1, sizeof name, file) == sizeof name)
#include "state_fields.inc"
#undef FIELD
        /* Pointer-bearing records: serialize their values and symbolic identity. */
        for (int i = 0; i < 42; ++i) {
            ImageConfigRow *row = i < 36 ? alt_image_records+i : ctrl_image_records+i-36;
            uint16_t fields[] = {row->size, row->enabled, row->continuous, row->key, current_image_draw == row->draw};
            assert(fwrite(fields, 1, sizeof fields, file) == sizeof fields);
        }
        for (int i = 0; i < 24; ++i) assert(fwrite(&fade_records[i].enabled, 1, 2, file) == 2);
        if (palette_buffer) assert(fwrite(palette_buffer, 1, palette_buffer_bytes, file) == palette_buffer_bytes);
        assert(!fclose(file));
    }
    const char *capture = getenv("DAZZLE_NATIVE_CAPTURE");
    if (capture && renderer) {
        assert(dz_presenter_draw(presenter, dz_display.presentation, 1) == 1);
        SDL_Surface *pixels = SDL_RenderReadPixels(renderer, NULL);
        assert(pixels && SDL_SavePNG(pixels, capture)); SDL_DestroySurface(pixels);
    }
}
int main(int argc, char **argv)
{
    dz_testctl_state_observer = observe;
    int result = native_cli_main(argc, argv);
    dz_testctl_state_observer = NULL;
    dz_presenter_destroy(presenter); SDL_DestroyTexture(target);
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);
    return result;
}
