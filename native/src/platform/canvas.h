/* Faithful indexed display memory. Address arithmetic and work charges stay
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
void dz_canvas_raster_xor(int enabled);
void dz_canvas_fill(uint32_t address, size_t count, uint8_t index);
void dz_canvas_clear(uint8_t index); /* entire allocated logical aperture */
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
};
struct dz_canvas_view dz_canvas_visible(void);
int dz_canvas_take_restart(void);
void dz_canvas_mode_changed(void);
size_t dz_canvas_bytes(void);
void dz_canvas_shutdown(void);
#endif
