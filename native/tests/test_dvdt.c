/* Focused headless tests. Expected values are an ASM MODEL transcribed from
 * asm/dvdt.asm (including its quirks), NOT captures from the native candidate.
 * build/workers/n-dvdt/plasma_model.py independently supplies plasma constants.
 * A later DOS reference harness must check these expectations against execution.
 *
 * This file provides the tiny platform/application fixture needed to link only
 * dvdt.c. A static dazzle_core archive also works: no platform/app objects need
 * to be pulled in. These are unit tests, not an SDL integration test.
 */
#include "dazzle.h"
#include "dvdt.h"
#include <assert.h>
#include <stdio.h>

struct dz_video dz_video;
uint16_t background_fill_word, countdown_ticks, current_graphics_key;
uint16_t palette_cycle_ticks, draw_delay_count, exit_requested, mouse_callback_installed;
uint16_t palette_base_green, palette_base_blue, color_count, mouse_abort_pending;
uint16_t palette_elapsed_ticks, input_pending, show_duration_ticks;
int16_t screen_half_height, screen_half_width, ega_border_color, draw_x, draw_y;
uint8_t *palette_buffer;
uint16_t *current_image_header;
void (*plot_callback)(uint16_t);
uint16_t (*read_pixel_callback)(int16_t, int16_t);

static uint64_t charged;
static unsigned retraces, mode_calls, key_peeks, cycle_calls, poll_calls, rand_calls;
static uint16_t queued_key;
static uint32_t fixed_bios_time;
static int16_t poll_result;
static uint8_t *watch_palette;

static void check_reserved_substitution(void)
{
    if (watch_palette) {
        assert(watch_palette[0] == (uint8_t)background_fill_word);
        assert(watch_palette[1] == (uint8_t)palette_base_green);
        assert(watch_palette[2] == (uint8_t)palette_base_blue);
        assert(watch_palette[16] == (uint8_t)ega_border_color);
    }
}

void dz_charge(uint32_t cost) { charged += cost; check_reserved_substitution(); }
void dz_wait_retrace(void) { ++retraces; check_reserved_substitution(); }
uint32_t dz_bios_time(void) { return fixed_bios_time; }
int16_t dz_rand(void) { ++rand_calls; return 0x1234; }
uint16_t dz_bioskey(int command)
{
    assert(command == 1);
    ++key_peeks;
    return queued_key;
}
void dz_video_set_mode(enum dz_video_mode mode, int clear_memory)
{
    ++mode_calls;
    dz_video.mode = mode;
    dz_video.width = mode == DZ_MODE_EGA_350 ? 640 : mode == DZ_MODE_TEXT ? 80 : 320;
    dz_video.height = mode == DZ_MODE_EGA_350 ? 350 : mode == DZ_MODE_TEXT ? 25
        : mode == DZ_MODE_VGA_200 ? 200 : 400;
    dz_video.pixels_per_byte = mode == DZ_MODE_EGA_350 ? 8 : mode == DZ_MODE_VGA_400 ? 4 : 1;
    dz_video.display_start_row = 0;
    dz_video.line_compare = 1023;
    dz_video.ega_xor = 0;
    if (clear_memory)
        memset(dz_video.vram, 0, sizeof dz_video.vram);
}
void cycle_palette(void) { ++cycle_calls; palette_elapsed_ticks = 0; }
int16_t poll_graphics_keys(void) { ++poll_calls; return poll_result; }

struct kernel {
    void (*plot)(uint16_t);
    uint16_t (*read)(int16_t, int16_t);
    enum dz_video_mode mode;
    int16_t half_width, half_height;
    unsigned scale;
    int halve_y;
};

static const struct kernel kernels[] = {
    { ega_plot_symmetric_640x700, ega_read_pixel_640x700, DZ_MODE_EGA_350, 320, 350, 8, 1 },
    { ega_plot_symmetric_640x350, ega_read_pixel_640x350, DZ_MODE_EGA_350, 320, 175, 8, 0 },
    { vga_plot_symmetric_320x400, vga_read_pixel_320x400, DZ_MODE_VGA_400, 160, 200, 4, 0 },
    { vga_plot_symmetric_320x200, vga_read_pixel_320x200, DZ_MODE_VGA_200, 160, 100, 1, 0 }
};

static void reset_kernel(const struct kernel *kernel)
{
    memset(&dz_video, 0, sizeof dz_video);
    dz_video_set_mode(kernel->mode, 1);
    screen_half_width = kernel->half_width;
    screen_half_height = kernel->half_height;
    page_paragraph = 0;
    draw_delay_count = 1;
    plot_color = 11;
    draw_x = draw_y = 0;
    charged = 0;
    retraces = mode_calls = 0;
}

static size_t pixel_at(const struct kernel *kernel, int x, int y)
{
    int px = screen_half_width + x;
    int py = screen_half_height + y;
    if (kernel->halve_y)
        py /= 2;
    return (size_t)page_paragraph * 16u * kernel->scale +
        (size_t)py * (unsigned)dz_video.width + (unsigned)px;
}

static unsigned colored_pixels(void)
{
    unsigned count = 0;
    for (size_t i = 0; i < sizeof dz_video.vram; ++i)
        count += dz_video.vram[i] != 0;
    return count;
}

static void test_symmetry_and_edges(void)
{
    for (unsigned k = 0; k < sizeof kernels / sizeof *kernels; ++k) {
        const struct kernel *kernel = &kernels[k];
        for (uint16_t symmetry = 0; symmetry <= 3; ++symmetry) {
            reset_kernel(kernel);
            draw_x = 4; draw_y = 6;
            kernel->plot(symmetry);
            assert(colored_pixels() == (symmetry == 3 ? 8u : (symmetry & 1u) ? 4u : 1u));
            assert(dz_video.vram[pixel_at(kernel, 4, 6)] == 11);
            if (symmetry & 1u) {
                assert(dz_video.vram[pixel_at(kernel, 4, -6)] == 11);
                assert(dz_video.vram[pixel_at(kernel, -4, 6)] == 11);
                assert(dz_video.vram[pixel_at(kernel, -4, -6)] == 11);
            }
            if (symmetry == 3) {
                assert(dz_video.vram[pixel_at(kernel, 6, 4)] == 11);
                assert(dz_video.vram[pixel_at(kernel, 6, -4)] == 11);
                assert(dz_video.vram[pixel_at(kernel, -6, 4)] == 11);
                assert(dz_video.vram[pixel_at(kernel, -6, -4)] == 11);
            }
            assert(charged == 2); /* One base unit + one masked LOOP iteration. */
        }
        reset_kernel(kernel);
        kernel->plot(0);
        assert(colored_pixels() == 1); /* Center. */
        assert(kernel->read(0, 0) == 11);
        reset_kernel(kernel);
        draw_y = 6;
        kernel->plot(1);
        assert(colored_pixels() == 2); /* Vertical axis, duplicate writes retained. */
        reset_kernel(kernel);
        draw_x = 4;
        kernel->plot(1);
        assert(colored_pixels() == 2); /* Horizontal axis. */

        reset_kernel(kernel);
        draw_x = kernel->half_width;
        kernel->plot(0);
        assert(colored_pixels() == 0 && symmetry_edge == 1);
        kernel->plot(1);
        assert(colored_pixels() == 1); /* Plus edge omitted; reflected X=0 survives. */
        assert(dz_video.vram[pixel_at(kernel, -kernel->half_width, 0)] == 11);
        reset_kernel(kernel);
        draw_y = kernel->half_height;
        draw_x = 4;
        kernel->plot(1);
        assert(colored_pixels() == 2 && symmetry_y_edge == 1);
        assert(dz_video.vram[pixel_at(kernel, -4, -kernel->half_height)] == 11);
        assert(dz_video.vram[pixel_at(kernel, 4, -kernel->half_height)] == 11);

        reset_kernel(kernel);
        draw_x = (int16_t)(-kernel->half_width - 1);
        kernel->plot(1);
        assert(colored_pixels() == 0);
        draw_x = 0; draw_y = (int16_t)(-kernel->half_height - 1);
        kernel->plot(1);
        assert(colored_pixels() == 0);
        draw_x = INT16_MAX; draw_y = 0;
        kernel->plot(1);
        assert(colored_pixels() == 0); /* ADD wraps to a negative signed word. */
        draw_x = INT16_MIN;
        kernel->plot(1);
        assert(colored_pixels() == 0);

        reset_kernel(kernel);
        draw_x = (int16_t)-kernel->half_width;
        kernel->plot(1);
        /* Negative endpoint does not set the positive-edge flag: xm==width
         * is actually written and aliases the next physical row at X=0. */
        assert(colored_pixels() == 2);
        size_t left = pixel_at(kernel, -kernel->half_width, 0);
        assert(dz_video.vram[left] == 11);
        assert(dz_video.vram[left + (unsigned)dz_video.width] == 11);
    }
    reset_kernel(&kernels[2]);
    draw_x = 161; draw_y = 0;
    vga_plot_symmetric_320x400(3);
    assert(colored_pixels() == 2); /* Failed first pass still tries swapped axes. */
    assert(dz_video.vram[39u * 320u + 160u] == 11);
    assert(dz_video.vram[361u * 320u + 160u] == 11);
}

static void test_ega_parity_xor_and_delays(void)
{
    reset_kernel(&kernels[0]);
    draw_x = 2; draw_y = 1;
    ega_plot_symmetric_640x700(1);
    assert(colored_pixels() == 0);
    ega_plot_symmetric_640x700(3);
    assert(colored_pixels() == 4); /* Swapped Y=2 is even. */
    assert(dz_video.vram[176u * 640u + 321u] == 11);
    assert(dz_video.vram[174u * 640u + 319u] == 11);
    reset_kernel(&kernels[0]);
    draw_x = draw_y = 1;
    ega_plot_symmetric_640x700(3);
    assert(colored_pixels() == 0);
    screen_half_height = 349; /* Test translated parity, rather than draw_y parity. */
    ega_plot_symmetric_640x700(1);
    assert(colored_pixels() == 4);

    for (unsigned k = 0; k < 2; ++k) {
        const struct kernel *kernel = &kernels[k];
        reset_kernel(kernel);
        dz_video.ega_xor = 1;
        draw_x = 4; draw_y = 6;
        plot_color = 0x25; /* Only low four bits are rasterized. */
        kernel->plot(1);
        assert(kernel->read(4, 6) == 5);
        kernel->plot(1);
        assert(colored_pixels() == 0);
        draw_x = draw_y = 0;
        kernel->plot(1);
        assert(colored_pixels() == 0); /* Four writes cancel at center. */
        kernel->plot(0);
        assert(kernel->read(0, 0) == 5);
        memset(dz_video.vram, 0, sizeof dz_video.vram);
        draw_x = 0; draw_y = 6;
        kernel->plot(1);
        assert(colored_pixels() == 0); /* Each vertical-axis pixel hit twice. */
        draw_x = 4; draw_y = 0;
        kernel->plot(1);
        assert(colored_pixels() == 0);
        draw_x = draw_y = 4;
        kernel->plot(3);
        assert(colored_pixels() == 0); /* Identical swapped pass cancels. */
    }
    reset_kernel(&kernels[2]);
    dz_video.ega_xor = 1;
    vga_plot_symmetric_320x400(1);
    assert(vga_read_pixel_320x400(0, 0) == 11); /* VGA never XORs. */
    static const uint16_t delays[] = { 0, 1, 0x2000, 0x2001, 0xffff };
    static const uint32_t costs[] = { 8193, 2, 8193, 2, 8192 };
    for (unsigned i = 0; i < sizeof delays / sizeof *delays; ++i) {
        charged = 0;
        draw_delay_count = delays[i];
        vga_plot_symmetric_320x400(0);
        assert(charged == costs[i]);
    }
}

static void test_reads_and_pages(void)
{
    for (unsigned k = 0; k < sizeof kernels / sizeof *kernels; ++k) {
        const struct kernel *kernel = &kernels[k];
        reset_kernel(kernel);
        page_paragraph = kernel->scale == 8 ? 0x6d6 : kernel->scale == 4 ? 0x7d0 : 0x10;
        plot_color = kernel->scale == 8 ? 0x1d : 0xad;
        draw_x = 4; draw_y = 6;
        kernel->plot(0);
        assert(kernel->read(4, 6) == (kernel->scale == 8 ? 13u : 173u));
        assert(dz_video.vram[pixel_at(kernel, 4, 6)] == (kernel->scale == 8 ? 13u : 173u));
        if (kernel->halve_y)
            assert(kernel->read(4, 7) == 13); /* Odd reads halve instead of rejecting. */
        assert(colored_pixels() == 1);
        page_paragraph = 0;
        assert(kernel->read(4, 6) == 0);
        memset(dz_video.vram, 0x5a, sizeof dz_video.vram);
        assert(kernel->read((int16_t)(-kernel->half_width - 1), 0) == 0);
        assert(kernel->read(kernel->half_width, 0) == 0);
        assert(kernel->read(0, (int16_t)(-kernel->half_height - 1)) == 0);
        assert(kernel->read(0, kernel->half_height) == 0);
        assert(kernel->read(INT16_MAX, INT16_MIN) == 0);
        page_paragraph = 0x1000; /* Beyond the modeled 64 KiB aperture. */
        assert(kernel->read(0, 0) == 0);
        kernel->plot(0); /* Must also be memory-safe. */
    }
    /* Plane/bit selection spans every VGA lane and every EGA pixel bit. */
    for (unsigned k = 0; k < sizeof kernels / sizeof *kernels; ++k) {
        reset_kernel(&kernels[k]);
        for (int16_t x = 0; x < 16; ++x) {
            draw_x = x;
            plot_color = (uint16_t)x;
            kernels[k].plot(0);
        }
        for (int16_t x = 0; x < 16; ++x)
            assert(kernels[k].read(x, 0) == (uint16_t)x);
    }
}

static void test_planar_rows(void)
{
    reset_kernel(&kernels[1]);
    memset(dz_video.vram, 0xff, sizeof dz_video.vram);
    ega_init_row_table_and_mode();
    assert(dz_video.mode == DZ_MODE_EGA_350 && colored_pixels() == 0);
    for (unsigned row = 0; row < 350; ++row)
        assert(ega_row_offsets[row] == 80u * row);
    uint8_t planes[] = { 0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08 };
    static const uint8_t expected[] = { 1, 2, 4, 8, 0, 0, 0, 0, 0, 0, 0, 0, 8, 4, 2, 1 };
    page_paragraph = 0x6d6; /* Row transfers ignore the drawing page. */
    ega_write_planar_row(planes, 2, 349);
    assert(memcmp(dz_video.vram + 349u * 640u, expected, sizeof expected) == 0);
    assert(colored_pixels() == 8);
    ega_write_planar_row(planes, 2, 350);
    ega_write_planar_row(planes, 2, 0xffff);
    assert(colored_pixels() == 8);
}

static void test_clear_wrap_and_modes(void)
{
    reset_kernel(&kernels[2]);
    memset(dz_video.vram, 0xa5, sizeof dz_video.vram);
    clear_vram_words(0x3412, 0xfa00, 0);
    assert(retraces == 1);
    for (size_t i = 0; i < 262144; ++i)
        assert(dz_video.vram[i] == ((i / 4u) & 1u ? 0x34 : 0x12));
    assert(dz_video.vram[262144] == 0xa5); /* DI wrap, no write beyond the plane. */
    memset(dz_video.vram, 0xa5, sizeof dz_video.vram);
    clear_vram_words(0x3412, 0x3e80, 0x7d0);
    assert(dz_video.vram[127999] == 0xa5);
    assert(dz_video.vram[128000] == 0x12 && dz_video.vram[255999] == 0x34);
    assert(dz_video.vram[256000] == 0xa5);
    memset(dz_video.vram, 0xa5, sizeof dz_video.vram);
    clear_vram_words(0x3412, 32769, 1);
    assert(dz_video.vram[0] == 0xa5 && dz_video.vram[63] == 0xa5);
    assert(dz_video.vram[64] == 0x12 && dz_video.vram[68] == 0x34);
    /* DI wraps within the fixed page segment, not (base+DI) modulo 65536. */
    uint8_t before = dz_video.vram[64];
    clear_vram_words(0xffff, 0, 1);
    assert(dz_video.vram[64] == before);

    reset_kernel(&kernels[3]);
    memset(dz_video.vram, 0xa5, sizeof dz_video.vram);
    clear_vram_words(0x3412, 32000, 0);
    assert(dz_video.vram[0] == 0x12 && dz_video.vram[63999] == 0x34);
    assert(dz_video.vram[64000] == 0xa5);
    reset_kernel(&kernels[1]);
    clear_vram_words(0x1a25, 1, 0);
    for (unsigned i = 0; i < 16; ++i)
        assert(dz_video.vram[i] == (i < 8 ? 5 : 10));

    uint8_t palette[27];
    for (unsigned i = 0; i < sizeof palette; ++i)
        palette[i] = (uint8_t)(i + 65u);
    palette_buffer = palette;
    background_fill_word = 0x3412;
    reset_kernel(&kernels[2]);
    vga_set_mode_320x400(1);
    assert(video_mode_cookie == 0x43 && dz_video.mode == DZ_MODE_VGA_400);
    assert(dz_video.vram[0] == 0x12 && dz_video.vram[7] == 0x34);
    assert(dz_video.vram[262143] == 0x34);
    for (unsigned i = 0; i < 27; ++i)
        assert(dz_video.dac[i / 3u][i % 3u] == (palette[i] & 63u));
    for (unsigned i = 9; i < 256; ++i)
        for (unsigned channel = 0; channel < 3; ++channel)
            assert(dz_video.dac[i][channel] == 0); /* DAC read before background fill. */
    dz_video.vram[30] = 59;
    vga_set_mode_320x400(0);
    assert(dz_video.vram[30] == 59 && dz_video.dac[10][0] == 59);
    vga_set_mode_320x200(1);
    assert(video_mode_cookie == 0x44 && dz_video.mode == DZ_MODE_VGA_200);
    assert(dz_video.vram[63999] == 0x34 && dz_video.vram[64000] == 0);
    dz_video.vram[0] = 9;
    vga_set_mode_320x200(0);
    assert(dz_video.vram[0] == 9);
    ega_set_mode(0);
    assert(video_mode_cookie == 0x41 && dz_video.mode == DZ_MODE_EGA_350);
    assert(dz_video.vram[0] == 9);
    unsigned calls = mode_calls;
    ega_set_mode(2);
    assert(mode_calls == calls && dz_video.vram[0] == 9);
    ega_set_mode(1);
    assert(colored_pixels() == 0);
    palette_buffer = NULL;
}

static void test_display_and_obsolete_services(void)
{
    reset_kernel(&kernels[2]);
    set_display_start(0, 400);
    assert(dz_video.display_start_row == 400 && retraces == 1);
    set_display_start(639, 5);
    assert(dz_video.display_start_row == 5); /* Residual bytes not in platform model. */
    set_display_start(640, 5);
    assert(dz_video.display_start_row == 6);
    set_display_start(0, 819);
    assert(dz_video.display_start_row == 819);
    set_display_start(0, 820);
    assert(dz_video.display_start_row == 0); /* Low-word MUL/ADD wrap. */
    ega_set_line_compare(0xffff);
    assert(dz_video.line_compare == 511);
    ega_set_line_compare(350);
    assert(dz_video.line_compare == 350);
    ega_set_line_compare(512);
    assert(dz_video.line_compare == 0);
    vga_set_line_compare(0xffff);
    assert(dz_video.line_compare == 1023);
    vga_set_line_compare(700);
    assert(dz_video.line_compare == 700);
    assert(retraces == 10);

    dz_video_set_mode(DZ_MODE_TEXT, 1);
    dz_video.cursor_visible = 0;
    save_video_state();
    assert(saved_bios_mode == 3 && saved_text_last_row == 24);
    assert(saved_cursor_shape == 0x2607);
    dz_video_set_mode(DZ_MODE_VGA_400, 1);
    restore_video_state(0);
    assert(dz_video.mode == DZ_MODE_VGA_400);
    restore_video_state(1);
    assert(dz_video.mode == DZ_MODE_TEXT && !dz_video.cursor_visible);
    dz_video.cursor_visible = 1;
    save_video_state();
    dz_video_set_mode(DZ_MODE_EGA_350, 1);
    restore_video_state(1);
    assert(dz_video.mode == DZ_MODE_TEXT && dz_video.cursor_visible);

    force_ega = force_vga = 0;
    detect_video_adapter();
    assert(video_adapter == 8);
    force_ega = 1;
    detect_video_adapter();
    assert(video_adapter == 4);
    force_vga = 1;
    detect_video_adapter();
    assert(video_adapter == 8);
    assert(cpu_has_post_8086_shifts() == 1 && dvdt_seg() == 0);
    assert(read_cmos_byte(0x12) == 0 && read_cmos_byte(0xff) == 0);
    disk_count = 0;
    park_disk_heads();
    assert(disk_count == 0);
}

static void test_palette_writers(void)
{
    uint8_t palette[759], old[759];
    for (unsigned i = 0; i < sizeof palette; ++i)
        palette[i] = (uint8_t)(i * 17u + i / 3u);
    memcpy(old, palette, sizeof old);
    void (*writers[])(uint8_t *) = { vga_upload_palette_bios, vga_upload_palette_bios_split,
        vga_upload_palette_rep, vga_upload_palette_rep_split,
        vga_upload_palette_8086, vga_upload_palette_8086_split };
    for (unsigned writer = 0; writer < sizeof writers / sizeof *writers; ++writer) {
        memset(dz_video.dac, 7, sizeof dz_video.dac);
        retraces = 0;
        writers[writer](palette);
        assert(retraces == (writer < 2 ? 0u : (writer & 1u) ? 2u : 1u));
        for (unsigned index = 0; index < 256; ++index) {
            for (unsigned channel = 0; channel < 3; ++channel) {
                unsigned source = index >= 3 ? index - 3u : 0;
                if (writer == 1 && index >= 129)
                    source = index - 129u + 42u; /* ASM ADD DX,007Eh, in BYTES. */
                uint8_t expected = index < 3 ? 7 : palette[3u * source + channel] & 63u;
                assert(dz_video.dac[index][channel] == expected);
            }
        }
        assert(memcmp(old, palette, sizeof old) == 0);
    }
    background_fill_word = 0x1285;
    palette_base_green = 0x52;
    palette_base_blue = 0x63;
    ega_border_color = 0x74;
    void (*ega_writers[])(uint8_t *) = { ega_upload_palette_bios, ega_upload_palette_ports };
    for (unsigned writer = 0; writer < 2; ++writer) {
        retraces = 0;
        watch_palette = palette;
        ega_writers[writer](palette);
        watch_palette = NULL;
        assert(retraces == writer);
        assert(dz_video.ega_attr[0] == 5 && dz_video.ega_attr[1] == 18);
        assert(dz_video.ega_attr[2] == 35 && dz_video.ega_border == 52);
        for (unsigned i = 3; i < 16; ++i)
            assert(dz_video.ega_attr[i] == (old[i] & 63u));
        assert(memcmp(old, palette, sizeof old) == 0); /* Restores all four substitutions. */
    }
}

static void test_rgb_thresholds(void)
{
    static const uint8_t values[] = { 0, 0x33, 0x34, 0x77, 0x78, 0xbb, 0xbc, 0xff };
    static const uint8_t red_codes[] = { 0, 0, 32, 32, 4, 4, 36, 36 };
    static const uint8_t green_codes[] = { 0, 0, 16, 16, 2, 2, 18, 18 };
    static const uint8_t blue_codes[] = { 0, 0, 8, 8, 1, 1, 9, 9 };
    for (unsigned i = 0; i < sizeof values; ++i) {
        uint8_t rgb[] = { values[i], 0, 0, 0, values[i], 0, 0, 0, values[i],
                          values[i], values[i], values[i] };
        uint8_t result[4];
        rgb8_to_ega_palette(result, rgb, 4);
        assert(result[0] == red_codes[i]);
        assert(result[1] == green_codes[i]);
        assert(result[2] == blue_codes[i]);
        assert(result[3] == (red_codes[i] | green_codes[i] | blue_codes[i]));
    }
    uint8_t byte = 19;
    rgb8_to_ega_palette(&byte, NULL, 0);
    assert(byte == 19); /* Documented native zero-count safety policy. */
    /* BX and effective addresses are words: the last triple straddles the
     * segment boundary. Expected code 24h comes from FFh red, 00h/00h G/B. */
    static uint8_t segment[65536], result[21846];
    segment[65535] = 255;
    rgb8_to_ega_palette(result, segment, 21846);
    assert(result[21845] == 0x24);
}

static uint16_t plasma_pixels[8][8];
static uint32_t trace_hash;
static unsigned trace_events, reads, plots;
static int mutate_plot_color;

static void trace(unsigned kind, int16_t x, int16_t y, uint16_t color)
{
    assert(x >= 0 && x < 8 && y >= 0 && y < 8);
    uint8_t bytes[] = { (uint8_t)kind, (uint8_t)x, (uint8_t)((uint16_t)x >> 8),
        (uint8_t)y, (uint8_t)((uint16_t)y >> 8), (uint8_t)color, (uint8_t)(color >> 8) };
    for (unsigned i = 0; i < sizeof bytes; ++i)
        trace_hash = (trace_hash ^ bytes[i]) * UINT32_C(16777619);
    ++trace_events;
}
static void plasma_plot(uint16_t symmetry)
{
    assert(symmetry == 1);
    ++plots;
    trace(1, draw_x, draw_y, plot_color);
    plasma_pixels[draw_y][draw_x] = plot_color;
    if (mutate_plot_color)
        plot_color = (uint16_t)(plot_color + 1u);
}
static uint16_t plasma_read(int16_t x, int16_t y)
{
    ++reads;
    trace(2, x, y, plasma_pixels[y][x]);
    return plasma_pixels[y][x];
}
static void reset_plasma(void)
{
    memset(plasma_pixels, 0, sizeof plasma_pixels);
    trace_hash = UINT32_C(2166136261);
    trace_events = reads = plots = cycle_calls = poll_calls = rand_calls = 0;
    plot_callback = plasma_plot;
    read_pixel_callback = plasma_read;
    background_fill_word = 0;
    palette_elapsed_ticks = input_pending = current_graphics_key = 0;
    palette_cycle_ticks = 1;
    color_count = 256;
    plasma_color_limit = 255;
    screen_half_width = screen_half_height = 3;
    fixed_bios_time = 0;
    poll_result = 0;
    mutate_plot_color = 0;
}

static void test_plasma(void)
{
    /* Literal model results, with time seed 0000h and rotate/add state updates.
     * Hash covers every read AND plot in order, including supplied read colors. */
    static const uint16_t seed2[3][3] = { {18,33,52}, {70,70,69}, {125,107,88} };
    static const uint16_t seed4[5][5] = {
        {18,22,31,41,52}, {41,46,49,55,59}, {68,70,70,67,69},
        {96,92,88,83,79}, {125,114,107,95,88} };
    static const uint16_t seed3[4][4] = {
        {18,23,32,52}, {41,47,50,55}, {69,70,70,69}, {125,92,107,88} };
    reset_plasma();
    seed_plasma();
    assert(plasma_width == 2 && plasma_height == 2 && plasma_color_limit == 255);
    assert(plasma_random == 0x2f1c && plots == 9 && reads == 5 && trace_events == 14);
    assert(trace_hash == UINT32_C(0x4dbf4f64));
    assert(rand_calls == 0); /* The fixed Borland RNG stub must NOT be consumed. */
    for (unsigned y = 0; y < 3; ++y)
        assert(memcmp(plasma_pixels[y], seed2[y], sizeof seed2[y]) == 0);
    reset_plasma();
    screen_half_width = screen_half_height = 5;
    seed_plasma();
    assert(plasma_random == 0x8ef4 && trace_events == 50 && plots == 25 && reads == 25);
    assert(trace_hash == UINT32_C(0x07ae8c31));
    for (unsigned y = 0; y < 5; ++y)
        assert(memcmp(plasma_pixels[y], seed4[y], sizeof seed4[y]) == 0);
    reset_plasma();
    screen_half_width = screen_half_height = 4;
    seed_plasma();
    assert(plasma_random == 0x4aa7 && trace_events == 36);
    assert(trace_hash == UINT32_C(0xdbb3376b)); /* Odd spans: SHR/ADC rounds up. */
    for (unsigned y = 0; y < 4; ++y)
        assert(memcmp(plasma_pixels[y], seed3[y], sizeof seed3[y]) == 0);

    reset_plasma();
    plasma_random = 0x1234;
    plasma_pixels[0][0] = 10; plasma_pixels[0][2] = 20;
    plasma_pixels[2][2] = 30; plasma_pixels[2][0] = 40;
    subdivide_plasma(0, 0, 2, 2, 10, 20, 30, 40);
    assert(plasma_random == 0x9c25 && trace_hash == UINT32_C(0x67a9a597));
    assert(plasma_pixels[0][1] == 13 && plasma_pixels[1][0] == 24);
    assert(plasma_pixels[1][1] == 25 && plasma_pixels[1][2] == 26);
    assert(plasma_pixels[2][1] == 33);

    reset_plasma();
    subdivide_plasma(0, 0, 2, 2, 0xffff, 0xffff, 0xffff, 0xffff);
    assert(plasma_pixels[1][1] == 16383); /* Wrapped four-word sum, logical >>2. */
    assert(plasma_pixels[0][1] == 3); /* Signed noisy edge arithmetic/clamp. */
    reset_plasma();
    subdivide_plasma(0, 0, 2, 2, 300, 300, 300, 300);
    assert(plasma_pixels[1][1] == 300); /* Center is never clamped. */
    assert(plasma_pixels[0][1] == 255 && plasma_pixels[1][0] == 255);

    reset_plasma();
    fixed_bios_time = 1; /* Five hundredths: seed 0005h -> ROR(924Dh,3)=B249h. */
    seed_plasma();
    assert(plasma_pixels[0][0] == 178);
    reset_plasma();
    fixed_bios_time = 36413; /* Exactly 2000 seconds -> DOS seconds byte 20. */
    seed_plasma();
    assert(plasma_pixels[0][0] == 20); /* ROR(1400h+9248h,3)=14C9h. */

    reset_plasma();
    plasma_random = 0x1234;
    for (unsigned y = 0; y < 3; ++y)
        for (unsigned x = 0; x < 3; ++x)
            plasma_pixels[y][x] = 77;
    subdivide_plasma(0, 0, 2, 2, 10, 20, 30, 40);
    assert(plasma_random == 0x1234 && plots == 0 && reads == 5); /* Reuse assigned samples. */
    reset_plasma();
    background_fill_word = 0x0201;
    for (unsigned y = 0; y < 3; ++y)
        for (unsigned x = 0; x < 3; ++x)
            plasma_pixels[y][x] = (uint16_t)(1u + (x & 1u));
    subdivide_plasma(0, 0, 2, 2, 1, 1, 1, 1);
    assert(plots == 5 && plasma_pixels[1][1] == 1); /* Center isn't clamped. */
    assert(plasma_pixels[0][1] == 3 && plasma_pixels[1][0] == 3);
    reset_plasma();
    color_count = 16;
    seed_plasma();
    assert(plasma_color_limit == 15);
    for (unsigned y = 0; y < 3; ++y)
        for (unsigned x = 0; x < 3; ++x)
            assert(plasma_pixels[y][x] >= 3 && plasma_pixels[y][x] <= 15);

    reset_plasma();
    mutate_plot_color = 1;
    subdivide_plasma(0, 0, 2, 2, 10, 20, 30, 40);
    assert(plot_color == (uint16_t)(plasma_pixels[2][1] + 1u));
    reset_plasma();
    palette_elapsed_ticks = 1;
    input_pending = 1; poll_result = 1;
    subdivide_plasma(0, 0, 2, 2, 10, 20, 30, 40);
    assert(cycle_calls == 1 && poll_calls == 1 && trace_events == 0);
    reset_plasma();
    current_graphics_key = 0x126e;
    subdivide_plasma(0, 0, 2, 2, 10, 20, 30, 40);
    assert(trace_events == 0);
    reset_plasma();
    subdivide_plasma(0, 0, 1, 1, 10, 20, 30, 40);
    assert(trace_events == 0);
}

static void test_isr_bodies(void)
{
    palette_elapsed_ticks = 0xffff;
    countdown_ticks = 1; show_duration_ticks = 2;
    input_poll_divider = 5;
    input_pending = 0; queued_key = 0x1c0d; key_peeks = 0;
    mouse_callback_installed = 0;
    graphics_timer_tick_isr();
    assert(palette_elapsed_ticks == 0 && countdown_ticks == 0 && show_duration_ticks == 1);
    assert(input_poll_divider == 4 && input_pending == 0 && key_peeks == 0);
    for (unsigned i = 0; i < 4; ++i)
        graphics_timer_tick_isr();
    assert(show_duration_ticks == 0 && input_pending == 1 && key_peeks == 1);
    assert(input_poll_divider == 5);
    uint16_t header[10] = {0};
    current_image_header = header;
    header[6] = 0x1234; header[9] = 0x777;
    mouse_callback_installed = 1;
    exit_requested = 0xffff; input_pending = 0xffff;
    input_poll_divider = 1;
    graphics_timer_tick_isr();
    assert(exit_requested == 0 && input_pending == 0 && header[6] == 0x0234);
    assert(input_poll_divider == 5);
    header[9] = 0;
    header[6] = 0x1000;
    input_poll_divider = 1; queued_key = 0;
    graphics_timer_tick_isr();
    assert(header[6] == 0x1000 && exit_requested == 0);
    current_image_header = NULL;
    input_poll_divider = 1;
    graphics_timer_tick_isr(); /* Native safety for missing header. */
    input_poll_divider = 0;
    graphics_timer_tick_isr();
    assert(input_poll_divider == 0xffff); /* Original DEC wraps, doesn't poll early. */
    input_pending = 0xffff;
    keyboard_pending_isr();
    assert(input_pending == 0);
    mouse_abort_pending = 0xffff;
    mouse_pending_callback();
    assert(input_pending == 1 && mouse_abort_pending == 0);
    mouse_pending_callback();
    assert(input_pending == 2 && mouse_abort_pending == 1);
}

int main(void)
{
    test_symmetry_and_edges();
    test_ega_parity_xor_and_delays();
    test_reads_and_pages();
    test_planar_rows();
    test_clear_wrap_and_modes();
    test_display_and_obsolete_services();
    test_palette_writers();
    test_rgb_thresholds();
    test_plasma();
    test_isr_bodies();
    puts("DVDT: all 38 entries covered; ASM-model kernel/palette/plasma/ISR tests passed");
    return 0;
}
