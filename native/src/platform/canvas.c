/* Uniform physical-pixel pages, with a separate logical scheduling receipt. */
#include "canvas.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern uint16_t page_paragraph;
extern int16_t display_cleared;
static uint8_t *aperture, *visible, *coverage, *under, *visible_coverage, *visible_under;
static int xe[641], ye[401], left, top, right, bottom;
static int out_w, out_h, logical_w, logical_h, raster_h, ring_h, pages;
static enum dz_video_mode mode;
static enum dz_presentation presentation;
static SDL_Renderer *mapping_renderer;
static struct dz_canvas_view view;
static uint32_t capacity;
static uint64_t serial, gathered;
static unsigned seen_start, seen_split;
static size_t allocated;
static int restart, native_ink, suppress, override_index = -1, full_fade;
static unsigned fade_key;
static uint8_t sample;

static int active(void)
{
    return dz_display.canvas == DZ_CANVAS_SCREEN && aperture && mode == dz_video.mode &&
        logical_w == dz_video.width && logical_h == dz_video.height;
}
static uint32_t pixel_address(uint16_t x, uint16_t y, unsigned ppb)
{
    uint16_t swapped = (uint16_t)((uint16_t)(y << 8) | (y >> 8));
    uint16_t offset = ppb == 1 ? (uint16_t)(x + swapped + (swapped >> 2))
        : (uint16_t)(80u * y + x / ppb);
    uint32_t address = (uint32_t)page_paragraph * 16u + offset;
    return address < DZ_VRAM_PLANE_BYTES ? address * ppb + x % ppb : DZ_VRAM_PIXELS;
}
static void bounds(uint32_t address, int *x0, int *y0, int *x1, int *y1)
{
    unsigned row = address / (unsigned)logical_w, x = address % (unsigned)logical_w;
    unsigned page = row / (unsigned)logical_h, y = row % (unsigned)logical_h;
    *x0 = xe[x]; *x1 = xe[x + 1];
    *y0 = (int)page * out_h + ye[y]; *y1 = (int)page * out_h + ye[y + 1];
    if (full_fade) {
        *x0 = (int)((uint64_t)x * out_w / logical_w);
        *x1 = (int)((uint64_t)(x + 1) * out_w / logical_w);
        *y0 = (int)page * out_h + (int)((uint64_t)y * out_h / logical_h);
        *y1 = (int)page * out_h + (int)((uint64_t)(y + 1) * out_h / logical_h);
    }
}
static uint8_t resolved(size_t i)
{
    return coverage && coverage[i] < 128 ? under[i] : aperture[i];
}
uint8_t dz_canvas_read(uint32_t address)
{
    if (active() && address < capacity) {
        int x0, y0, x1, y1;
        bounds(address, &x0, &y0, &x1, &y1);
        if (x0 < x1 && y0 < y1) return resolved((size_t)((y0 + y1 - 1) / 2) * out_w + (x0 + x1 - 1) / 2);
    }
    return address < DZ_VRAM_PIXELS ? dz_video.vram[address] : 0;
}
void dz_canvas_plot(uint16_t x, uint16_t y, unsigned ppb, uint8_t index)
{ dz_canvas_store(pixel_address(x, y, ppb), index, ppb == 8); }
uint8_t dz_canvas_control_pixel_read(uint16_t x, uint16_t y, unsigned ppb)
{
    uint32_t address = pixel_address(x, y, ppb);
    sample = dz_canvas_read(address);
    /* The original control lane determines work/RNG/branches. Sampling fades
     * explicitly use sample for their raster lane, without a second charge. */
    uint8_t index = address < DZ_VRAM_PIXELS ? dz_video.vram[address] : 0;
    return ppb == 8 ? index & 15u : index;
}
uint8_t dz_canvas_pixel_read(uint16_t x, uint16_t y, unsigned ppb)
{
    uint8_t c = dz_canvas_read(pixel_address(x, y, ppb));
    return ppb == 8 ? c & 15u : c;
}
void dz_canvas_sample_reset(void) { sample = 0; }
uint8_t dz_canvas_sample(void) { return sample; }
void dz_canvas_override(int index) { override_index = index; }
void dz_canvas_suppress(int enabled) { suppress = enabled; }
void dz_canvas_raster_xor(int enabled) { dz_video.ega_xor = enabled != 0; }
static void stamp(uint32_t address, uint8_t index, int xor)
{
    int x0, y0, x1, y1;
    bounds(address, &x0, &y0, &x1, &y1);
    for (int y = y0; y < y1; ++y) {
        size_t i = (size_t)y * out_w + x0, n = (size_t)(x1 - x0);
        if (xor) for (size_t x = 0; x < n; ++x) aperture[i + x] = resolved(i + x) ^ index;
        else memset(aperture + i, index, n);
        if (coverage) memset(coverage + i, 255, n);
    }
}
void dz_canvas_write(uint32_t address, uint8_t index)
{
    if (address >= DZ_VRAM_PIXELS) return;
    int changed = dz_video.vram[address] != index;
    dz_video.vram[address] = index;
    if (active() && !suppress && address < capacity && (changed || native_ink || override_index >= 0)) {
        stamp(address, override_index < 0 ? index : (uint8_t)override_index, 0); ++serial;
    }
}
void dz_canvas_store(uint32_t address, uint8_t index, int ega)
{
    if (ega && dz_video.ega_xor && active() && !suppress && address < capacity) {
        uint8_t ink = override_index < 0 ? index & 15u : (uint8_t)override_index & 15u;
        stamp(address, ink, 1); ++serial;
        if (address < DZ_VRAM_PIXELS) dz_video.vram[address] ^= index & 15u;
    } else {
        if (ega) {
            index &= 15u;
            if (dz_video.ega_xor && address < DZ_VRAM_PIXELS) index ^= dz_video.vram[address] & 15u;
        }
        dz_canvas_write(address, index);
    }
}
void dz_canvas_fill(uint32_t address, size_t count, uint8_t index)
{
    if (address >= DZ_VRAM_PIXELS) return;
    if (!address && count >= DZ_VRAM_PIXELS) { dz_canvas_clear(index); return; }
    if (count > DZ_VRAM_PIXELS - address) count = DZ_VRAM_PIXELS - address;
    if (!active()) memset(dz_video.vram + address, index, count);
    else for (size_t i = 0; i < count; ++i) dz_canvas_write(address + (uint32_t)i, index);
}
void dz_canvas_clear(uint8_t index)
{
    memset(dz_video.vram, index, DZ_VRAM_PIXELS);
    if (active()) {
        memset(aperture, index, (size_t)out_w * raster_h);
        if (coverage) memset(coverage, 255, (size_t)out_w * raster_h);
        native_ink = 0; ++serial;
    }
}
void dz_canvas_clear_page(uint16_t paragraph, uint16_t words, uint16_t pattern)
{
    if (!active() || !native_ink) return;
    uint32_t first = (uint32_t)paragraph*16u*(unsigned)dz_video.pixels_per_byte;
    uint32_t count = (uint32_t)words*2u*(unsigned)dz_video.pixels_per_byte;
    uint32_t page_size = (unsigned)logical_w*logical_h;
    if (count < page_size || first % page_size) return;
    unsigned p = first/page_size, n = count/page_size;
    if (p >= (unsigned)pages) return;
    if (n > (unsigned)pages-p) n = (unsigned)pages-p;
    size_t i = (size_t)p*out_w*out_h, bytes = (size_t)n*out_w*out_h;
    /* The literal loop still maintains the exact patterned logical receipt.
     * Margins use the first pattern byte; no stale native samples survive. */
    memset(aperture+i, (uint8_t)pattern & (dz_video.mode == DZ_MODE_EGA_350 ? 15u : 255u), bytes);
    if (coverage) memset(coverage+i, 255, bytes);
    ++serial;
}
void dz_canvas_rect(int x, int y, int width, int height, uint8_t index)
{
    if (width <= 0 || height <= 0) return;
    if (active() && native_ink) {
        int x0,y0,x1,y1,unused_x,unused_y;
        bounds((uint32_t)(y*logical_w+x),&x0,&y0,&x1,&y1);
        bounds((uint32_t)((y+height-1)*logical_w+x+width-1),&unused_x,&unused_y,&x1,&y1);
        if (!x) x0 = 0;
        if (x+width == logical_w) x1 = out_w;
        if (!(y % logical_h)) y0 = y/logical_h*out_h;
        if (!((y+height) % logical_h)) y1 = (y+height)/logical_h*out_h;
        for (int row = y0; row < y1; ++row) {
            size_t i = (size_t)row*out_w+x0;
            memset(aperture+i,index,(size_t)(x1-x0));
            if (coverage) memset(coverage+i,255,(size_t)(x1-x0));
        }
        ++serial;
    }
    for (int row = 0; row < height; ++row)
        dz_canvas_fill((uint32_t)((y + row) * dz_video.width + x), (size_t)width, index);
}
void dz_canvas_move(int dx, int dy, int sx, int sy, int width, int height)
{
    if (width <= 0 || height <= 0) return;
    if (!active() || !native_ink) {
        for (int j = 0; j < height; ++j) {
            int row = dy > sy ? height-1-j : j;
            for (int i = 0; i < width; ++i) {
                int col = dy == sy && dx > sx ? width-1-i : i;
                dz_canvas_write((uint32_t)((dy+row)*dz_video.width+dx+col),
                    dz_canvas_read((uint32_t)((sy+row)*dz_video.width+sx+col)));
            }
        }
        return;
    }
    /* Copy full raster rectangles, preserving subpixel ink and coverage. */
    if (active()) {
        int x0, y0, x1, y1, tx, ty, tx1, ty1, unused_x, unused_y;
        bounds((uint32_t)(sy * logical_w + sx), &x0, &y0, &x1, &y1);
        bounds((uint32_t)((sy + height - 1) * logical_w + sx + width - 1), &tx, &ty, &x1, &y1);
        bounds((uint32_t)(dy * logical_w + dx), &tx, &ty, &tx1, &ty1);
        /* Fractional blocks can differ by one pixel: nearest resize the source
         * rectangle via a temporary copy so overlap has memmove semantics. */
        bounds((uint32_t)((dy + height - 1) * logical_w + dx + width - 1), &unused_x, &unused_y, &tx1, &ty1);
        if (!sx) x0 = 0;
        if (sx+width == logical_w) x1 = out_w;
        if (!dx) tx = 0;
        if (dx+width == logical_w) tx1 = out_w;
        if (!(sy % logical_h)) y0 = sy/logical_h*out_h;
        if (!((sy+height) % logical_h)) y1 = (sy+height)/logical_h*out_h;
        if (!(dy % logical_h)) ty = dy/logical_h*out_h;
        if (!((dy+height) % logical_h)) ty1 = (dy+height)/logical_h*out_h;
        int sw = x1 - x0, sh = y1 - y0, dw = tx1 - tx, dh = ty1 - ty;
        if (sw > 0 && sh > 0 && dw > 0 && dh > 0) {
            size_t n = (size_t)sw * sh;
            uint8_t *copy = malloc(n * (coverage ? 3u : 1u));
            if (!copy) { SDL_OutOfMemory(); return; }
            for (int y = 0; y < sh; ++y) {
                size_t i = (size_t)(y0 + y) * out_w + x0;
                memcpy(copy + (size_t)y * sw, aperture + i, (size_t)sw);
                if (coverage) {
                    memcpy(copy + n + (size_t)y * sw, coverage + i, (size_t)sw);
                    memcpy(copy + 2*n + (size_t)y * sw, under + i, (size_t)sw);
                }
            }
            for (int y = 0; y < dh; ++y) for (int x = 0; x < dw; ++x) {
                size_t a = (size_t)(ty + y) * out_w + tx + x;
                size_t b = (size_t)(y * sh / dh) * sw + x * sw / dw;
                aperture[a] = copy[b];
                if (coverage) { coverage[a] = copy[n+b]; under[a] = copy[2*n+b]; }
            }
            free(copy); ++serial;
        }
    }
    for (int j = 0; j < height; ++j) {
        int row = dy > sy ? height - 1 - j : j;
        memmove(dz_video.vram + (dy + row) * dz_video.width + dx,
            dz_video.vram + (sy + row) * dz_video.width + sx, (size_t)width);
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
    if (address >= base && address - base < DZ_VRAM_PIXELS) dz_canvas_write((uint32_t)(address - base), index);
    else *destination = index;
}
void dz_canvas_shutdown(void)
{
    free(aperture); free(visible); free(coverage); free(under); free(visible_coverage); free(visible_under);
    aperture = visible = coverage = under = visible_coverage = visible_under = NULL;
    view = (struct dz_canvas_view){0}; allocated = 0; mapping_renderer = NULL;
    restart = native_ink = suppress = full_fade = 0; override_index = -1;
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
    if (active() && mapping_renderer == renderer && out_w == w && out_h == h && presentation == policy) return 1;
    int resized = active() && (out_w != w || out_h != h || presentation != policy);
    int pending = restart || resized;
    dz_canvas_shutdown();
    if (w <= 0 || h <= 0 || w > 32768 || h > 32768 || (uint64_t)w*h*4 > 268435456u)
        return SDL_SetError("Canvas mapping exceeds 256 MiB or invalid size"), -1;
    SDL_Surface *map = nearest_map(renderer, w, h, policy);
    if (!map) return -1;
    logical_w = dz_video.width; logical_h = dz_video.height; mode = dz_video.mode;
    capacity = DZ_VRAM_PLANE_BYTES * (unsigned)dz_video.pixels_per_byte;
    pages = (int)((capacity + (unsigned)logical_w*logical_h - 1) / ((unsigned)logical_w*logical_h));
    left = w; top = h; right = bottom = 0;
    for (int x = 0; x <= logical_w; ++x) xe[x] = -1;
    for (int y = 0; y <= logical_h; ++y) ye[y] = -1;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        uint32_t v = map_pixel(map, x, y);
        if (!v) continue;
        int sx = (int)((v-1) % logical_w), sy = (int)((v-1) / logical_w);
        if (xe[sx] < 0) xe[sx] = x;
        if (ye[sy] < 0) ye[sy] = y;
        if (x < left) left = x;
        if (x >= right) right = x+1;
        if (y < top) top = y;
        if (y >= bottom) bottom = y+1;
    }
    SDL_DestroySurface(map);
    xe[logical_w] = right; ye[logical_h] = bottom;
    for (int x = logical_w-1; x >= 0; --x) if (xe[x] < 0) xe[x] = xe[x+1];
    for (int y = logical_h-1; y >= 0; --y) if (ye[y] < 0) ye[y] = ye[y+1];
    out_w = w; out_h = h; raster_h = pages*h;
    ring_h = (int)lround((double)capacity/logical_w*h/logical_h);
    if (ring_h < h) ring_h = h;
    size_t n = (size_t)w*raster_h, v = (size_t)w*h;
    int aa = dz_display.native_aa;
    allocated = (n+v) * (aa ? 3u : 1u);
    if (allocated > 268435456u) return SDL_SetError("Canvas storage exceeds 256 MiB"), -1;
    aperture = calloc(n, 1); visible = calloc(v, 1);
    if (aa) {
        coverage = malloc(n); under = calloc(n, 1);
        visible_coverage = malloc(v); visible_under = calloc(v, 1);
        if (coverage) memset(coverage, 255, n);
        if (visible_coverage) memset(visible_coverage, 255, v);
    }
    if (!aperture || !visible || (aa && (!coverage || !under || !visible_coverage || !visible_under))) {
        dz_canvas_shutdown(); return SDL_OutOfMemory(), -1;
    }
    presentation = policy; mapping_renderer = renderer;
    view = (struct dz_canvas_view){visible, 0, 0, w, h, ++serial, visible_coverage, visible_under};
    gathered = 0; seen_start = seen_split = UINT32_MAX;
    if (resized) dz_canvas_clear(0);
    else for (uint32_t a = 0; a < capacity; ++a) stamp(a, dz_video.vram[a], 0);
    restart = pending;
    return 1;
}
struct dz_canvas_view dz_canvas_visible(void)
{
    if (!active()) return (struct dz_canvas_view){0};
    if (restart || (gathered == serial && seen_start == dz_video.display_start_row && seen_split == dz_video.line_compare)) return view;
    int start = (int)llround((double)dz_video.display_start_row * out_h / logical_h);
    int split = dz_video.line_compare >= (unsigned)logical_h ? out_h :
        (int)llround((double)(dz_video.line_compare+1) * out_h / logical_h);
    if (!native_ink && (dz_video.display_start_row || dz_video.line_compare < (unsigned)logical_h)) {
        /* Faithful cells remain constant. Gather their actual nearest blocks
         * during pans, retaining the entire phase-2 oracle without row padding. */
        memset(visible, 0, (size_t)out_w*out_h);
        for (int y = 0; y < logical_h; ++y) {
            unsigned row = (unsigned)y > dz_video.line_compare ? (unsigned)y-dz_video.line_compare-1 :
                dz_video.display_start_row+(unsigned)y;
            for (int x = 0; x < logical_w; ++x) {
                uint8_t c = dz_canvas_read((row*(unsigned)logical_w+(unsigned)x)%capacity);
                for (int py = ye[y]; py < ye[y+1]; ++py)
                    memset(visible+(size_t)py*out_w+xe[x], c, (size_t)(xe[x+1]-xe[x]));
            }
        }
        if (coverage) memset(visible_coverage, 255, (size_t)out_w*out_h);
    } else for (int y = 0; y < out_h; ++y) {
        int sy = (y >= split ? y-split : y+start) % ring_h;
        size_t a = (size_t)sy*out_w, b = (size_t)y*out_w;
        memcpy(visible+b, aperture+a, (size_t)out_w);
        if (coverage) {
            memcpy(visible_coverage+b, coverage+a, (size_t)out_w);
            memcpy(visible_under+b, under+a, (size_t)out_w);
        }
    }
    gathered = serial; seen_start = dz_video.display_start_row; seen_split = dz_video.line_compare;
    view.revision = ++serial; gathered = serial;
    return view;
}
int dz_canvas_has_native(void) { return active() && native_ink; }
SDL_FRect dz_canvas_content(void) { return (SDL_FRect){(float)left,(float)top,(float)(right-left),(float)(bottom-top)}; }
void dz_canvas_native_size(int *w, int *h)
{ *w = active() ? out_w : 0; *h = active() ? out_h : 0; }
/* A capsule is a rectangle plus its two circular caps. Intersect it with
 * pixel-centre scanlines; opaque interiors become short memset spans. */
struct capsule { double ax,ay,bx,by,r,px[4],py[4]; };
static struct capsule capsule(double ax, double ay, double bx, double by, double r)
{
    double dx = bx-ax, dy = by-ay, length = hypot(dx,dy);
    double nx = length ? -dy*r/length : 0, ny = length ? dx*r/length : 0;
    return (struct capsule){ax,ay,bx,by,r,{ax+nx,bx+nx,bx-nx,ax-nx},{ay+ny,by+ny,by-ny,ay-ny}};
}
static int capsule_span(const struct capsule *c, double y, int *first, int *last)
{
    double lo = HUGE_VAL, hi = -HUGE_VAL;
    const double cy[] = {c->ay,c->by}, cx[] = {c->ax,c->bx};
    for (int i = 0; i < 2; ++i) {
        double dy = y-cy[i];
        if (fabs(dy) <= c->r) {
            double x = sqrt(fmax(0,c->r*c->r-dy*dy));
            lo = fmin(lo,cx[i]-x); hi = fmax(hi,cx[i]+x);
        }
    }
    for (int i = 0; i < 4; ++i) {
        int j = (i+1)&3;
        double dy = c->py[j]-c->py[i];
        if (!dy) continue;
        double t = (y-c->py[i])/dy;
        if (t >= 0 && t <= 1) {
            double x = c->px[i]+t*(c->px[j]-c->px[i]);
            lo = fmin(lo,x); hi = fmax(hi,x);
        }
    }
    if (lo > hi || hi < 0.5 || lo > out_w-0.5) return 0;
    *first = (int)ceil(fmax(0.5,lo)-0.5);
    *last = (int)floor(fmin(out_w-0.5,hi)-0.5);
    return *first <= *last;
}
void dz_canvas_stroke(double ax, double ay, double bx, double by, double width, uint8_t index)
{
    if (!active() || restart || width <= 0) return;
    if ((uint32_t)page_paragraph*16u >= DZ_VRAM_PLANE_BYTES) return;
    double row = (double)page_paragraph*16*dz_video.pixels_per_byte/logical_w;
    int base = (int)llround(row*out_h/logical_h) % ring_h;
    double radius = width*0.5, fringe = coverage ? 0.5 : 0;
    int y0 = (int)fmax(0,floor(fmin(ay,by)-radius-fringe));
    int y1 = (int)fmin(out_h-1,ceil(fmax(ay,by)+radius+fringe));
    struct capsule outer = capsule(ax,ay,bx,by,radius+fringe);
    struct capsule inner = capsule(ax,ay,bx,by,fmax(0,radius-fringe));
    double dx = bx-ax, dy = by-ay, length = dx*dx+dy*dy;
    int xor = dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor;
    for (int y = y0; y <= y1; ++y) {
        int x0,x1,ix0 = 1,ix1 = 0;
        if (!capsule_span(&outer,y+0.5,&x0,&x1)) continue;
        size_t offset = (size_t)((base+y)%ring_h)*out_w;
        if (!coverage) {
            if (xor) for (int x = x0; x <= x1; ++x) aperture[offset+x] ^= index & 15u;
            else memset(aperture+offset+x0,index,(size_t)(x1-x0+1));
            continue;
        }
        if (!xor && capsule_span(&inner,y+0.5,&ix0,&ix1)) {
            memset(aperture+offset+ix0,index,(size_t)(ix1-ix0+1));
            memset(coverage+offset+ix0,255,(size_t)(ix1-ix0+1));
        }
        for (int x = x0; x <= x1; ++x) {
            if (!xor && x >= ix0 && x <= ix1) continue;
            double t = length ? ((x+0.5-ax)*dx+(y+0.5-ay)*dy)/length : 0;
            t = fmax(0,fmin(1,t));
            double ex = x+0.5-ax-t*dx, ey = y+0.5-ay-t*dy;
            unsigned c = (unsigned)lround(fmax(0,fmin(1,radius+0.5-sqrt(ex*ex+ey*ey)))*255);
            if (!c) continue;
            size_t i = offset+x;
            if (aperture[i] == index && coverage[i] < 255 && !xor) {
                if (c > coverage[i]) coverage[i] = (uint8_t)c;
            } else {
                under[i] = resolved(i);
                aperture[i] = xor ? under[i] ^ (index & 15u) : index;
                coverage[i] = (uint8_t)c;
            }
        }
    }
    native_ink = 1; ++serial;
}
void dz_canvas_fade_begin(unsigned key)
{
    fade_key = key;
    full_fade = native_ink && (key == 'v' || key == 'w' || key == 'z');
    if (!active() || !native_ink || key == 'j' || key == 'p') return;
    /* Unadapted procedural/sampling fades own the faithful rectangle. Clear
     * every page's margins once, including interrupted/repeated transitions. */
    for (int p = 0; p < pages; ++p) for (int y = 0; y < out_h; ++y) {
        int a = y < top || y >= bottom ? out_w : left;
        int b = y < top || y >= bottom ? out_w : right;
        size_t i = (size_t)(p*out_h+y)*out_w;
        memset(aperture+i, 0, (size_t)a);
        memset(aperture+i+b, 0, (size_t)(out_w-b));
        if (coverage) {
            memset(coverage+i, 255, (size_t)a);
            memset(coverage+i+b, 255, (size_t)(out_w-b));
        }
    }
    ++serial;
}
void dz_canvas_fade_end(void)
{
    /* A completed spatial eraser owns the whole physical canvas. In particular,
     * tetrYs must not strand native samples in logically empty control columns. */
    if (active() && native_ink && display_cleared &&
        (fade_key == 'v' || fade_key == 'w' || fade_key == 'y' || fade_key == 'z')) {
        memset(aperture,0,(size_t)out_w*raster_h);
        if (coverage) memset(coverage,255,(size_t)out_w*raster_h);
        ++serial;
    }
    full_fade = 0; override_index = -1;
}
int dz_canvas_take_restart(void) { int pending = restart; restart = 0; return pending; }
void dz_canvas_mode_changed(void) { dz_canvas_shutdown(); }
size_t dz_canvas_bytes(void) { return allocated; }
