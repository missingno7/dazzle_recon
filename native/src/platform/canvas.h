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
uint8_t dz_canvas_pixel_read(uint16_t x, uint16_t y, unsigned pixels_per_byte);
uint8_t dz_canvas_control_pixel_read(uint16_t x, uint16_t y, unsigned pixels_per_byte);
void dz_canvas_raster_xor(int enabled);
void dz_canvas_fill(uint32_t address, size_t count, uint8_t index);
void dz_canvas_clear(uint8_t index); /* whole logical receipt and physical aperture */
void dz_canvas_clear_page(uint16_t paragraph, uint16_t words, uint16_t pattern); /* physical page clear accompanying literal DOS stores */
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
    int x, y, width, height;
    uint64_t revision;
    const uint8_t *coverage, *under; /* optional AA experiment */
};
struct dz_canvas_view dz_canvas_visible(void);
int dz_canvas_take_restart(void);
void dz_canvas_mode_changed(void);
size_t dz_canvas_bytes(void);
void dz_canvas_shutdown(void);
/* Native output pixels; selected page, same overwrite order and palette. */
int dz_canvas_has_native(void);
SDL_FRect dz_canvas_content(void);
void dz_canvas_sample_reset(void);
void dz_canvas_native_size(int *width, int *height);
void dz_canvas_stroke(double ax, double ay, double bx, double by, double width, uint8_t index);
void dz_canvas_suppress(int enabled);
uint8_t dz_canvas_sample(void); /* canvas result of the last charged logical read */
void dz_canvas_override(int index); /* raster colour; -1 restores logical colour */
void dz_canvas_fade_begin(unsigned key);
void dz_canvas_fade_end(void);
#endif
