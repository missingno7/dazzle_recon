#include "extended.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Two sparse side apertures use the central stride/address wrap. Distance zero
 * is adjacent to the center. At 21:9 each side needs only 3/8 of the original width.
 * Dots keep the maximum domain; sine fits whole motifs to the current side width. */
static uint8_t *ink[2];
static unsigned image, ordinal, extent, half_height, halve_y, columns, capacity;
static enum dz_video_mode mode;
static int active, output_width, output_height;
static uint16_t background_pattern;
static unsigned sine_columns, sine_center;
static uint64_t revision;
struct side_state {
    uint32_t rng;
    uint64_t budget;
    double phase, mx, my, ax, ay, sx, sy;
    double gain_x, gain_y, bias_x, bias_y, scale;
};
static struct side_state side[2];

struct dz_extended_layout dz_extended_layout(int width, int height, int logical_width)
{
    struct dz_extended_layout l = {0};
    l.center = dz_present_viewport(width, height, DZ_PRESENT_ORIGINAL);
    if (logical_width <= 0 || l.center.w <= 0) return l;
    double extra = fmin((width - l.center.w) / 2.0, l.center.h / 2.0);
    if (extra <= 0) return l;
    l.columns = (float)(extra * logical_width / l.center.w);
    l.storage_columns = (int)ceil(l.columns);
    l.left = (SDL_FRect){l.center.x - (float)extra, l.center.y, (float)extra, l.center.h};
    l.right = (SDL_FRect){l.center.x + l.center.w, l.center.y, (float)extra, l.center.h};
    /* Form the outer endpoint once: repeated float additions can make 701 into
     * 700.99994, dropping the final column in SDL's software rasterizer. */
    l.right.w = ((float)width - l.left.x) - l.right.x;
    return l;
}

static uint32_t mix(uint32_t x)
{
    x ^= x >> 16; x *= UINT32_C(0x7feb352d);
    x ^= x >> 15; x *= UINT32_C(0x846ca68b);
    return x ^ (x >> 16);
}
static unsigned random_side(unsigned s, unsigned limit)
{
    side[s].rng += UINT32_C(0x9e3779b9);
    return (unsigned)(((uint64_t)mix(side[s].rng) * limit) >> 32);
}
static void reset_contexts(void)
{
    int w = output_width ? output_width : dz_display.window_width ? dz_display.window_width : 960;
    int h = output_height ? output_height : dz_display.window_height ? dz_display.window_height : 720;
    struct dz_extended_layout l = dz_extended_layout(w, h, dz_video.width);
    sine_columns = (unsigned)floor(l.columns);
    /* Below 15% of the original width the complete pixel motif is too small.
     * Floor the fractional outside cell so no ink can be cropped through it. */
    if (sine_columns < (unsigned)dz_video.width * 3u / 20u) sine_columns = 0;
    sine_center = sine_columns ? (sine_columns - 1u) / 2u : 0;
    memset(side, 0, sizeof side);
    for (unsigned s = 0; s < 2; ++s) {
        side[s].rng = mix(dz_options.seed ^ mix(ordinal) ^ mix(image) ^ mix(s + 1u));
        struct side_state *v = &side[s];
        v->ax = extent / 8 + random_side(s, extent / 8);
        v->ay = extent / 8 + random_side(s, extent / 8);
        v->sx = 1.0 / (1.0 + v->ax);
        v->sy = 1.0 / (1.0 + v->ay);
        v->phase = random_side(s, 62832) / 10000.0;
        v->gain_x = 1.0 / (2.0 * sin(v->sx / 2.0));
        v->gain_y = 1.0 / (2.0 * sin(v->sy / 2.0));
        v->bias_x = cos(v->sx / 2.0);
        v->bias_y = sin(v->sy / 2.0);
        /* Bounds of the accumulated sine/cosine modulation, for all time and
         * both swapped axes. One constant scale preserves the curve's proportions. */
        double bound = fmax(v->ax + v->gain_x * (1.0 + fabs(v->bias_x)),
                            v->ay + v->gain_y * (1.0 + fabs(v->bias_y)));
        int margin = dz_video.width / 80, parity = halve_y ? 2 : 0;
        double room = fmin((double)sine_center - margin - parity,
                          (double)half_height - 1 - margin * (halve_y ? 2 : 1) - parity);
        v->scale = fmax(0, fmin(1, room / bound));
    }
}
void dz_extended_retire(void)
{
    active = 0;
    for (unsigned s = 0; s < 2; ++s) if (ink[s]) memset(ink[s], 0, DZ_VRAM_PIXELS);
    ++revision;
}
void dz_extended_clear(uint16_t pattern)
{
    background_pattern = pattern;
    /* VGA mode setup first zeroes the aperture, then fills 64000 plane bytes
     * (two VGA400 pages / one VGA200 page). Preserve the unused tail as zero. */
    unsigned filled = mode == DZ_MODE_VGA_400 ? 256000u : mode == DZ_MODE_VGA_200 ? 64000u : capacity;
    for (unsigned s = 0; s < 2; ++s) if (ink[s])
        for (unsigned a = 0; a < capacity; ++a)
            /* Continue the clear's plane-byte lattice across each central edge. */
            ink[s][a] = a < filled ? (uint8_t)(pattern >>
                (8u * (((a / (unsigned)dz_video.pixels_per_byte) & 1u) ^ (s == 0)))) : 0;
    reset_contexts();
    if (image == DZ_EXT_SINE && !sine_columns) active = 0;
    ++revision;
}
void dz_extended_image(unsigned selected, int span, int hh, int hy, uint16_t background)
{
    int retain = dz_extended_active();
    active = 0; ++revision;
    ++ordinal;
    image = selected; extent = span > 0 ? (unsigned)span : 1;
    half_height = hh > 0 ? (unsigned)hh : 1; halve_y = hy != 0;
    mode = dz_video.mode;
    if (!dz_extended_enabled() || mode == DZ_MODE_TEXT ||
        (mode == DZ_MODE_EGA_350 && dz_video.ega_xor) ||
        (selected != DZ_EXT_DOTS && selected != DZ_EXT_SINE)) { dz_extended_retire(); return; }
    columns = (unsigned)dz_video.width * 3u / 8u;
    capacity = DZ_VRAM_PLANE_BYTES * (unsigned)dz_video.pixels_per_byte;
    for (unsigned s = 0; s < 2; ++s) {
        if (!ink[s]) ink[s] = calloc(DZ_VRAM_PIXELS, 1);
        if (!ink[s]) { dz_extended_retire(); return; }
    }
    if (retain) reset_contexts(); /* supported overlays retain page ink, like the center */
    else dz_extended_clear(background);
    if (image == DZ_EXT_SINE && !sine_columns) { dz_extended_retire(); return; }
    active = 1;
}
void dz_extended_fade(char key) { if (key != 'j') dz_extended_retire(); }
int dz_extended_active(void)
{
    return active && dz_extended_enabled() && mode == dz_video.mode &&
        !(mode == DZ_MODE_EGA_350 && dz_video.ega_xor);
}
uint64_t dz_extended_revision(void) { return revision; }
uint8_t dz_extended_index(unsigned s, unsigned row, unsigned distance)
{
    if (s > 1 || distance >= columns || !dz_extended_active()) return 0;
    return ink[s][((uint64_t)row * (unsigned)dz_video.width + distance) % capacity];
}
static void point(unsigned s, int x, int y, unsigned color, unsigned page)
{
    if (x < 0 || x >= (int)columns || y < 0 || y >= dz_video.height) return;
    uint64_t a = (uint64_t)page * 16u * (unsigned)dz_video.pixels_per_byte +
        (unsigned)y * (unsigned)dz_video.width + (unsigned)x;
    if (a < capacity) ink[s][a] = (uint8_t)(color & (mode == DZ_MODE_EGA_350 ? 15u : 255u));
}
static int word(unsigned x) { return (x & 65535u) < 32768u ? (int)(x & 65535u) : (int)(x & 65535u) - 65536; }
static int asr5(int x) { return x >= 0 ? x / 32 : -((-x + 31) / 32); }
static unsigned dot_color(unsigned s, int rule, unsigned color, int x, int y)
{
    unsigned mask = mode == DZ_MODE_EGA_350 ? 15u : 255u;
    int xx = word((unsigned)(x * x)), yy = word((unsigned)(y * y));
    switch (rule) {
    case 1: color = random_side(s, mask + 1); break;
    case 2: color = (unsigned)abs(x); break;
    case 3: color = (unsigned)abs(y); break;
    case 4: color = (unsigned)(abs(x) + abs(y)); break;
    case 5: color = ((uint32_t)xx + (uint32_t)yy) >> 5; break;
    case 6: color = ((uint32_t)xx - (uint32_t)yy) >> 5; break;
    case 7: color = (unsigned)(asr5(yy) - abs(x)); break;
    case 8: color = (unsigned)asr5(word((unsigned)abs(word((unsigned)(x * y))))); break;
    default: break; /* same temporal color as the supplying central step */
    }
    color &= mask;
    return color < 3 ? 3 : color;
}
void dz_extended_dots(int rule, unsigned color, unsigned symmetry, unsigned page)
{
    if (!dz_extended_active() || image != DZ_EXT_DOTS) return;
    uint64_t denominator = (uint64_t)extent * extent;
    for (unsigned s = 0; s < 2; ++s) {
        side[s].budget += (uint64_t)columns * (unsigned)dz_video.height * (symmetry == 3 ? 2u : 1u);
        while (side[s].budget >= denominator) {
            side[s].budget -= denominator;
            int x = (int)random_side(s, columns), y = (int)random_side(s, (unsigned)dz_video.height);
            int wx = dz_video.width / 2 + x, wy = y * (halve_y ? 2 : 1) - (int)half_height;
            point(s, x, y, dot_color(s, rule, color, s ? wx : -wx - 1, wy), page);
        }
    }
    ++revision;
}
static void sine_point(unsigned s, int x, int y, unsigned color, unsigned page)
{
    int py = (int)half_height + y;
    if (halve_y && (py & 1)) return;
    point(s, (int)sine_center + x, halve_y ? py / 2 : py, color, page);
}
void dz_extended_sine(unsigned color, unsigned symmetry, unsigned page)
{
    if (!dz_extended_active() || image != DZ_EXT_SINE) return;
    for (unsigned s = 0; s < 2; ++s) {
        struct side_state *v = &side[s];
        /* Closed forms of A += sin(mod_x), B += cos(mod_y). Wrapped double
         * phases keep their envelope bounded even in very long side-only runs. */
        double ax = v->ax + v->gain_x * (v->bias_x - cos(v->mx + v->sx / 2.0));
        double ay = v->ay + v->gain_y * (sin(v->my + v->sy / 2.0) - v->bias_y);
        int x = (int)(v->scale * ax * sin(v->phase)), y = (int)(v->scale * ay * cos(v->phase));
        if (halve_y) { x &= ~1; y &= ~1; }
        sine_point(s, x, y, color, page); sine_point(s, -x, y, color, page);
        sine_point(s, x, -y, color, page); sine_point(s, -x, -y, color, page);
        if (symmetry == 3) {
            sine_point(s, y, x, color, page); sine_point(s, -y, x, color, page);
            sine_point(s, y, -x, color, page); sine_point(s, -y, -x, color, page);
        }
        const double turn = 6.28318530717958647692;
        v->phase = fmod(v->phase + 0.01, turn);
        v->mx = fmod(v->mx + v->sx, turn); v->my = fmod(v->my + v->sy, turn);
    }
    ++revision;
}

struct dz_extended_presenter {
    SDL_Texture *texture;
    SDL_Palette *palette;
    struct dz_extended_layout layout;
    int visible, indexed, height;
    uint64_t revision;
    unsigned start, split;
    SDL_Color colors[256];
};
void dz_extended_presenter_destroy(struct dz_extended_presenter *p)
{
    if (!p) return;
    SDL_DestroyTexture(p->texture); SDL_DestroyPalette(p->palette); free(p);
}
int dz_extended_prepare(struct dz_extended_presenter **handle, SDL_Renderer *renderer,
                        enum dz_presentation policy, int width, int height, int indexed,
                        const SDL_Color colors[256])
{
    struct dz_extended_presenter *p = *handle;
    int visible = policy == DZ_PRESENT_EXTENDED && dz_extended_active();
    if (policy == DZ_PRESENT_EXTENDED) {
        int resized = output_width && (output_width != width || output_height != height);
        output_width = width; output_height = height;
        if (resized) dz_extended_clear(background_pattern);
        if (mode != dz_video.mode) { dz_extended_retire(); mode = dz_video.mode; visible = 0; }
    }
    struct dz_extended_layout l = dz_extended_layout(width, height, dz_video.width);
    visible = visible && dz_extended_active() && l.columns > 0;
    if (!p && !visible) return 0;
    if (!p) { p = *handle = calloc(1, sizeof *p); if (!p) { dz_extended_retire(); return 1; } }
    int changed = p->visible != visible || memcmp(&p->layout, &l, sizeof l);
    p->layout = l;
    if (!visible) { p->visible = 0; return changed; }
    int fresh = !p->texture || p->indexed != indexed || p->height != dz_video.height;
    if (fresh) {
        SDL_DestroyTexture(p->texture); p->texture = NULL;
        if (!p->palette) p->palette = SDL_CreatePalette(256);
        p->texture = SDL_CreateTexture(renderer, indexed ? SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, (int)columns * 2, dz_video.height);
        if (!p->texture || !p->palette || !SDL_SetTextureScaleMode(p->texture, SDL_SCALEMODE_NEAREST) ||
            !SDL_SetTextureBlendMode(p->texture, SDL_BLENDMODE_NONE) ||
            (indexed && !SDL_SetTexturePalette(p->texture, p->palette))) goto failure;
        p->indexed = indexed; p->height = dz_video.height;
    }
    int palette_changed = memcmp(p->colors, colors, sizeof p->colors) != 0;
    int pixels_changed = fresh || !p->visible || p->revision != revision ||
        p->start != dz_video.display_start_row || p->split != dz_video.line_compare;
    if (indexed && (fresh || palette_changed) && !SDL_SetPaletteColors(p->palette, colors, 0, 256)) goto failure;
    if (pixels_changed || (!indexed && palette_changed)) {
        void *pixels; int pitch;
        if (!SDL_LockTexture(p->texture, NULL, &pixels, &pitch)) goto failure;
        for (unsigned y = 0; y < (unsigned)dz_video.height; ++y) {
            unsigned row = y > dz_video.line_compare ? y - dz_video.line_compare - 1u : dz_video.display_start_row + y;
            uint8_t *dest = (uint8_t *)pixels + y * (unsigned)pitch;
            for (unsigned s = 0; s < 2; ++s) for (unsigned d = 0; d < columns; ++d) {
                unsigned x = s ? columns + d : columns - 1u - d;
                uint8_t index = dz_extended_index(s, row, d);
                if (indexed) dest[x] = index;
                else { SDL_Color c = colors[index]; ((uint32_t *)dest)[x] = UINT32_C(0xff000000) | (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b; }
            }
        }
        SDL_UnlockTexture(p->texture);
    }
    p->revision = revision; p->start = dz_video.display_start_row; p->split = dz_video.line_compare;
    memcpy(p->colors, colors, sizeof p->colors); p->visible = 1;
    return changed || fresh || pixels_changed || palette_changed;
failure:
    p->visible = 0; dz_extended_retire(); return 1;
}
static int draw_side(struct dz_extended_presenter *p, SDL_FRect source, SDL_FRect dest)
{
    /* SDL's software RenderTexture truncates each destination separately. A quad
     * keeps the shared fractional edges and logical density at odd output sizes. */
    float u0 = source.x / (columns * 2), u1 = (source.x + source.w) / (columns * 2);
    SDL_FColor white = {1,1,1,1};
    SDL_Vertex v[5] = {
        {{dest.x, dest.y}, white, {u0,0}},
        {{dest.x + dest.w, dest.y}, white, {u1,0}},
        {{dest.x + dest.w, dest.y + dest.h}, white, {u1,1}},
        {{dest.x, dest.y + dest.h}, white, {u0,1}},
        {{dest.x + dest.w / 2, dest.y + dest.h / 2}, white, {(u0 + u1) / 2,0.5f}}
    };
    /* A center fan avoids SDL's two-triangle rectangle shortcut back to RenderTexture. */
    const int indices[] = {0,1,4,1,2,4,2,3,4,3,0,4};
    return SDL_RenderGeometry(SDL_GetRendererFromTexture(p->texture), p->texture, v, 5, indices, 12);
}
int dz_extended_draw(struct dz_extended_presenter *p)
{
    if (!p || !p->visible) return 1;
    SDL_FRect left = {(float)columns - p->layout.columns, 0, p->layout.columns, (float)p->height};
    SDL_FRect right = {(float)columns, 0, p->layout.columns, (float)p->height};
    return draw_side(p, left, p->layout.left) && draw_side(p, right, p->layout.right);
}
void dz_extended_shutdown(void)
{
    for (unsigned s = 0; s < 2; ++s) { free(ink[s]); ink[s] = NULL; }
    active = 0; ordinal = 0; output_width = output_height = 0; ++revision;
}
