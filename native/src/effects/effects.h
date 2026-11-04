#ifndef DAZZLE_NATIVE_EFFECTS_H
#define DAZZLE_NATIVE_EFFECTS_H
#include <stdint.h>
/* Per-effect Faithful/Native selection. An effect is an image record: index 0-35 in
 * alt_image_records, 36-41 in ctrl_image_records; bit N of dz_display.native_effects. */
enum { DZ_EFFECT_COUNT = 42, DZ_EFFECT_ARCHES = 0, DZ_EFFECT_CORKSCREW = 7,
    DZ_EFFECT_CYCLOIDS = 9, DZ_EFFECT_DAISIES = 10, DZ_EFFECT_ELLIPSE = 13,
    DZ_EFFECT_SINE = 27, DZ_EFFECT_SQUIRALS = 31, DZ_EFFECT_SWEEP2 = 33 };
/* One imminent plot, consumed by DVDT after the unchanged logical stores. */
extern void (*dz_native_plot)(unsigned symmetry, unsigned char color);
extern void (*dz_native_plot_owner)(int16_t rebuild);
void dz_native_queue(void (*plot)(unsigned symmetry, unsigned char color));
int dz_native_available(unsigned effect); /* has a Native implementation */
int dz_native_enabled(unsigned effect);   /* available, selected, and the Screen canvas is active */
int dz_native_any(void);                  /* Native ink can exist on this run */
int dz_native_find(const char *name);     /* image record name to effect, or -1 */
#endif
