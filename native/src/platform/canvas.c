/* Uniform physical-pixel pages, with a separate logical scheduling receipt. */
#include "canvas.h"
#include "dazzle.h"
#include "effects/effects.h"
#include "effects/fades.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

extern uint16_t page_paragraph;
extern int16_t display_cleared;
static uint8_t *aperture, *coverage, *under, *side_written, *side_base;
/* Address decoding is prepared at mode/size change, never divided per plot. */
static uint32_t *cells;
static int *rows;
static SDL_Rect *dirty_tiles, *dirty_rects;
static int tile_columns, tile_count;
static int gpu_transfer;
static int xe[641], ye[401], left, top, right, bottom;
static int out_w, out_h, logical_w, logical_h, raster_h, ring_h, pages, page_origin;
static enum dz_video_mode mode;
static enum dz_presentation presentation;
static SDL_Renderer *mapping_renderer;
static struct dz_canvas_view view;
static uint32_t capacity;
static uint64_t serial;
static int pending_dirty;
static size_t allocated;
static int restart, native_ink, side_ink, suppress, override_index = -1;
static int fold_fade;
static int fade_active;
static unsigned fade_key,fade_mode_key;
static uint8_t *fade_erased;
static uint8_t *fade_before;
static int fade_zero;
static SDL_Renderer *fade_mode_renderer;
static int fade_mode_w, fade_mode_h;
static enum dz_presentation fade_mode_policy;
/* Reflected logical cells, prepared once with the output map. Each run belongs
 * to exactly one centre column, including at a turn of the repeated fold. */
struct fold_run { int first, end, lane, next; };
static struct fold_run *fold_runs;
static int fold_head[640], fold_count, fold_lanes;
static const uint8_t *side_override;
static uint32_t sample_address = UINT32_MAX;
/* Keep real paint and XOR reach separately: a real coloured head must not
 * prevent a folded zero tail from clearing it. The first real XOR
 * discards earlier fallback XOR, retaining the latest paint/erase result. */
enum { SIDE_WRITTEN = 1, REACH_PAINT = 2, REACH_XOR = 4, REACH_CELL = 8 };
static int literal_clear;
static int mode_import,native_finish;
static int plasma_retain;
static int side_stride;
static uint8_t sample;
static int sides_only;
static struct dz_native_mapping native_mapping;
static int mapping_hw, mapping_hh;
struct dz_canvas_store_audit *dz_canvas_store_audit;
/* Reuse the existing optional store-audit branch for the Extended hook. With
 * no audit and no Extended fade, the ordinary stamp executes no hook test. */
static struct dz_canvas_store_audit fold_audit;
static void select_fade_hook(void)
{
    if (fold_fade && !dz_canvas_store_audit) dz_canvas_store_audit = &fold_audit;
    if (!fold_fade && dz_canvas_store_audit == &fold_audit) dz_canvas_store_audit = NULL;
    /* -2 uses the supplied colour like -1, but forces unchanged receipt stores
     * through the raster too: a zero eraser may still own live side ink. */
    if (override_index < 0) override_index = fold_fade ? -2 : -1;
}

static int active(void)
{
    return dz_display.canvas == DZ_CANVAS_SCREEN && aperture && mode == dz_video.mode &&
        logical_w == dz_video.width && logical_h == dz_video.height;
}
static uint32_t pixel_address(uint16_t x, uint16_t y, unsigned ppb)
{
    uint16_t swapped = (uint16_t)((uint16_t)(y << 8) | (y >> 8));
    uint16_t offset = ppb == 1 ? (uint16_t)(x + swapped + (swapped >> 2))
        : (uint16_t)(80u * y + (x >> (ppb == 8 ? 3 : 2)));
    uint32_t address = (uint32_t)page_paragraph * 16u + offset;
    return address < DZ_VRAM_PLANE_BYTES ? address * ppb + (x & (ppb - 1)) : DZ_VRAM_PIXELS;
}
static inline __attribute__((always_inline)) void bounds(uint32_t address, int *x0, int *y0, int *x1, int *y1)
{
    uint32_t c = cells[address];
    unsigned row = c >> 10, x = c & 1023u;
    const int *xs = xe, *ys = rows;
    *x0 = xs[x]; *x1 = xs[x + 1];
    *y0 = ys[2*row]; *y1 = ys[2*row+1];
}
static void dirty(int x0, int y0, int x1, int y1)
{
    if (x0 >= x1 || y0 >= y1) return;
    SDL_Rect *r = &dirty_tiles[(y0>>8)*tile_columns+(x0>>8)];
    if (!r->w) *r = (SDL_Rect){x0,y0,x1-x0,y1-y0};
    else {
        int a = r->x < x0 ? r->x : x0, b = r->y < y0 ? r->y : y0;
        int c = r->x+r->w > x1 ? r->x+r->w : x1, d = r->y+r->h > y1 ? r->y+r->h : y1;
        *r = (SDL_Rect){a,b,c-a,d-b};
    }
    pending_dirty = 1;
}
static uint8_t resolved(size_t i)
{
    return coverage && coverage[i] < 128 ? under[i] : aperture[i];
}
static void fade_snapshot(void)
{
    if (!fold_fade) return;
    for (int y = 0; y < raster_h; ++y) for (int part = 0; part < 2; ++part) {
        int first = part ? right : 0, end = part ? out_w : left;
        size_t row = (size_t)y*out_w, base = (size_t)y*side_stride+(part ? left : 0);
        for (int x = first; x < end; ++x) {
            size_t i = row+x;
            side_base[base+x-first] = resolved(i);
            side_written[i] &= 1u;
        }
    }
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
void dz_canvas_side_plot(int x, int y, uint8_t index)
{
    if (!active() || restart || suppress || presentation != DZ_PRESENT_EXTENDED ||
        (x >= 0 && x < logical_w) || y < 0 || y > logical_h ||
        (uint32_t)page_paragraph*16u*(unsigned)dz_video.pixels_per_byte+(unsigned)y*logical_w >= capacity) return;
    SDL_FRect r = dz_present_viewport(out_w,out_h,presentation);
    int a = (int)ceil(r.x+(double)x*r.w/logical_w-0.5);
    int b = (int)ceil(r.x+(double)(x+1)*r.w/logical_w-0.5);
    if (a < 0) a = 0;
    if (b > out_w) b = out_w;
    /* Never let extrapolated rounding touch the reference centre. */
    if (x < 0 && b > left) b = left;
    if (x >= logical_w && a < right) a = right;
    if (a >= b) return;
    if (dz_canvas_store_audit) {
        ++dz_canvas_store_audit->reach;
        if (x < dz_canvas_store_audit->min_x) dz_canvas_store_audit->min_x = x;
        if (x > dz_canvas_store_audit->max_x) dz_canvas_store_audit->max_x = x;
    }
    /* Negative-axis reflection can legitimately store row H in the next page.
     * Preserve that one-row boundary without wrapping an invalid DOS address. */
    int cell_y = y == logical_h ? 0 : y, next_page = y == logical_h ? out_h : 0;
    if (dz_canvas_store_audit && ye[cell_y] < ye[cell_y+1]) ++dz_canvas_store_audit->faithful;
    int xor = dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor;
    if (dz_video.mode == DZ_MODE_EGA_350) index &= 15u;
    for (int row = ye[cell_y]; row < ye[cell_y+1]; ++row) {
        int dest = (page_origin+next_page+row)%ring_h;
        size_t i = (size_t)dest*out_w;
        if (fold_fade) {
            uint8_t color = index;
            if (side_override) {
                int tile = (int)floor((double)(a-left)/(right-left));
                int lane = tile < 0 ? tile+fold_lanes/2 : tile+fold_lanes/2-1;
                color = side_override[lane];
            }
            unsigned operation = xor ? REACH_XOR : REACH_PAINT;
            size_t base = (size_t)dest*side_stride+(a < left ? a : left+a-right);
            for (int col = a; col < b; ++col) {
                size_t at = i+col;
                if (xor && !(side_written[at]&REACH_XOR)) {
                    aperture[at] = side_base[base+col-a];
                    if (coverage) coverage[at] = 255;
                }
                side_written[at] |= SIDE_WRITTEN | operation;
                aperture[at] = xor ? resolved(at)^color : color;
                if (!xor) side_base[base+col-a] = color;
            }
        } else if (xor) for (int col = a; col < b; ++col) aperture[i+col] = resolved(i+col)^index;
        else memset(aperture+i+a,index,(size_t)(b-a));
        if (side_written && !fold_fade) memset(side_written+i+a,1,(size_t)(b-a));
        if (coverage) memset(coverage+i+a,255,(size_t)(b-a));

        dirty(a,dest,b,dest+1);
    }
    side_ink = 1;
}
uint8_t dz_canvas_control_pixel_read(uint16_t x, uint16_t y, unsigned ppb)
{
    uint32_t address = pixel_address(x, y, ppb);
    sample = dz_canvas_read(address);
    sample_address = address;
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
void dz_canvas_sample_reset(void) { sample = 0; sample_address = UINT32_MAX; }
uint8_t dz_canvas_sample(void) { return sample; }
void dz_canvas_override(int index) { override_index = index < 0 && fold_fade ? -2 : index; }
int dz_canvas_sample_lanes(void) { return fold_fade ? fold_lanes : 0; }
void dz_canvas_override_sides(const uint8_t *colors) { side_override = colors; }
void dz_canvas_sample_sides(uint8_t *colors)
{
    int n = dz_canvas_sample_lanes();
    if (!n) return;
    memset(colors,sample,(size_t)n);
    if (sample_address >= capacity) return;
    int x0,y0,x1,y1;
    bounds(sample_address,&x0,&y0,&x1,&y1);
    if (x0 >= x1 || y0 >= y1) return;
    int x = (x0+x1-1)/2, y = (y0+y1-1)/2, span = right-left;
    for (int lane = 0; lane < n; ++lane) {
        int tile = lane < n/2 ? lane-n/2 : lane-n/2+1;
        int col = tile*span + (tile & 1 ? right-1-(x-left) : x);
        if (col >= 0 && col < out_w && (col < left || col >= right))
            colors[lane] = resolved((size_t)y*out_w+col);
    }
}
/* Keep the cold Extended walk out of the ordinary store's register allocation. */
static void __attribute__((noinline)) stamp_sides(uint32_t address, uint8_t index, int xor, int y0, int y1)
{
    unsigned x = cells[address]&1023u;
    int painted = 0;
    for (int r = fold_head[x]; r >= 0; r = fold_runs[r].next) {
        struct fold_run run = fold_runs[r];
        uint8_t color = side_override ? side_override[run.lane] : index;
        /* A zero eraser is idempotent and always carries through, even where a
         * real coloured head has already established its own paint domain. */
        unsigned operation = xor ? REACH_XOR : color ? REACH_PAINT : 0;
        int wrote = 0;
        for (int y = y0; y < y1; ++y) {
            size_t i = (size_t)y*out_w;
            size_t base = (size_t)y*side_stride+
                (run.first < left ? run.first : left+run.first-right);
            for (int col = run.first; col < run.end; ++col) {
                size_t a = i+col;
                if (!literal_clear && (side_written[a]&operation)) continue;
                wrote = 1;
                aperture[a] = xor ? resolved(a)^color : color;
                side_written[a] = literal_clear ? SIDE_WRITTEN : side_written[a] | SIDE_WRITTEN;
                if (!xor) side_base[base+col-run.first] = aperture[a];
                if (coverage) coverage[a] = 255;
            }
        }
        if (wrote) {
            if (dz_canvas_store_audit) ++dz_canvas_store_audit->fold;
            dirty(run.first,y0,run.end,y1); painted = 1;
        }
    }
    if (painted) side_ink = 1;
}
void dz_canvas_sides_only(int enabled) { sides_only = enabled; }
void dz_canvas_suppress(int enabled)
{
    suppress = enabled;
    if (enabled && dz_canvas_store_audit) ++dz_canvas_store_audit->native;
}
void dz_canvas_raster_xor(int enabled) { dz_video.ega_xor = enabled != 0; }
static void __attribute__((noinline)) stamp_plain(uint32_t address, uint8_t index, int xor)
{
    int x0, y0, x1, y1;
    bounds(address, &x0, &y0, &x1, &y1);
    for (int y = y0; y < y1; ++y) {
        size_t i = (size_t)y * out_w + x0, n = (size_t)(x1 - x0);
        if (xor) for (size_t x = 0; x < n; ++x) aperture[i + x] = resolved(i + x) ^ index;
        else memset(aperture + i, index, n);
        if (coverage) memset(coverage + i, 255, n);
    }
    dirty(x0,y0,x1,y1);
}
static void __attribute__((noinline)) stamp_audited(uint32_t address, uint8_t index, int xor)
{
    stamp_plain(address,index,xor);
    int x0,y0,x1,y1;
    bounds(address,&x0,&y0,&x1,&y1);
    if (x0 >= x1 || y0 >= y1) return;
    if (native_finish) ++dz_canvas_store_audit->finish;
    else if (literal_clear || mode_import) ++dz_canvas_store_audit->literal;
    else ++dz_canvas_store_audit->faithful;
    if (dz_canvas_store_audit->written) dz_canvas_store_audit->written[address] = 1;
    if (fold_fade) stamp_sides(address,index,xor,y0,y1);
}
static void stamp(uint32_t address, uint8_t index, int xor)
{
    if (dz_canvas_store_audit) stamp_audited(address,index,xor);
    else stamp_plain(address,index,xor);
}
void dz_canvas_write(uint32_t address, uint8_t index)
{
    if (address >= DZ_VRAM_PIXELS) return;
    int changed = dz_video.vram[address] != index;
    dz_video.vram[address] = index;
    if (suppress && fade_erased && address < capacity) {
        fade_erased[address] = index == 0 && !(dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor) ? 1 : 2;
        if (fade_erased[address]==1) fade_zero=1;
        if (dz_canvas_store_audit && dz_canvas_store_audit->written) dz_canvas_store_audit->written[address] = 1;
    }
    if (active() && !restart && !suppress && address < capacity && (changed || native_ink || override_index != -1)) {
        stamp(address, override_index < 0 ? index : (uint8_t)override_index, 0);
    }
}
void dz_canvas_store(uint32_t address, uint8_t index, int ega)
{
    if (ega && dz_video.ega_xor && active() && !restart && !suppress && address < capacity) {
        uint8_t ink = override_index < 0 ? index & 15u : (uint8_t)override_index & 15u;
        stamp(address, ink, 1);
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
/* Extended sides continue the page's background word. Its two bytes alternate
 * every pixels_per_byte logical columns, which makes the stripes. */
static void background_sides(int first_row, int end_row, uint16_t pattern)
{
    if (presentation != DZ_PRESENT_EXTENDED || first_row >= end_row) return;
    SDL_FRect r = dz_present_viewport(out_w,out_h,presentation);
    uint8_t *row = aperture+(size_t)first_row*out_w;
    int ppb = dz_video.pixels_per_byte;
    uint8_t mask = dz_video.mode == DZ_MODE_EGA_350 ? 15u : 255u;
    for (int x = 0; x < out_w; ++x) {
        if (x == left) x = right;
        if (x == out_w) break;
        int cell = (int)floor((x+0.5-(double)r.x)*logical_w/r.w);
        int byte = cell >= 0 ? cell/ppb : -((-cell+ppb-1)/ppb);
        row[x] = (uint8_t)(byte & 1 ? pattern >> 8 : pattern) & mask;
    }
    for (int y = first_row+1; y < end_row; ++y) {
        uint8_t *to = aperture+(size_t)y*out_w;
        memcpy(to,row,(size_t)left);
        memcpy(to+right,row+right,(size_t)(out_w-right));
    }
    for (int y = first_row; y < end_row; ++y) {
        const uint8_t *from = aperture+(size_t)y*out_w;
        memset(side_written+(size_t)y*out_w,0,(size_t)out_w);
        memcpy(side_base+(size_t)y*side_stride,from,(size_t)left);
        memcpy(side_base+(size_t)y*side_stride+left,from+right,(size_t)(out_w-right));
    }
    side_ink = pattern != 0;
}
void dz_canvas_clear(uint8_t index)
{
    memset(dz_video.vram, index, DZ_VRAM_PIXELS);
    if (active()) {
        memset(aperture, index, (size_t)out_w * raster_h);
        if (coverage) memset(coverage, 255, (size_t)out_w * raster_h);
        if (side_written) memset(side_written,0,(size_t)out_w*raster_h);
        if (side_base) memset(side_base,index,(size_t)side_stride*raster_h);
        native_ink = side_ink = 0; dirty(0,0,out_w,raster_h);
    }
}
void dz_canvas_clear_page(uint16_t paragraph, uint16_t words, uint16_t pattern)
{
    literal_clear = 1;
    if (!active() || (!native_ink && !side_ink && presentation != DZ_PRESENT_EXTENDED)) return;
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
    background_sides((int)p*out_h,(int)(p+n)*out_h,pattern);
    dirty(0,(int)p*out_h,out_w,(int)(p+n)*out_h);
}
void dz_canvas_clear_page_end(void) { literal_clear = 0; }
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
        dirty(x0,y0,x1,y1);
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
            free(copy); dirty(tx,ty,tx1,ty1);
        }
    }
    for (int j = 0; j < height; ++j) {
        int row = dy > sy ? height - 1 - j : j;
        memmove(dz_video.vram + (dy + row) * dz_video.width + dx,
            dz_video.vram + (sy + row) * dz_video.width + sx, (size_t)width);
    }
}
static void select_origin(void)
{
    double row = (double)page_paragraph*16*dz_video.pixels_per_byte/logical_w;
    page_origin = (int)(llround(row*out_h/logical_h) % ring_h);
}
void dz_canvas_select_page(uint16_t paragraph)
{
    page_paragraph = paragraph;
    if (active()) select_origin();
}
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
    dz_native_plot = NULL;
    free(fade_erased); fade_erased=NULL;
    free(fade_before); fade_before=NULL;
    dz_native_fade_end();
    free(aperture); free(coverage); free(under); free(side_written); free(side_base); free(cells); free(rows);
    free(dirty_tiles); free(dirty_rects); dirty_tiles = dirty_rects = NULL;
    free(fold_runs); fold_runs = NULL; literal_clear = 0;
    fold_fade = fold_count = fold_lanes = 0; side_override = NULL;
    select_fade_hook();
    aperture = coverage = under = side_written = side_base = NULL; cells = NULL; rows = NULL;
    view = (struct dz_canvas_view){0}; allocated = 0; mapping_renderer = NULL;
    restart = native_ink = side_ink = suppress = pending_dirty = 0; override_index = -1;
    mapping_hw = mapping_hh = 0;
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
    side_stride = left+w-right;
    ring_h = (int)lround((double)capacity/logical_w*h/logical_h);
    if (ring_h < h) ring_h = h;
    select_origin();
    size_t n = (size_t)w*raster_h;
    unsigned row_count = (unsigned)pages*logical_h;
    uint32_t cell_count = row_count*(unsigned)logical_w;
    tile_columns = (w+255)>>8;
    tile_count = tile_columns*((raster_h+255)>>8);
    int aa = dz_display.native_aa;
    size_t side_bytes = (size_t)side_stride*raster_h;
    allocated = n * (aa ? 3u : 1u) + (size_t)cell_count*sizeof *cells + 2u*row_count*sizeof *rows +
        2u*(size_t)tile_count*sizeof *dirty_tiles;
    if (policy == DZ_PRESENT_EXTENDED) allocated += n+side_bytes;
    if (allocated > 268435456u) return SDL_SetError("Canvas storage exceeds 256 MiB"), -1;
    aperture = calloc(n, 1); cells = malloc((size_t)cell_count*sizeof *cells);
    if (policy == DZ_PRESENT_EXTENDED) {
        side_written = calloc(n,1); side_base = calloc(side_bytes ? side_bytes : 1,1);
    }
    if (policy == DZ_PRESENT_EXTENDED) {
        fold_runs = malloc((size_t)w*sizeof *fold_runs);
        allocated += (size_t)w*sizeof *fold_runs;
    }
    rows = malloc(2u*row_count*sizeof *rows);
    dirty_tiles = calloc((size_t)tile_count,sizeof *dirty_tiles);
    dirty_rects = malloc((size_t)tile_count*sizeof *dirty_rects);
    if (aa) {
        coverage = malloc(n); under = calloc(n, 1);
        if (coverage) memset(coverage, 255, n);
    }
    if (!aperture || !cells || !rows || !dirty_tiles || !dirty_rects ||
        (policy == DZ_PRESENT_EXTENDED && (!side_written || !side_base || !fold_runs)) ||
        allocated > 268435456u || (aa && (!coverage || !under))) {
        dz_canvas_shutdown(); return SDL_OutOfMemory(), -1;
    }
    presentation = policy; mapping_renderer = renderer;
    for (int x = 0; x < logical_w; ++x) fold_head[x] = -1;
    if (fold_runs && right > left) {
        int span = right-left, tiles = (w+span-1)/span+1;
        fold_lanes = tiles*2;
        for (int col = 0; col < w;) {
            if (col >= left && col < right) { col = right; continue; }
            int tile = (int)floor((double)(col-left)/span);
            int f = col-left-tile*span;
            if (tile & 1) f = span-1-f;
            int x = 0;
            while (x+1 < logical_w && xe[x+1] <= left+f) ++x;
            int end = col+1;
            int limit = col < left ? left : w;
            while (end < limit) {
                int t = (int)floor((double)(end-left)/span), q = end-left-t*span;
                if (t & 1) q = span-1-q;
                if (t != tile || left+q < xe[x] || left+q >= xe[x+1]) break;
                ++end;
            }
            int lane = tile < 0 ? tile+tiles : tile+tiles-1;
            fold_runs[fold_count] = (struct fold_run){col,end,lane,fold_head[x]};
            fold_head[x] = fold_count++;
            col = end;
        }
    }
    gpu_transfer = !strcmp(SDL_GetRendererName(renderer),"gpu");
    for (unsigned row = 0; row < row_count; ++row) {
        unsigned page = row/(unsigned)logical_h, y = row%(unsigned)logical_h;
        rows[2*row] = (int)page*h+ye[y]; rows[2*row+1] = (int)page*h+ye[y+1];
    }
    for (uint32_t a = 0; a < cell_count; ++a) cells[a] = ((a/(unsigned)logical_w)<<10) | (a%(unsigned)logical_w);
    view = (struct dz_canvas_view){.indices=aperture, .width=w, .height=raster_h,
        .revision=++serial, .coverage=coverage, .under=under, .dirty=dirty_rects};
    if (resized) dz_canvas_clear(0);
    else {
        mode_import=1;
        for (uint32_t a = 0; a < capacity; ++a) stamp(a, dz_video.vram[a], 0);
        mode_import=0;
    }
    dirty(0,0,out_w,raster_h);
    restart = pending;
    fold_fade = fade_active && policy == DZ_PRESENT_EXTENDED;
    select_fade_hook();
    fade_snapshot();
    return 1;
}
struct dz_canvas_view dz_canvas_surface(void)
{
    if (!active()) return (struct dz_canvas_view){0};
    if (pending_dirty) {
        view.dirty_count = 0;
        for (int i = 0; i < tile_count; ++i) {
            SDL_Rect r = dirty_tiles[i];
            if (!r.w) continue;
            /* Coalesce intersecting regions once per frame, including broad
             * clears/copies. Individual plots only union into one spatial cell. */
            for (int j = 0; j < view.dirty_count;) {
                SDL_Rect *a = &dirty_rects[j];
                if (r.x < a->x+a->w && a->x < r.x+r.w && r.y < a->y+a->h && a->y < r.y+r.h) {
                    SDL_GetRectUnion(&r,a,&r);
                    dirty_rects[j] = dirty_rects[--view.dirty_count];
                    j = 0;
                } else ++j;
            }
            dirty_rects[view.dirty_count++] = r;
        }
        /* Bound transfer setup costs: dense GPU writes use one bounding box;
         * D3D11 prefers a stable page shape. Keep small sparse regions exact. */
        uint64_t area = 0;
        int y0 = raster_h, y1 = 0;
        for (int i = 0; i < view.dirty_count; ++i) {
            const SDL_Rect *r = dirty_rects+i;
            area += (uint64_t)r->w*r->h;
            if (r->y < y0) y0 = r->y;
            if (r->y+r->h > y1) y1 = r->y+r->h;
        }
        if (view.dirty_count && y0/out_h == (y1-1)/out_h &&
            (area > (uint64_t)out_w*out_h/8u || (gpu_transfer && view.dirty_count > 2))) {
            if (gpu_transfer) {
                SDL_Rect r = dirty_rects[0];
                for (int i = 1; i < view.dirty_count; ++i) SDL_GetRectUnion(&r,dirty_rects+i,&r);
                dirty_rects[0] = r;
            } else dirty_rects[0] = (SDL_Rect){0,y0/out_h*out_h,out_w,out_h};
            view.dirty_count = 1;
        }
        view.revision = ++serial; pending_dirty = 0;
    }
    return view;
}
uint32_t dz_canvas_fold_address(int x, int y)
{
    if (!active() || right <= left || bottom <= top) return UINT32_MAX;
    int span = right-left, f = (x-left)%(2*span);
    if (f < 0) f += 2*span;
    if (f >= span) f = 2*span-1-f;
    int col = 0, end = logical_w;
    while (col+1 < end) {
        int mid = (col+end)/2;
        if (xe[mid] <= left+f) col = mid; else end = mid;
    }
    int start = (int)(llround((double)dz_video.display_start_row*out_h/logical_h)%ring_h);
    int split = dz_video.line_compare >= (unsigned)logical_h ? out_h :
        (int)llround((double)(dz_video.line_compare+1)*out_h/logical_h);
    int row = (y < split ? start+y : y-split)%ring_h;
    int page = row/out_h, local = row%out_h, sy = 0;
    end = logical_h;
    while (sy+1 < end) {
        int mid = (sy+end)/2;
        if (ye[mid] <= local) sy = mid; else end = mid;
    }
    uint32_t address = (unsigned)(page*logical_h+sy)*logical_w+col;
    return address < capacity ? address : UINT32_MAX;
}
void dz_canvas_uploaded(uint64_t revision)
{
    if (pending_dirty || revision != view.revision || !view.dirty_count) return;
    memset(dirty_tiles,0,(size_t)tile_count*sizeof *dirty_tiles);
    view.dirty_count = 0; view.dirty_base = revision;
}
int dz_canvas_quads(struct dz_canvas_quad quads[3])
{
    if (!active()) return 0;
    int start = (int)(llround((double)dz_video.display_start_row*out_h/logical_h) % ring_h);
    int split = dz_video.line_compare >= (unsigned)logical_h ? out_h :
        (int)llround((double)(dz_video.line_compare+1)*out_h/logical_h);
    int count = 0;
    for (int y = 0; y < out_h;) {
        int source = y < split ? (start+y)%ring_h : y-split;
        int n = y < split ? split-y : out_h-y;
        if (n > ring_h-source) n = ring_h-source;
        int full = presentation == DZ_PRESENT_EXTENDED;
        int x0 = full ? 0 : left, x1 = full ? out_w : right;
        int y0 = full || y > top ? y : top;
        int y1 = full || y+n < bottom ? y+n : bottom;
        if (x0 < x1 && y0 < y1) {
            quads[count++] = (struct dz_canvas_quad){
                {(float)x0,(float)(source+y0-y),(float)(x1-x0),(float)(y1-y0)},
                {(float)x0,(float)y0,(float)(x1-x0),(float)(y1-y0)}};
        }
        y += n;
    }
    return count;
}
int dz_canvas_has_native(void) { return active() && native_ink; }
SDL_FRect dz_canvas_content(void) { return (SDL_FRect){(float)left,(float)top,(float)(right-left),(float)(bottom-top)}; }
void dz_canvas_native_size(int *w, int *h)
{ *w = active() ? out_w : 0; *h = active() ? out_h : 0; }
struct dz_native_mapping dz_canvas_native_mapping(int hw, int hh)
{
    if (!active() || hw <= 0 || hh <= 0) return (struct dz_native_mapping){0};
    if (mapping_hw == hw && mapping_hh == hh) return native_mapping;
    SDL_FRect r = dz_present_viewport(out_w,out_h,presentation);
    mapping_hw = hw; mapping_hh = hh;
    native_mapping = (struct dz_native_mapping){
        r.x+r.w*0.5+0.5*r.w/logical_w, r.y+r.h*0.5+0.5*r.h/logical_h,
        (double)r.w/(2*hw), (double)r.h/(2*hh),
        fmax(1,(double)r.w/logical_w), fmax(1,(double)r.h/logical_h)};
    return native_mapping;
}
/* A capsule is a rectangle plus its two circular caps. Intersect it with
 * pixel-centre scanlines; opaque interiors become short memset spans. */
struct capsule { double ax,ay,bx,by,r,px[4],py[4]; };
static struct capsule capsule(double ax, double ay, double bx, double by, double r)
{
    double dx = bx-ax, dy = by-ay, length = hypot(dx,dy);
    double nx = length ? -dy*r/length : 0, ny = length ? dx*r/length : 0;
    return (struct capsule){ax,ay,bx,by,r,{ax+nx,bx+nx,bx-nx,ax-nx},{ay+ny,by+ny,by-ny,ay-ny}};
}
static int capsule_span(const struct capsule *c, double y, int *first, int *last, int ties)
{
    double lo = HUGE_VAL, hi = -HUGE_VAL, condition = 1;
    const double cy[] = {c->ay,c->by}, cx[] = {c->ax,c->bx};
    for (int i = 0; i < 2; ++i) {
        double dy = y-cy[i];
        if (fabs(dy) <= c->r) {
            double square = c->r*c->r-dy*dy;
            double x = sqrt(square > 0 ? square : 0);
            if (cx[i]-x < lo) lo = cx[i]-x;
            if (cx[i]+x > hi) hi = cx[i]+x;
        }
    }
    for (int i = 0; i < 4; ++i) {
        int j = (i+1)&3;
        double dy = c->py[j]-c->py[i];
        if (!dy) continue;
        double t = (y-c->py[i])/dy;
        if (t >= 0 && t <= 1) {
            if (ties) condition = fmax(condition,1+fabs((c->px[j]-c->px[i])/dy));
            double x = c->px[i]+t*(c->px[j]-c->px[i]);
            if (x < lo) lo = x;
            if (x > hi) hi = x;
        }
    }
    if (lo > hi) return 0;
    if (ties) {
        /* Span intersection arithmetic must not lose an exact half tie.
         * The directional tie rule below chooses its actual owner. */
        double error = 8*DBL_EPSILON*fmax(1,fmax(fabs(lo),fabs(hi)))*condition;
        lo -= error; hi += error;
    }
    if (lo > hi || hi < 0.5 || lo > out_w-0.5) return 0;
    /* Clipped nonnegative bounds: integer truncation supplies floor, with
     * one comparison for ceil. Same pixel centres, no libm calls per span. */
    double a = (lo > 0.5 ? lo : 0.5)-0.5;
    double b = (hi < out_w-0.5 ? hi : out_w-0.5)-0.5;
    *first = (int)a + (a > (int)a);
    *last = (int)b;
    return *first <= *last;
}
static int rejected_tie(double x, double y, double ax, double ay,
                        double dx, double dy, double length, double boundary)
{
    double t = (x-ax)*dx+(y-ay)*dy;
    if (!length || t < 0 || t > length) return 0;
    double cross = dx*(y-ay)-dy*(x-ax);
    double error = (fabs(x)+fabs(ax))*fabs(dy)+(fabs(y)+fabs(ay))*fabs(dx);
    if (fabs(fabs(cross)-boundary) > 8*DBL_EPSILON*fmax(1,error)) return 0;
    /* line_step's error >= 0 advances the minor coordinate on a half tie. */
    return fabs(dx) >= fabs(dy) ? cross*dx*dy < 0 : cross*dx*dy > 0;
}
static void stroke(double ax, double ay, double bx, double by,
                    double width, double height, uint8_t index, int clip, unsigned caps)
{
    if (!active() || restart || width <= 0 || height <= 0) return;
    if ((uint32_t)page_paragraph*16u >= DZ_VRAM_PLANE_BYTES) return;
    int base = page_origin;
    double ratio = height/width, radius = width*0.5, fringe = coverage ? 0.5 : 0;
    double low = (ay < by ? ay : by)-(radius+fringe)*ratio;
    double high = (ay > by ? ay : by)+(radius+fringe)*ratio;
    int y0 = low <= 0 ? 0 : low >= out_h ? out_h : (int)low;
    if (high > out_h-1) high = out_h-1;
    int y1 = (int)high;
    y1 += high > y1;
    if (clip && y0 < top) y0 = top;
    if (clip && y1 >= bottom) y1 = bottom-1;
    ay /= ratio; by /= ratio;
    struct capsule outer = capsule(ax,ay,bx,by,radius+fringe);
    struct capsule inner = coverage ? capsule(ax,ay,bx,by,fmax(0,radius-fringe)) : outer;
    double dx = bx-ax, dy = by-ay, length = dx*dx+dy*dy;
    double boundary = radius*sqrt(length);
    int xor = dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor;
    double low_x = (ax < bx ? ax : bx)-radius-fringe;
    double high_x = (ax > bx ? ax : bx)+radius+fringe;
    int first_x = low_x <= 0 ? 0 : low_x >= out_w ? out_w : (int)low_x;
    int end_x = high_x < 0 ? 0 : high_x >= out_w ? out_w : (int)high_x+1;
    if (y0 <= y1) {
        int row = base+y0, count = y1-y0+1;
        if (row >= ring_h) row -= ring_h;
        int first = count < ring_h-row ? count : ring_h-row;
        dirty(first_x,row,end_x,row+first);
        if (first < count) dirty(first_x,0,end_x,count-first);
    }
    for (int y = y0; y <= y1; ++y) {
        int x0,x1,ix0 = 1,ix1 = 0;
        double scan_y = (y+0.5)/ratio;
        if (!capsule_span(&outer,scan_y,&x0,&x1,caps & 4u)) continue;
        /* Internal colour boundaries are butt cuts through the same pen.
         * Round caps there would give the next sample the previous centre. */
        if (dx) {
            /* Neighbouring samples cut along one line from different anchors.
             * Rounding must not let a pixel centre on it fall to neither side. */
            double a = ceil(ax-dy*(scan_y-ay)/dx-0.5-1e-9);
            double b = ceil(bx-dy*(scan_y-by)/dx-0.5-1e-9);
            if (!(caps & 1u)) {
                if (dx > 0 && x0 < a) x0 = (int)a;
                if (dx < 0 && x1 >= a) x1 = (int)a-1;
            }
            if (!(caps & 2u)) {
                if (dx > 0 && x1 >= b) x1 = (int)b-1;
                if (dx < 0 && x0 < b) x0 = (int)b;
            }
        } else if (dy) {
            double t = (scan_y-ay)/dy;
            if ((!(caps & 1u) && t < 0) || (!(caps & 2u) && t >= 1)) continue;
        }
        if (x0 > x1) continue;
        if (caps & 4u) {
            while (x0 <= x1 && rejected_tie(x0+0.5,scan_y,ax,ay,dx,dy,length,boundary)) ++x0;
            while (x0 <= x1 && rejected_tie(x1+0.5,scan_y,ax,ay,dx,dy,length,boundary)) --x1;
            if (x0 > x1) continue;
        }
        if (clip && presentation != DZ_PRESENT_EXTENDED) {
            if (x0 < left) x0 = left;
            if (x1 >= right) x1 = right-1;
            if (x0 > x1) continue;
        }
        if (sides_only) {
            if (x0 >= left && x1 < right) continue;
            if (x0 < left && x1 >= left) x1 = left-1;
            else if (x0 >= left && x0 < right) x0 = right;
        }
        int row = base+y;
        if (row >= ring_h) row -= ring_h;
        size_t offset = (size_t)row*out_w;
        if (!coverage) {
            if (xor) for (int x = x0; x <= x1; ++x) aperture[offset+x] ^= index & 15u;
            else memset(aperture+offset+x0,index,(size_t)(x1-x0+1));
            continue;
        }
        if (!xor && capsule_span(&inner,scan_y,&ix0,&ix1,caps & 4u)) {
            if (ix0 < x0) ix0 = x0;
            if (ix1 > x1) ix1 = x1;
            if (ix0 > ix1) { ix0 = 1; ix1 = 0; }
        }
        if (!xor && ix0 <= ix1) {
            memset(aperture+offset+ix0,index,(size_t)(ix1-ix0+1));
            memset(coverage+offset+ix0,255,(size_t)(ix1-ix0+1));
        }
        for (int x = x0; x <= x1; ++x) {
            if (!xor && x >= ix0 && x <= ix1) continue;
            double t = length ? ((x+0.5-ax)*dx+(scan_y-ay)*dy)/length : 0;
            t = fmax(0,fmin(1,t));
            double ex = x+0.5-ax-t*dx, ey = scan_y-ay-t*dy;
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
    native_ink = 1;
}
void dz_canvas_stroke(double ax, double ay, double bx, double by, double width, uint8_t index)
{ stroke(ax,ay,bx,by,width,width,index,0,3); }
void dz_canvas_stroke_ellipse(double ax, double ay, double bx, double by,
                              double width, double height, uint8_t index)
{ stroke(ax,ay,bx,by,width,height,index,1,3); }
void dz_canvas_stroke_cell(double ax, double ay, double bx, double by,
                           double width, double height, uint8_t index, unsigned caps)
{ stroke(ax,ay,bx,by,width,height,index,1,caps|4u); }
/* Opaque fill spans; callers own analytic boundaries and logical colour bands. */
void dz_canvas_native_span(int y, int first, int end, uint8_t index)
{
    if (!active() || restart || y < 0 || y >= out_h ||
        (uint32_t)page_paragraph*16u >= DZ_VRAM_PLANE_BYTES) return;
    if (first < 0) first = 0;
    if (end > out_w) end = out_w;
    if (y < top || y >= bottom) return;
    if (presentation != DZ_PRESENT_EXTENDED) {
        if (first < left) first = left;
        if (end > right) end = right;
    }
    if (sides_only) {
        if (first >= left && end <= right) return;
        if (first < left && end > left) end = left;
        else if (first >= left && first < right) first = right;
    }
    if (first >= end) return;
    int row = page_origin+y;
    if (row >= ring_h) row -= ring_h;
    size_t offset = (size_t)row*out_w+first, count = (size_t)(end-first);
    memset(aperture+offset,index,count);
    if (coverage) memset(coverage+offset,255,count);
    dirty(first,row,end,row+1); native_ink = 1;
}
void dz_canvas_native_quad(const double x[4], const double y[4], uint8_t index)
{
    double low = y[0], high = y[0];
    for (int i = 1; i < 4; ++i) { low = fmin(low,y[i]); high = fmax(high,y[i]); }
    int first = (int)fmax(0,ceil(low-0.5)), end = (int)fmin(out_h,ceil(high-0.5));
    for (int row = first; row < end; ++row) {
        double py = row+0.5, left = HUGE_VAL, right = -HUGE_VAL;
        for (int i = 0; i < 4; ++i) {
            int j = (i+1)%4;
            if (py < fmin(y[i],y[j]) || py >= fmax(y[i],y[j])) continue;
            double px = x[i]+(x[j]-x[i])*(py-y[i])/(y[j]-y[i]);
            left = fmin(left,px); right = fmax(right,px);
        }
        if (left <= right) dz_canvas_native_span(row,(int)ceil(left-0.5),(int)ceil(right-0.5),index);
    }
}
static void fade_span(int y, int first, int end, uint8_t index, int real)
{
    if (first < 0) first = 0;
    if (end > out_w) end = out_w;
    if (first >= end) return;
    int row = (page_origin+y)%ring_h;
    if (plasma_retain && fade_before) {
        /* This is the generator's assignment predicate on the actual source,
         * not a logical occupancy mask. Previously assigned fine ink stays
         * untouched, including modular-field wraps and thin Native curves. */
        unsigned a=background_fill_word&255u,b=background_fill_word>>8;
        int run=first;
        while (run<end) {
            while (run<end && fade_before[(size_t)row*out_w+run]!=a &&
                   fade_before[(size_t)row*out_w+run]!=b) ++run;
            int last=run;
            while (last<end && (fade_before[(size_t)row*out_w+last]==a ||
                   fade_before[(size_t)row*out_w+last]==b)) ++last;
            if (last>run) {
                plasma_retain=0;fade_span(y,run,last,index,real);plasma_retain=1;
            }
            run=last;
        }
        return;
    }
    int xor = dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor;
    if (dz_video.mode == DZ_MODE_EGA_350) index &= 15u;
    if (!fold_fade || (first>=left && end<=right)) {
        size_t at=(size_t)row*out_w+first, count=(size_t)(end-first);
        if (xor) for (size_t x=0;x<count;++x) aperture[at+x]=resolved(at+x)^index;
        else memset(aperture+at,index,count);
        if (coverage) memset(coverage+at,255,count);
        dirty(first,row,end,row+1); native_ink=1;
        return;
    }
    unsigned operation = xor ? REACH_XOR : index ? REACH_PAINT : 0;
    for (int x=first;x<end;++x) {
        size_t at=(size_t)row*out_w+x;
        int side=x<left || x>=right;
        if (fold_fade && side) {
            size_t base=(size_t)row*side_stride+(x<left ? x : left+x-right);
            if (real) {
                if (xor && !(side_written[at]&REACH_XOR)) aperture[at]=side_base[base];
                side_written[at] |= SIDE_WRITTEN | (xor ? REACH_XOR : REACH_PAINT);
            } else {
                if (side_written[at]&operation) continue;
                side_written[at] |= SIDE_WRITTEN;
            }
            aperture[at]=xor ? resolved(at)^index : index;
            if (!xor) side_base[base]=index;
        } else aperture[at]=xor ? resolved(at)^index : index;
        if (coverage) coverage[at]=255;
    }
    dirty(first,row,end,row+1);
    native_ink=1;
}
static void mapped_fade_span(int y,int first,int end,uint8_t index,int sampling)
{
    if (!active() || restart || y<top || y>=bottom ||
        (uint32_t)page_paragraph*16u>=DZ_VRAM_PLANE_BYTES) return;
    int a=first>left ? first : left, b=end<right ? end : right;
    if (sampling && override_index>=0) index=(uint8_t)override_index;
    if (!fold_fade) { fade_span(y,a,b,index,1); return; }
    /* Real geometry precedes fallback, as side_pass precedes the centre store.
     * Reflect the interval itself, never a list of its logical samples. */
    if (sampling && side_override) {
        int span=right-left;
        for (int tile=-fold_lanes/2;tile<=fold_lanes/2;++tile) {
            int x0=left+tile*span,x1=x0+span;
            if (x0<first) x0=first;
            if (x1>end) x1=end;
            int lane=tile<0 ? tile+fold_lanes/2 : tile+fold_lanes/2-1;
            if (x0<x1) fade_span(y,x0,x1,tile ? side_override[lane] : index,1);
        }
    } else fade_span(y,first,end,index,1);
    if (a>=b) return;
    int span=right-left;
    for (int tile=-fold_lanes/2;tile<=fold_lanes/2;++tile) {
        if (!tile) continue;
        int x0=left+tile*span+(tile&1 ? right-b : a-left);
        int x1=left+tile*span+(tile&1 ? right-a : b-left);
        int lane=tile<0 ? tile+fold_lanes/2 : tile+fold_lanes/2-1;
        fade_span(y,x0,x1,sampling && side_override ? side_override[lane] : index,0);
    }
}
void dz_canvas_fade_span(int y,int first,int end,uint8_t index)
{ mapped_fade_span(y,first,end,index,0); }
void dz_canvas_fade_sample_span(int y,int first,int end,uint8_t index)
{ mapped_fade_span(y,first,end,index,1); }
void dz_canvas_fade_plasma_span(int y,int first,int end,uint8_t index)
{ plasma_retain=1;mapped_fade_span(y,first,end,index,0);plasma_retain=0; }
void dz_canvas_fade_source(void)
{
    if (!active() || !fade_before) return;
    size_t n=(size_t)out_w*raster_h;
    if (!coverage) memcpy(fade_before,aperture,n);
    else for (size_t i=0;i<n;++i) fade_before[i]=resolved(i);
}
static int fade_rectangle(int x,int y,int width,int height,SDL_Rect *rect)
{
    if (!active() || y<0 || height<=0 || y+height>logical_h || width<=0) return 0;
    int first,end;
    if (x>=0 && x+width<=logical_w) { first=xe[x];end=xe[x+width]; }
    else {
        SDL_FRect r=dz_present_viewport(out_w,out_h,presentation);
        first=(int)ceil(r.x+(double)x*r.w/logical_w-0.5);
        end=(int)ceil(r.x+(double)(x+width)*r.w/logical_w-0.5);
    }
    if (first<0) first=0;
    if (end>out_w) end=out_w;
    *rect=(SDL_Rect){first,ye[y],end-first,ye[y+height]-ye[y]};
    return rect->w>0 && rect->h>0;
}
int dz_canvas_fade_cell_rect(int x,int y,SDL_Rect *rect)
{ return fade_rectangle(x,y,1,1,rect); }
void dz_canvas_fade_cell_owner(int x,int y)
{
    SDL_Rect r;
    if (!fold_fade || (x>=0 && x<logical_w) || !dz_canvas_fade_cell_rect(x,y,&r)) return;
    for (int py=r.y;py<r.y+r.h;++py) {
        size_t at=(size_t)((page_origin+py)%ring_h)*out_w;
        for (int px=r.x;px<r.x+r.w;++px) if (px<left || px>=right) side_written[at+px]|=REACH_CELL;
    }
}
void dz_canvas_fade_refold(void)
{
    if (!fold_fade || !active()) return;
    int span=right-left;
    for (int y=top;y<bottom;++y) {
        int row=(page_origin+y)%ring_h;size_t at=(size_t)row*out_w;
        for (int x=0;x<out_w;++x) {
            if (x==left) x=right;
            if (x>=out_w) break;
            if (side_written[at+x]&REACH_CELL) continue;
            int tile=(int)floor((double)(x-left)/span),q=x-left-tile*span;
            if (tile&1) q=span-1-q;
            uint8_t c=resolved(at+left+q);aperture[at+x]=c;
            if (coverage) coverage[at+x]=255;
            side_base[(size_t)row*side_stride+(x<left ? x : left+x-right)]=c;
        }
        dirty(0,row,left,row+1);dirty(right,row,out_w,row+1);
    }
}
struct fade_exchange {
    SDL_Rect r[2];uint8_t *pixels[2];size_t size[2];
    unsigned flip,step;int count,width,height,alias;
};
static struct fade_exchange exchange;
void dz_canvas_fade_exchange_end(void)
{
    free(exchange.pixels[0]);free(exchange.pixels[1]);
    memset(&exchange,0,sizeof exchange);
}
/* Quilt tiles from one subdivision grid are disjoint or identical. Retain
 * both physical source patches for this exchange, then reveal their detail in
 * the original store order. Identical reversed tiles are swapped twice; their
 * second visits restore the original orientation, including intermediate frames. */
int dz_canvas_fade_exchange_begin(int ax,int ay,int bx,int by,int width,int height,unsigned flip)
{
    dz_canvas_fade_exchange_end();
    if (!active() || restart || width<=0 || height<=0) return 0;
    int enhanced=video_backend==BACKEND_EGA_640X700;
    int x[]={screen_half_width+ax,screen_half_width+bx};
    int y[]={screen_half_height+ay,screen_half_height+by};
    if (enhanced) { y[0]/=2;y[1]/=2;height/=2; }
    if (height<=0) return 0;
    for (int i=0;i<2;++i) {
        SDL_Rect *r=&exchange.r[i];
        /* A tile can have visible pixels even when either endpoint cell is
         * subpixel in a preview. Map its full interval, not those two cells. */
        if (!fade_rectangle(x[i],y[i],width,height,r)) return 0;
    }
    exchange.alias=x[0]==x[1] && y[0]==y[1];
    exchange.flip=flip;exchange.width=width;exchange.height=height;
    exchange.count=fold_fade ? fold_lanes+1 : 1;
    int span=right-left;
    for (int i=0;i<2;++i) {
        SDL_Rect r=exchange.r[i];
        exchange.size[i]=(size_t)r.w*r.h;
        exchange.pixels[i]=malloc(exchange.size[i]*exchange.count);
        if (!exchange.pixels[i]) { dz_canvas_fade_exchange_end();return 0; }
        for (int lane=0;lane<exchange.count;++lane) {
            int tile=lane-exchange.count/2;
            for (int py=0;py<r.h;++py) for (int px=0;px<r.w;++px) {
                int main_x=r.x+px;
                int col=tile&1 ? 2*left+(tile+1)*span-1-main_x : main_x+tile*span;
                if (col<0 || col>=out_w) col=main_x;
                exchange.pixels[i][lane*exchange.size[i]+(size_t)py*r.w+px]=
                    resolved((size_t)((page_origin+r.y+py)%ring_h)*out_w+col);
            }
        }
    }
    return 1;
}
void dz_canvas_fade_exchange_plot(int x,int y)
{
    if (!exchange.pixels[0] || !exchange.pixels[1] || !active() || restart) return;
    unsigned pair=exchange.step/2,flip=exchange.flip;
    int dest=(int)(exchange.step++&1u),src=1-dest;
    int ix=(int)pair%exchange.width,iy=(int)pair/exchange.width;
    int partner=(flip&2 ? exchange.width-1-ix : ix)+
        (flip&1 ? exchange.height-1-iy : iy)*exchange.width;
    if (exchange.alias && pair>=(unsigned)partner) { src=dest;flip=0; }
    SDL_Rect cell;
    y+=screen_half_height;
    if (video_backend==BACKEND_EGA_640X700) y/=2;
    if (!dz_canvas_fade_cell_rect(x+screen_half_width,y,&cell)) return;
    SDL_Rect a=exchange.r[dest],b=exchange.r[src];
    struct dz_native_mapping m=dz_canvas_native_mapping(screen_half_width,screen_half_height);
    int mirror_x=(int)llround(2*m.cx-1),mirror_y=(int)llround(2*m.cy-1),span=right-left;
    int xor=dz_video.mode==DZ_MODE_EGA_350 && dz_video.ega_xor;
    for (int branch=0;branch<4;++branch) for (int py=cell.y;py<cell.y+cell.h;++py) {
        int row_y=branch&1 ? mirror_y-py : py;
        if (row_y<top || row_y>=bottom) continue;
        int row=(page_origin+row_y)%ring_h;
        int sy=(int)((py+0.5-a.y)*b.h/a.h);
        if (flip&1) sy=b.h-1-sy;
        for (int lane=0;lane<exchange.count;++lane) {
            int tile=lane-exchange.count/2,low=out_w,high=0;
            for (int px=cell.x;px<cell.x+cell.w;++px) {
                int main_x=branch>=2 ? mirror_x-px : px;
                int col=tile&1 ? 2*left+(tile+1)*span-1-main_x : main_x+tile*span;
                if (col<0 || col>=out_w || (!fold_fade && (col<left || col>=right))) continue;
                if (tile && col>=left && col<right) continue;
                int sx=(int)((px+0.5-a.x)*b.w/a.w);
                if (flip&2) sx=b.w-1-sx;
                size_t at=(size_t)row*out_w+col;
                uint8_t color=exchange.pixels[src][lane*exchange.size[src]+(size_t)sy*b.w+sx];
                aperture[at]=xor ? resolved(at)^color : color;
                if (coverage) coverage[at]=255;
                if (col<low) low=col;
                if (col+1>high) high=col+1;
                if (fold_fade && (col<left || col>=right)) {
                    side_written[at]|=SIDE_WRITTEN;
                    side_base[(size_t)row*side_stride+(col<left ? col : left+col-right)]=aperture[at];
                }
            }
            if (low<high) dirty(low,row,high,row+1);
        }
    }
    native_ink=1;if (fold_fade) side_ink=1;
}
void dz_canvas_fade_begin(unsigned key)
{
    fade_key=key;
    /* A rebuild can leave its unplotted outer point pending in Semi-Auto. */
    dz_native_plot = NULL;
    dz_native_fade_begin(key);
    fade_zero=0;
    free(fade_erased); fade_erased=NULL;
    free(fade_before); fade_before=NULL;
    if (active() && dz_native_fade_enabled(key) && key!='j' && key!='p') {
        fade_erased=calloc(capacity,1);
        fade_before=malloc((size_t)out_w*raster_h);
        if (!fade_erased || !fade_before) {
            free(fade_erased); fade_erased=NULL;
            free(fade_before); fade_before=NULL;
            dz_native_fade_end();
        } else if (!coverage) memcpy(fade_before,aperture,(size_t)out_w*raster_h);
        else for (size_t i=0;i<(size_t)out_w*raster_h;++i) fade_before[i]=resolved(i);
    }
    fade_active = 1;
    fold_fade = active() && presentation == DZ_PRESENT_EXTENDED;
    select_fade_hook();
    /* A fade store has the same cell as an image store. The former full_fade
     * stretch moved even Original's centre pixels during v/w/z. */
    side_override = NULL;
    literal_clear = 0;
    fade_snapshot();
}
void dz_canvas_fade_finish(void)
{
    /* Continuous erasers remove their own pens. Finish the exact cells of every
     * last zero store too, clearing pre-existing Native overhang. No logical
     * store/charge, and untouched cells are deliberately retained. */
    if (fade_zero && fade_erased && active() && fade_key!='q' && fade_key!='g') {
        for (uint32_t a=0;a<capacity;++a) if (!fade_erased[a]) {
            int x0,y0,x1,y1; bounds(a,&x0,&y0,&x1,&y1);
            for (int y=y0;y<y1;++y) {
                size_t row=(size_t)y*out_w;
                int restored=0;
                for (int x=x0;x<x1;++x) if (!resolved(row+x) && fade_before[row+x]) {
                    aperture[row+x]=fade_before[row+x];
                    if (coverage) coverage[row+x]=255;
                    restored=1;
                }
                if (restored) dirty(x0,y,x1,y+1);
                if (fold_fade) for (int r=fold_head[cells[a]&1023u];r>=0;r=fold_runs[r].next) {
                    struct fold_run run=fold_runs[r]; restored=0;
                    for (int x=run.first;x<run.end;++x) if (!resolved(row+x) && fade_before[row+x]) {
                        aperture[row+x]=fade_before[row+x];
                        if (coverage) coverage[row+x]=255;
                        restored=1;
                    }
                    if (restored) dirty(run.first,y,run.end,y+1);
                }
            }
        }
        native_finish=1;
        for (uint32_t a=0;a<capacity;++a) if (fade_erased[a]==1) stamp(a,0,0);
        native_finish=0;
        fade_zero=0;
    }
}
void dz_canvas_fade_end(void)
{
    dz_canvas_fade_finish();
    free(fade_erased); fade_erased=NULL;
    free(fade_before); fade_before=NULL;
    dz_native_fade_end();
    override_index = -1;
    fade_active = fold_fade = 0; side_override = NULL;
    select_fade_hook();
}
int dz_canvas_take_restart(void) { int pending = restart; restart = 0; return pending; }
void dz_canvas_mode_changed(void)
{
    /* Fireworks resets the mode in the middle of its own transition. Rebuild
     * the map before its next store, including in headless oracle runs. */
    if (fade_active && active()) {
        fade_mode_renderer = mapping_renderer;
        fade_mode_w = out_w; fade_mode_h = out_h; fade_mode_policy = presentation;
        fade_mode_key=dz_native_fade_enabled(fade_key) ? fade_key : 0;
    }
    dz_canvas_shutdown();
}
void dz_canvas_mode_ready(void)
{
    SDL_Renderer *renderer = fade_mode_renderer;
    fade_mode_renderer = NULL;
    if (fade_active && renderer && dz_video.mode != DZ_MODE_TEXT) {
        dz_canvas_configure(renderer,fade_mode_w,fade_mode_h,fade_mode_policy);
        if (fade_mode_key) dz_canvas_fade_begin(fade_mode_key);
    }
    fade_mode_key=0;
}
size_t dz_canvas_bytes(void) { return allocated; }
