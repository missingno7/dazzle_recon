#include "canvas.h"
#include <stdlib.h>
#include <string.h>

extern uint16_t page_paragraph;

/* Each aperture row has max nearest-block height, so changing display start
 * never changes the sampling phase of stored ink. Unsampled columns have a
 * one-byte guard for readback (small outputs and cropped content). */
static uint8_t *aperture, *visible, *row_buffer;
static int *columns, *rows;
static int edges[641], stride, repeat, out_w, out_h, logical_w, logical_h;
static enum dz_video_mode mode;
static enum dz_presentation presentation;
static SDL_Renderer *mapping_renderer;
static struct dz_canvas_view view;
static uint32_t capacity;
static uint64_t serial, row_serial[1640], seen_a[32768], seen_b[32768];
static uint32_t seen_address[32768];
static size_t allocated;
static int restart;

/* DI wraps before adding the page segment base. Keep the mode-13h byte swap
 * (320*y is equivalent only for ordinary rows 0..255). */
static uint32_t pixel_address(uint16_t x, uint16_t y, unsigned pixels_per_byte)
{
    uint16_t swapped = (uint16_t)((uint16_t)(y << 8) | (y >> 8));
    uint16_t offset = pixels_per_byte == 1 ? (uint16_t)(x + swapped + (swapped >> 2))
        : (uint16_t)(80u * y + x / pixels_per_byte);
    uint32_t address = (uint32_t)page_paragraph * 16u + offset;
    return address < DZ_VRAM_PLANE_BYTES ? address * pixels_per_byte + x % pixels_per_byte : DZ_VRAM_PIXELS;
}

void dz_canvas_plot(uint16_t x, uint16_t y, unsigned pixels_per_byte, uint8_t index)
{
    dz_canvas_store(pixel_address(x, y, pixels_per_byte), index, pixels_per_byte == 8);
}
uint8_t dz_canvas_pixel_read(uint16_t x, uint16_t y, unsigned pixels_per_byte)
{
    uint8_t index = dz_canvas_read(pixel_address(x, y, pixels_per_byte));
    return pixels_per_byte == 8 ? index & 15u : index;
}
void dz_canvas_raster_xor(int enabled) { dz_video.ega_xor = enabled != 0; }

static int active(void)
{
    return dz_display.canvas == DZ_CANVAS_SCREEN && aperture &&
        dz_video.mode == mode && dz_video.width == logical_w && dz_video.height == logical_h;
}

static size_t cell_offset(uint32_t address)
{
    return (size_t)(address / (unsigned)logical_w) * (unsigned)repeat * (unsigned)stride +
        (unsigned)edges[address % (unsigned)logical_w];
}

static void stamp(uint32_t address, uint8_t index)
{
    size_t offset = cell_offset(address);
    int width = edges[address % (unsigned)logical_w + 1] - edges[address % (unsigned)logical_w];
    for (int y = 0; y < repeat; ++y) memset(aperture + offset + (size_t)y * stride, index, (size_t)width);
}

uint8_t dz_canvas_read(uint32_t address)
{
    if (active() && address < capacity) return aperture[cell_offset(address)];
    return address < DZ_VRAM_PIXELS ? dz_video.vram[address] : 0;
}

void dz_canvas_write(uint32_t address, uint8_t index)
{
    if (address >= DZ_VRAM_PIXELS || dz_video.vram[address] == index) return;
    dz_video.vram[address] = index;
    if (active() && address < capacity) {
        stamp(address, index);
        row_serial[address / (unsigned)logical_w] = ++serial;
    }
}

void dz_canvas_store(uint32_t address, uint8_t index, int ega)
{
    if (ega) {
        index &= 15u;
        if (dz_video.ega_xor) index ^= dz_canvas_read(address) & 15u;
    }
    dz_canvas_write(address, index);
}

void dz_canvas_fill(uint32_t address, size_t count, uint8_t index)
{
    if (address >= DZ_VRAM_PIXELS) return;
    if (count > DZ_VRAM_PIXELS - address) count = DZ_VRAM_PIXELS - address;
    if (!active()) memset(dz_video.vram + address, index, count);
    else for (size_t i = 0; i < count; ++i) dz_canvas_write(address + (uint32_t)i, index);
}
void dz_canvas_clear(uint8_t index) { dz_canvas_fill(0, DZ_VRAM_PIXELS, index); }

void dz_canvas_rect(int x, int y, int width, int height, uint8_t index)
{
    if (width <= 0 || height <= 0) return;
    for (int row = 0; row < height; ++row)
        dz_canvas_fill((uint32_t)((y + row) * dz_video.width + x), (size_t)width, index);
}

/* memmove order preserves overlapping scrolls; callers supply in-aperture rectangles. */
void dz_canvas_move(int dx, int dy, int sx, int sy, int width, int height)
{
    if (width <= 0 || height <= 0) return;
    for (int j = 0; j < height; ++j) {
        int row = dy > sy ? height - 1 - j : j;
        for (int i = 0; i < width; ++i) {
            int col = dy == sy && dx > sx ? width - 1 - i : i;
            dz_canvas_write((uint32_t)((dy + row) * dz_video.width + dx + col),
                dz_canvas_read((uint32_t)((sy + row) * dz_video.width + sx + col)));
        }
    }
}

void dz_canvas_select_page(uint16_t paragraph) { page_paragraph = paragraph; }
void dz_canvas_start(unsigned row) { dz_video.display_start_row = row; }
void dz_canvas_split(unsigned row) { dz_video.line_compare = row; }
const uint8_t *dz_canvas_memory(void) { return dz_video.vram; }
uint8_t *dz_canvas_destination(void) { return dz_video.vram; }
void dz_canvas_buffer_store(uint8_t *destination, uint8_t index)
{
    uintptr_t address = (uintptr_t)destination, base = (uintptr_t)dz_video.vram;
    if (address >= base && address - base < DZ_VRAM_PIXELS)
        dz_canvas_write((uint32_t)(address - base), index);
    else *destination = index;
}
void dz_canvas_shutdown(void)
{
    free(aperture); free(visible); free(columns); free(rows); free(row_buffer);
    aperture = visible = row_buffer = NULL; columns = rows = NULL;
    view = (struct dz_canvas_view){0}; allocated = 0; mapping_renderer = NULL;
    restart = 0;
}

/* Ask SDL for its actual nearest address map, including float viewport edges,
 * clipping and backend-specific fixed-point rounding. This texture contains
 * coordinates only, never application pixels. No simulation service is called. */
static SDL_Surface *nearest_map(SDL_Renderer *renderer, int w, int h, enum dz_presentation policy)
{
    SDL_Texture *saved = SDL_GetRenderTarget(renderer);
    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_Texture *source = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
        dz_video.width, dz_video.height);
    uint32_t *pixels = malloc((size_t)dz_video.width * dz_video.height * sizeof *pixels);
    SDL_Surface *raw = NULL, *result = NULL;
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    if (!target || !source || !pixels) goto done;
    for (int i = 0; i < dz_video.width * dz_video.height; ++i) pixels[i] = UINT32_C(0xff000000) | (unsigned)(i + 1);
    SDL_FRect dest = dz_present_viewport(w, h, policy);
    if (!SDL_SetTextureScaleMode(source, SDL_SCALEMODE_NEAREST) ||
        !SDL_SetTextureBlendMode(source, SDL_BLENDMODE_NONE) ||
        !SDL_UpdateTexture(source, NULL, pixels, dz_video.width * (int)sizeof *pixels) ||
        !SDL_SetRenderTarget(renderer, target) || !SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) ||
        !SDL_RenderClear(renderer) || !SDL_RenderTexture(renderer, source, NULL, &dest)) goto done;
    raw = SDL_RenderReadPixels(renderer, NULL);
    if (raw) result = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_ARGB8888);
done:
    SDL_SetRenderTarget(renderer, saved);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_DestroySurface(raw); SDL_DestroyTexture(target); SDL_DestroyTexture(source); free(pixels);
    return result;
}

static uint32_t map_pixel(SDL_Surface *map, int x, int y)
{
    return ((uint32_t *)((uint8_t *)map->pixels + (size_t)y * map->pitch))[x] & UINT32_C(0xffffff);
}

int dz_canvas_configure(SDL_Renderer *renderer, int w, int h, enum dz_presentation policy)
{
    if (dz_display.canvas != DZ_CANVAS_SCREEN || dz_video.mode == DZ_MODE_TEXT) return 0;
    if (aperture && mapping_renderer == renderer && out_w == w && out_h == h && presentation == policy &&
        mode == dz_video.mode && logical_w == dz_video.width && logical_h == dz_video.height) return 1;
    int resized = aperture && mode == dz_video.mode &&
        (out_w != w || out_h != h || presentation != policy);
    int pending = restart || resized;
    dz_canvas_shutdown();
    if (w <= 0 || h <= 0 || w > 32768 || h > 32768) return SDL_SetError("Invalid canvas size"), -1;
    /* Bound transient coordinate readback and permanent storage before allocation. */
    if ((uint64_t)w * h * 4 > UINT64_C(268435456)) return SDL_SetError("Canvas mapping exceeds 256 MiB"), -1;
    SDL_Surface *map = nearest_map(renderer, w, h, policy);
    if (!map) return -1;
    int left = w, top = h, right = 0, bottom = 0;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (map_pixel(map, x, y)) {
        if (x < left) left = x;
        if (x >= right) right = x + 1;
        if (y < top) top = y;
        if (y >= bottom) bottom = y + 1;
    }
    logical_w = dz_video.width; logical_h = dz_video.height; mode = dz_video.mode;
    capacity = DZ_VRAM_PLANE_BYTES * (unsigned)dz_video.pixels_per_byte;
    int vw = right - left, vh = bottom - top;
    if (vw <= 0 || vh <= 0) { SDL_DestroySurface(map); return SDL_SetError("Empty canvas viewport"), -1; }
    columns = malloc((size_t)vw * sizeof *columns); rows = malloc((size_t)vh * sizeof *rows);
    if (!columns || !rows) goto fail;
    int counts[640] = {0}, heights[400] = {0};
    for (int x = 0; x < vw; ++x) {
        columns[x] = (int)((map_pixel(map, left + x, top) - 1) % (unsigned)logical_w);
        ++counts[columns[x]];
    }
    repeat = 1;
    for (int y = 0; y < vh; ++y) {
        rows[y] = (int)((map_pixel(map, left, top + y) - 1) / (unsigned)logical_w);
        if (++heights[rows[y]] > repeat) repeat = heights[rows[y]];
    }
    SDL_DestroySurface(map); map = NULL;
    edges[0] = 0;
    for (int x = 0; x < logical_w; ++x) edges[x + 1] = edges[x] + (counts[x] ? counts[x] : 1);
    stride = edges[logical_w];
    size_t aperture_bytes = (size_t)((capacity + logical_w - 1) / logical_w) * repeat * stride;
    allocated = aperture_bytes + (size_t)vw * vh + (size_t)(vw + vh) * sizeof(int) + vw;
    if (allocated > 268435456u) { SDL_SetError("Canvas storage exceeds 256 MiB"); goto fail; }
    aperture = calloc(aperture_bytes, 1); visible = calloc((size_t)vw * vh, 1); row_buffer = malloc((size_t)vw);
    if (!aperture || !visible || !row_buffer) goto fail;
    out_w = w; out_h = h; presentation = policy; mapping_renderer = renderer;
    view = (struct dz_canvas_view){visible, left, top, vw, vh, ++serial};
    memset(seen_a, 0, sizeof seen_a); memset(seen_b, 0, sizeof seen_b);
    for (unsigned i = 0; i < 1640; ++i) row_serial[i] = serial;
    for (int y = 0; y < vh; ++y) seen_address[y] = UINT32_MAX;
    if (resized) dz_canvas_clear(0);
    else for (uint32_t address = 0; address < capacity; ++address) stamp(address, dz_video.vram[address]);
    restart = pending;
    return 1;
fail:
    SDL_DestroySurface(map); dz_canvas_shutdown();
    if (!*SDL_GetError()) SDL_OutOfMemory();
    return -1;
}

struct dz_canvas_view dz_canvas_visible(void)
{
    if (!active()) return (struct dz_canvas_view){0};
    /* A generator can be on the stack at resize. Show the cleared canvas
     * until its next safe image boundary discards that step and rebuilds. */
    if (restart) return view;
    unsigned width = (unsigned)logical_w;
    for (int y = 0; y < view.height; ++y) {
        unsigned sy = (unsigned)rows[y];
        unsigned row = sy > dz_video.line_compare ? sy - dz_video.line_compare - 1u : dz_video.display_start_row + sy;
        uint32_t address = (row * width) % capacity;
        uint64_t a = row_serial[address / width], b = row_serial[((address + width - 1) % capacity) / width];
        if (seen_address[y] == address && seen_a[y] == a && seen_b[y] == b) continue;
        for (int x = 0; x < view.width;) {
            int end = x + 1;
            while (end < view.width && columns[end] == columns[x]) ++end;
            memset(row_buffer + x, dz_canvas_read((address + (unsigned)columns[x]) % capacity), (size_t)(end - x));
            x = end;
        }
        uint8_t *dest = visible + (size_t)y * view.width;
        if (memcmp(dest, row_buffer, (size_t)view.width)) {
            memcpy(dest, row_buffer, (size_t)view.width); view.revision = ++serial;
        }
        seen_address[y] = address; seen_a[y] = a; seen_b[y] = b;
    }
    return view;
}
int dz_canvas_take_restart(void) { int pending = restart; restart = 0; return pending; }
void dz_canvas_mode_changed(void) { dz_canvas_shutdown(); }
size_t dz_canvas_bytes(void) { return allocated; }
