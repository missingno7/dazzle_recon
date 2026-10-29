/* DAZZLE DOSI: native video/palette foundation. Original function order retained. */
#include "dazzle.h"
#include "dzconst.h"
#include <math.h>

/* Platform adapter for the original no-argument EGA builder. */
extern void dz_build_ega_adapter(int16_t unused);

uint16_t display_freeze_active, display_freeze_cycle_requested;
uint16_t palette_callback_active;
uint16_t ega_red_limit, ega_blue_limit, ega_green_limit;

/* Fills the EGA/VGA palette buffer using random, fixed, or cycling color rules, then advances
 * the plot color.
 * Original 11C1:0E45, 884 bytes. */
void generate_palette(void) {
    uint16_t palette_index;
    int16_t channel_level[3];
    int16_t channel_phase[3];
    int16_t channel_period[3];
    int16_t channel_direction[3];

    palette_base_green = 10;
    palette_base_blue = 12;
    palette_buffer[0] = (uint8_t)(background_fill_word & 0xff);
    palette_buffer[1] = (uint8_t)palette_base_green;
    palette_buffer[2] = (uint8_t)palette_base_blue;

    if (palette_random_mode) {
        for (palette_index = 3; palette_index < palette_entries; ++palette_index)
            palette_buffer[palette_index] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
    } else {
        channel_period[0] = dz_random((uint16_t)(color_count - 4)) + 4;
        channel_period[1] = dz_random((uint16_t)(color_count - 4)) + 4;
        channel_period[2] = dz_random((uint16_t)(color_count - 4)) + 4;
        if (palette_alternate_mode) {
            if (channel_period[0] >= 8) channel_period[0] >>= 1;
            if (channel_period[1] >= 8) channel_period[1] >>= 1;
            if (channel_period[2] >= 8) channel_period[2] >>= 1;
        }
        channel_phase[0] = channel_phase[1] = channel_phase[2] = 0;
        if (palette_fixed_mode) {
            channel_period[0] = (int16_t)color_count;
            channel_period[1] = (int16_t)(color_count / 2);
            channel_period[2] = channel_level[0] = channel_level[1] = channel_level[2] = 0;
        } else {
            channel_level[0] = dz_random(4);
            channel_level[1] = dz_random(4);
            channel_level[2] = dz_random(4);
        }
        channel_direction[0] = channel_direction[1] = channel_direction[2] = 1;
        for (palette_index = 3; palette_index < palette_entries; ++palette_index) {
            if (++channel_phase[0] > channel_period[0]) {
                channel_phase[0] = 0;
                if ((channel_level[0] += channel_direction[0]) >= 3 || channel_level[0] <= 0)
                    channel_direction[0] = -channel_direction[0];
            }
            if (++channel_phase[1] > channel_period[1]) {
                channel_phase[1] = 0;
                if ((channel_level[1] += channel_direction[1]) >= 3 || channel_level[1] <= 0)
                    channel_direction[1] = -channel_direction[1];
            }
            if (++channel_phase[2] > channel_period[2]) {
                channel_phase[2] = 0;
                if ((channel_level[2] += channel_direction[2]) >= 3 || channel_level[2] <= 0)
                    channel_direction[2] = -channel_direction[2];
            }
            palette_buffer[palette_index] = ((channel_level[0] & 1) << 5) | ((channel_level[0] & 2) << 1);
            palette_buffer[palette_index] |= ((channel_level[1] & 1) << 4) | (channel_level[1] & 2);
            palette_buffer[palette_index] |= ((channel_level[2] & 1) << 3) | ((channel_level[2] & 2) >> 1);
        }
    }
    if (palette_four_ega_colors) {
        palette_buffer[1] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
        palette_buffer[2] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
        palette_buffer[3] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
        for (palette_index = 4; palette_index < palette_entries; ++palette_index)
            palette_buffer[palette_index] = palette_buffer[palette_index & 3];
    }
    memmove(palette_buffer + palette_index, palette_buffer + 3, color_count);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
}

/* Seeds the reserved EGA palette bytes and optionally replaces the background colors.
 * Original 11C1:11B9, 401 bytes. */
void randomize_palette_state(uint16_t replace_base_colors) {
    uint8_t seed;

    if (dz_random(20) && !replace_base_colors)
        seed = 0;
    else
        seed = (uint8_t)(dz_random(16) + 0xef);
    memset(palette_buffer, seed, 9);
    if (replace_base_colors) {
        palette_buffer[0] = (uint8_t)dz_random(seed);
        palette_buffer[1] = (uint8_t)dz_random(seed);
        palette_buffer[2] = (uint8_t)dz_random(seed);
    }
    palette_buffer[4] = (uint8_t)dz_random(ega_green_limit);
    palette_buffer[5] = (uint8_t)(ega_green_limit + dz_random(ega_green_limit));
    palette_buffer[6] = (uint8_t)dz_random(ega_blue_limit);
    palette_buffer[8] = (uint8_t)ega_green_limit;
}

/* Builds a changing EGA palette from channel phases and amplitudes while servicing input and
 * palette timing.
 * Original 11C1:13EF, 1089 bytes. */
void build_ega_palette(int16_t update_cycle) {
    int16_t palette_index;
    uint16_t sample_index;
    uint16_t channel_amplitude[3];
    uint16_t channel_value[3];
    uint16_t channel_phase[3];
    float channel_frequency[3];

    keyboard_led_flags(0x40, 0);
    if (palette_fixed_mode) {
        for (palette_index = 0; palette_index <= 2; ++palette_index) {
            channel_phase[palette_index] = 0;
            channel_amplitude[palette_index] = ega_red_limit;
        }
    } else {
        for (palette_index = 0; palette_index <= 2; ++palette_index) {
            channel_phase[palette_index] = (uint16_t)dz_random((uint16_t)color_count);
            if (dz_random(3))
                channel_amplitude[palette_index] = ega_red_limit;
            else
                channel_amplitude[palette_index] = (uint16_t)(ega_red_limit / 2 + dz_random((uint16_t)(ega_red_limit / 2)));
        }
    }
    if (palette_alternate_mode) {
        channel_frequency[0] = 0.075f; channel_frequency[1] = 0.087f; channel_frequency[2] = 0.100f;
    } else {
        channel_frequency[0] = 0.025f; channel_frequency[1] = 0.033f; channel_frequency[2] = 0.040f;
    }
    if (palette_random_mode) {
        channel_frequency[0] = 0.150f; channel_frequency[1] = 0.175f; channel_frequency[2] = 0.200f;
    }
    randomize_palette_state(0);
    if (dz_random(50)) {
        for (sample_index = 0, palette_index = 9; palette_index < palette_entries * 3; ++sample_index) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            channel_value[0] = (uint16_t)((float)channel_amplitude[0] *
                cos((channel_phase[0] + sample_index) * channel_frequency[0]) + (channel_amplitude[0] + 1));
            channel_value[1] = (uint16_t)((float)channel_amplitude[1] *
                cos((channel_phase[1] + sample_index) * channel_frequency[1]) + (channel_amplitude[1] + 1));
            channel_value[2] = (uint16_t)((float)channel_amplitude[2] *
                cos((channel_phase[2] + sample_index) * channel_frequency[2]) + (channel_amplitude[2] + 1));
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)(channel_value[0] > ega_green_limit ? channel_value[0] - ega_green_limit : 0);
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)(channel_value[1] > ega_green_limit ? channel_value[1] - ega_green_limit : 0);
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)(channel_value[2] > ega_green_limit ? channel_value[2] - ega_green_limit : 0);
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    } else {
        for (sample_index = 0, palette_index = 9; palette_index < palette_entries * 3; ++sample_index) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            channel_value[0] = (uint16_t)((float)ega_green_limit * cos(sample_index * channel_frequency[0]) + ega_green_limit);
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)channel_value[0];
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)channel_value[0];
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)channel_value[0];
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    }
    memmove(palette_buffer + palette_index, palette_buffer + 9, (size_t)color_count * 3u);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    keyboard_led_flags(0x40, 1);
}

/* Fills the 64-entry sine and cosine lookup table used by drawing routines.
 * Original 11C1:1D64, 113 bytes. */

void initialize_math_tables(void) {
    float angle;
    register int16_t i;

    angle = 0.0;
    for (i = 0; i < 64; ++i) {
        trig_table[i].cosine = (float)cos(angle);
        trig_table[i].sine = (float)sin(angle);
        angle = (float)(angle + 0.098174770421875);
    }
}

/* Selects adapter modes, drawing/palette callbacks, dimensions, and allocates palette and image
 * work buffers.
 * Original 11C1:21A6, 879 bytes. */

void configure_video_backend(int16_t allocate) {
    register int16_t ball_scale_divisor;

    if (allocate) freeze_led_state = 0;
    display_start_callback = set_display_start;
    ega_red_limit = 0x2f;
    ega_blue_limit = 0x3f;
    ega_green_limit = 0x1f;
    if (video_adapter == 8) {
        default_palette_cycle_ticks = 1;
        if (allocate) palette_cycle_ticks = use_8086_palette_io ? default_palette_cycle_ticks << 2 : default_palette_cycle_ticks;
        palette_entries = 576;
        color_count = 256;
        color_index_mask = color_count - 1;
        screen_half_width = 160;
        if (bios_resolution_mode) {
            screen_half_height = 100;
            mode_setup_callback = vga_set_mode_320x200;
            plot_callback = vga_plot_symmetric_320x200;
            read_pixel_callback = vga_read_pixel_320x200;
            screen_extent = screen_half_width + 1;
        } else {
            screen_half_height = 200;
            mode_setup_callback = vga_set_mode_320x400;
            plot_callback = vga_plot_symmetric_320x400;
            read_pixel_callback = vga_read_pixel_320x400;
            if (use_bios_palette || split_vga_palette_updates || use_8086_palette_io)
                screen_extent = screen_half_height - 28;
            else
                screen_extent = screen_half_height + 1;
        }
        palette_build_callback = build_ega_palette;
        if (split_vga_palette_updates) {
            if (use_bios_palette)
                palette_upload_callback = vga_upload_palette_bios_split;
            else if (use_8086_palette_io)
                palette_upload_callback = vga_upload_palette_8086_split;
            else
                palette_upload_callback = vga_upload_palette_rep_split;
        } else if (use_bios_palette) {
            palette_upload_callback = vga_upload_palette_bios;
        } else if (use_8086_palette_io) {
            palette_upload_callback = vga_upload_palette_8086;
        } else {
            palette_upload_callback = vga_upload_palette_rep;
        }
    }
    if (video_adapter == 4) {
        default_palette_cycle_ticks = 2;
        if (allocate) palette_cycle_ticks = default_palette_cycle_ticks;
        palette_entries = 64;
        color_count = 16;
        color_index_mask = color_count - 1;
        screen_half_width = 320;
        mode_setup_callback = ega_set_mode;
        if (bios_resolution_mode) {
            plot_callback = ega_plot_symmetric_640x350;
            read_pixel_callback = ega_read_pixel_640x350;
            screen_half_height = 175;
            screen_extent = screen_half_width + 1;
        } else {
            plot_callback = ega_plot_symmetric_640x700;
            read_pixel_callback = ega_read_pixel_640x700;
            screen_half_height = 350;
            screen_extent = screen_half_height + 1;
        }
        if (use_bios_palette)
            palette_upload_callback = ega_upload_palette_bios;
        else
            palette_upload_callback = ega_upload_palette_ports;
        palette_build_callback = dz_build_ega_adapter;
    }
    maximum_image_size = (uint16_t)(0xffffu / (uint16_t)(screen_extent + (screen_extent >> 2)) - 1u);
    palette_cycle_wrap_bytes = palette_entries * 3;
    if (!allocate) return;

    if (video_adapter == 4)
        palette_buffer_bytes = color_count + palette_entries;
    else
        palette_buffer_bytes = (uint16_t)((color_count + palette_entries) * 3);
    palette_buffer = (uint8_t *)malloc(palette_buffer_bytes + 3);
    if (!palette_buffer) {
        exit_requested = 1;
        fprintf(stderr, "\r Memory error: EPAL - ABORT\n\a");
        print_closing_text_and_exit(0x3c);
    }
    /* Slice-only: linewipe/ball workspaces are deferred to their image ports.
     * Preserve the original divisor draw and dimensions (and therefore RNG order).
     * The old PaletteMapSpec overlay cannot be used with native pointer alignment. */
    ball_scale_divisor = dz_random(5) + 2;
    ball_bitmap_height = (uint16_t)(screen_half_height / ball_scale_divisor);
    ball_bitmap_width = (uint16_t)(screen_half_width / ball_scale_divisor);
}

/* Advances and uploads the palette window unless the frozen display is static, then resets its
 * timer.
 * Original 11C1:2515, 89 bytes. */

void cycle_palette(void) {
    if (!display_freeze_active || display_freeze_cycle_requested) {
        if (video_adapter == 4) {
            ++palette_cycle_offset;
            if (palette_cycle_offset >= palette_entries) palette_cycle_offset = 3;
        } else {
            palette_cycle_offset = palette_cycle_offset + palette_step;
            if (palette_cycle_offset >= palette_cycle_wrap_bytes) palette_cycle_offset = 9;
        }
        palette_upload_callback(palette_buffer + palette_cycle_offset);
    }
    palette_elapsed_ticks = 0;
}
