/* Ctrl-image smoke and explicit-backend regression checks. */
#include "dazzle.h"
#include "dzconst.h"
#include "platform/platform.h"

#include <assert.h>
#include <stdio.h>

extern int16_t butterflies_center_x, butterflies_center_y, tiles_y_extent;

static uint16_t observed_plots;
static void observe_plot(uint16_t symmetry)
{
    (void)symmetry;
    ++observed_plots;
}

static int plot_tick_pending;
static void observe_plot_with_tick(uint16_t symmetry)
{
    observe_plot(symmetry);
    if (plot_tick_pending) {
        plot_tick_pending = 0;
        dz_delay_ms(60);
    }
}

/* Original-executable stack watches establish both caller paths. EGA shading
 * consumes two random values; a zero wedges gate adds seven step draws. */
static void check_step_gates(void)
{
    uint64_t before;

    video_adapter = ADAPTER_EGA;
    bios_resolution_mode = 0;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_EGA_350, 1);
    plot_callback = observe_plot;
    draw_symmetry = 1;
    palette_elapsed_ticks = 0;
    dz_srand(0x4128);

    for (int quiet = 0; quiet < 2; ++quiet) {
        semi_auto_mode = 0;
        semi_auto_continuous = 0;
        muted = (int16_t)quiet;
        current_graphics_key = previous_graphics_key = 0;
        wedges(1);
        before = dz_rand_calls();
        wedges(0);
        assert(dz_rand_calls() - before == 2);
    }

    semi_auto_mode = semi_auto_continuous = 1;
    muted = 0;
    wedges(1);
    before = dz_rand_calls();
    wedges(0);
    assert(dz_rand_calls() - before == 2); /* beep's delay return address */

    muted = 1;
    current_graphics_key = BIOS_KEY_ALT_F10;
    previous_graphics_key = 0;
    wedges(1);
    current_graphics_key = BIOS_KEY_F8; /* automatic continuation after rebuild */
    plot_tick_pending = 1;
    plot_callback = observe_plot_with_tick;
    before = dz_rand_calls();
    wedges(0);
    assert(!plot_tick_pending);
    assert(dz_rand_calls() - before == 9);
    plot_callback = observe_plot;
    for (int step = 0; step < 3; ++step) {
        before = dz_rand_calls();
        wedges(0);
        assert(dz_rand_calls() - before == 9);
    }
    /* The outer plotter's frame places the interrupt return IP at BP-12h.
     * A tick between calls overwrites zero; ticks during a draw are deeper. */
    dz_delay_ms(60);
    before = dz_rand_calls();
    wedges(0);
    assert(dz_rand_calls() - before == 2);

    current_graphics_key = BIOS_KEY_ALT_F10;
    wedges(1);
    current_graphics_key = previous_graphics_key = BIOS_KEY_F10;
    before = dz_rand_calls();
    wedges(0);
    assert(dz_rand_calls() - before == 2); /* explicit resume after idle */

    current_graphics_key = BIOS_KEY_ALT_F10;
    previous_graphics_key = 0;
    wedges(1);
    current_graphics_key = previous_graphics_key = BIOS_KEY_F8;
    before = dz_rand_calls();
    wedges(0);
    assert(dz_rand_calls() - before == 2); /* explicit F8 after idle */

    /* The shallower diamonds slot is overwritten positively by the plotter. */
    current_graphics_key = BIOS_KEY_ALT_F10;
    previous_graphics_key = 0;
    diamonds(1);
    current_graphics_key = BIOS_KEY_F8;
    before = dz_rand_calls();
    diamonds(0);
    assert(dz_rand_calls() - before == 2);

    semi_auto_mode = semi_auto_continuous = 0;
    muted = 0;
    current_graphics_key = previous_graphics_key = 0;
}

int main(void)
{
    dz_options = (struct dz_options){.deterministic = 1, .seed = 7, .headless = 1, .speed = 1.0};
    assert(dz_initialize());
    dz_srand(7);

    video_adapter = 8;
    bios_resolution_mode = 0;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    palette_cycle_ticks = 10;
    palette_elapsed_ticks = 0;
    butterflies(1);
    assert(butterflies_center_x >= 0 && butterflies_center_x < screen_half_width);
    assert(butterflies_center_y >= 0 && butterflies_center_y < screen_half_height);
    butterflies(0);
    diamonds(1);
    gumballs(1);
    tiles(1);
    wedges(1);
    yarnball(1);
    yarnball(0);
    /* The images above drew through the real draw_polygon/draw_ellipse (merged DZLE port). */

    video_adapter = 4;
    bios_resolution_mode = 0;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_EGA_350, 1);
    /* The callback pointer is intentionally unrelated to the old EGA plotter identity. */
    plot_callback = observe_plot;
    draw_symmetry = 1;
    palette_cycle_ticks = 10;
    palette_elapsed_ticks = 0;
    screen_half_width = 320;
    screen_half_height = 350;
    observed_plots = 0;
    dz_srand(1);
    tiles(1);
    assert(video_backend == BACKEND_EGA_640X700);
    assert(tiles_y_extent == 175);
    assert(observed_plots != 0);

    video_adapter = 8;
    configure_video_backend(0);
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    plot_callback = observe_plot;
    observed_plots = 0;
    palette_cycle_ticks = 10;
    palette_elapsed_ticks = 0;
    dz_srand(1);
    tiles(1);
    assert(video_backend == BACKEND_VGA_320X400);
    assert(tiles_y_extent == 200);
    assert(observed_plots != 0);

    check_step_gates();
    dz_shutdown();
    puts("Ctrl image routines and video_backend selection passed");
    return 0;
}
