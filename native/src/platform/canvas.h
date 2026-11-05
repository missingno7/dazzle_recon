/* Indexed display memory and uniform Native output-pixel pages. Address arithmetic and work charges stay
 * with the original algorithms; these operations never advance virtual time. */
#ifndef DAZZLE_CANVAS_H
#define DAZZLE_CANVAS_H
#include "platform.h"

uint8_t dz_canvas_read(uint32_t address);
void dz_canvas_write(uint32_t address, uint8_t index); /* literal store */
void dz_canvas_store(uint32_t address, uint8_t index, int ega); /* EGA mask / XOR */
/* Original physical pixel coordinates after kernel translation/reflection and
 * EGA700 parity handling; selected-page and DOS word wrapping are preserved. */
void dz_canvas_plot(uint16_t x, uint16_t y, unsigned pixels_per_byte, uint8_t index);
/* Extra horizontal plots only: no address arithmetic, receipt or readback. */
void dz_canvas_side_plot(int x, int y, uint8_t index);
uint8_t dz_canvas_pixel_read(uint16_t x, uint16_t y, unsigned pixels_per_byte);
uint8_t dz_canvas_control_pixel_read(uint16_t x, uint16_t y, unsigned pixels_per_byte);
void dz_canvas_raster_xor(int enabled);
void dz_canvas_fill(uint32_t address, size_t count, uint8_t index);
void dz_canvas_clear(uint8_t index); /* whole logical receipt and physical aperture */
void dz_canvas_clear_page(uint16_t paragraph, uint16_t words, uint16_t pattern); /* physical page clear accompanying literal DOS stores */
void dz_canvas_clear_page_end(void);
void dz_canvas_rect(int x, int y, int width, int height, uint8_t index);
void dz_canvas_move(int dx, int dy, int sx, int sy, int width, int height);
void dz_canvas_select_page(uint16_t paragraph);
void dz_canvas_start(unsigned row);
void dz_canvas_split(unsigned row);
const uint8_t *dz_canvas_memory(void); /* borrowed read-only logical aperture */
/* Legacy decoder destination: every write must use buffer_store, which also
 * accepts ordinary RAM. No caller may dereference this pointer for stores. */
uint8_t *dz_canvas_destination(void);
void dz_canvas_buffer_store(uint8_t *destination, uint8_t index);
/* Geometry is sampled once from the active SDL nearest renderer. Mode changes
 * import logical memory; output/policy changes request a clean image restart. */
int dz_canvas_configure(SDL_Renderer *renderer, int width, int height, enum dz_presentation policy);
struct dz_canvas_view {
    const uint8_t *indices;
    int width, height; /* complete persistent aperture, not visible scanout */
    uint64_t revision;
    const uint8_t *coverage, *under; /* optional AA experiment */
    const uint8_t *side_written; /* Extended: bit 0 stores; higher bits real-reach ownership */
    const SDL_Rect *dirty;
    int dirty_count;
    uint64_t dirty_base; /* partial update valid only from this revision */
};
struct dz_canvas_view dz_canvas_surface(void);
void dz_canvas_uploaded(uint64_t revision);
struct dz_canvas_quad { SDL_FRect source, dest; };
int dz_canvas_quads(struct dz_canvas_quad quads[3]);
/* Screen upload receipts for regression tests and developer measurements. */
struct dz_upload_stats { uint64_t textures, uploads, bytes; };
struct dz_upload_stats dz_presenter_upload_stats(const struct dz_presenter *presenter); /* NULL: window presenter */
int dz_canvas_take_restart(void);
void dz_canvas_mode_changed(void);
void dz_canvas_mode_ready(void);
size_t dz_canvas_bytes(void);
void dz_canvas_shutdown(void);
/* Native output pixels; selected page, same overwrite order and palette. */
int dz_canvas_has_native(void);
SDL_FRect dz_canvas_content(void);
void dz_canvas_sample_reset(void);
void dz_canvas_native_size(int *width, int *height);
/* The same policy rectangle as Faithful. Centres are logical pixel centres;
 * unit_y also accounts for EGA700's doubled software coordinates. */
struct dz_native_mapping { double cx, cy, unit_x, unit_y, weight_x, weight_y; };
struct dz_native_mapping dz_canvas_native_mapping(int half_width, int half_height);
void dz_canvas_stroke(double ax, double ay, double bx, double by, double width, uint8_t index);
void dz_canvas_stroke_ellipse(double ax, double ay, double bx, double by,
                              double width_x, double width_y, uint8_t index);
void dz_canvas_stroke_cell(double ax, double ay, double bx, double by,
                           double width_x, double width_y, uint8_t index, unsigned caps);
void dz_canvas_native_span(int y, int first, int end, uint8_t index);
void dz_canvas_native_quad(const double x[4], const double y[4], uint8_t index);
void dz_canvas_suppress(int enabled);
/* Actual unsuppressed stamps and consumed Native image plot groups. Normally
 * NULL; an Extended fade installs its private hook through this existing seam.
 * Native image generators still suppress their Faithful stamps. */
struct dz_canvas_store_audit {
    uint64_t faithful, native, reach, fold; int min_x, max_x;
    uint8_t *written; /* optional capacity-byte oracle map, actual centre stores */
};
extern struct dz_canvas_store_audit *dz_canvas_store_audit;
uint32_t dz_canvas_fold_address(int x, int y); /* oracle: visible pixel's folded logical address */
uint8_t dz_canvas_sample(void); /* canvas result of the last charged logical read */
/* Parallel reflected samples; logical reads remain the sole control lane.
 * Callers retain these colours alongside their existing centre samples. */
int dz_canvas_sample_lanes(void);
void dz_canvas_sample_sides(uint8_t *colors);
void dz_canvas_override_sides(const uint8_t *colors);
void dz_canvas_override(int index); /* raster colour; -1 restores logical colour */
void dz_canvas_fade_begin(unsigned key);
void dz_canvas_fade_end(void);
#endif
