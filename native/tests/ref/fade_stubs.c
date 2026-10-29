/* Only uncalled DZFD hardware dependencies; selected fades use model callbacks. */
#include "dazzle.h"
void seed_plasma(void) { }
void clear_vram_words(unsigned value, unsigned count, unsigned paragraph)
{ (void)value; (void)count; (void)paragraph; }
void vga_set_line_compare(unsigned row) { (void)row; }
void ega_set_line_compare(unsigned row) { (void)row; }
void randomize_palette_state(unsigned replace) { (void)replace; }
