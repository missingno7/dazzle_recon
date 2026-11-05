/* Classic scanout, texture upload and physical-pixel presentation. Simulation stays in video.c. */
#include "platform.h"
#include "canvas.h"
#include <stdlib.h>
#include <string.h>

extern void dzconio_cursor_rows(int *start, int *end);

struct dz_presenter {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Palette *palette;
    enum dz_renderer requested, path;
    SDL_PixelFormat format;
    int texture_width, texture_height;
    int valid, output_width, output_height, black;
    enum dz_presentation policy;
    enum dz_video_mode mode;
    unsigned start, split;
    int width, height, pixels_per_byte;
    uint8_t indices[640 * 400];
    SDL_Color colors[256];
    uint32_t rgba[720 * 400];
    uint8_t text[25][80][2];
    int cursor_x, cursor_y, cursor_visible, cursor_start, cursor_end, blink;
    uint64_t canvas_revision;
    int screen;
    int native_ink;
    struct dz_upload_stats stats;
};

static SDL_Window *window;
static SDL_Renderer *renderer;
static struct dz_presenter *presenter;
static uint64_t frame_count;
static int invalidated = 1, failed;

SDL_FRect dz_present_viewport(int width, int height, enum dz_presentation policy)
{
    if (policy == DZ_PRESENT_EXTENDED) policy = DZ_PRESENT_ORIGINAL;
    if (width <= 0 || height <= 0) return (SDL_FRect){0, 0, 0, 0};
    double w = width, h = height;
    if (policy != DZ_PRESENT_STRETCH) {
        int wide = w * 3.0 > h * 4.0;
        if (wide == (policy == DZ_PRESENT_ORIGINAL)) w = h * 4.0 / 3.0;
        else h = w * 3.0 / 4.0;
    }
    return (SDL_FRect){(float)((width - w) / 2.0), (float)((height - h) / 2.0), (float)w, (float)h};
}

static int native_indexed(SDL_Renderer *target)
{
    /* SDL can silently emulate INDEX8 by expanding it on the CPU. Require native
     * support on hardware; the software test renderer intentionally exercises emulation. */
    if (!strcmp(SDL_GetRendererName(target), "software")) return 1;
    const SDL_PixelFormat *formats = SDL_GetPointerProperty(SDL_GetRendererProperties(target),
        SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, NULL);
    if (formats) for (; *formats != SDL_PIXELFORMAT_UNKNOWN; ++formats)
        if (*formats == SDL_PIXELFORMAT_INDEX8) return 1;
    return SDL_SetError("Renderer has no native INDEX8 texture support");
}

static int make_texture(struct dz_presenter *p, SDL_PixelFormat format, int w, int h)
{
    SDL_DestroyTexture(p->texture);
    SDL_TextureAccess access = dz_display.canvas == DZ_CANVAS_SCREEN && dz_video.mode != DZ_MODE_TEXT ?
        SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_STREAMING;
    p->texture = SDL_CreateTexture(p->renderer, format, access, w, h);
    p->valid = 0;
    if (!p->texture || !SDL_SetTextureScaleMode(p->texture, SDL_SCALEMODE_NEAREST) ||
        !SDL_SetTextureBlendMode(p->texture, SDL_BLENDMODE_NONE) ||
        (format == SDL_PIXELFORMAT_INDEX8 && !SDL_SetTexturePalette(p->texture, p->palette))) return 0;
    ++p->stats.textures;
    p->format = format; p->texture_width = w; p->texture_height = h;
    return 1;
}

struct dz_presenter *dz_presenter_create(SDL_Renderer *target, enum dz_renderer path)
{
    struct dz_presenter *p = calloc(1, sizeof *p);
    if (!p) { SDL_OutOfMemory(); return NULL; }
    p->renderer = target; p->requested = path; p->path = path;
    if (path != DZ_RENDERER_RGBA) {
        p->palette = SDL_CreatePalette(256);
        if (p->palette && native_indexed(target) && make_texture(p, SDL_PIXELFORMAT_INDEX8, 1, 1))
            p->path = DZ_RENDERER_INDEXED;
        else if (path == DZ_RENDERER_AUTO) {
            SDL_DestroyTexture(p->texture); p->texture = NULL;
            p->path = DZ_RENDERER_RGBA;
        } else { dz_presenter_destroy(p); return NULL; }
    }
    return p;
}

void dz_presenter_destroy(struct dz_presenter *p)
{
    if (!p) return;
    SDL_DestroyTexture(p->texture);
    SDL_DestroyPalette(p->palette);
    free(p);
}

enum dz_renderer dz_presenter_path(const struct dz_presenter *p) { return p->path; }
struct dz_upload_stats dz_presenter_upload_stats(const struct dz_presenter *p)
{
    if (!p) p = presenter;
    return p ? p->stats : (struct dz_upload_stats){0};
}

static int upload(struct dz_presenter *p, const SDL_Rect *rect, const void *pixels, int pitch)
{
    if (!SDL_UpdateTexture(p->texture,rect,pixels,pitch)) return 0;
    ++p->stats.uploads;
    size_t area = rect ? (size_t)rect->w*rect->h : (size_t)p->texture_width*p->texture_height;
    p->stats.bytes += area*(p->format == SDL_PIXELFORMAT_INDEX8 ? 1u : 4u);
    return 1;
}

static void scanout_palette(SDL_Color colors[256])
{
    for (unsigned i = 0; i < 256; ++i) {
        unsigned r, g, b;
        if (dz_video.mode == DZ_MODE_EGA_350) {
            uint8_t c = dz_video.ega_attr[i & 15u];
            r = ((c >> 2) & 1u) * 170u + ((c >> 5) & 1u) * 85u;
            g = ((c >> 1) & 1u) * 170u + ((c >> 4) & 1u) * 85u;
            b = (c & 1u) * 170u + ((c >> 3) & 1u) * 85u;
        } else {
            r = (dz_video.dac[i][0] & 63u) * 255u / 63u;
            g = (dz_video.dac[i][1] & 63u) * 255u / 63u;
            b = (dz_video.dac[i][2] & 63u) * 255u / 63u;
        }
        colors[i] = (SDL_Color){(uint8_t)r, (uint8_t)g, (uint8_t)b, 255};
    }
}

static int scanout_indices(struct dz_presenter *p)
{
    uint32_t capacity = DZ_VRAM_PLANE_BYTES * (uint32_t)dz_video.pixels_per_byte;
    unsigned width = (unsigned)dz_video.width;
    int changed = 0;
    for (unsigned y = 0; y < (unsigned)dz_video.height; ++y) {
        unsigned row = y > dz_video.line_compare ? y - dz_video.line_compare - 1u : dz_video.display_start_row + y;
        uint32_t address = (row * width) % capacity;
        unsigned first = width < capacity - address ? width : capacity - address;
        uint8_t *dest = p->indices + y * width;
        if (memcmp(dest, dz_canvas_memory() + address, first)) {
            memcpy(dest, dz_canvas_memory() + address, first); changed = 1;
        }
        if (first < width && memcmp(dest + first, dz_canvas_memory(), width - first)) {
            memcpy(dest + first, dz_canvas_memory(), width - first); changed = 1;
        }
    }
    return changed;
}

static int text_changed(struct dz_presenter *p)
{
    int start, end, blink = (int)((dz_virtual_ns() / UINT64_C(500000000)) & 1u);
    dzconio_cursor_rows(&start, &end);
    int blinking = dz_video.cursor_visible && dz_video.cursor_x >= 0 && dz_video.cursor_x < 80 &&
        dz_video.cursor_y >= 0 && dz_video.cursor_y < 25 && start <= end && start < 16 && end >= 0;
    for (int y = 0; y < 25 && !blinking; ++y)
        for (int x = 0; x < 80; ++x) if (dz_video.text[y][x][1] & 128u) { blinking = 1; break; }
    int changed = memcmp(p->text, dz_video.text, sizeof p->text) ||
        p->cursor_x != dz_video.cursor_x || p->cursor_y != dz_video.cursor_y ||
        p->cursor_visible != dz_video.cursor_visible || p->cursor_start != start ||
        p->cursor_end != end || (blinking && p->blink != blink);
    memcpy(p->text, dz_video.text, sizeof p->text);
    p->cursor_x = dz_video.cursor_x; p->cursor_y = dz_video.cursor_y;
    p->cursor_visible = dz_video.cursor_visible; p->cursor_start = start; p->cursor_end = end; p->blink = blink;
    return changed;
}

static int draw_screen(struct dz_presenter *p, enum dz_presentation policy, int force, int w, int h)
{
    if (dz_canvas_configure(p->renderer, w, h, policy) < 0) return -1;
    struct dz_canvas_view v = dz_canvas_surface();
    SDL_Color colors[256];
    scanout_palette(colors);
    int palette_changed = memcmp(p->colors, colors, sizeof colors) != 0;
    int pixels_changed = !p->valid || !p->screen || p->canvas_revision != v.revision;
    int registers_changed = p->start != dz_video.display_start_row || p->split != dz_video.line_compare ||
        p->native_ink != dz_canvas_has_native();
    int coloured_zero = v.side_written && (colors[0].r || colors[0].g || colors[0].b);
    SDL_PixelFormat format = p->path == DZ_RENDERER_INDEXED && !v.coverage && !coloured_zero ?
        SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_ARGB8888;
    int new_texture = !p->texture || p->format != format || p->texture_width != v.width || p->texture_height != v.height;
    if (new_texture && !make_texture(p, format, v.width, v.height)) {
        if (format != SDL_PIXELFORMAT_INDEX8 || p->requested != DZ_RENDERER_AUTO) return -1;
        p->path = DZ_RENDERER_RGBA;
        format = SDL_PIXELFORMAT_ARGB8888;
        if (!make_texture(p, format, v.width, v.height)) return -1;
    }
    SDL_Rect full = {0,0,v.width,v.height};
    const SDL_Rect *rects = v.dirty;
    int rect_count = v.dirty_count;
    if (new_texture || !p->valid || !p->screen || p->canvas_revision != v.dirty_base || !rect_count ||
        (format != SDL_PIXELFORMAT_INDEX8 && palette_changed)) {
        rects = &full; rect_count = 1;
    }
    if (format == SDL_PIXELFORMAT_INDEX8) {
        if ((palette_changed || !p->valid) && !SDL_SetPaletteColors(p->palette, colors, 0, 256)) return -1;
        if (pixels_changed || new_texture) for (int i = 0; i < rect_count; ++i) {
            const SDL_Rect *r = rects+i;
            if (!upload(p,r,v.indices+(size_t)r->y*v.width+r->x,v.width)) return -1;
        }
    } else if (pixels_changed || palette_changed || new_texture) for (int n = 0; n < rect_count; ++n) {
        SDL_Rect rect = rects[n];
        uint32_t *rgba = malloc((size_t)rect.w * rect.h * sizeof *rgba);
        if (!rgba) { SDL_OutOfMemory(); return -1; }
        SDL_FRect clip = dz_canvas_content();
        for (int y = 0; y < rect.h; ++y) for (int x = 0; x < rect.w; ++x) {
            size_t i = (size_t)(y+rect.y)*v.width+x+rect.x;
            SDL_Color c = colors[v.indices[i]];
            if (v.coverage) {
                unsigned a = v.coverage[i], b = 255-a;
                SDL_Color u = colors[v.under[i]];
                c.r = (uint8_t)((c.r*a+u.r*b+127)/255);
                c.g = (uint8_t)((c.g*a+u.g*b+127)/255);
                c.b = (uint8_t)((c.b*a+u.b*b+127)/255);
            }
            int column = x+rect.x;
            if (v.side_written && !v.side_written[i] && (column < clip.x || column >= clip.x+clip.w))
                c = (SDL_Color){0,0,0,255};
            rgba[(size_t)y*rect.w+x] = UINT32_C(0xff000000) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
        }
        int ok = upload(p, &rect, rgba, rect.w * (int)sizeof *rgba);
        free(rgba);
        if (!ok) return -1;
    }
    dz_canvas_uploaded(v.revision);
    if (!force && !pixels_changed && !palette_changed && !new_texture && !registers_changed && p->output_width == w &&
        p->output_height == h && p->policy == policy) return 0;
    if (!SDL_SetRenderDrawColor(p->renderer, 0, 0, 0, 255) || !SDL_RenderClear(p->renderer)) return -1;
    struct dz_canvas_quad quads[3];
    int count = dz_canvas_quads(quads);
    for (int i = 0; i < count; ++i)
        if (!SDL_RenderTexture(p->renderer, p->texture, &quads[i].source, &quads[i].dest)) return -1;
    memcpy(p->colors, colors, sizeof colors);
    p->canvas_revision = v.revision; p->screen = p->valid = 1;
    p->start = dz_video.display_start_row; p->split = dz_video.line_compare;
    p->native_ink = dz_canvas_has_native();
    p->output_width = w; p->output_height = h; p->policy = policy;
    return 1;
}

int dz_presenter_draw(struct dz_presenter *p, enum dz_presentation policy, int force)
{
    int ow, oh;
    if (!SDL_GetCurrentRenderOutputSize(p->renderer, &ow, &oh)) return -1;
    if (ow <= 0 || oh <= 0) return 0;
    int text = dz_video.mode == DZ_MODE_TEXT;
    if (dz_display.canvas == DZ_CANVAS_SCREEN && !text)
        return draw_screen(p, policy, force, ow, oh);
    if (p->screen) { p->screen = 0; p->valid = 0; }
    if (policy == DZ_PRESENT_EXTENDED) policy = DZ_PRESENT_ORIGINAL;
    int black = text && (dz_display.screensaver || dz_display.preview_parent);
    int w = text ? 720 : dz_video.width, h = text ? 400 : dz_video.height;
    int mode_changed = !p->valid || p->mode != dz_video.mode || p->width != w || p->height != h ||
        p->pixels_per_byte != dz_video.pixels_per_byte || p->black != black;
    int changed = force || mode_changed || p->output_width != ow || p->output_height != oh ||
        p->policy != policy || p->start != dz_video.display_start_row || p->split != dz_video.line_compare;
    if (!black) {
        SDL_PixelFormat format = !text && p->path == DZ_RENDERER_INDEXED ?
            SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_ARGB8888;
        int new_texture = !p->texture || p->format != format || p->texture_width != w || p->texture_height != h;
        if (new_texture && !make_texture(p, format, w, h)) {
            if (format != SDL_PIXELFORMAT_INDEX8 || p->requested != DZ_RENDERER_AUTO) goto error;
            p->path = DZ_RENDERER_RGBA;
            format = SDL_PIXELFORMAT_ARGB8888;
            if (!make_texture(p, format, w, h)) goto error;
        }
        int pixels_changed = text ? text_changed(p) : scanout_indices(p);
        SDL_Color colors[256];
        int palette_changed = 0;
        if (!text) {
            scanout_palette(colors);
            palette_changed = memcmp(p->colors, colors, sizeof colors) != 0;
        }
        changed |= pixels_changed || palette_changed || new_texture || !p->valid;
        if (format == SDL_PIXELFORMAT_INDEX8) {
            if ((palette_changed || !p->valid) && !SDL_SetPaletteColors(p->palette, colors, 0, 256)) goto error;
            if ((pixels_changed || new_texture || !p->valid) &&
                !SDL_UpdateTexture(p->texture, NULL, p->indices, w)) goto error;
        } else if (pixels_changed || palette_changed || new_texture || mode_changed || !p->valid) {
            dz_video_rgba(p->rgba);
            if (!SDL_UpdateTexture(p->texture, NULL, p->rgba, w * (int)sizeof(uint32_t))) goto error;
        }
        if (!text) memcpy(p->colors, colors, sizeof colors);
    }
    if (!changed) return 0;
    if (!SDL_SetRenderDrawColor(p->renderer, 0, 0, 0, 255) || !SDL_RenderClear(p->renderer)) goto error;
    if (!black) {
        SDL_FRect dest = dz_present_viewport(ow, oh, policy);
        if (!SDL_RenderTexture(p->renderer, p->texture, NULL, &dest)) goto error;
    }
    p->mode = dz_video.mode; p->width = w; p->height = h; p->pixels_per_byte = dz_video.pixels_per_byte;
    p->start = dz_video.display_start_row; p->split = dz_video.line_compare;
    p->output_width = ow; p->output_height = oh; p->policy = policy; p->black = black; p->valid = 1;
    return 1;
error:
    p->valid = 0;
    return -1;
}

static int draw_frame(int force)
{
    if (failed) return 0;
    if (!window) {
        window = dz_create_window();
        if (!window) goto error;
        /* GPU transfers suit the persistent Screen aperture. Honor explicit
         * backend hints, and retain the normal renderer as compatibility fallback. */
        int gpu = dz_display.canvas == DZ_CANVAS_SCREEN && !SDL_GetHint(SDL_HINT_RENDER_DRIVER);
        renderer = gpu ? SDL_CreateGPURenderer(NULL,window) : SDL_CreateRenderer(window,NULL);
        if (!renderer && gpu) renderer = SDL_CreateRenderer(window,NULL);
        if (!renderer) goto error;
        presenter = dz_presenter_create(renderer, dz_display.renderer);
        if (!presenter) goto error;
    }
    int result = dz_presenter_draw(presenter, dz_display.presentation, force || invalidated);
    if (result < 0) goto error;
    invalidated = 0;
    return result;
error:
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "DAZZLE presenter: %s", SDL_GetError());
    failed = 1;
    return 0;
}

void dz_present_invalidate(void) { invalidated = 1; }
void dz_present(void)
{
    ++frame_count; /* preserve virtual frame receipts, including headless and unchanged frames */
    if (!dz_options.headless && draw_frame(0) && !SDL_RenderPresent(renderer)) dz_present_invalidate();
}
uint64_t dz_presented_frames(void) { return frame_count; }
void dz_toggle_fullscreen(void)
{
    if (window) SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
    dz_present_invalidate();
}
int dz_save_screenshot(const char *filename)
{
    if (dz_options.headless || !draw_frame(1)) return 0;
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    if (!surface) return 0;
    const char *extension = strrchr(filename, '.');
    int ok = extension && !SDL_strcasecmp(extension, ".png") ?
        SDL_SavePNG(surface, filename) : SDL_SaveBMP(surface, filename);
    SDL_DestroySurface(surface);
    SDL_RenderPresent(renderer);
    return ok;
}
void dz_video_shutdown(void)
{
    dz_canvas_shutdown();
    dz_presenter_destroy(presenter); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    presenter = NULL; renderer = NULL; window = NULL;
    frame_count = 0; invalidated = 1; failed = 0;
}
