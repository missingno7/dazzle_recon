#ifndef DAZZLE_NATIVE_EFFECTS_H
#define DAZZLE_NATIVE_EFFECTS_H
#include <stdint.h>
/* Per-effect Faithful/Native selection. An effect is an image record: index 0-35 in
 * alt_image_records, 36-41 in ctrl_image_records; bit N of dz_display.native_effects. */
enum { DZ_EFFECT_COUNT = 42, DZ_EFFECT_ARCHES = 0, DZ_EFFECT_BARS = 3, DZ_EFFECT_BIGSTAR = 4,
    DZ_EFFECT_BOXES = 5, DZ_EFFECT_CONES = 6, DZ_EFFECT_CORKSCREW = 7,
    DZ_EFFECT_CYCLOIDS = 9, DZ_EFFECT_DAISIES = 10, DZ_EFFECT_ELLIPSE = 13,
    DZ_EFFECT_LITTLESTARS = 19, DZ_EFFECT_PUFFS = 22, DZ_EFFECT_SINE = 27,
    DZ_EFFECT_HEXAGON = 16, DZ_EFFECT_HILBERT = 17,
    DZ_EFFECT_SQUIRALS = 31, DZ_EFFECT_SWEEP = 32, DZ_EFFECT_SWEEP2 = 33,
    DZ_EFFECT_WANDERING = 34, DZ_EFFECT_ZIGSTREAM = 35,
    DZ_EFFECT_SPIRAL = 28, DZ_EFFECT_SPIRAL1 = 29, DZ_EFFECT_SQUARES = 30,
    DZ_EFFECT_GUMBALLS = 38, DZ_EFFECT_YARNBALL = 41 };
/* One imminent plot, consumed by DVDT after the unchanged logical stores. */
extern void (*dz_native_plot)(unsigned symmetry, unsigned char color);
extern void (*dz_native_plot_owner)(int16_t rebuild);
void dz_native_queue(void (*plot)(unsigned symmetry, unsigned char color));
/* Runs after the plot, Faithful or Native, without suppressing its stamps. Ink
 * past the logic's own limit: sides only, no logic, charges or logical stores. */
extern void (*dz_extended_after_plot)(unsigned symmetry, unsigned char color);
void dz_extended_queue(void (*plot)(unsigned symmetry, unsigned char color));
void dz_extended_stamp(int x, int y, unsigned symmetry, unsigned char color);
void dz_native_no_ink(unsigned effect); /* consume a non-geometric outer plot */
int dz_native_available(unsigned effect); /* has a Native implementation */
int dz_native_enabled(unsigned effect);   /* available, selected, and the Screen canvas is active */
int dz_native_any(void);                  /* Native ink can exist on this run */
int dz_native_find(const char *name);     /* image record name to effect, or -1 */
/* Instance status for verification, independent of whether its next plot has
 * a callback. A missing callback is never an implicit fallback declaration. */
enum dz_native_instance { DZ_NATIVE_UNSELECTED, DZ_NATIVE_DRAWING,
    DZ_NATIVE_ACCELERATING, DZ_NATIVE_CACHE_SPILL, DZ_NATIVE_XOR, DZ_NATIVE_UNBOUNDED };
enum dz_native_instance dz_native_instance(unsigned effect);
#endif
