/* Bounded CPU indexed stroke layer. No application callbacks, time or RNG here. */
#include "hybrid.h"
#include "dazzle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ADDRESS_BITS 19u
#define ADDRESS_MASK ((1u << ADDRESS_BITS) - 1u)
#define GENERATION_LIMIT 8190u
#define STORAGE_LIMIT (320u * 1024u * 1024u)
#define OWNED 1u
#define REPLACE 2u
#define SELF 4u

struct cell {
    uint32_t sequence, stroke;
    uint16_t generation;
    uint8_t under, flags;
};
struct sample {
    double x, y;
    uint32_t address, token;
    int valid;
};
static struct cell *cells;
/* Six bytes/sample: exact 4x4 union bits, plus the single top owner token. */
static uint32_t *owners;
static uint16_t *coverage;
static uint32_t *output;
static uint32_t *cell_offsets;
static struct column { unsigned cell, fraction; } *columns;
static uint32_t sequence, next_stroke, current_stroke, capacity;
static struct sample previous[8];
static struct { double x, y; int sx, sy; dz_stroke_id id; } pending;
static int out_w, out_h, raster_w, raster_h, logical_w, physical_h;
static enum dz_presentation presentation;
static double density_x, density_y;
static SDL_FRect viewport;
static size_t samples, allocated;
static uint64_t revision;
static int xor_active;
static uint64_t composed_revision = UINT64_MAX;
static SDL_Color composed_palette[256];
static unsigned composed_start, composed_split;

void dz_hybrid_retire(void)
{
    if (!dz_hybrid_enabled()) return;
    if (cells) memset(cells, 0, DZ_VRAM_PIXELS * sizeof *cells);
    if (owners) memset(owners, 0, samples * sizeof *owners);
    if (coverage) memset(coverage, 0, samples * sizeof *coverage);
    memset(previous, 0, sizeof previous);
    pending.id = current_stroke = sequence = 0;
    xor_active = 0;
    ++revision;
}

void dz_hybrid_shutdown(void)
{
    free(cells); free(owners); free(coverage); free(output); free(cell_offsets); free(columns);
    cells = NULL; owners = NULL; coverage = NULL; output = NULL;
    cell_offsets = NULL; columns = NULL;
    samples = allocated = 0; out_w = out_h = 0;
    pending.id = current_stroke = 0;
    composed_revision = UINT64_MAX;
    memset(previous, 0, sizeof previous);
}

size_t dz_hybrid_bytes(void) { return allocated; }
uint64_t dz_hybrid_revision(void) { return revision; }

int dz_hybrid_resize(int w, int h, enum dz_presentation policy)
{
    if (!dz_hybrid_enabled() || dz_video.mode == DZ_MODE_TEXT) return 0;
    uint32_t cap = DZ_VRAM_PLANE_BYTES * (uint32_t)dz_video.pixels_per_byte;
    if (output && w == out_w && h == out_h && presentation == policy &&
        logical_w == dz_video.width && physical_h == dz_video.height && capacity == cap) return 1;
    dz_hybrid_shutdown();
    ++revision;
    if (w <= 0 || h <= 0) return 0;
    viewport = dz_present_viewport(w, h, policy);
    /* Integral samples/cell keep a fragment's destination unambiguous even
     * when the viewport's fractional edge crosses an output pixel. Density is
     * at least output density; scanout retains the fractional viewport. */
    double dx = ceil(viewport.w / dz_video.width), dy = ceil(viewport.h / dz_video.height);
    double rw = dz_video.width * dx, rh = ceil((double)cap / dz_video.width) * dy;
    double bytes = rw * rh * 6.0 + (double)DZ_VRAM_PIXELS * sizeof *cells + (double)w * h * 4.0 +
        cap * sizeof *cell_offsets + w * sizeof *columns;
    /* No density reduction: cap failure preserves the original pixel presenter. */
    if (bytes > STORAGE_LIMIT || rw > 32768 || rh > 131072) return 0;
    samples = (size_t)rw * (size_t)rh;
    cells = calloc(DZ_VRAM_PIXELS, sizeof *cells);
    owners = calloc(samples, sizeof *owners);
    coverage = calloc(samples, sizeof *coverage);
    output = malloc((size_t)w * h * sizeof *output);
    cell_offsets = malloc(cap * sizeof *cell_offsets);
    columns = malloc((size_t)w * sizeof *columns);
    if (!cells || !owners || !coverage || !output || !cell_offsets || !columns) { dz_hybrid_shutdown(); return 0; }
    allocated = samples * 6u + DZ_VRAM_PIXELS * sizeof *cells + (size_t)w * h * 4u +
        cap * sizeof *cell_offsets + (size_t)w * sizeof *columns;
    raster_w = (int)rw; raster_h = (int)rh;
    density_x = dx; density_y = dy;
    logical_w = dz_video.width; physical_h = dz_video.height; capacity = cap;
    out_w = w; out_h = h; presentation = policy;
    for (uint32_t a = 0; a < cap; ++a)
        cell_offsets[a] = (a / (unsigned)logical_w) * (unsigned)dy * (unsigned)raster_w +
            (a % (unsigned)logical_w) * (unsigned)dx;
    for (int x = 0; x < w; ++x) {
        double lx = (x + 0.5 - viewport.x) * logical_w / viewport.w;
        columns[x].cell = lx < 0 || lx >= logical_w ? UINT32_MAX : (unsigned)lx;
        columns[x].fraction = lx < 0 || lx >= logical_w ? 0 : (unsigned)((lx - floor(lx)) * dx);
    }
    sequence = current_stroke = pending.id = 0;
    return 1;
}

dz_stroke_id dz_stroke_begin(enum dz_stroke_kind kind)
{
    if (!dz_hybrid_enabled()) return 0;
    (void)kind;
    if (++next_stroke == 0) { dz_hybrid_retire(); next_stroke = 1; }
    current_stroke = next_stroke;
    memset(previous, 0, sizeof previous);
    pending.id = 0;
    return current_stroke;
}

void dz_stroke_break(dz_stroke_id id)
{
    if (!dz_hybrid_enabled()) return;
    if (current_stroke == id) {
        memset(previous, 0, sizeof previous);
        pending.id = 0;
    }
}

void dz_stroke_sample(dz_stroke_id id, double x, double y, int sx, int sy)
{
    if (!dz_hybrid_enabled()) return;
    if (!id || !isfinite(x) || !isfinite(y) || fabs(x - sx) > 2 || fabs(y - sy) > 2) {
        dz_stroke_break(current_stroke); return;
    }
    if (current_stroke != id) {
        memset(previous, 0, sizeof previous); current_stroke = id;
    }
    pending.x = x; pending.y = y; pending.sx = sx; pending.sy = sy; pending.id = id;
}

void dz_stroke_line(dz_stroke_id id, double x0, double y0, double x1, double y1, int sx, int sy)
{
    if (!dz_hybrid_enabled()) return;
    double dx = x1 - x0, dy = y1 - y0, length = dx * dx + dy * dy;
    double t = length ? ((sx - x0) * dx + (sy - y0) * dy) / length : 0;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    dz_stroke_sample(id, x0 + t * dx, y0 + t * dy, sx, sy);
}

void dz_hybrid_plot_end(void) { pending.id = 0; }

void dz_hybrid_boundary(void)
{
    pending.id = current_stroke = 0;
    memset(previous, 0, sizeof previous);
}

void dz_hybrid_plot_begin(void)
{
    int active = dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor;
    if (active && !xor_active) dz_hybrid_retire();
    xor_active = active;
}

static int valid(uint32_t token, uint32_t destination)
{
    if (!token) return 0;
    uint32_t owner = token & ADDRESS_MASK;
    return owner < capacity && (token >> ADDRESS_BITS) == cells[owner].generation &&
        (cells[owner].flags & OWNED) && cells[destination].sequence <= cells[owner].sequence;
}

static uint32_t destination(int x, int y)
{
    unsigned lx = (unsigned)((x + 0.5) / density_x);
    unsigned ly = (unsigned)((y + 0.5) / density_y);
    return lx < (unsigned)logical_w && ly * (uint32_t)logical_w + lx < capacity ?
        ly * (uint32_t)logical_w + lx : capacity;
}

/* A swept axis-aligned one-cell box: segment intersects the box about p. */
static int swept(double px, double py, double x0, double y0, double x1, double y1)
{
    double t0 = 0, t1 = 1, a[2] = {x0, y0}, d[2] = {x1 - x0, y1 - y0};
    double p[2] = {px, py};
    for (int axis = 0; axis < 2; ++axis) {
        if (d[axis] == 0) { if (fabs(p[axis] - a[axis]) > 0.5) return 0; }
        else {
            double lo = (p[axis] - 0.5 - a[axis]) / d[axis];
            double hi = (p[axis] + 0.5 - a[axis]) / d[axis];
            if (lo > hi) { double swap = lo; lo = hi; hi = swap; }
            if (lo > t0) t0 = lo;
            if (hi < t1) t1 = hi;
            if (t0 > t1) return 0;
        }
    }
    return 1;
}

static void bounds(uint32_t address, int *left, int *top, int *right, int *bottom)
{
    int x = (int)(address % (unsigned)logical_w), y = (int)(address / (unsigned)logical_w);
    *left = (int)floor((x - 1) * density_x); *right = (int)ceil((x + 2) * density_x);
    *top = (int)floor((y - 1) * density_y); *bottom = (int)ceil((y + 2) * density_y);
    if (*left < 0) *left = 0;
    if (*top < 0) *top = 0;
    if (*right > raster_w) *right = raster_w;
    if (*bottom > raster_h) *bottom = raster_h;
}

static void retag(uint32_t address, uint32_t old_token, uint32_t token, uint32_t old_sequence)
{
    for (unsigned i = 0; i < 8; ++i)
        if (previous[i].valid && previous[i].token == old_token) previous[i].token = token;
    int l, t, r, b; bounds(address, &l, &t, &r, &b);
    for (int y = t; y < b; ++y) for (int x = l; x < r; ++x) {
        size_t i = (size_t)y * raster_w + x;
        if (owners[i] == old_token) {
            uint32_t q = destination(x, y);
            if (q < capacity && (q == address || cells[q].sequence <= old_sequence)) owners[i] = token;
            else { owners[i] = 0; coverage[i] = 0; }
        }
    }
}

static uint32_t write_cell(uint32_t address, uint8_t old, uint8_t index, dz_stroke_id id)
{
    if (!cells || address >= capacity) return 0;
    if (sequence == UINT32_MAX || cells[address].generation >= GENERATION_LIMIT) dz_hybrid_retire();
    struct cell *c = &cells[address];
    uint32_t old_sequence = c->sequence;
    uint32_t old_token = ((uint32_t)c->generation << ADDRESS_BITS) | address;
    int duplicate = id && (c->flags & OWNED) && c->stroke == id && old == index;
    uint8_t under = duplicate ? c->under : old;
    uint8_t flags = id ? OWNED | ((!(c->flags & OWNED) || duplicate) ? REPLACE : 0) : 0;
    if (duplicate && !(c->flags & REPLACE)) flags &= (uint8_t)~REPLACE;
    c->sequence = ++sequence; ++c->generation;
    c->stroke = id; c->under = under; c->flags = flags;
    uint32_t token = ((uint32_t)c->generation << ADDRESS_BITS) | address;
    if (duplicate) retag(address, old_token, token, old_sequence);
    ++revision;
    return token;
}

void dz_hybrid_write(uint32_t address, uint8_t old, uint8_t index, enum dz_write_kind kind, dz_stroke_id id)
{
    (void)kind; (void)id;
    write_cell(address, old, index, 0);
}

/* One half of the committed interval, supplied by an actual accepted plot cell.
 * Center projection splits discrete colors/ownership at the sample midpoint. */
static void rasterize(struct sample a, struct sample b, struct sample supplier, int half)
{
    if (!supplier.valid || !valid(supplier.token, supplier.address)) return;
    int l, t, r, bot; bounds(supplier.address, &l, &t, &r, &bot);
    int sl = (int)floor((fmin(a.x, b.x) - 0.5) * density_x);
    int sr = (int)ceil((fmax(a.x, b.x) + 0.5) * density_x);
    int st = (int)floor((fmin(a.y, b.y) - 0.5) * density_y);
    int sb = (int)ceil((fmax(a.y, b.y) + 0.5) * density_y);
    if (sl > l) l = sl;
    if (sr < r) r = sr;
    if (st > t) t = st;
    if (sb < bot) bot = sb;
    double vx = b.x - a.x, vy = b.y - a.y, length = vx * vx + vy * vy;
    int ox = (int)(supplier.address % (unsigned)logical_w), oy = (int)(supplier.address / (unsigned)logical_w);
    for (int y = t; y < bot; ++y) for (int x = l; x < r; ++x) {
        uint32_t q = destination(x, y);
        if (q >= capacity || !valid(supplier.token, q)) continue;
        /* Indices are point sampled. Pick the supplying band at the raster
         * center, then measure the whole stroke footprint at 4x4 positions.
         * Clipping coverage at a color boundary would punch a half-pixel hole. */
        double center_x = (x + 0.5) / density_x, center_y = (y + 0.5) / density_y;
        double along = length ? ((center_x - a.x) * vx + (center_y - a.y) * vy) / length : 1;
        if (length && ((half == 0 && along >= 0.5) || (half == 1 && along < 0.5))) continue;
        uint16_t bits = 0;
        for (int yy = 0; yy < 4; ++yy) for (int xx = 0; xx < 4; ++xx) {
            double px = (x + (xx + 0.5) / 4) / density_x;
            double py = (y + (yy + 0.5) / 4) / density_y;
            if (px < ox - 1 || px >= ox + 2 || py < oy - 1 || py >= oy + 2) continue;
            if (swept(px, py, a.x, a.y, b.x, b.y)) bits |= (uint16_t)(1u << (yy * 4 + xx));
        }
        if (!bits) continue;
        size_t i = (size_t)y * raster_w + x;
        uint32_t old = owners[i], o = old & ADDRESS_MASK;
        int live = valid(old, q);
        if (live && cells[o].stroke == cells[supplier.address].stroke &&
            dz_video.vram[o] == dz_video.vram[supplier.address]) {
            coverage[i] |= bits;
            /* Keep the nearest supplying cell; equal distances follow store order. */
            double px = (x + 0.5) / density_x, py = (y + 0.5) / density_y;
            double ax = o % (unsigned)logical_w + 0.5, ay = o / (unsigned)logical_w + 0.5;
            double bx = ox + 0.5, by = oy + 0.5;
            if ((px-bx)*(px-bx)+(py-by)*(py-by) <= (px-ax)*(px-ax)+(py-ay)*(py-ay)) owners[i] = supplier.token;
        } else if (!live || cells[o].sequence <= cells[supplier.address].sequence) {
            owners[i] = supplier.token; coverage[i] = bits;
        }
    }
}

void dz_hybrid_store(uint32_t address, uint8_t old, uint8_t index,
                     unsigned x, unsigned y, unsigned branch, int halve_y)
{
    if (!cells) return;
    if (dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor) {
        dz_hybrid_write(address, old, index, DZ_WRITE_PIXEL, 0); return;
    }
    int eligible = pending.id && pending.sx == draw_x && pending.sy == draw_y && branch < 8;
    /* Reject word aliases rather than guessing geometry from the wrapped address. */
    uint32_t base = (uint32_t)page_paragraph * 16u * (unsigned)dz_video.pixels_per_byte;
    if (base % (unsigned)logical_w || x >= (unsigned)logical_w || y >= (unsigned)physical_h ||
        address != base + y * (unsigned)logical_w + x)
        eligible = 0;
    uint32_t token = write_cell(address, old, index, eligible ? pending.id : 0);
    if (!eligible || !token) { if (branch < 8) previous[branch].valid = 0; return; }
    double px = branch & 4 ? pending.y : pending.x, py = branch & 4 ? pending.x : pending.y;
    if (branch & 2) px = -px;
    if (branch & 1) py = -py;
    px += screen_half_width; py += screen_half_height;
    if (halve_y) py /= 2;
    px += 0.5; py += 0.5 + base / (unsigned)logical_w;
    struct sample now = {px, py, address, token, 1}, before = previous[branch];
    if (!before.valid || !valid(before.token, before.address) ||
        fabs(now.x - before.x) > 8 || fabs(now.y - before.y) > 8) before = now;
    if (before.address == now.address) rasterize(before, now, now, 2);
    else {
        rasterize(before, now, before, 0);
        rasterize(before, now, now, 1);
    }
    previous[branch] = now;
}

static unsigned alpha(uint16_t bits)
{
    unsigned n = bits;
    n -= (n >> 1) & 0x5555u;
    n = (n & 0x3333u) + ((n >> 2) & 0x3333u);
    n = (n + (n >> 4)) & 0x0f0fu;
    n = (n + (n >> 8)) & 31u;
    return (n * 255u + 8u) / 16u;
}

uint32_t *dz_hybrid_compose(const SDL_Color palette[256])
{
    if (!output) return NULL;
    if (composed_revision == revision && composed_start == dz_video.display_start_row &&
        composed_split == dz_video.line_compare && !memcmp(composed_palette, palette, sizeof composed_palette))
        return output;
    /* Validate top fragments, then retire masks without self-owned support.
     * A discarded top is never replaced with hidden geometry. */
    for (uint32_t a = 0; a < capacity; ++a) cells[a].flags &= (uint8_t)~SELF;
    for (int y = 0; y < raster_h; ++y) for (int x = 0; x < raster_w; ++x) {
        size_t i = (size_t)y * raster_w + x;
        if (!owners[i]) continue;
        uint32_t q = destination(x, y);
        if (q >= capacity || !valid(owners[i], q)) { owners[i] = 0; coverage[i] = 0; }
        else if ((owners[i] & ADDRESS_MASK) == q) cells[q].flags |= SELF;
    }
    for (uint32_t a = 0; a < capacity; ++a)
        if ((cells[a].flags & REPLACE) && !(cells[a].flags & SELF)) cells[a].flags = 0;
    for (int y = 0; y < out_h; ++y) {
        double ly = (y + 0.5 - viewport.y) * physical_h / viewport.h;
        if (ly < 0 || ly >= physical_h) {
            for (int x = 0; x < out_w; ++x) output[(size_t)y * out_w + x] = 0xff000000u;
            continue;
        }
        unsigned sy = (unsigned)ly;
        unsigned row = sy > dz_video.line_compare ? sy - dz_video.line_compare - 1u : dz_video.display_start_row + sy;
        uint32_t row_address = row * (unsigned)logical_w;
        unsigned fraction_y = (unsigned)((ly - sy) * density_y) * (unsigned)raster_w;
        for (int x = 0; x < out_w; ++x) {
            size_t i = (size_t)y * out_w + x;
            if (columns[x].cell == UINT32_MAX) { output[i] = 0xff000000u; continue; }
            uint32_t q = (row_address + columns[x].cell) & (capacity - 1u);
            /* Both base and layer use the wrapped address plus the same cell fraction. */
            size_t r = cell_offsets[q] + fraction_y + columns[x].fraction;
            uint32_t token = r < samples ? owners[r] : 0;
            SDL_Color base = palette[cells[q].flags & REPLACE ? cells[q].under : dz_video.vram[q]];
            unsigned a = token && valid(token, q) ? alpha(coverage[r]) : 0;
            SDL_Color ink = palette[dz_video.vram[token & ADDRESS_MASK]];
            unsigned red = (ink.r * a + base.r * (255u-a) + 127u) / 255u;
            unsigned green = (ink.g * a + base.g * (255u-a) + 127u) / 255u;
            unsigned blue = (ink.b * a + base.b * (255u-a) + 127u) / 255u;
            output[i] = 0xff000000u | red << 16 | green << 8 | blue;
        }
    }
    composed_revision = revision; composed_start = dz_video.display_start_row; composed_split = dz_video.line_compare;
    memcpy(composed_palette, palette, sizeof composed_palette);
    return output;
}
