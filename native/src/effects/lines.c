#include "lines.h"
#include "effects.h"
#include "geometry.h"
#include "dazzle.h"
#include "platform/canvas.h"

static const unsigned walk_effects[] = {
    DZ_EFFECT_HEXAGON, DZ_EFFECT_SPIRAL, DZ_EFFECT_SPIRAL1,
    DZ_EFFECT_SQUARES, DZ_EFFECT_SWEEP, DZ_EFFECT_ZIGSTREAM
};
static struct dz_native_path walk_paths[sizeof walk_effects / sizeof walk_effects[0]];
static unsigned active_walk;

static void draw_walk(unsigned symmetry, unsigned char color)
{ dz_native_path_draw(&walk_paths[active_walk],symmetry,color); }

void dz_native_walk(unsigned effect, int reset, int x, int y)
{
    dz_native_plot = NULL;
    unsigned slot;
    for (slot = 0; slot < sizeof walk_effects / sizeof walk_effects[0]; ++slot)
        if (walk_effects[slot] == effect) break;
    if (slot == sizeof walk_effects / sizeof walk_effects[0]) return;
    if (!dz_native_enabled(effect)) {
        walk_paths[slot].valid = 0;
        return;
    }
    active_walk = slot;
    dz_native_path_prepare(&walk_paths[slot],x,y,reset);
    dz_native_queue(draw_walk);
}

static struct dz_native_path hilbert_path;
static int hilbert_active, hilbert_xor;

int dz_native_hilbert_begin(int reset)
{
    if (reset) {
        dz_native_plot = NULL;
        hilbert_xor = dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor &&
            dz_display.canvas == DZ_CANVAS_SCREEN &&
            (dz_display.native_effects & (UINT64_C(1) << DZ_EFFECT_HILBERT));
        hilbert_active = !hilbert_xor && dz_native_enabled(DZ_EFFECT_HILBERT);
        hilbert_path.valid = 0;
    }
    return hilbert_active;
}

int dz_native_hilbert_xor_fallback(void) { return hilbert_xor; }

void dz_native_hilbert_background(int x, int y, unsigned char color)
{ dz_native_raster_cell(0,0,x,y,1,color); }

void dz_native_hilbert_step(int x, int y, int width, unsigned char color)
{
    double half = (width-1)*0.5;
    dz_native_path_prepare(&hilbert_path,x+half,y+half,0);
    dz_native_path_draw_width(&hilbert_path,1,color,width);
}

void dz_native_hilbert_finish(void)
{ if (hilbert_active) dz_native_no_ink(DZ_EFFECT_HILBERT); }
