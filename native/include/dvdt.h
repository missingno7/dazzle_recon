/* Native interfaces for all 38 entries in asm/dvdt.asm. */
#ifndef DAZZLE_DVDT_H
#define DAZZLE_DVDT_H

#include <stdint.h>

/* State originally allocated by DVDT, rather than the C application modules. */
extern uint16_t video_mode_cookie, input_poll_divider;
extern uint8_t symmetry_edge, symmetry_y_edge, disk_count;
extern uint16_t first_disk_cylinders, second_disk_cylinders;
extern uint16_t force_ega, force_vga, video_adapter, plot_color, page_paragraph;
extern uint16_t saved_bios_mode, saved_cursor_shape, saved_text_last_row, ega_memory_code;
extern uint16_t plasma_random, plasma_width, plasma_height, plasma_color_limit;
extern uint16_t ega_row_offsets[350];

void rgb8_to_ega_palette(uint8_t *destination, uint8_t *source, uint16_t count);
void ega_write_planar_row(uint8_t *source, uint16_t bytes_per_plane, uint16_t row);
void ega_init_row_table_and_mode(void);
void ega_plot_symmetric_640x700(uint16_t symmetry);
void ega_plot_symmetric_640x350(uint16_t symmetry);
void vga_plot_symmetric_320x400(uint16_t symmetry);
void vga_plot_symmetric_320x200(uint16_t symmetry);
/* Reads outside the logical rectangle or modeled aperture return zero. Within
 * it, EGA 700 reads halve odd Y too. Page offsets are independent of visibility. */
uint16_t ega_read_pixel_640x700(int16_t x, int16_t y);
uint16_t ega_read_pixel_640x350(int16_t x, int16_t y);
uint16_t vga_read_pixel_320x400(int16_t x, int16_t y);
uint16_t vga_read_pixel_320x200(int16_t x, int16_t y);
void save_video_state(void);
/* Void, like the original: writes video_adapter = 4 (EGA) or 8 (VGA).
 * Default VGA; force_vga takes precedence over force_ega. */
void detect_video_adapter(void);
void ega_set_mode(uint16_t clear_policy);
void clear_vram_words(uint16_t pattern, uint16_t count, uint16_t page_para);
void vga_set_mode_320x400(uint16_t clear_memory);
void vga_set_mode_320x200(uint16_t clear_memory);
void restore_video_state(int16_t restore_mode);
/* Palette pointers replace the original offset/segment argument pair. EGA
 * buffers contain 17 writable bytes; VGA buffers contain 759 RGB bytes. */
void ega_upload_palette_bios(uint8_t *palette);
void ega_upload_palette_ports(uint8_t *palette);
void vga_upload_palette_bios(uint8_t *palette);
void vga_upload_palette_bios_split(uint8_t *palette);
void vga_upload_palette_rep(uint8_t *palette);
void vga_upload_palette_rep_split(uint8_t *palette);
void vga_upload_palette_8086(uint8_t *palette);
void vga_upload_palette_8086_split(uint8_t *palette);
void ega_set_line_compare(uint16_t line);
void vga_set_line_compare(uint16_t line);
void set_display_start(uint16_t x, uint16_t physical_y);
void park_disk_heads(void);
uint8_t read_cmos_byte(uint8_t index);
int16_t cpu_has_post_8086_shifts(void);
void subdivide_plasma(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint16_t top_left, uint16_t top_right,
                     uint16_t bottom_right, uint16_t bottom_left);
void seed_plasma(void);
void graphics_timer_tick_isr(void);
void keyboard_pending_isr(void);
void mouse_pending_callback(void);
uint16_t dvdt_seg(void);

#endif
