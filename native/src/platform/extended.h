/* Independent indexed side compositions; observations never call simulation services. */
#ifndef DAZZLE_EXTENDED_H
#define DAZZLE_EXTENDED_H
#include "platform.h"

enum { DZ_EXT_DOTS = 20, DZ_EXT_SINE = 27 };
static inline int dz_extended_enabled(void)
{
    return dz_display.presentation == DZ_PRESENT_EXTENDED && dz_display.quality == DZ_QUALITY_CLASSIC;
}
struct dz_extended_layout {
    SDL_FRect center, left, right;
    float columns; /* visible columns per side; fractional outer edge, never rescale the center */
    int storage_columns;
};
struct dz_extended_layout dz_extended_layout(int width, int height, int logical_width);
void dz_extended_image(unsigned image, int extent, int half_height, int halve_y, uint16_t background);
void dz_extended_retire(void); /* sticky until a supported image boundary */
void dz_extended_clear(uint16_t pattern); /* whole mode clear; retain the current capability */
void dz_extended_fade(char key); /* only palette-only j is supported by the dispatcher */
void dz_extended_dots(int rule, unsigned color, unsigned symmetry, unsigned page);
void dz_extended_sine(unsigned color, unsigned symmetry, unsigned page);
void dz_extended_shutdown(void);
int dz_extended_active(void);
uint64_t dz_extended_revision(void);
uint8_t dz_extended_index(unsigned side, unsigned row, unsigned distance);
struct dz_extended_presenter;
/* prepare reports changes even if the central image is unchanged. Allocation/upload
 * failure retires the sides and lets the ordinary presenter draw Original. */
int dz_extended_prepare(struct dz_extended_presenter **p, SDL_Renderer *renderer,
                        enum dz_presentation policy, int width, int height, int indexed,
                        const SDL_Color colors[256]);
int dz_extended_draw(struct dz_extended_presenter *p);
void dz_extended_presenter_destroy(struct dz_extended_presenter *p);
#endif
