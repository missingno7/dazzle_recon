/* Temporary Milestone A driver/glue; full program flow is ported in later slices. */
#include "dazzle.h"
#include "platform.h"
#include "dzconst.h"

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
    free(linewipe_history); linewipe_history = NULL;
    free(ball_bitmap_pixels); ball_bitmap_pixels = NULL;
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
}

/* Temporary pending entry: delete this function when the DZLE port is merged. */
int dazzle_main(int argc, char **argv)
{
    int16_t option_position;
    /* The pending DZLE objects are zero-initialized; retain useful CLI help
     * until its original initialized data and real entry are merged. */
    if (!executable_name_pointer) executable_name_pointer = "DAZZLE.EXE";
    if (!help_program_name) help_program_name = "DAZZLE";
    if (!program_revision) program_revision = "5.0";
    if (!registration_notice_pointer)
        registration_notice_pointer = "Comments and inquires about updates and other products are welcome at\n\rour mailing address. $15 ($20US for non-U.S.A. sites) registration payable\n\rto MicroTronics required after 10 days usage, which makes you eligible\n\rfor an update disk and puts you on our PRIVATE mailing list.\n\rAdd $5 extra for a printed manual. DOS & Windows Screen Savers available\n\rfor an additional cost.\n\r";
    initialize_program_defaults();
    (void)parse_command_line((int16_t)argc, argv, &option_position);
    fputs("DAZZLE native: dazzle_main awaits the DZLE port\n", stderr);
    return 2;
}
