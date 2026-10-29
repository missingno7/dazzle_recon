/* Native regression receipts, not DOS differential evidence. Every Alt table
 * entry runs its original rebuild, then eight steps, on the VGA 400 backend.
 * Compile with GCC bounds/undefined traps for the memory/arithmetic audit. */
#include "dazzle.h"
#include "dzconst.h"
#include "platform/platform.h"
#include <assert.h>
#include <inttypes.h>

static const uint32_t expected_pixels[36] = {
    UINT32_C(0x438085e7), UINT32_C(0x060c76fd), UINT32_C(0xb39943b5), UINT32_C(0x52822a2d),
    UINT32_C(0x97c43ccd), UINT32_C(0x1009c191), UINT32_C(0xe6a2ca5d), UINT32_C(0xfc37b5a5),
    UINT32_C(0x6b361b75), UINT32_C(0x63dcfd09), UINT32_C(0xbcf4fd15), UINT32_C(0xe2f92581),
    UINT32_C(0x8192d7c1), UINT32_C(0x24676ae6), UINT32_C(0x18b6135d), UINT32_C(0xa857c9f8),
    UINT32_C(0xd052f165), UINT32_C(0x4e1e0299), UINT32_C(0x6abc9dc5), UINT32_C(0x344a216c),
    UINT32_C(0xf48eba75), UINT32_C(0x9e7b6eed), UINT32_C(0x401f94c5), UINT32_C(0x51d6ab0d),
    UINT32_C(0x078a8ae5), UINT32_C(0x6c29681a), UINT32_C(0x41cc9705), UINT32_C(0x1065a685),
    UINT32_C(0x4f6147e5), UINT32_C(0x5c1c9635), UINT32_C(0x80f0f5c5), UINT32_C(0xac6fbfb4),
    UINT32_C(0x207d26bd), UINT32_C(0xfd8fc881), UINT32_C(0xbb7979a3), UINT32_C(0xc79129d5)
};
static const uint64_t expected_rng[36] = {
    1, 0, 3, 5, 4, 4, 61, 3, 2, 14, 7, 2, 1, 4, 3, 2, 0, 19, 1, 82, 19, 0, 5, 10, 6, 1, 13, 2, 2, 4, 4, 54, 0, 1, 5, 3
};
static uint8_t ball_pixels[160 * 175];

static void begin_mode(uint16_t adapter, uint16_t bios_mode)
{
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    video_adapter = adapter;
    bios_resolution_mode = bios_mode;
    configure_video_backend(1);
    initialize_math_tables();
    dz_video_set_mode(adapter == ADAPTER_EGA ? DZ_MODE_EGA_350 :
        bios_mode ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400, 1);
    draw_x = draw_y = image_work_count = ega_color_substep = 0;
    plot_color = FIRST_CYCLING_COLOR;
    draw_delay_count = 1;
    draw_symmetry = 3;
    palette_elapsed_ticks = 0;
    palette_cycle_ticks = UINT16_MAX;
    input_pending = exit_requested = current_graphics_key = 0;
    page_paragraph = 0;
    corkscrew_color_counter = 0;
    /* DOSI owns allocation/map generation. Until that port lands, use a fixed
     * nonzero, column-major fixture with the real configured dimensions. */
    assert((size_t)ball_bitmap_width * ball_bitmap_height <= sizeof ball_pixels);
    for (size_t i = 0; i < sizeof ball_pixels; ++i)
        ball_pixels[i] = (uint8_t)(3 + i % 253);
    ball_bitmap_pixels = ball_pixels;
    dz_clock_reset();
    dz_srand(1);
}

static void begin(void) { begin_mode(ADAPTER_VGA, 0); }

static void end(void)
{
    free(palette_buffer);
    palette_buffer = NULL;
    ball_bitmap_pixels = NULL;
}

static void images(void)
{
    for (unsigned i = 0; i < 36; ++i) {
        begin();
        ImageConfigRow *row = &alt_image_records[i];
        assert(row->draw && row->name && row->key);
        printf("running %s\n", row->name); fflush(stdout);
        row->draw(1);
        plot_callback(draw_symmetry);
        for (unsigned step = 0; step < 8; ++step) {
            row->draw(0);
            plot_callback(draw_symmetry);
        }
        uint32_t pixels = dz_framebuffer_hash();
        uint64_t calls = dz_rand_calls();
        printf("%s: pixels=%08" PRIx32 " rng=%" PRIu64 "\n", row->name, pixels, calls);
        assert(pixels == expected_pixels[i]);
        assert(calls == expected_rng[i]);
        end();
    }
    assert(ctrl_image_records[0].draw == butterflies);
    assert(ctrl_image_records[1].draw == diamonds);
    assert(ctrl_image_records[5].draw == yarnball);
    assert(fade_records[23].key == 'z' && fade_records[23].enabled == 1);
}

static unsigned plots;
static void count_plot(uint16_t symmetry) { (void)symmetry; ++plots; }

static void geometry(void)
{
    begin();
    plot_callback = count_plot;
    draw_x = 5; draw_y = 6; plots = 0;
    square(3, 1);
    assert(draw_x == 5 && draw_y == 6 && plots == 9);
    int16_t x[] = {10, 3, 3, 0, 0}, y[] = {20, 0, 2, 2, 0};
    plots = 0;
    draw_polygon(1, 4, x, y, 3, 4, SHAPE_FILL);
    assert(plots == 12 && x[0] == 10 && y[0] == 20 && x[4] == 0 && y[4] == 0);
    assert(polygon_min_y == 0 && polygon_max_y == 2);
    plots = 0;
    draw_ellipse(1, 12, 16, 0, 0, 3, 4, SHAPE_FILL_AND_OUTLINE);
    assert(plots > 400 && ellipse_trace_limit > 12 && ellipse_trace_limit < 32);
    /* Original EGA cache spill: slot 700 aliases the completion flag and
     * inclusive replay's slot 701 aliases the fill cursor. */
    plots = 0; ellipse_fill_x = 0;
    draw_ellipse(1, 349, 638, 0, 0, 3, 4, SHAPE_OUTLINE);
    assert(ellipse_trace_limit == 701 && ellipse_trace_done != 0 && plots == 4 * 701);
    /* Backend checks must work even with a logging callback installed. */
    video_backend = BACKEND_EGA_640X700;
    dz_srand(1); grid(1); assert((draw_y & 1) == 0);
    dz_srand(1); sine(1); assert((draw_x & 1) == 0 && (draw_y & 1) == 0);
    plots = 0;
    swap_pixel_rectangles(0, 2, 3, 10, 12, 1, 4);
    assert(plots == 4); /* two enhanced EGA rows, two writes per row */
    video_backend = BACKEND_VGA_320X400;
    /* A wrapped product, negative arithmetic shift, and signed /2 differ
     * from naive 32-bit evaluation in formula1 and Sierpinski. */
    draw_x = 300; draw_y = 200; screen_extent = 351;
    formula1_y_square_factor = 3; formula1_x_shift = 2; formula1_color_shift = 4;
    color_index_mask = UINT16_MAX;
    formula1(0);
    int16_t term = (int16_t)((int16_t)(201 * 201) * 3 - (int16_t)(300 * 4));
    assert(plot_color == ((uint16_t)dz_asr16(term, 4) & color_index_mask));
    end();

    begin();
    for (unsigned y_index = 0; y_index < 32; ++y_index)
        for (unsigned x_index = 0; x_index < 32; ++x_index)
            dz_video.vram[(200 + y_index) * 320 + 160 + x_index] = (uint8_t)(x_index + 5 * y_index);
    swap_pixel_rectangles(3, 2, 3, 10, 12, 3, 2);
    assert(dz_video.vram[203 * 320 + 162] == 12 + 5 * 13);
    assert(dz_video.vram[204 * 320 + 164] == 10 + 5 * 12);
    /* The original speculative fetch must never read beyond this one-byte
     * fixture, even when its cardinal walk immediately escapes the bitmap. */
    uint8_t one_pixel = 7;
    ball_bitmap_pixels = &one_pixel; ball_bitmap_width = ball_bitmap_height = 1;
    draw_x = 12; draw_y = 13; dz_srand(1);
    balls(1);
    int16_t saved_x = draw_x, saved_y = draw_y;
    balls(0);
    assert(draw_x == saved_x && draw_y == saved_y && dz_rand_calls() == 3);
    /* A word-wrapped coordinate can still alias a valid bitmap offset. */
    balls_sample_x = UINT16_MAX; balls_sample_y = 0;
    balls_step_x = 1; balls_step_y = 0; balls_run_progress = 0; balls_run_length = 10;
    balls_origin_x = balls_origin_y = 0; effect_parameter = 1;
    balls(0);
    assert(balls_sample_x == 0 && draw_x == 0 && draw_y == 0 && plot_color == 7);
    end();
}

static void stress(void)
{
    for (unsigned mode = 0; mode < 4; ++mode) {
        for (uint32_t seed = 1; seed <= 32; ++seed) {
            for (unsigned i = 0; i < 36; ++i) {
                begin_mode(mode < 2 ? ADAPTER_VGA : ADAPTER_EGA, (uint16_t)(mode & 1));
                uint32_t rng_seed = seed * 1999;
                dz_srand(rng_seed);
                printf("stress mode=%u seed=%" PRIu32 " image=%s\n", mode, rng_seed, alt_image_records[i].name);
                fflush(stdout);
                alt_image_records[i].draw(1);
                plot_callback(draw_symmetry);
                for (unsigned step = 0; step < 8; ++step) {
                    alt_image_records[i].draw(0);
                    plot_callback(draw_symmetry);
                }
                end();
            }
        }
    }
}

int main(void)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL};
    assert(dz_initialize());
    images();
    geometry();
    stress();
    dz_shutdown();
    return 0;
}
