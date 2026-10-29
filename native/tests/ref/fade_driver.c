#include "imgmodel.h"

static void fade_cases(void)
{
    unsigned mode, which;
    unsigned long before, i;
    for (mode = 0; mode < 2; ++mode) {
        setup_video_model(mode ? ADAPTER_EGA : ADAPTER_VGA);
        for (which = 0; which < 8; ++which) {
            reset_frame();
            /* A nonempty deterministic starting picture exercises erasers. */
            for (i = 0; i < frame_bytes; ++i) framebuffer[i] = (unsigned char)((i * 7 + i / 101) & color_index_mask);
            current_image = mode * 8 + which;
            ref_srand(0x7200 + current_image);
            plot_calls = cycle_calls = 0; plot_crc = 0xffffffffUL;
            palette_elapsed_ticks = 0; palette_cycle_offset = 9;
            draw_x = draw_y = 0; draw_symmetry = 3; plot_color = 3;
            current_graphics_key = input_pending = exit_requested = 0;
            tick_period = 37;
            before = crc_frame();
            switch (which) {
            case 0: fade_wipe(0); break;
            case 1: fade_wipe(1); break;
            case 2: fade_exclusive_or(); break;
            case 3: fade_moving_bars(); break;
            default: swap_pixel_rectangles(which - 4, -3, -5, 11, 17, 16, 19); break;
            }
            fprintf(summary, "%s,%u,%u,%lu,%08lX,%lu,%lu,%08lX,%08lX,%u\n",
                which < 4 ? "fade" : "swap", mode, which, plot_calls, plot_crc ^ 0xffffffffUL, rng_calls, cycle_calls,
                before, crc_frame(), palette_elapsed_ticks);
            fflush(summary);
        }
    }
}

#ifndef REF_NATIVE
void main(int argc, char far * far *argv)
{
    (void)argc; (void)argv;
    summary = fopen("FADE.TXT", "w"); full_log = fopen("PLOT.BIN", "wb"); first_log = fopen("PLOT.LOG", "w");
    if (!summary || !full_log || !first_log) exit(2);
    framebuffer = (unsigned char huge *)farmalloc(224000UL);
    if (!framebuffer) exit(3);
    make_crc_table(); initialize_math_tables(); fade_cases();
    fclose(summary); fclose(full_log); fclose(first_log); exit(0);
}
#endif
