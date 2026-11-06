#include "effects.h"
#include "fades.h"
#include "ellipses.h"
#include "lines.h"
#include "dazzle.h"
#include "platform/canvas.h"
#include <string.h>

void (*dz_native_plot)(unsigned symmetry, unsigned char color);
void (*dz_native_plot_owner)(int16_t rebuild);
void (*dz_extended_after_plot)(unsigned symmetry, unsigned char color);
void dz_extended_queue(void (*plot)(unsigned symmetry, unsigned char color))
{
    dz_extended_after_plot = plot;
    dz_native_plot_owner = current_image_draw;
}
void dz_native_queue(void (*plot)(unsigned symmetry, unsigned char color))
{
    dz_native_plot = plot;
    dz_native_plot_owner = current_image_draw;
}
static void no_ink(unsigned symmetry, unsigned char color) { (void)symmetry; (void)color; }
void dz_native_no_ink(unsigned effect)
{ if (dz_native_enabled(effect)) dz_native_queue(no_ink); }
static const uint64_t available = (UINT64_C(1) << DZ_EFFECT_SINE) |
    (UINT64_C(1) << DZ_EFFECT_ATOM) | (UINT64_C(1) << DZ_EFFECT_BALLS) |
    (UINT64_C(1) << DZ_EFFECT_CURVES) | (UINT64_C(1) << DZ_EFFECT_DIAG1) |
    (UINT64_C(1) << DZ_EFFECT_DIAG2) | (UINT64_C(1) << DZ_EFFECT_FORMULA1) |
    (UINT64_C(1) << DZ_EFFECT_GRID) | (UINT64_C(1) << DZ_EFFECT_LINES) |
    (UINT64_C(1) << DZ_EFFECT_LOTSADOTS) | (UINT64_C(1) << DZ_EFFECT_PEDALS) |
    (UINT64_C(1) << DZ_EFFECT_RIBBONS) | (UINT64_C(1) << DZ_EFFECT_TRIANGLES) |
    (UINT64_C(1) << DZ_EFFECT_TUNNEL) | (UINT64_C(1) << DZ_EFFECT_SIERPINSKI) |
    (UINT64_C(1) << DZ_EFFECT_HEXAGON) | (UINT64_C(1) << DZ_EFFECT_HILBERT) |
    (UINT64_C(1) << DZ_EFFECT_SPIRAL) | (UINT64_C(1) << DZ_EFFECT_SPIRAL1) |
    (UINT64_C(1) << DZ_EFFECT_SQUARES) | (UINT64_C(1) << DZ_EFFECT_SWEEP) |
    (UINT64_C(1) << DZ_EFFECT_ZIGSTREAM) |
    (UINT64_C(1) << DZ_EFFECT_CYCLOIDS) | (UINT64_C(1) << DZ_EFFECT_SWEEP2) |
    (UINT64_C(1) << DZ_EFFECT_ELLIPSE) | (UINT64_C(1) << DZ_EFFECT_ARCHES) |
    (UINT64_C(1) << DZ_EFFECT_DAISIES) | (UINT64_C(1) << DZ_EFFECT_SQUIRALS) |
    (UINT64_C(1) << DZ_EFFECT_CORKSCREW) |
    (UINT64_C(1) << DZ_EFFECT_BOXES) | (UINT64_C(1) << DZ_EFFECT_PUFFS) |
    (UINT64_C(1) << DZ_EFFECT_BARS) | (UINT64_C(1) << DZ_EFFECT_WANDERING) |
    (UINT64_C(1) << DZ_EFFECT_BIGSTAR) | (UINT64_C(1) << DZ_EFFECT_LITTLESTARS) |
    (UINT64_C(1) << DZ_EFFECT_CONES) | (UINT64_C(1) << DZ_EFFECT_BUTTERFLIES) |
    (UINT64_C(1) << DZ_EFFECT_DIAMONDS) | (UINT64_C(1) << DZ_EFFECT_GUMBALLS) |
    (UINT64_C(1) << DZ_EFFECT_TILES) | (UINT64_C(1) << DZ_EFFECT_WEDGES) |
    (UINT64_C(1) << DZ_EFFECT_YARNBALL);

int dz_native_available(unsigned effect)
{
    return effect < DZ_EFFECT_COUNT && (available >> effect & 1u);
}
int dz_native_any(void)
{
    return dz_display.canvas == DZ_CANVAS_SCREEN &&
        ((dz_display.native_effects & available) != 0 || dz_native_fades_any());
}
int dz_native_enabled(unsigned effect)
{
    return !(dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor) &&
        dz_native_available(effect) && dz_display.canvas == DZ_CANVAS_SCREEN &&
        (dz_display.native_effects >> effect & 1u);
}
int dz_native_find(const char *name)
{
    for (unsigned i = 0; i < DZ_EFFECT_COUNT; ++i) {
        const ImageConfigRow *image = i < 36 ? &alt_image_records[i] : &ctrl_image_records[i - 36];
        if (!strcmp(image->name, name)) return (int)i;
    }
    return -1;
}
enum dz_native_instance dz_native_instance(unsigned effect)
{
    if (!dz_native_available(effect) || dz_display.canvas != DZ_CANVAS_SCREEN ||
        !(dz_display.native_effects >> effect & 1u)) return DZ_NATIVE_UNSELECTED;
    if (dz_video.mode == DZ_MODE_EGA_350 && dz_video.ega_xor) return DZ_NATIVE_XOR;
    if (effect == DZ_EFFECT_HILBERT && dz_native_hilbert_xor_fallback()) return DZ_NATIVE_XOR;
    if (effect == DZ_EFFECT_SWEEP2 && effect_parameter) return DZ_NATIVE_ACCELERATING;
    if (effect == DZ_EFFECT_ARCHES && dz_native_arches_fallback())
        return (enum dz_native_instance)dz_native_arches_fallback();
    return DZ_NATIVE_DRAWING;
}
