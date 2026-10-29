/* Ctrl-image smoke and explicit-backend regression checks. */
#include "dazzle.h"
#include "platform/platform.h"

#include <assert.h>
#include <stdio.h>

extern int16_t butterflies_center_x, butterflies_center_y, tiles_y_extent;

static uint16_t observed_plots;
static unsigned polygon_calls, ellipse_calls;

void draw_polygon(int16_t initialize, int16_t count, int16_t *x, int16_t *y,
                  uint16_t fill, uint16_t edge, uint16_t flags)
{
    (void)initialize; (void)x; (void)y; (void)fill; (void)edge; (void)flags;
    assert(count >= 3 && count <= 5);
    ++polygon_calls;
}

void draw_ellipse(int16_t initialize, int16_t ry, int16_t rx, int16_t cx, int16_t cy,
                  uint16_t fill, uint16_t edge, uint16_t flags)
{
    (void)initialize; (void)ry; (void)rx; (void)cx; (void)cy;
    (void)fill; (void)edge; (void)flags;
    ++ellipse_calls;
}

static void observe_plot(uint16_t symmetry)
{
    (void)symmetry;
    ++observed_plots;
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
    assert(polygon_calls != 0 && ellipse_calls != 0);

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

    dz_shutdown();
    puts("Ctrl image routines and video_backend selection passed");
    return 0;
}
