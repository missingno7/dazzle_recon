/* Temporary Milestone A driver/glue; full program flow is ported in later slices. */
#include "dazzle.h"
#include "platform.h"
#include "dzconst.h"

void dz_build_ega_adapter(int16_t unused) { (void)unused; generate_palette(); }

/* LEDs have no host side effect. Preserve the old BIOS flag return convention. */
uint16_t keyboard_led_flags(uint16_t mask, uint16_t operation)
{
    static uint16_t leds;
    uint16_t previous = leds;
    if (operation == 0) leds |= mask;
    else if (operation == 1) leds &= (uint16_t)~mask;
    else { leds |= mask; leds &= (uint16_t)(mask | 0xff8fu); }
    return previous;
}
int16_t poll_graphics_keys(void)
{
    while (dz_bioskey(1)) {
        current_graphics_key = dz_bioskey(0);
        if ((current_graphics_key & 255u) == 27u) exit_requested = 1;
    }
    input_pending = 0;
    return (int16_t)exit_requested;
}
void print_closing_text_and_exit(int16_t status) { dz_shutdown(); exit(status); }
void set_ega_raster_xor(int16_t value) { dz_video.ega_xor = value != 0; }

int dz_slice_begin(const char *name)
{
    if (!strcmp(name, "sierpinski")) current_image_draw = sierpinski;
    else if (!strcmp(name, "lines")) current_image_draw = lines;
    else return 0;
    exit_requested = input_pending = mouse_abort_pending = 0;
    palette_elapsed_ticks = countdown_ticks = show_duration_ticks = 0;
    palette_cycle_offset = 0;
    plot_color = FIRST_CYCLING_COLOR;
    draw_x = draw_y = 0;
    draw_delay_count = 1;
    draw_symmetry = 3;
    palette_step = 3;
    /* Bootstrap video/palette without charging a 128000-byte clear at plot speed.
     * The temporary driver starts its clock after setup. No image work or RNG
     * calls are skipped; per-image timing below is always cooperative. */
    int saved_deterministic = dz_options.deterministic, saved_headless = dz_options.headless;
    dz_options.deterministic = dz_options.headless = 1;
    dz_timer_tick_handler = dz_keyboard_irq_handler = NULL;
    /* Mouse abort is left disabled for the temporary slice. */
    configure_video_backend(1);
    initialize_math_tables();
    palette_build_callback(0);
    mode_setup_callback(1);
    palette_upload_callback(palette_buffer + palette_cycle_offset);
    dz_options.deterministic = saved_deterministic;
    dz_options.headless = saved_headless;
    dz_clock_reset();
    palette_elapsed_ticks = 0;
    dz_timer_tick_handler = graphics_timer_tick_isr;
    dz_keyboard_irq_handler = keyboard_pending_isr;
    current_image_draw(1); /* original rebuild also advances to the first point */
    plot_callback(draw_symmetry);
    return 1;
}
void dz_slice_step(void)
{
    if (input_pending) poll_graphics_keys();
    if (exit_requested) return;
    current_image_draw(0);
    plot_callback(draw_symmetry);
    if (palette_elapsed_ticks >= palette_cycle_ticks) cycle_palette();
}
void dz_slice_end(void)
{
    free(palette_buffer); palette_buffer = NULL;
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
}
