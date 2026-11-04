#include "effects.h"
#include "dazzle.h"
#include "platform/canvas.h"
#include <string.h>

void (*dz_native_plot)(unsigned symmetry, unsigned char color);
void (*dz_native_plot_owner)(int16_t rebuild);
void dz_native_queue(void (*plot)(unsigned symmetry, unsigned char color))
{
    dz_native_plot = plot;
    dz_native_plot_owner = current_image_draw;
}
static const uint64_t available = (UINT64_C(1) << DZ_EFFECT_SINE) |
    (UINT64_C(1) << DZ_EFFECT_CYCLOIDS) | (UINT64_C(1) << DZ_EFFECT_SWEEP2) |
    (UINT64_C(1) << DZ_EFFECT_ELLIPSE) | (UINT64_C(1) << DZ_EFFECT_ARCHES) |
    (UINT64_C(1) << DZ_EFFECT_DAISIES) | (UINT64_C(1) << DZ_EFFECT_SQUIRALS) |
    (UINT64_C(1) << DZ_EFFECT_CORKSCREW);

int dz_native_available(unsigned effect)
{
    return effect < DZ_EFFECT_COUNT && (available >> effect & 1u);
}
int dz_native_any(void)
{
    return dz_display.canvas == DZ_CANVAS_SCREEN && (dz_display.native_effects & available) != 0;
}
int dz_native_enabled(unsigned effect)
{
    return dz_native_available(effect) && dz_display.canvas == DZ_CANVAS_SCREEN &&
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
