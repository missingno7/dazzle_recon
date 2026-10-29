/* Presentation observations only. The Classic branch never reads observer state. */
#ifndef DAZZLE_HYBRID_H
#define DAZZLE_HYBRID_H
#include "platform.h"
typedef uint32_t dz_stroke_id;
enum dz_stroke_kind { DZ_STROKE_CYCLOIDS, DZ_STROKE_SINE, DZ_STROKE_SWEEP2 };
enum dz_write_kind { DZ_WRITE_PIXEL, DZ_WRITE_BULK };
static inline int dz_hybrid_enabled(void) { return dz_display.quality == DZ_QUALITY_HYBRID; }
dz_stroke_id dz_stroke_begin(enum dz_stroke_kind kind);
void dz_stroke_break(dz_stroke_id id);
void dz_stroke_sample(dz_stroke_id id, double x, double y, int sample_x, int sample_y);
void dz_stroke_line(dz_stroke_id id, double x0, double y0, double x1, double y1,
                    int sample_x, int sample_y);
void dz_hybrid_store(uint32_t address, uint8_t old_index, uint8_t new_index,
                     unsigned x, unsigned y, unsigned branch, int halve_y);
void dz_hybrid_write(uint32_t address, uint8_t old_index, uint8_t new_index,
                     enum dz_write_kind kind, dz_stroke_id id);
void dz_hybrid_plot_end(void);
void dz_hybrid_plot_begin(void);
void dz_hybrid_boundary(void); /* end intent/continuity, retain existing ink */
void dz_hybrid_retire(void);
void dz_hybrid_shutdown(void);
/* Returns 0 on allocation/cap failure: caller uses Classic. Resize retires ink. */
int dz_hybrid_resize(int width, int height, enum dz_presentation policy);
uint64_t dz_hybrid_revision(void);
uint32_t *dz_hybrid_compose(const SDL_Color colors[256]);
size_t dz_hybrid_bytes(void);
#endif
