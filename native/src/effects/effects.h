#ifndef DAZZLE_NATIVE_EFFECTS_H
#define DAZZLE_NATIVE_EFFECTS_H
/* Per-effect Faithful/Native selection. An effect is an image record: index 0-35 in
 * alt_image_records, 36-41 in ctrl_image_records; bit N of dz_display.native_effects. */
enum { DZ_EFFECT_COUNT = 42, DZ_EFFECT_SINE = 27 };
int dz_native_available(unsigned effect); /* has a Native implementation */
int dz_native_enabled(unsigned effect);   /* available, selected, and the Screen canvas is active */
int dz_native_any(void);                  /* Native ink can exist on this run */
int dz_native_find(const char *name);     /* image record name to effect, or -1 */
#endif
