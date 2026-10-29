/* Unused hardware/UI dependencies for palette-only DOSI diagnostics. */
#include "dazzle.h"

unsigned video_adapter;
unsigned plot_color;
unsigned int menu_active;
unsigned force_ega;
unsigned force_vga;

void far park_disk_heads(void) { }
void far detect_video_adapter(void) { }
void far save_video_state(void) { }
void far restore_video_state(int restore_text) { (void)restore_text; }
void far prepare_logo_image_resources(void) { }
void far menu_error_message(char far *message) { (void)message; }
void far upload_reserved_dac_entries(unsigned int count) { (void)count; }
void far help_screen(void) { }
void far configuration_screen(void) { }
void far save_pcx(void) { }
void far draw_closing_graphic(void) { }
void far extract_manual(void) { }
void far display_boundary_fade_and_pan(void) { }
void far set_display_start(unsigned x, unsigned y) { (void)x; (void)y; }
int far cpu_has_post_8086_shifts(void) { return 0; }

unsigned int far dzl2_seg(void) { return 0; }
unsigned int far dzfd_seg(void) { return 0; }
unsigned int far dvdt_seg(void) { return 0; }
unsigned int far dhuf_seg(void) { return 0; }
unsigned int far dlogo_seg(void) { return 0; }
unsigned int far dazpcx_seg(void) { return 0; }

void butterflies(int draw) { (void)draw; }
void diamonds(int draw) { (void)draw; }
void gumballs(int draw) { (void)draw; }
void tiles(int draw) { (void)draw; }
void wedges(int draw) { (void)draw; }
void yarnball(int draw) { (void)draw; }

void far ega_set_mode(unsigned mode) { (void)mode; }
void far ega_plot_symmetric_640x350(void) { }
void far ega_plot_symmetric_640x700(unsigned symmetry) { (void)symmetry; }
void far vga_set_mode_320x200(unsigned mode) { (void)mode; }
void far vga_set_mode_320x400(unsigned mode) { (void)mode; }
void far vga_plot_symmetric_320x200(unsigned symmetry) { (void)symmetry; }
void far vga_plot_symmetric_320x400(unsigned symmetry) { (void)symmetry; }
unsigned far vga_read_pixel_320x200(int x, int y) { (void)x; (void)y; return 0; }
unsigned far vga_read_pixel_320x400(int x, int y) { (void)x; (void)y; return 0; }
unsigned far ega_read_pixel_640x350(int x, int y) { (void)x; (void)y; return 0; }
unsigned far ega_read_pixel_640x700(int x, int y) { (void)x; (void)y; return 0; }

#define PALETTE_UPLOAD(name) \
    void far name(unsigned start, unsigned count) { (void)start; (void)count; }
PALETTE_UPLOAD(vga_upload_palette_bios_split)
PALETTE_UPLOAD(vga_upload_palette_8086_split)
PALETTE_UPLOAD(vga_upload_palette_rep_split)
PALETTE_UPLOAD(vga_upload_palette_bios)
PALETTE_UPLOAD(vga_upload_palette_8086)
PALETTE_UPLOAD(vga_upload_palette_rep)
PALETTE_UPLOAD(ega_upload_palette_bios)
PALETTE_UPLOAD(ega_upload_palette_ports)

void far rgb8_to_ega_palette(unsigned char far *destination,
                             unsigned char far *source, unsigned mask)
{
    (void)destination;
    (void)source;
    (void)mask;
}

void interrupt far graphics_timer_tick_isr(void) { }
void interrupt far keyboard_pending_isr(void) { }
void far mouse_pending_callback(void) { }
