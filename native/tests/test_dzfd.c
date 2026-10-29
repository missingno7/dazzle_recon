/* Exercise the actual DZFD source with module-local DZLE/DOSI dependencies.
 * These hooks keep pending.c's unrelated groups intact and this test linkable
 * before the other native modules are integrated. */
#include "dazzle.h"
#include "dzconst.h"
#include "platform/platform.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>

static void test_square(int16_t side, int16_t symmetry);
static void test_hexagon(int16_t rebuild);
static void test_swap(int16_t flip, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      int16_t width, int16_t height);
static int16_t test_poll(void);
static uint16_t test_led(uint16_t mask, uint16_t mode);
#define square test_square
#define hexagon test_hexagon
#define swap_pixel_rectangles test_swap
#define poll_graphics_keys test_poll
#define keyboard_led_flags test_led
#include "../src/app/dzfd.c"
#undef square
#undef hexagon
#undef swap_pixel_rectangles
#undef poll_graphics_keys
#undef keyboard_led_flags

/* Exact DZLE algorithms used as test dependencies (no replacement fades).
 * Original 066A:026B, 80 bytes. */
static void test_square(int16_t side, int16_t symmetry)
{
    for (int16_t column = 1; column <= side; ++column) {
        for (int16_t row = 1; row <= side; ++row) {
            plot_callback((uint16_t)symmetry);
            ++draw_y;
        }
        ++draw_x;
        draw_y = (int16_t)(draw_y - side);
        if (palette_elapsed_ticks >= palette_cycle_ticks) cycle_palette();
    }
    draw_x = (int16_t)(draw_x - side);
}

/* Original 066A:2F53, 119 bytes. */
static void test_hexagon(int16_t rebuild)
{
    if (rebuild || (draw_x > screen_extent && draw_y > screen_extent)) {
        hexagon_start_x = hexagon_vertical_limit = 1;
        draw_x = draw_y = 0;
        if (++plot_color >= color_count) plot_color = FIRST_CYCLING_COLOR;
    }
    if (draw_x < 0) {
        draw_y = 0;
        draw_x = ++hexagon_start_x;
        ++hexagon_vertical_limit;
        choose_color(0);
    } else {
        ++draw_y;
        if (draw_y > hexagon_vertical_limit) --draw_x;
    }
}

/* Original 066A:0BA5, 296 bytes. */
static void test_swap(int16_t flip, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      int16_t width, int16_t height)
{
    int16_t step = 1;
    --width;
    if (plot_callback == ega_plot_symmetric_640x700) {
        y1 = (int16_t)(dz_asr16(y1, 1) * 2);
        y2 = (int16_t)(dz_asr16(y2, 1) * 2);
        height = (int16_t)(dz_asr16((int16_t)(height - 2), 1) * 2);
        step = 2;
    } else --height;
    for (int16_t row = 0; row <= height; row = (int16_t)(row + step)) {
        int16_t py1 = (int16_t)(y1 + row);
        int16_t py2 = (int16_t)(y2 + ((flip & 1) ? height - row : row));
        for (int16_t column = 0; column <= width; ++column) {
            int16_t px1 = (int16_t)(x1 + column);
            int16_t px2 = (int16_t)(x2 + (flip < 2 ? column : width - column));
            uint16_t a = (uint8_t)read_pixel_callback(px1, py1);
            uint16_t b = (uint8_t)read_pixel_callback(px2, py2);
            plot_color = b; draw_x = px1; draw_y = py1; plot_callback(1);
            plot_color = a; draw_x = px2; draw_y = py2; plot_callback(1);
        }
        if (palette_elapsed_ticks >= palette_cycle_ticks) cycle_palette();
        if ((input_pending || exit_requested) && test_poll()) break;
        if (current_graphics_key == 'n') break;
    }
}

static unsigned led_calls[3], poll_calls;
static uint16_t test_led(uint16_t mask, uint16_t mode)
{
    assert(mode <= 2);
    ++led_calls[mode];
    return keyboard_led_flags(mask, mode);
}

static int16_t test_poll(void)
{
    ++poll_calls;
    if (input_pending) --input_pending;
    if (exit_requested) return 1;
    if (dz_bioskey(1)) {
        uint16_t key = dz_bioskey(0);
        current_graphics_key = (key & 255u) ? (uint16_t)tolower(key & 127u) : key;
        previous_graphics_key = current_graphics_key;
        if (current_graphics_key == 27) exit_requested = 1;
    }
    return (int16_t)exit_requested;
}

static uint8_t *palette_allocation, *ball_allocation;
static struct { uint32_t before; struct LineWipePoint points[250]; uint32_t after; } history;
static const uint32_t canary = UINT32_C(0x4badcafe);
static int16_t original_bounds[8][4];
static uint64_t tick_limit, inject_tick;
static uint16_t inject_key;

static void tick(void)
{
    assert(dz_virtual_ticks() < tick_limit);
    graphics_timer_tick_isr();
    if (inject_key && dz_virtual_ticks() >= inject_tick) {
        assert(dz_input_push(inject_key));
        keyboard_pending_isr();
        inject_key = 0;
    }
}

static void setup(int ega, int bios, uint16_t seed)
{
    dz_timer_tick_handler = NULL;
    dz_input_reset();
    exit_requested = input_pending = current_graphics_key = previous_graphics_key = 0;
    show_mode = show_duration_ticks = display_pan_state = semi_auto_mode = 0;
    palette_alternate_mode = palette_random_mode = palette_fixed_mode = 0;
    targa64_mode = use_bios_palette = split_vga_palette_updates = use_8086_palette_io = 0;
    background_fill_word = page_paragraph = palette_cycle_offset = 0;
    display_freeze_active = display_freeze_cycle_requested = 0;
    draw_delay_count = 1;
    palette_step = 3;
    auto_fade_pause_ticks = 240;
    video_adapter = ega ? ADAPTER_EGA : ADAPTER_VGA;
    bios_resolution_mode = (uint16_t)bios;
    dz_srand(seed);
    configure_video_backend(1);
    /* Guard the allocations supplied by DOSI. Its current foundation defers
     * linewipe/ball generation, so a reproducible nonzero bitmap is provided. */
    free(palette_buffer);
    palette_allocation = malloc((size_t)palette_buffer_bytes + 32u);
    assert(palette_allocation);
    memset(palette_allocation, 0xa5, (size_t)palette_buffer_bytes + 32u);
    palette_buffer = palette_allocation + 16;
    initialize_math_tables();
    palette_build_callback(0);
    mode_setup_callback(1);
    dz_video_set_mode(ega ? DZ_MODE_EGA_350 : bios ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400, 1);
    palette_upload_callback(palette_buffer + palette_cycle_offset);
    memset(led_calls, 0, sizeof led_calls);
    poll_calls = 0;
    history.before = history.after = canary;
    memset(history.points, 0, sizeof history.points);
    linewipe_history = history.points;
    size_t pixels = (size_t)ball_bitmap_width * ball_bitmap_height;
    ball_allocation = malloc(pixels + 32u);
    assert(ball_allocation);
    memset(ball_allocation, 0xa5, pixels + 32u);
    ball_bitmap_pixels = ball_allocation + 16;
    for (size_t i = 0; i < pixels; ++i)
        ball_bitmap_pixels[i] = (uint8_t)(i % 7u ? 3u + i % (color_count - 3u) : 0);
    /* Draw an asymmetric, nonempty source through the real DVDT kernel. */
    for (int16_t y = (int16_t)-screen_half_height; y < screen_half_height; ++y)
        for (int16_t x = (int16_t)-screen_half_width; x < screen_half_width; ++x) {
            draw_x = x; draw_y = y;
            plot_color = (uint16_t)(3 + ((x + screen_half_width) / 7 +
                         (y + screen_half_height) / 11) % (color_count - 3));
            plot_callback(0);
        }
    for (unsigned i = 0; i < 24; ++i) fade_records[i].enabled = 1;
    memcpy(countdown_segment_bounds, original_bounds, sizeof original_bounds);
    memset(firefly_path_state, 0, sizeof firefly_path_state);
    memset(fireworks_particles, 0, sizeof fireworks_particles);
    memset(fireworks_active, 0, sizeof fireworks_active);
    ega_color_substep = image_work_count = 0;
    draw_x = draw_y = 0;
    plot_color = FIRST_CYCLING_COLOR;
    display_cleared = 0;
    palette_elapsed_ticks = countdown_ticks = 0;
    dz_clock_reset();
    tick_limit = 100000;
    inject_key = 0;
    dz_timer_tick_handler = tick;
    dz_keyboard_irq_handler = keyboard_pending_isr;
}

static void teardown(void)
{
    for (unsigned i = 0; i < 16; ++i) {
        assert(palette_allocation[i] == 0xa5);
        assert(palette_buffer[palette_buffer_bytes + i] == 0xa5);
        assert(ball_allocation[i] == 0xa5);
        assert(ball_bitmap_pixels[(size_t)ball_bitmap_width * ball_bitmap_height + i] == 0xa5);
    }
    assert(history.before == canary && history.after == canary);
    assert(dz_video.display_start_row < DZ_VRAM_PLANE_BYTES / 80u);
    assert(palette_cycle_offset < palette_buffer_bytes);
    if (video_adapter == ADAPTER_EGA)
        for (size_t i = 0; i < DZ_VRAM_PIXELS; ++i) assert(dz_video.vram[i] < 16);
    free(palette_allocation); palette_buffer = NULL;
    free(ball_allocation); ball_bitmap_pixels = NULL;
    linewipe_history = NULL;
    dz_timer_tick_handler = NULL;
}

struct receipt { uint32_t fb, palette; uint64_t ticks, rng; int16_t cleared, repeat; };

static uint32_t fnv_bytes(const uint8_t *data, size_t size)
{
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < size; ++i) hash = (hash ^ data[i]) * UINT32_C(16777619);
    return hash;
}

static uint32_t active_palette_hash(void)
{
    if (video_adapter == ADAPTER_VGA) return dz_palette_hash();
    uint8_t colors[17];
    memcpy(colors, dz_video.ega_attr, 16); colors[16] = dz_video.ega_border;
    return fnv_bytes(colors, sizeof colors);
}

static struct receipt run_key(char key, int ega, int bios, uint16_t seed)
{
    setup(ega, bios, seed);
    current_graphics_key = (uint16_t)key;
    struct receipt r = {0};
    r.repeat = dispatch_fade_key();
    r.fb = dz_framebuffer_hash(); r.palette = active_palette_hash();
    r.ticks = dz_virtual_ticks(); r.rng = dz_rand_calls(); r.cleared = display_cleared;
    assert(!exit_requested);
    if (key == 'g')
        assert(dz_video.display_start_row == (page_paragraph ? 401u : 0u));
    else assert(dz_video.display_start_row == 0);
    assert(dz_video.line_compare >= (unsigned)dz_video.height);
    printf("%s%s %c seed=%u fb=%08" PRIx32 " pal=%08" PRIx32
           " ticks=%" PRIu64 " rng=%" PRIu64 " cleared=%d repeat=%d\n",
           ega ? "EGA" : "VGA", bios ? "/T" : "", key, seed, r.fb, r.palette, r.ticks, r.rng, r.cleared, r.repeat);
    fflush(stdout);
    teardown();
    return r;
}

static void test_all_fades(void)
{
    const char *keys = "abcdefgijklmoqrstuvwxyz";
    /* FNV of the ordered, little-endian 64-bit receipt fields below.
     * Seed 1; native regression receipts, not DOS differential evidence. */
    const uint32_t expected[4] = {UINT32_C(0xb4684d97), UINT32_C(0x8d86c7b5),
                                  UINT32_C(0x7c84ddd8), UINT32_C(0x385cbfbe)};
    for (int mode = 0; mode < 4; ++mode) {
        int ega = mode / 2, bios = mode % 2;
        uint32_t aggregate = UINT32_C(2166136261);
        for (const char *key = keys; *key; ++key) {
            if (*key == ' ' || (ega && strchr("gjuvz", *key))) continue;
            struct receipt a = run_key(*key, ega, bios, 1);
            struct receipt b = run_key(*key, ega, bios, 1);
            assert(a.fb == b.fb && a.palette == b.palette && a.ticks == b.ticks &&
                   a.rng == b.rng && a.cleared == b.cleared && a.repeat == b.repeat);
            /* Pin the whole receipt sequence without depending on struct padding. */
            uint64_t fields[] = {(uint8_t)*key, a.fb, a.palette, a.ticks, a.rng,
                                 (uint16_t)a.cleared, (uint16_t)a.repeat};
            for (unsigned i = 0; i < sizeof fields / sizeof fields[0]; ++i)
                for (unsigned byte = 0; byte < 8; ++byte)
                    aggregate = (aggregate ^ (uint8_t)(fields[i] >> (byte * 8u))) * UINT32_C(16777619);
        }
        printf("aggregate mode=%d %08" PRIx32 "\n", mode, aggregate);
        assert(aggregate == expected[mode]);
    }
}

static void test_variants_and_interrupts(void)
{
    for (int ega = 0; ega <= 1; ++ega) {
        for (uint16_t seed = 2; seed <= 5; ++seed) {
            setup(ega, 0, seed);
            fade_wipe(0);
            assert(display_cleared);
            teardown();
            setup(ega, 0, seed);
            if (!ega && seed == 2) background_fill_word = 0x0501; /* striped erase */
            firefly_transition(1);
            assert(firefly_path_state[0] >= 6 && firefly_path_state[0] <= 9);
            teardown();
            setup(ega, 0, seed);
            randomize_helper(2);
            randomize_helper(0);
            for (unsigned i = 0; i < 4; ++i) assert(!fireworks_active[i]);
            teardown();
            setup(ega, 0, seed);
            current_graphics_key = 'b';
            dispatch_fade_key();
            teardown();
        }
        const char *keys = "abcdefgijklmoqrstuvwxyz";
        for (const char *key = keys; *key; ++key) {
            if (ega && strchr("gjuvz", *key)) continue;
            setup(ega, 0, 1);
            current_graphics_key = (uint16_t)*key;
            inject_tick = 1; inject_key = 'n';
            dispatch_fade_key();
            if (inject_key || current_graphics_key != 'n')
                fprintf(stderr, "interrupt %s %c: ticks=%" PRIu64 " key=%u pending=%u polls=%u\n",
                        ega ? "EGA" : "VGA", *key, dz_virtual_ticks(), current_graphics_key, inject_key, poll_calls);
            assert(!inject_key && current_graphics_key == 'n');
            assert(dz_virtual_ticks() < 1000); /* fireworks checks after whole bursts */
            teardown();
        }
    }
    /* Empty blister bitmaps and static palettes must still advance time. */
    setup(0, 0, 1);
    memset(ball_bitmap_pixels, 0, (size_t)ball_bitmap_width * ball_bitmap_height);
    display_freeze_active = 1;
    fade_blister();
    assert(dz_virtual_ticks() >= 90 && dz_virtual_ticks() < 400);
    teardown();
    setup(0, 0, 1);
    display_freeze_active = 1;
    mesh_transition();
    assert(fade_timer_ticks == 0);
    spiral_transition();
    assert(fade_timer_ticks == 0);
    teardown();
}

static void test_waits_and_palette(void)
{
    setup(0, 0, 1);
    display_freeze_active = 1; /* cycling alone must not be required for progress */
    wait_fade_tick(32767, 0);
    assert(dz_virtual_ticks() == 120 && fade_timer_ticks == 0);
    current_graphics_key = 'n';
    wait_fade_tick(40, 1);
    assert(fade_timer_ticks == 40);
    current_graphics_key = 0;
    inject_tick = dz_virtual_ticks() + 2; inject_key = 0x306e;
    wait_fade_tick(40, 1);
    assert(current_graphics_key == 'n' && fade_timer_ticks > 0 && poll_calls);
    current_graphics_key = 0;
    fade_dac_to_black(1);
    for (unsigned i = 0; i < palette_buffer_bytes; ++i) assert(palette_buffer[i] == 0);
    assert(display_cleared);
    teardown();
    for (int ega = 0; ega <= 1; ++ega) {
        setup(ega, 0, 1);
        background_fill_word = 7;
        palette_cycle_offset = (uint16_t)(ega ? palette_entries - 1 : palette_entries * 3 - 3);
        uint16_t begin = palette_cycle_offset;
        /* Select the moving-window VGA branch, rather than the DAC fallback. */
        uint16_t seed = 1;
        do { dz_srand(seed++); } while (!dz_random(2));
        dz_srand((uint16_t)(seed - 1));
        uint32_t pixels = dz_framebuffer_hash();
        fade_palette_to_background();
        unsigned extent = (unsigned)color_count * (ega ? 1u : 3u);
        for (unsigned i = 0; i < extent; ++i)
            assert(palette_buffer[(begin + i) % palette_buffer_bytes] == 7);
        assert(dz_framebuffer_hash() == pixels);
        teardown();
    }
}

static void test_display_boundary(void)
{
    for (int ega = 0; ega <= 1; ++ega) {
        setup(ega, 0, 1);
        draw_delay_count = 9;
        current_graphics_key = 'p';
        display_boundary_fade_and_pan();
        assert(display_pan_state == 2 && page_paragraph == (ega ? 1750 : 2000));
        assert(draw_delay_count == 1 && saved_pan_draw_delay == 9 && led_calls[0]);
        size_t page_pixels = (size_t)dz_video.width * (unsigned)dz_video.height;
        uint32_t visible = fnv_bytes(dz_video.vram, page_pixels);
        uint32_t hidden = fnv_bytes(dz_video.vram + page_pixels, page_pixels);
        draw_x = draw_y = 0; plot_color = 15; plot_callback(0);
        assert(fnv_bytes(dz_video.vram, page_pixels) == visible);
        assert(fnv_bytes(dz_video.vram + page_pixels, page_pixels) != hidden);
        current_graphics_key = 'n';
        display_boundary_fade_and_pan();
        assert(!display_pan_state && !page_paragraph && draw_delay_count == 9);
        assert(dz_video.display_start_row == 0 && dz_video.line_compare >= (unsigned)dz_video.height);
        assert(!palette_elapsed_ticks && !current_graphics_key && !previous_graphics_key);
        assert(display_cleared && led_calls[1]);
        teardown();
    }
    setup(0, 0, 1);
    /* H reloads the seven-tick hold; a queued N releases and clears it. */
    auto_fade_pause_ticks = 2;
    current_graphics_key = 'h'; inject_tick = 30; inject_key = 0x306e;
    display_boundary_fade_and_pan();
    assert(dz_virtual_ticks() >= 30 && !inject_key && display_cleared);
    teardown();
    setup(0, 0, 1);
    semi_auto_mode = 1;
    current_graphics_key = 'p';
    display_boundary_fade_and_pan();
    assert(!display_pan_state && !page_paragraph); /* P is unsupported in Semi-Auto */
    teardown();
    setup(0, 0, 1);
    show_mode = show_duration_ticks = 1;
    current_graphics_key = 'n';
    dz_delay_ms(60);
    display_boundary_fade_and_pan();
    assert(exit_requested && !show_duration_ticks);
    teardown();
}

static void test_pan_and_split(void)
{
    for (int ega = 0; ega <= 1; ++ega) {
        for (int bios = 0; bios <= 1; ++bios) {
            setup(ega, bios, 1);
            int16_t height = (int16_t)(ega ? 350 : 400);
            animate_vertical_display_start(0, height);
            assert(dz_video.display_start_row == (unsigned)height);
            animate_vertical_display_start(height, 0);
            assert(!dz_video.display_start_row);
            animate_line_compare_split(1);
            assert(dz_video.line_compare == 0);
            animate_line_compare_split(0);
            /* Original forward animation ends at 349/399 (also in /T).
             * The next mode setup disables line compare. */
            assert(dz_video.line_compare == (unsigned)(ega ? 349 : 399));
            mode_setup_callback(1);
            assert(dz_video.line_compare >= (unsigned)dz_video.height);
            targa64_mode = 1;
            unsigned split = dz_video.line_compare;
            animate_line_compare_split(1);
            assert(dz_video.line_compare == split);
            teardown();
        }
    }
}

int main(void)
{
    dz_options = (struct dz_options){1, 1, 1, 1.0, NULL};
    assert(dz_initialize());
    memcpy(original_bounds, countdown_segment_bounds, sizeof original_bounds);
    test_all_fades();
    test_variants_and_interrupts();
    test_waits_and_palette();
    test_display_boundary();
    test_pan_and_split();
    dz_shutdown();
    return 0;
}
