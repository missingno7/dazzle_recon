/* DAZZLE 5.0 fade module (DZFD), BC++ 3.00 -ml -O1. */
#include <dos.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "dazzle.h"


/* Reserved colors precede the cycling palette; these are internal adapter IDs. */
#define FIRST_CYCLING_COLOR 3
#define ADAPTER_EGA 4
#define ADAPTER_VGA 8

#define KEY_CLEAR 'n'
#define KEY_PAN_SCROLL 'p'
#define KEY_HOLD 'h'
#define FIRST_FADE_KEY 'a'
#define LAST_FADE_KEY 'z'

#define DOS_IDLE_INTERRUPT 0x28
#define DOS_MULTIPLEX_INTERRUPT 0x2f
#define DOS_RELEASE_TIME_SLICE 0x1680
#define LED_SCROLL_LOCK 16
#define LED_NUM_LOCK 32
#define LED_NUM_AND_CAPS_LOCK 96

/* Page offsets are paragraphs within A000 video memory; heights are physical rows. */
#define EGA_SECOND_PAGE_PARAGRAPH 1750
#define VGA_SECOND_PAGE_PARAGRAPH 2000
#define VGA_PAGE_WORDS 16000
#define EGA_PHYSICAL_HEIGHT 350
#define VGA_ENHANCED_PHYSICAL_HEIGHT 400
#define DAC_COMPONENT_MASK 0x3f
#define DAC_HIGH_BITS_MASK 0xc0
#define RGB_COMPONENTS 3

/* Firefly's word slots; FF_LONG views span a low word and the following high word. */
#define FF_DEPTH 0
#define FF_HISTORY_INDEX 1
#define FF_TRAIL_COLOR 2
#define FF_STAMP_SIZE 3
#define FF_ERASE_MODE 4
#define FF_BACKGROUND_A 5
#define FF_BACKGROUND_B 6
#define FF_HEAD_COLOR 7
#define FF_BACKGROUND_X_MASK 8
#define FF_STEP_X 9
#define FF_STEP_Y 11
#define FF_POSITION_X 13
#define FF_POSITION_X_WHOLE 14
#define FF_POSITION_Y 15
#define FF_POSITION_Y_WHOLE 16

/* The two carriage-return uses share the original DS:0BFC pool entry. */
#pragma option -d

/* 0CCD:0000..000A is the preceding 0B84 TU's trailing switch table.
 * DOSI's signal diagnostic calls the far getter at this module's 000A start. */

/* Returns this module's code segment for the signal diagnostic.
 * Original 0CCD:000A, 3 bytes. */
unsigned int dzfd_seg(void)
{
    return _CS;
}

/* Waits for the requested timer ticks, capped at half the Auto pause, while polling
 * input and yielding DOS time. skip_palette suppresses palette cycling during the wait.
 * Original 0CCD:000D, 128 bytes. */
void wait_fade_tick(int requested_ticks, int skip_palette)
{
    struct REGPACK idle_args;

    if (requested_ticks > (int)auto_fade_pause_ticks / 2)
        requested_ticks = (int)auto_fade_pause_ticks / 2;
    fade_timer_ticks = requested_ticks;

    while (fade_timer_ticks != 0) {
        if (input_pending != 0 || exit_requested != 0) {
            if (poll_graphics_keys() != 0)
                break;
        }
        if (current_graphics_key == KEY_CLEAR)
            break;
        if (skip_palette == 0) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        geninterrupt(DOS_IDLE_INTERRUPT);
        if (_osmajor >= 3) {
            idle_args.r_ax = DOS_RELEASE_TIME_SLICE;
            intr(DOS_MULTIPLEX_INTERRUPT, &idle_args);
        }
    }
}
/* Initializes fireworks velocities/trail modes when initialize_mode != 0; zero runs four
 * staggered 64-particle bursts to completion. Positions and velocities use 1/64 pixels.
 * Original 0CCD:008D, 2792 bytes. */
void randomize_helper(int initialize_mode)
{
    register int mode = initialize_mode;
    int burst_index, particle_index, tail_catchup, origin_x_bias, origin_y_bias, speed, angle_index;

    /* Nonzero selects parameters; zero launches new bursts with the saved velocities. */
    if (mode != 0) {
        fireworks_aspect_ratio = (float)screen_half_width / (float)screen_half_height * 0.75;
        fireworks_displaced_origins = 1;
        if (mode == 1 && random(4) == 0)
            fireworks_displaced_origins = 0;
        if (fireworks_displaced_origins != 0)
            fireworks_acceleration = 2;
        else
            fireworks_acceleration = 0;
        if (random(3) != 0)
            fireworks_trail_mode = 0;
        else
            fireworks_trail_mode = 1;
        for (burst_index = 0; burst_index <= 3; ++burst_index) {
            for (particle_index = 0; particle_index <= 63; ++particle_index) {
                speed = random(65) + 64;
                angle_index = random(64);
                fireworks_particles[burst_index][particle_index].velocity_x =
                    speed * trig_table[angle_index].sine * fireworks_aspect_ratio;
                fireworks_particles[burst_index][particle_index].velocity_y = -(speed * trig_table[angle_index].cosine);
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        if (mode > 1) {
            if (random(3) == 0)
                fireworks_trail_mode = 3;
            else {
                fireworks_trail_mode = 2;
                fireworks_background_a = 0;
                if (video_adapter == ADAPTER_VGA) {
                    fireworks_background_b = background_fill_word >> 8;
                    fireworks_background_a = background_fill_word & 0xff;
                    if (fireworks_background_a != fireworks_background_b)
                        fireworks_trail_mode = 4;
                    if (bios_resolution_mode == 0)
                        fireworks_background_x_mask = 4;
                    else
                        fireworks_background_x_mask = 1;
                }
            }
        }
        return;
    }

    {
        origin_x_bias = random(screen_half_width) - screen_half_width / 2;
        origin_y_bias = -random(screen_half_height / 2);

        for (burst_index = 0; burst_index <= 3; ++burst_index) {
            if (fireworks_displaced_origins != 0) {
                fireworks_origin_x[burst_index] = origin_x_bias + random(screen_half_width) - screen_half_width / 2;
                fireworks_origin_y[burst_index] = origin_y_bias + random(screen_half_height) - screen_half_height / 2;
            } else
                fireworks_origin_x[burst_index] = fireworks_origin_y[burst_index] = 0;
            fireworks_colors[burst_index] = random(color_count - FIRST_CYCLING_COLOR) + FIRST_CYCLING_COLOR;
            fireworks_age[burst_index] = fireworks_finished_particles[burst_index] = 0;
            fireworks_gravity[burst_index] = 0;
            fireworks_active[burst_index] = 0;
            fireworks_launch_delay[burst_index] = screen_half_height / 8 + random(screen_half_height / 2);

            for (particle_index = 0; particle_index <= 63; ++particle_index) {
                fireworks_particles[burst_index][particle_index].head_x =
                    (fireworks_origin_x[burst_index] + random(9) - 4) * 64;
                fireworks_particles[burst_index][particle_index].head_y =
                    (fireworks_origin_y[burst_index] + random(9) - 4) * 64;
                fireworks_particles[burst_index][particle_index].tail_x =
                    fireworks_particles[burst_index][particle_index].head_x;
                fireworks_particles[burst_index][particle_index].tail_y =
                    fireworks_particles[burst_index][particle_index].head_y;
                fireworks_particles[burst_index][particle_index].tail_start =
                    screen_half_height / 8 + random(screen_half_height / 4);
                fireworks_particles[burst_index][particle_index].tail_end =
                    fireworks_particles[burst_index][particle_index].tail_start * 2
                    + random(screen_half_height / 4);
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }

        if (fireworks_trail_mode == 0)
            set_ega_raster_xor(1);
    }

    burst_index = 0;
    fireworks_active[0] = 1;
    while (fireworks_active[0] != 0 || fireworks_active[1] != 0 ||
           fireworks_active[2] != 0 || fireworks_active[3] != 0) {
        if (fireworks_active[burst_index] != 0) {
            for (particle_index = 0; particle_index <= 63; ++particle_index) {
                int end_age;
                plot_color = fireworks_colors[burst_index];
                if (fireworks_trail_mode == 1)
                    end_age = fireworks_particles[burst_index][particle_index].tail_start;
                else
                    end_age = fireworks_particles[burst_index][particle_index].tail_end;
                if (fireworks_age[burst_index] < end_age) {
                    draw_x = fireworks_particles[burst_index][particle_index].head_x >> 6;
                    draw_y = fireworks_particles[burst_index][particle_index].head_y >> 6;
                    if (abs(draw_x) <= screen_half_width && abs(draw_y) <= screen_half_height) {
                        plot_callback(0);
                        fireworks_particles[burst_index][particle_index].head_x +=
                            fireworks_particles[burst_index][particle_index].velocity_x;
                        fireworks_particles[burst_index][particle_index].head_y +=
                            fireworks_particles[burst_index][particle_index].velocity_y
                            + fireworks_gravity[burst_index];
                    } else if (fireworks_trail_mode == 1)
                        fireworks_particles[burst_index][particle_index].tail_start = fireworks_age[burst_index];
                }

                /* Even modes draw delayed tails; odd modes draw heads alone. */
                if ((fireworks_trail_mode & 1) == 0 &&
                    fireworks_particles[burst_index][particle_index].tail_start <= fireworks_age[burst_index] &&
                    fireworks_particles[burst_index][particle_index].tail_end >= fireworks_age[burst_index]) {
                    plot_color = fireworks_colors[burst_index];
                    draw_x = fireworks_particles[burst_index][particle_index].tail_x >> 6;
                    draw_y = fireworks_particles[burst_index][particle_index].tail_y >> 6;
                    if (fireworks_trail_mode == 2)
                        plot_color = fireworks_background_a;
                    if (fireworks_trail_mode == 4 && (draw_x & fireworks_background_x_mask) != 0)
                        plot_color = fireworks_background_b;
                    if (abs(draw_x) <= screen_half_width && abs(draw_y) <= screen_half_height)
                        plot_callback(0);
                    else
                        fireworks_particles[burst_index][particle_index].tail_end = fireworks_age[burst_index];

                    tail_catchup = fireworks_particles[burst_index][particle_index].tail_start * 2;
                    /* Young erasers advance twice per update to close the initial trail gap. */
                    if (fireworks_age[burst_index] < tail_catchup - 1) {
                        tail_catchup = fireworks_gravity[burst_index] * 2 - tail_catchup * fireworks_acceleration;
                        fireworks_particles[burst_index][particle_index].tail_x +=
                            fireworks_particles[burst_index][particle_index].velocity_x;
                        fireworks_particles[burst_index][particle_index].tail_y +=
                            fireworks_particles[burst_index][particle_index].velocity_y + tail_catchup;
                        draw_x = fireworks_particles[burst_index][particle_index].tail_x >> 6;
                        draw_y = fireworks_particles[burst_index][particle_index].tail_y >> 6;
                        if (fireworks_trail_mode == 2)
                            plot_color = fireworks_background_a;
                        if (fireworks_trail_mode == 4 && (draw_x & fireworks_background_x_mask) != 0)
                            plot_color = fireworks_background_b;
                        plot_callback(0);
                        fireworks_particles[burst_index][particle_index].tail_x +=
                            fireworks_particles[burst_index][particle_index].velocity_x;
                        fireworks_particles[burst_index][particle_index].tail_y +=
                            fireworks_particles[burst_index][particle_index].velocity_y
                            + tail_catchup + fireworks_acceleration;
                    } else {
                        fireworks_particles[burst_index][particle_index].tail_x +=
                            fireworks_particles[burst_index][particle_index].velocity_x;
                        fireworks_particles[burst_index][particle_index].tail_y +=
                            fireworks_particles[burst_index][particle_index].velocity_y
                            + fireworks_gravity[burst_index] - fireworks_acceleration;
                    }
                }

                if ((fireworks_trail_mode == 1 &&
                     fireworks_particles[burst_index][particle_index].tail_start == fireworks_age[burst_index]) ||
                    fireworks_particles[burst_index][particle_index].tail_end == fireworks_age[burst_index])
                    ++fireworks_finished_particles[burst_index];
            }

            if ((fireworks_trail_mode & 1) != 0) {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
                fireworks_colors[burst_index] = plot_color;
            }
            fireworks_gravity[burst_index] += fireworks_acceleration;
            ++fireworks_age[burst_index];
            if ((fireworks_age[burst_index] == fireworks_launch_delay[burst_index] ||
                 (fireworks_finished_particles[burst_index] > 63 &&
                  fireworks_age[burst_index] < fireworks_launch_delay[burst_index])) && burst_index < 3)
                fireworks_active[burst_index + 1] = 1;
            if (fireworks_finished_particles[burst_index] > 63)
                fireworks_active[burst_index] = 0;
        }
        if (++burst_index > 3)
            burst_index = 0;
        if (!(palette_elapsed_ticks < palette_cycle_ticks))
            cycle_palette();
    }

    draw_x = screen_half_width + 1;
    draw_y = screen_half_height + 1;
    image_work_count += screen_extent * 8;
}

/* Chooses a fireworks variant and repeats complete bursts during a random timer interval,
 * servicing palette timing and interruption commands between bursts.
 * Original 0CCD:0B75, 133 bytes. */
void random_delay_transition(void)
{
    randomize_helper(random(2) + 1);
    fade_timer_ticks = random(200) + 200;
    while (fade_timer_ticks > 0) {
        randomize_helper(0);
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
    }
}
/* Swaps random tiles from a 2..8 subdivision grid for a timed interval,
 * choosing a new pair and flip mode on each pass.
 * Original 0CCD:0BFA, 369 bytes. */
void quilt_transition(void)
{
    fade_timer_ticks = random(180) + 180;
    quilt_grid_divisions = random(7) + 2;
    while (fade_timer_ticks != 0) {
        quilt_tile_width = screen_half_width / quilt_grid_divisions;
        quilt_tile_height = screen_half_height / quilt_grid_divisions;
        quilt_first_x = random(quilt_grid_divisions) * quilt_tile_width;
        quilt_first_y = random(quilt_grid_divisions) * quilt_tile_height;
        quilt_second_x = random(quilt_grid_divisions) * quilt_tile_width;
        quilt_second_y = random(quilt_grid_divisions) * quilt_tile_height;
        quilt_flip_mode = random(4);
        swap_pixel_rectangles(quilt_flip_mode, quilt_first_x, quilt_first_y, quilt_second_x, quilt_second_y,
                         quilt_tile_width, quilt_tile_height);
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
    }
}
/* Builds a plasma image at minimum drawing delay, then pauses or scrolls it into view
 * from the alternate VGA page when the resolution and Pan/scroll setting allow it.
 * Original 0CCD:0D6B, 198 bytes. */
void gas_cloud(void)
{
    int scroll_target_row = 0;

    saved_pan_draw_delay = draw_delay_count;
    draw_delay_count = 1;
    if (bios_resolution_mode == 0 && fade_pan_scroll_enabled != 0 && random(5) != 0) {
        scroll_target_row = screen_half_height * 2 + 1;
        if (page_paragraph == 0)
            page_paragraph = VGA_SECOND_PAGE_PARAGRAPH;
        else
            page_paragraph = 0;
        clear_vram_words(0, VGA_PAGE_WORDS, page_paragraph);
    }
    seed_plasma();
    draw_delay_count = saved_pan_draw_delay;

    if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
        return;
    if (current_graphics_key != KEY_CLEAR) {
        if (scroll_target_row == 0) {
            wait_fade_tick(0x78, 0);
        } else if (page_paragraph == 0) {
            animate_vertical_display_start(scroll_target_row, 0);
        } else {
            animate_vertical_display_start(0, scroll_target_row);
        }
    }
}
/* Visits square cells in LFSR order with successively smaller sizes and cycling colors.
 * Requests EGA XOR drawing and leaves display_cleared false for a later clearing fade.
 * Original 0CCD:0E31, 330 bytes. */
void randomize_transition(void)
{
    unsigned int lfsr_state;
    signed char cell_size;
    signed char minimum_cell_size;
    unsigned int feedback_mask;

    set_ega_raster_xor(1);
    feedback_mask = 0x6000;
    cell_size = 0x40;
    minimum_cell_size = 2;
    if (video_adapter == ADAPTER_EGA)
        minimum_cell_size = 4;
    do {
        plot_color = 0 & color_index_mask;
        lfsr_state = 1;
        do {
            draw_y = lfsr_state / ((screen_extent + 1) / cell_size);
            if (draw_y <= screen_half_height / cell_size) {
                draw_x = lfsr_state % ((screen_extent + 1) / cell_size);
                draw_x *= cell_size;
                draw_y *= cell_size;
                if (cell_size > 1) {
                    plot_color++;
                    if (plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                }
                square(cell_size, 1);
            }
            if (lfsr_state & 1)
                lfsr_state = (lfsr_state >> 1) ^ feedback_mask;
            else
                lfsr_state >>= 1;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
        } while (lfsr_state != 1);

        /* The nonzero feedback cycle omits the origin cell. */
        draw_y = draw_x = 0;
        if (cell_size > 1) {
            plot_color++;
            if (plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
        square(cell_size, 1);
        cell_size >>= 1;
        if (cell_size < minimum_cell_size) {
            display_cleared = 0;
            return;
        }
    } while (1);
}
/* Plots a symmetric lattice with successively halved spacing and randomized color rules,
 * pausing between refinements while servicing palette timing and input.
 * Original 0CCD:0F7B, 677 bytes. */
void mesh_transition(void)
{
    int initial_spacing;
    register int diagonal;
    register int spacing;
    int color_per_point;
    int line_color_mode;
    int point_color_mode;

    if (video_adapter == ADAPTER_VGA)
        initial_spacing = 0x100;
    else
        initial_spacing = 0x200;
    spacing = initial_spacing;
    color_per_point = random(2);
    line_color_mode = random(3);
    point_color_mode = random(7) + 2;
    plot_color = FIRST_CYCLING_COLOR & color_index_mask;
    draw_x = 0;
    draw_y = 0;
    plot_callback(1);
    do {
        spacing >>= 1;
        diagonal = spacing;
        do {
            draw_x = diagonal;
            draw_y = diagonal;
            if (draw_y <= screen_half_height) {
                if (color_per_point != 0)
                    choose_color(point_color_mode);
                else
                    choose_color(line_color_mode);
                plot_callback(1);
                do {
                    draw_x -= spacing;
                    if (draw_x <= screen_half_width) {
                        if (color_per_point != 0)
                            choose_color(point_color_mode);
                        plot_callback(1);
                    }
                } while (draw_x > 0);
            }

            draw_x = diagonal;
            if (draw_x <= screen_half_width) {
                if (color_per_point == 0)
                    choose_color(line_color_mode);
                draw_y = diagonal;
                do {
                    draw_y -= spacing;
                    if (draw_y <= screen_half_height) {
                        if (color_per_point != 0)
                            choose_color(point_color_mode);
                        plot_callback(1);
                    }
                } while (draw_y > 0);
            }

            diagonal += spacing;
            draw_x = diagonal;
            draw_y = diagonal;
            if (draw_y <= screen_half_height) {
                if (color_per_point == 0)
                    choose_color(line_color_mode);
                draw_x += spacing;
                do {
                    draw_x -= spacing * 2;
                    if (draw_x <= screen_half_width) {
                        if (color_per_point != 0)
                            choose_color(point_color_mode);
                        plot_callback(1);
                    }
                } while (draw_x > spacing);
            }

            draw_x = diagonal;
            if (draw_x <= screen_half_width) {
                if (color_per_point == 0)
                    choose_color(line_color_mode);
                draw_y += spacing;
                do {
                    draw_y -= spacing * 2;
                    if (draw_y <= screen_half_height) {
                        if (color_per_point != 0)
                            choose_color(point_color_mode);
                        plot_callback(1);
                    }
                } while (draw_y > spacing);
            }

            diagonal += spacing;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
        } while ((diagonal <= screen_half_width || diagonal <= screen_half_height) && diagonal <= initial_spacing);

        fade_timer_ticks = 5;
        while (fade_timer_ticks > 0 && current_graphics_key != KEY_CLEAR) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        if (spacing <= 1) {
            fade_timer_ticks = 0x3c;
            while (fade_timer_ticks > 0 && current_graphics_key != KEY_CLEAR) {
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
            return;
        }
    } while (1);
}

#define FF_STATE(i)       (firefly_path_state[(i)])
#define FF_LONG(i)        (*(long *)&firefly_path_state[(i)])
#define FF_PART(l,p,f)    (fireworks_particles[(l)][(p)][(f)])

/* Traces a recursive orthogonal path with 16.16 steps and a five-position erasure trail.
 * initialize seeds recursion, stamp size and background handling, then renders the path.
 * Original 0CCD:1220, 1210 bytes. */
void firefly_transition(int initialize)
{
    int leg, trail_index;
    int detail_level;
    long x_step, y_step;
    volatile long saved_step;

    if (initialize != 0) {
        detail_level = random(3);
        FF_STATE(FF_DEPTH) = detail_level + 6;
        FF_STATE(FF_STAMP_SIZE) = 8 >> detail_level;
        FF_STATE(FF_HEAD_COLOR) = 2;
        if (video_adapter == ADAPTER_VGA) {
            FF_STATE(FF_BACKGROUND_B) = background_fill_word >> 8;
            FF_STATE(FF_BACKGROUND_A) = background_fill_word & 0xff;
            if (FF_STATE(FF_BACKGROUND_A) == FF_STATE(FF_BACKGROUND_B)) {
                if (FF_STATE(FF_HEAD_COLOR) == FF_STATE(FF_BACKGROUND_A))
                    FF_STATE(FF_HEAD_COLOR) = 1;
                FF_STATE(FF_ERASE_MODE) = 1;
                if (FF_STATE(FF_BACKGROUND_A) == 0 && random(3) == 0)
                    FF_STATE(FF_ERASE_MODE) = 2;
            } else {
                FF_STATE(FF_STAMP_SIZE) = 1;
                FF_STATE(FF_DEPTH) = 9;
                FF_STATE(FF_ERASE_MODE) = 0;
                if (bios_resolution_mode == 0)
                    FF_STATE(FF_BACKGROUND_X_MASK) = 4;
                else
                    FF_STATE(FF_BACKGROUND_X_MASK) = 1;
            }
        } else {
            FF_STATE(FF_BACKGROUND_A) = 0;
            FF_STATE(FF_STAMP_SIZE) <<= 1;
            FF_STATE(FF_ERASE_MODE) = 1;
        }

        FF_LONG(FF_STEP_X) = screen_half_height;
        FF_LONG(FF_POSITION_X) = -screen_half_width;
        FF_LONG(FF_POSITION_Y) = screen_half_height;
        FF_STATE(FF_HISTORY_INDEX) = 0;
        while (FF_STATE(FF_HISTORY_INDEX) <= 4) {
            firefly_x_history[FF_STATE(FF_HISTORY_INDEX)] = -screen_half_width;
            firefly_y_history[FF_STATE(FF_HISTORY_INDEX)] = screen_half_height;
            ++FF_STATE(FF_HISTORY_INDEX);
        }
        FF_STATE(FF_HISTORY_INDEX) = 0;
        /* Move the integer seeds into high words to form 16.16 positions. */
        FF_STATE(FF_POSITION_X_WHOLE) = FF_STATE(FF_POSITION_X);
        FF_STATE(FF_POSITION_X) = 0;
        FF_STATE(FF_POSITION_Y_WHOLE) = FF_STATE(FF_POSITION_Y);
        FF_STATE(FF_POSITION_Y) = 0;
        FF_LONG(FF_STEP_X) = (FF_LONG(FF_STEP_X) << 17) /
                     ((2 << (FF_STATE(FF_DEPTH) - 1)) - 1);
        FF_LONG(FF_STEP_Y) = 0;
        FF_STATE(FF_TRAIL_COLOR) = FIRST_CYCLING_COLOR;
        firefly_transition(0);

        plot_color = FF_STATE(FF_ERASE_MODE) < 2 ? FF_STATE(FF_BACKGROUND_A) : FF_STATE(FF_TRAIL_COLOR);
        /* Flush the five remaining trail positions after the recursive path finishes. */
        trail_index = 0;
        do {
            draw_x = firefly_x_history[FF_STATE(FF_HISTORY_INDEX)];
            draw_y = firefly_y_history[FF_STATE(FF_HISTORY_INDEX)];
            if (FF_STATE(FF_ERASE_MODE) == 2 && FF_STATE(FF_HISTORY_INDEX) == 0) {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            }
            square(FF_STATE(FF_STAMP_SIZE), 0);
            ++FF_STATE(FF_HISTORY_INDEX);
            FF_STATE(FF_HISTORY_INDEX) %= 5;
        } while (++trail_index <= 4);
    } else {
        --FF_STATE(FF_DEPTH);
        if (FF_STATE(FF_DEPTH) != 0) {
            /* Swap axes for the first recursive subpath, then restore the parent step. */
            saved_step = FF_LONG(FF_STEP_X);
            FF_LONG(FF_STEP_X) = FF_LONG(FF_STEP_Y);
            FF_LONG(FF_STEP_Y) = saved_step;
            firefly_transition(0);
            saved_step = FF_LONG(FF_STEP_Y);
            FF_LONG(FF_STEP_Y) = FF_LONG(FF_STEP_X);
            FF_LONG(FF_STEP_X) = saved_step;
        }

        for (leg = 1; leg <= 3; ++leg) {
            switch (leg) {
            case 1:
                x_step = FF_LONG(FF_STEP_X);
                y_step = FF_LONG(FF_STEP_Y);
                break;
            case 2:
                x_step = FF_LONG(FF_STEP_Y);
                y_step = FF_LONG(FF_STEP_X);
                break;
            case 3:
                x_step = -FF_LONG(FF_STEP_X);
                y_step = -FF_LONG(FF_STEP_Y);
                FF_LONG(FF_STEP_X) = y_step;
                FF_LONG(FF_STEP_Y) = x_step;
                break;
            default:
                break;
            }

            /* Erase the oldest history entry before replacing it with the next path point. */
            draw_x = firefly_x_history[FF_STATE(FF_HISTORY_INDEX)];
            draw_y = firefly_y_history[FF_STATE(FF_HISTORY_INDEX)];
            switch (FF_STATE(FF_ERASE_MODE)) {
            case 0:
                plot_color = (draw_x & FF_STATE(FF_BACKGROUND_X_MASK)) ?
                             FF_STATE(FF_BACKGROUND_B) : FF_STATE(FF_BACKGROUND_A);
                plot_callback(0);
                break;
            case 1:
                plot_color = FF_STATE(FF_BACKGROUND_A);
                square(FF_STATE(FF_STAMP_SIZE), 0);
                break;
            case 2:
                plot_color = FF_STATE(FF_TRAIL_COLOR);
                if (FF_STATE(FF_HISTORY_INDEX) == 0) {
                    ++plot_color;
                    if (plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                }
                FF_STATE(FF_TRAIL_COLOR) = plot_color;
                square(FF_STATE(FF_STAMP_SIZE), 0);
                break;
            }

            FF_LONG(FF_POSITION_X) += ((long)screen_half_width * x_step) / screen_half_height;
            FF_LONG(FF_POSITION_Y) -= y_step;
            draw_x = FF_LONG(FF_POSITION_X) >> 16;
            draw_y = FF_LONG(FF_POSITION_Y) >> 16;
            if ((FF_LONG(FF_POSITION_X) & 0x8000L) != 0)
                ++draw_x;
            if ((FF_LONG(FF_POSITION_Y) & 0x8000L) != 0)
                ++draw_y;
            firefly_x_history[FF_STATE(FF_HISTORY_INDEX)] = draw_x;
            firefly_y_history[FF_STATE(FF_HISTORY_INDEX)] = draw_y;
            ++FF_STATE(FF_HISTORY_INDEX);
            FF_STATE(FF_HISTORY_INDEX) %= 5;

            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;

            plot_color = FF_STATE(FF_HEAD_COLOR);
            if (FF_STATE(FF_ERASE_MODE) == 0)
                plot_callback(0);
            else
                square(FF_STATE(FF_STAMP_SIZE), 0);
            if (FF_STATE(FF_DEPTH) != 0)
                firefly_transition(0);
        }

        FF_LONG(FF_STEP_X) = -x_step;
        FF_LONG(FF_STEP_Y) = -y_step;
        ++FF_STATE(FF_DEPTH);
    }
}
#undef FF_PART
#undef FF_LONG
#undef FF_STATE

/* Expands a 360-sample spiral by joining each stored 16.16 point to a point 1.5 times
 * farther out, advancing colors and stopping after the positive drawing limits are passed.
 * Original 0CCD:16DA, 734 bytes. */
void spiral_transition(void)
{
    int color_counter, limit_y, limit_x, symmetry;
    float radius, radius_growth, angle, aspect_ratio;
    transition_point_t next_x, next_y;
    transition_point_t spiral_x[360], spiral_y[360];
    register int angle_index;
    register int line_done;

    radius_growth = 0.001127f;
    aspect_ratio = (float)screen_half_width / (float)screen_half_height * 0.75;
    limit_y = (screen_half_height * 3) >> 1;
    limit_x = (screen_half_width * 3) >> 1;
    color_counter = 0;
    plot_color = FIRST_CYCLING_COLOR;
    symmetry = random(3);
    if (symmetry == 2)
        symmetry = 0;
    radius = 1.0f;
    angle = 0.0f;

    for (angle_index = 0; angle_index < 360; ++angle_index) {
        radius *= 1 + radius_growth;
        spiral_x[angle_index].value = radius * cos(angle) * aspect_ratio * 65536.0;
        spiral_y[angle_index].value = radius * sin(angle) * 65536.0;
        angle += 0.017453292519444445;
    }

    angle_index = 0;
    do {
        /* Each ring visit grows one angular sample before storing it for the next turn. */
        next_x.value = spiral_x[angle_index].value + (spiral_x[angle_index].value >> 1);
        next_y.value = spiral_y[angle_index].value + (spiral_y[angle_index].value >> 1);
        draw_x = spiral_x[angle_index].part.whole;
        draw_y = spiral_y[angle_index].part.whole;
        line_done = 1;
        do {
            line_done = line_step(next_x.part.whole, next_y.part.whole, line_done);
            plot_callback(symmetry);
        } while (line_done == 0);

        spiral_x[angle_index].value = next_x.value;
        spiral_y[angle_index].value = next_y.value;
        ++angle_index;
        angle_index = angle_index % 360;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;

        ++color_counter;
        if ((color_counter = color_counter & 0x0f) == 0) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
    } while (next_x.part.whole <= limit_x || next_y.part.whole <= limit_y);
    fade_timer_ticks = 60;
    while (fade_timer_ticks > 0 && current_graphics_key != KEY_CLEAR) {
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* Recursively fills a profile between first_index and last_index with randomly displaced midpoints.
 * Drip uses the unsigned-long entries as 16.16 fall speeds for its columns.
 * Original 0CCD:19B8, 233 bytes. */
void spiral_midpoint(int first_index, int last_index, unsigned long far profile[])
{
    int midpoint;
    int jitter_amplitude;

    jitter_amplitude = 0x800 << random(3);
    midpoint = (first_index + last_index) >> 1;
    profile[midpoint] = ((profile[first_index] + profile[last_index]) >> 1) + jitter_amplitude
                   - random(jitter_amplitude * 2);
    if (midpoint - first_index > 1) {
        spiral_midpoint(first_index, midpoint, profile);
        spiral_midpoint(midpoint, last_index, profile);
    }
}
/* Sweeps downward from the top edge using per-column 16.16 positions and randomized
 * midpoint-generated speeds, changing color between passes as columns reach the bottom.
 * Original 0CCD:1AA1, 544 bytes. */

void drip_transition(void)
{
    spiral_point_t fall_speed[641];
    spiral_point_t column_y[641];
    int finished_columns = 0;
    int last_column;
    register int seed_spacing;
    register int column;

    plot_color = FIRST_CYCLING_COLOR;
    last_column = screen_half_width << 1;
    seed_spacing = 8 << random(4);
    for (column = 0; column <= 0x280; ++column) {
        column_y[column].part.whole = -screen_half_height;
        column_y[column].part.fraction = (fall_speed[column].part.whole = 0);
    }
    for (column = 0; column <= last_column; column += seed_spacing)
        fall_speed[column].part.fraction = random(0xa000) + 0x4000;
    for (column = 0; column <= last_column - seed_spacing; column += seed_spacing)
        spiral_midpoint(column, column + seed_spacing,
                        (unsigned long far *)&fall_speed[0].value);

    if (finished_columns < screen_half_width) do {
        draw_x = -screen_half_width;
        for (column = 1; column <= last_column; ++column) {
            if (column_y[column].part.whole <= screen_half_height) {
                draw_y = column_y[column].part.whole;
                column_y[column].value = column_y[column].value + fall_speed[column].value;
                /* Plot the previous row only when the fixed-point position enters a new row. */
                if (draw_y < column_y[column].part.whole)
                    plot_callback(0);
                if (screen_half_height <= column_y[column].part.whole) {
                    ++column_y[column].part.whole;
                    ++finished_columns;
                }
            }
            ++draw_x;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
        }
        if (video_adapter == ADAPTER_VGA) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        } else {
            ++ega_color_substep;
            if ((ega_color_substep &= 0x0f) == 0) {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            }
        }
    } while (finished_columns < screen_half_width);
}

/* Sweeps colored rays from a selected center/edge origin toward the screen boundary.
 * The first pass requests EGA XOR; the second repeats with replace drawing.
 * Original 0CCD:1CC1, 728 bytes. */
void cleanup_transition(void)
{
    int origin_y, origin_x, edge_pattern, origin_choice, color_mode;
    int first_pattern, last_pattern, target_x, target_y;
    int start_x, start_y, bound_x, bound_y, line_done, pass_index;
    register int step_y, step_x;

    origin_y = 0;
    origin_x = 0;
    color_mode = random(8) + 2;
    origin_choice = random(5);

    switch (origin_choice) {
    case 0:
        first_pattern = 0;
        last_pattern = 7;
        break;
    case 1:
        origin_y = screen_half_height;
        first_pattern = 0;
        last_pattern = 3;
        break;
    case 2:
        origin_y = -screen_half_height;
        first_pattern = 4;
        last_pattern = 7;
        break;
    case 3:
        origin_x = -screen_half_width;
        first_pattern = 2;
        last_pattern = 5;
        break;
    case 4:
        origin_x = screen_half_width;
        first_pattern = 6;
        last_pattern = 9;
        break;
    }

    set_ega_raster_xor(1);
    edge_pattern = first_pattern;
    pass_index = 1;
    while (pass_index <= 2) {
        for (; edge_pattern <= last_pattern; ++edge_pattern) {
            switch (edge_pattern) {
            case 0:
            case 8:
                start_x = bound_x = -screen_half_width;
                step_x = 0;
                start_y = origin_y;
                bound_y = -screen_half_height;
                step_y = -1;
                break;
            case 1:
            case 9:
                start_x = -screen_half_width;
                bound_x = origin_x;
                step_x = 1;
                start_y = bound_y = -screen_half_height;
                step_y = 0;
                break;
            case 2:
                start_x = origin_x;
                bound_x = screen_half_width;
                step_x = 1;
                start_y = bound_y = -screen_half_height;
                step_y = 0;
                break;
            case 3:
                start_x = bound_x = screen_half_width;
                step_x = 0;
                start_y = -screen_half_height;
                bound_y = origin_y;
                step_y = 1;
                break;
            case 4:
                start_x = bound_x = screen_half_width;
                step_x = 0;
                start_y = origin_y;
                bound_y = screen_half_height;
                step_y = 1;
                break;
            case 5:
                start_x = screen_half_width;
                bound_x = origin_x;
                step_x = -1;
                start_y = bound_y = screen_half_height;
                step_y = 0;
                break;
            case 6:
                start_x = origin_x;
                bound_x = -screen_half_width;
                step_x = -1;
                start_y = bound_y = screen_half_height;
                step_y = 0;
                break;
            case 7:
                start_x = bound_x = -screen_half_width;
                step_x = 0;
                start_y = screen_half_height;
                bound_y = origin_y;
                step_y = -1;
                break;
            }

            target_x = start_x;
            target_y = start_y;
            while (step_x * target_x <= step_x * bound_x &&
                   step_y * target_y <= step_y * bound_y) {
                draw_x = origin_x;
                draw_y = origin_y;
                plot_color = FIRST_CYCLING_COLOR;
                line_done = 1;
                do {
                    line_done = line_step(target_x, target_y, line_done);
                    if (color_mode < 9)
                        choose_color(color_mode);
                    else {
                        ++plot_color;
                        if (plot_color >= color_count)
                            plot_color = FIRST_CYCLING_COLOR;
                    }
                    plot_callback(0);
                } while (line_done == 0);

                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == KEY_CLEAR)
                    return;

                target_x += step_x;
                target_y += step_y;
            }
        }
        ++pass_index;
        edge_pattern = first_pattern;
        set_ega_raster_xor(0);
    }
}

/* Paints five countdown digit patterns from rectangle-stroke tables, followed by a
 * full-screen pass; uses coordinate colors and EGA XOR outside the central digit panel.
 * Original 0CCD:1F99, 587 bytes. */

void fade_countdown(void)
{
    int panel_half_width, panel_half_height, digit_half_width, digit_half_height;
    int digit_index, stroke_index, segment_index, xor_enabled;
    int coordinate, segment;

    panel_half_width = 40;
    panel_half_height = 60;
    digit_half_width = 24;
    digit_half_height = 44;
    if (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) &&
        FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700)) {
        /* The shared stroke table is doubled in place on every enhanced-EGA call. */
        for (segment = 0; segment < 8; ++segment)
            for (coordinate = 0; coordinate < 4; ++coordinate)
                countdown_segment_bounds[segment][coordinate] <<= 1;
        panel_half_width <<= 1;
        panel_half_height <<= 1;
        digit_half_width <<= 1;
        digit_half_height <<= 1;
    }

    set_ega_raster_xor(1);
    xor_enabled = 1;
    digit_index = 0;
    while (digit_index <= 5) {
        draw_y = -screen_half_height;
        while (draw_y <= screen_half_height) {
            draw_x = -screen_half_width;
            while (draw_x <= screen_half_width) {
                if (abs(draw_x) >= abs(draw_y))
                    plot_color = abs(draw_x) & color_index_mask;
                else
                    plot_color = abs(draw_y) & color_index_mask;
                if (plot_color < FIRST_CYCLING_COLOR)
                    plot_color = FIRST_CYCLING_COLOR;

                if (digit_index < 5 && draw_x <= panel_half_width && draw_x >= -panel_half_width &&
                    draw_y <= panel_half_height && draw_y >= -panel_half_height) {
                    if (xor_enabled) {
                        set_ega_raster_xor(0);
                        xor_enabled = 0;
                    }
                    if (draw_x <= digit_half_width && draw_x >= -digit_half_width &&
                        draw_y <= digit_half_height && draw_y >= -digit_half_height) {
                        stroke_index = 0;
                        while (countdown_digit_segments[digit_index][stroke_index] > 0) {
                            segment_index = countdown_digit_segments[digit_index][stroke_index] - 1;
                            if (countdown_segment_bounds[segment_index][0] <= draw_x &&
                                draw_x <= countdown_segment_bounds[segment_index][2] &&
                                draw_y <= countdown_segment_bounds[segment_index][1] &&
                                countdown_segment_bounds[segment_index][3] <= draw_y) {
                                plot_color = 1;
                                break;
                            }
                            ++stroke_index;
                        }
                    }
                } else if (!xor_enabled) {
                    set_ega_raster_xor(1);
                    xor_enabled = 1;
                }

                plot_callback(0);
                ++draw_x;
            }
            if (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) &&
                FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700))
                ++draw_y;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
            ++draw_y;
        }
        ++digit_index;
        if (digit_index == 5)
            set_ega_raster_xor(0);
    }
}

/* Sweeps a colored diagonal line and a delayed zero-color line across the display.
 * One of eight tables selects direction, bounds, separation and plotting symmetry.
 * Original 0CCD:21E4, 745 bytes. */

void fade_ziti(void)
{
    int bar_width, pixel_step, line_color, saved_palette_entries, line_height, first_origin_x;
    unsigned int symmetry;
    int finish_x, base_y, opposite_y, trailing_offset, step_x, step_y;
    int paint_active, erase_active, paint_x;
    volatile int erase_x;
    volatile int skip_palette_cycle;
    register int pattern_index, start_x;
    int line_steps;

    skip_palette_cycle = 0;
    bar_width = ((screen_half_width * 3) / 8) + random((screen_half_width * 3) / 8);
    pattern_index = random(8);
    saved_palette_entries = palette_entries;
    line_color = bar_width + FIRST_CYCLING_COLOR;
    finish_x = ziti_sweep_patterns[pattern_index][0] * screen_half_width;
    start_x = ziti_sweep_patterns[pattern_index][1] * screen_half_width;
    base_y = ziti_sweep_patterns[pattern_index][2] * screen_half_height;
    opposite_y = ziti_sweep_patterns[pattern_index][3] * screen_half_height;
    step_x = ziti_sweep_patterns[pattern_index][4];
    step_y = ziti_sweep_patterns[pattern_index][5];
    trailing_offset = ziti_sweep_patterns[pattern_index][6] * bar_width;
    symmetry = ziti_sweep_patterns[pattern_index][7];
    if (pattern_index & 1)
        trailing_offset >>= 1;
    line_height = abs(base_y - opposite_y);
    first_origin_x = start_x - step_x * line_height;
    paint_x = first_origin_x;
    erase_x = first_origin_x - trailing_offset;
    paint_active = 1;
    erase_active = 0;

    while (erase_x != finish_x) {
        if (paint_active) {
            draw_x = paint_x;
            draw_y = base_y;
            line_steps = line_height;
            plot_color = line_color;
            if (first_origin_x < 0 && paint_x < start_x) {
                draw_y = base_y + (start_x - paint_x) * step_y;
                draw_x = start_x;
                line_steps -= start_x - paint_x;
            }
            if (first_origin_x > 0 && paint_x > start_x) {
                draw_y = base_y + (paint_x - start_x) * step_y;
                draw_x = start_x;
                line_steps -= paint_x - start_x;
            }
            pixel_step = 0;
            while (pixel_step <= line_steps) {
                plot_callback(symmetry);
                if (draw_x == finish_x)
                    break;
                draw_x += step_x;
                draw_y += step_y;
                ++pixel_step;
            }
            if (video_adapter == ADAPTER_VGA) {
                --line_color;
                if (line_color < FIRST_CYCLING_COLOR)
                    line_color = color_count - 1;
            }
        }

        if (erase_active) {
            draw_x = erase_x;
            draw_y = base_y;
            line_steps = line_height;
            plot_color = 0;
            if (first_origin_x < 0 && erase_x < start_x) {
                draw_y = base_y + (start_x - erase_x) * step_y;
                draw_x = start_x;
                line_steps -= start_x - erase_x;
            }
            if (first_origin_x > 0 && erase_x > start_x) {
                draw_y = base_y + (erase_x - start_x) * step_y;
                draw_x = start_x;
                line_steps -= erase_x - start_x;
            }
            pixel_step = 0;
            while (pixel_step <= line_steps) {
                plot_callback(symmetry);
                if (draw_x == finish_x)
                    break;
                draw_x += step_x;
                draw_y += step_y;
                ++pixel_step;
            }
        }

        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
        if ((skip_palette_cycle ^ 1) != 0) {
            cycle_palette();
        } else {
            ++palette_cycle_offset;
            if (video_adapter == ADAPTER_VGA)
                palette_cycle_offset += 2;
        }
        paint_x += step_x;
        erase_x += step_x;
        if (paint_x == finish_x)
            paint_active = 0;
        if (erase_x == first_origin_x)
            erase_active = 1;
    }
    display_cleared = 1;
    palette_entries = saved_palette_entries;
}

/* Draws opposing line fans between screen edges with one of four orientations,
 * changing VGA colors and leaving the display marked for a later clearing fade.
 * Original 0CCD:24CD, 421 bytes. */
void fade_umbra(void)
{
    volatile int initial_color_offset;
    int line_pair, saved_palette_entries;
    volatile int symmetry;
    volatile int target_x;
    int end_y;
    volatile int start_x;
    int start_y;
    volatile unsigned int pattern;
    volatile int skip_palette_cycle;
    register int line_done, end_x;

    skip_palette_cycle = 0;
    initial_color_offset = 160;
    pattern = random(4);
    saved_palette_entries = palette_entries;
    plot_color = initial_color_offset + FIRST_CYCLING_COLOR;
    switch (pattern) {
    case 0:
        symmetry = 0;
        start_x = -screen_half_width;
        end_x = screen_half_width;
        start_y = -screen_half_height;
        end_y = screen_half_height;
        break;
    case 1:
        symmetry = 0;
        start_x = -screen_half_width;
        end_x = screen_half_width;
        start_y = screen_half_height;
        end_y = -screen_half_height;
        break;
    case 2:
        symmetry = 1;
        start_x = 0;
        end_x = screen_half_width;
        start_y = 0;
        end_y = screen_half_height;
        break;
    case 3:
        symmetry = 1;
        start_x = 0;
        end_x = screen_half_width;
        start_y = screen_half_height;
        end_y = 0;
        break;
    default:
        break;
    }

    target_x = start_x;
    while (target_x != end_x) {
        for (line_pair = 0; line_pair <= 1; ++line_pair) {
            draw_x = start_x;
            draw_y = end_y;
            line_done = 1;
            do {
                line_done = line_step(target_x, start_y, line_done);
                plot_callback(symmetry);
            } while (line_done == 0);

            draw_x = end_x;
            draw_y = start_y;
            do {
                line_done = line_step(end_x + start_x - target_x, end_y, line_done);
                plot_callback(symmetry);
            } while (line_done == 0);

            ++target_x;
        }

        if (video_adapter == ADAPTER_VGA) {
            --plot_color;
            if (plot_color < FIRST_CYCLING_COLOR)
                plot_color = color_count - 1;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
        if ((skip_palette_cycle ^ 1) != 0) {
            cycle_palette();
        } else {
            ++palette_cycle_offset;
            if (video_adapter == ADAPTER_VGA)
                palette_cycle_offset += 2;
        }
    }

    palette_entries = saved_palette_entries;
    display_cleared = 0;
}

/* Sweeps horizontal or vertical colored bars with an optional delayed zero-color bar.
 * Eight patterns select motion, bounds and symmetry; the trailing bar clears its path.
 * Original 0CCD:2672, 804 bytes. */
void fade_moving_bars(void)
{
    int saved_palette_entries, bar_color, keep_vga_trail, paint_active, erase_active;
    int finish_position, start_position, sweep_step, trail_offset, line_end, line_start, symmetry;
    volatile int skip_palette_cycle;
    register int pattern_index, sweep_position;
    int bar_width;

    skip_palette_cycle = 0;
    pattern_index = random(8);
    keep_vga_trail = 0;
    if ((pattern_index & 1) != 0 && video_adapter == ADAPTER_VGA)
        keep_vga_trail = random(2);

    if (pattern_index <= 3) {
        bar_width = screen_half_height / 4 + random(screen_half_height / 4);
        finish_position = moving_bars_patterns[pattern_index][0] * screen_half_height;
        start_position = moving_bars_patterns[pattern_index][1] * screen_half_height;
        trail_offset = moving_bars_patterns[pattern_index][3] * bar_width;
        line_end = moving_bars_patterns[pattern_index][4] * screen_half_width;
        line_start = moving_bars_patterns[pattern_index][5] * screen_half_width;
    } else {
        bar_width = screen_half_width / 4 + random(screen_half_width / 4);
        finish_position = moving_bars_patterns[pattern_index][0] * screen_half_width;
        start_position = moving_bars_patterns[pattern_index][1] * screen_half_width;
        trail_offset = moving_bars_patterns[pattern_index][3] * bar_width;
        line_end = moving_bars_patterns[pattern_index][4] * screen_half_height;
        line_start = moving_bars_patterns[pattern_index][5] * screen_half_height;
    }
    sweep_step = moving_bars_patterns[pattern_index][2];
    symmetry = moving_bars_patterns[pattern_index][6];
    if (pattern_index & 1)
        trail_offset >>= 1;
    saved_palette_entries = palette_entries;
    bar_color = bar_width + FIRST_CYCLING_COLOR;
    paint_active = 1;
    erase_active = 0;
    sweep_position = start_position;

    while (finish_position + trail_offset != sweep_position) {
        if (finish_position + sweep_step == sweep_position)
            paint_active = 0;
        if (start_position + trail_offset == sweep_position && keep_vga_trail == 0)
            erase_active = 1;

        if (paint_active) {
            plot_color = bar_color;
            if (pattern_index <= 3) {
                draw_y = sweep_position;
                draw_x = line_start;
                while (draw_x <= line_end) {
                    plot_callback(symmetry);
                    ++draw_x;
                }
            } else {
                draw_x = sweep_position;
                draw_y = line_start;
                while (draw_y <= line_end) {
                    plot_callback(symmetry);
                    ++draw_y;
                }
            }
        }
        if (erase_active) {
            plot_color = 0;
            if (pattern_index <= 3) {
                draw_y = sweep_position - trail_offset;
                draw_x = line_start;
                while (draw_x <= line_end) {
                    plot_callback(symmetry);
                    ++draw_x;
                }
            } else {
                draw_x = sweep_position - trail_offset;
                draw_y = line_start;
                while (draw_y <= line_end) {
                    plot_callback(symmetry);
                    ++draw_y;
                }
            }
        }

        if (video_adapter == ADAPTER_VGA) {
            --bar_color;
            if (bar_color < FIRST_CYCLING_COLOR)
                bar_color = color_count - 1;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
        if ((skip_palette_cycle ^ 1) != 0) {
            cycle_palette();
        } else {
            ++palette_cycle_offset;
            if (video_adapter == ADAPTER_VGA)
                palette_cycle_offset += 2;
        }
        sweep_position += sweep_step;
    }
    if (erase_active)
        display_cleared = 1;
    palette_entries = saved_palette_entries;
}

/* Pixelates the image with increasingly large symmetric squares, then moves sampled
 * colored blocks upward in independent columns while erasing their trailing edges.
 * Original 0CCD:2996, 881 bytes. */
void fade_tetris(void)
{
    int diagonal_y, block_index, max_level, finished_columns, last_column;
    register int block_size, column_x;

    max_level = 4;
    if (video_adapter == ADAPTER_EGA)
        max_level = 5;
    block_size = 1 << max_level;
    last_column = screen_half_width / block_size;
    /* Coarsen the existing image before treating the final square size as moving blocks. */
    for (block_index = 1; block_index <= max_level; ++block_index) {
        block_size = 1 << block_index;
        column_x = diagonal_y = 0;
        while (column_x <= screen_half_width || diagonal_y <= screen_half_height) {
            draw_x = column_x;
            draw_y = diagonal_y;
            while (draw_y >= 0 && draw_x <= screen_half_width) {
                plot_color = (unsigned char)read_pixel_callback(draw_x, draw_y);
                square(block_size, 1);
                draw_y -= block_size;
                draw_x += block_size;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == KEY_CLEAR)
                    return;
            }
            if (diagonal_y <= screen_half_height)
                diagonal_y += block_size;
            else
                column_x += block_size;
        }
    }

    /* Find a nonzero source block in each column, wrapping once before declaring it empty. */
    finished_columns = 0;
    for (block_index = 0; block_index <= last_column; ++block_index) {
        tetris_column_active[block_index] = 1;
        column_x = block_size * block_index;
        tetris_block_y[block_index] = random(screen_half_height / block_size + 1) * block_size;
        tetris_source_y[block_index] = tetris_block_y[block_index];
        while (!((tetris_block_colors[block_index] =
                  (unsigned char)read_pixel_callback(column_x, tetris_block_y[block_index])),
                 tetris_block_colors[block_index])) {
            tetris_block_y[block_index] += block_size;
            if (tetris_block_y[block_index] > screen_half_height)
                tetris_block_y[block_index] = 0;
            if (tetris_block_y[block_index] == tetris_source_y[block_index]) {
                tetris_column_active[block_index] = 0;
                ++finished_columns;
                break;
            }
        }
        tetris_source_y[block_index] = tetris_block_y[block_index];
    }

    while (finished_columns < last_column) {
        for (block_index = 0; block_index <= last_column; ++block_index) {
            column_x = block_size * block_index;
            if (tetris_column_active[block_index] != 0) {
                while (tetris_block_colors[block_index] == 0) {
                    tetris_block_y[block_index] += block_size;
                    if (tetris_block_y[block_index] > screen_half_height) {
                        tetris_column_active[block_index] = 0;
                        ++finished_columns;
                        break;
                    }
                    tetris_block_colors[block_index] =
                        (unsigned char)read_pixel_callback(column_x, tetris_block_y[block_index]);
                    tetris_source_y[block_index] = tetris_block_y[block_index];
                }
                draw_y = tetris_block_y[block_index] - 1;
                if (draw_y >= 0) {
                    plot_color = tetris_block_colors[block_index];
                    draw_x = column_x;
                    while (block_size + column_x - 1 >= draw_x) {
                        plot_callback(1);
                        ++draw_x;
                    }
                    draw_x = column_x;
                }
                draw_y = block_size + tetris_block_y[block_index] - 1;
                if (draw_y > screen_half_height)
                    draw_y = screen_half_height;
                plot_color = 0;
                draw_x = column_x;
                while (block_size + column_x - 1 >= draw_x) {
                    plot_callback(1);
                    ++draw_x;
                }
                draw_x = column_x;
                if (--tetris_block_y[block_index] <= -block_size) {
                    tetris_block_y[block_index] = tetris_source_y[block_index];
                    tetris_block_colors[block_index] = 0;
                }
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
        }
    }
    display_cleared = 1;
}

/* Requests EGA XOR and retraces shrinking horizontal/vertical line pairs with symmetry
 * for an even number of passes, using the current cycling color.
 * Original 0CCD:2D07, 281 bytes. */

void fade_exclusive_or(void)
{
    int line_done, end_x, edge_position, end_y;
    unsigned int passes_left;
    register int side_length, edge_phase;

    set_ega_raster_xor(1);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;

    passes_left = random(4) + 4;
    passes_left &= 0xfe;
    while (passes_left--) {
        side_length = edge_position = screen_half_width > screen_half_height ? screen_half_width : screen_half_height;
        edge_phase = 1;
        while (side_length > 0) {
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();

            switch (edge_phase) {
            case 1:
                draw_x = 1;
                draw_y = edge_position;
                end_x = side_length;
                end_y = edge_position;
                edge_phase++;
                break;
            case 2:
                draw_x = side_length;
                draw_y = edge_position;
                end_x = side_length;
                end_y = 1;
                edge_phase++;
                break;
            default:
                edge_phase = 1;
                --side_length;
                --edge_position;
                /* This phase reuses the preceding line endpoints after shrinking the bounds. */
            }

            line_done = 1;
            do {
                line_done = line_step(end_x, end_y, line_done);
                plot_callback(1);
            } while (line_done == 0);
        }
    }
}

/* Draws a quadrilateral, moves each corner a small fraction toward its neighbor,
 * and repeats in cycling colors until the twisted outline shrinks to the center.
 * Original 0CCD:2E20, 626 bytes. */
void fade_twist(void)
{
    float neighbor_weight, self_weight;
    float corner_x[4], corner_y[4], next_corner_x[4], next_corner_y[4];
    register int corner_index, line_done;
    int next_corner;

    effect_parameter = 0;
    if (random(3) == 2)
        effect_parameter = 1;
    corner_x[0] = corner_x[1] = screen_extent;
    corner_x[2] = corner_x[3] = -screen_extent;
    corner_y[0] = corner_y[3] = screen_extent;
    corner_y[1] = corner_y[2] = -screen_extent;
    neighbor_weight = (random(6) + 2) * 0.005;
    self_weight = 1.0 - neighbor_weight;
    plot_color = FIRST_CYCLING_COLOR & color_index_mask;
    do {
        ++plot_color;
        if (plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        draw_x = corner_x[3];
        draw_y = corner_y[3];
        /* Compute all next corners from the same old quadrilateral before copying them back. */
        for (corner_index = 0; corner_index < 4; ++corner_index) {
            line_done = 1;
            do {
                line_done = line_step(corner_x[corner_index], corner_y[corner_index], line_done);
                plot_callback(effect_parameter);
            } while (!line_done);
            next_corner = (corner_index + 1) % 4;
            next_corner_x[corner_index] =
                corner_x[corner_index] * self_weight + corner_x[next_corner] * neighbor_weight;
            next_corner_y[corner_index] =
                corner_y[corner_index] * self_weight + corner_y[next_corner] * neighbor_weight;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        for (corner_index = 0; corner_index < 4; ++corner_index) {
            corner_x[corner_index] = next_corner_x[corner_index];
            corner_y[corner_index] = next_corner_y[corner_index];
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
    } while ((abs(draw_x) + abs(draw_y)) >> 2);
    wait_fade_tick(120, 0);
}

/* Draws the outline of the rectangle (x1,y1)..(x2,y2), rotated by rectangles_angle_index,
 * with quadrant symmetry. This routine draws edges rather than swapping pixel regions.
 * Original 0CCD:3092, 660 bytes. */

void swap_rectangle(int x1, int y1, int x2, int y2)
{
    int corner_a_x, corner_c_x, corner_b_x, corner_a_y;
    volatile int corner_c_y;
    int corner_b_y, corner_d_x, corner_d_y, line_done;

    if (rectangles_angle_index > 64)
        return;

    corner_a_x = (int)(x1 * trig_table[rectangles_angle_index].cosine - y1 * trig_table[rectangles_angle_index].sine);
    corner_a_y = (int)(x1 * trig_table[rectangles_angle_index].sine + y1 * trig_table[rectangles_angle_index].cosine);
    corner_b_x = (int)(x2 * trig_table[rectangles_angle_index].cosine - y2 * trig_table[rectangles_angle_index].sine);
    corner_b_y = (int)(x2 * trig_table[rectangles_angle_index].sine + y2 * trig_table[rectangles_angle_index].cosine);
    corner_c_x = (int)(x2 * trig_table[rectangles_angle_index].cosine - y1 * trig_table[rectangles_angle_index].sine);
    corner_c_y = (int)(x2 * trig_table[rectangles_angle_index].sine + y1 * trig_table[rectangles_angle_index].cosine);
    corner_d_x = (int)(x1 * trig_table[rectangles_angle_index].cosine - y2 * trig_table[rectangles_angle_index].sine);
    corner_d_y = (int)(x1 * trig_table[rectangles_angle_index].sine + y2 * trig_table[rectangles_angle_index].cosine);

    draw_x = corner_a_x;
    draw_y = corner_a_y;
    line_done = 1;
    do {
        line_done = line_step(corner_c_x, corner_c_y, line_done);
        plot_callback(1);
    } while (!line_done);
    draw_x = corner_c_x;
    draw_y = corner_c_y;
    do {
        line_done = line_step(corner_b_x, corner_b_y, line_done);
        plot_callback(1);
    } while (!line_done);
    draw_x = corner_b_x;
    draw_y = corner_b_y;
    do {
        line_done = line_step(corner_d_x, corner_d_y, line_done);
        plot_callback(1);
    } while (!line_done);
    draw_x = corner_d_x;
    draw_y = corner_d_y;
    do {
        line_done = line_step(corner_a_x, corner_a_y, line_done);
        plot_callback(1);
    } while (!line_done);
}

/* Draws expanding square outlines with advancing rotation and color.
 * One variant follows each outline with a smaller zero-color outline at a nearby angle.
 * Original 0CCD:3326, 273 bytes. */

void fade_rectangles(void)
{
    int size_limit, size, saved_color;

    rectangles_angle_index = 0;
    effect_parameter = random(3);
    size_limit = screen_extent / 2 + random(screen_extent / 2);
    for (size = 0; size < size_limit; ++size) {
        rectangles_angle_index += 4;
        rectangles_angle_index %= 64;
        ++plot_color;
        if (plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        swap_rectangle(0, 0, size, size);
        if (effect_parameter == 0) {
            rectangles_angle_index -= 3;
            if (rectangles_angle_index < 0)
                rectangles_angle_index += 64;
            saved_color = plot_color;
            plot_color = 0;
            swap_rectangle(0, 0, size - 3, size - 3);
            plot_color = saved_color;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
    }
}

/* Moves two reflected line endpoints for a timed interval, replacing an older line
 * from a ring buffer with each new line; chooses zero-color or EGA XOR trail erasure.
 * Original 0CCD:3437, 958 bytes. */

void fade_linewipe(void)
{
    int start_x, start_y, end_x, end_y;
    int steps_left, end_step_x, start_step_y, end_step_y, line_done;
    unsigned int saved_color;
    int trail_length, xor_trail;
    register int history_index, start_step_x;

    for (history_index = 0; history_index < 250; ++history_index) {
        ((struct LineWipePoint far *)linewipe_history)[history_index].x0 = 0;
        ((struct LineWipePoint far *)linewipe_history)[history_index].x1 = 0;
        ((struct LineWipePoint far *)linewipe_history)[history_index].y0 = 0;
        ((struct LineWipePoint far *)linewipe_history)[history_index].y1 = 0;
    }
    start_x = start_y = end_x = end_y = steps_left = history_index = 0;
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    if (video_adapter == ADAPTER_EGA)
        trail_length = random(125) + 82;
    else
        trail_length = random(125) + 124;
    switch (random(3)) {
    case 0:
        effect_parameter = 1;
        break;
    case 1:
        effect_parameter = 1;
        break;
    default:
        effect_parameter = 3;
    }
    xor_trail = !random(3);
    set_ega_raster_xor(xor_trail);
    fade_timer_ticks = random(300) + 90;

    while (fade_timer_ticks != 0 && current_graphics_key != KEY_CLEAR) {
        if (steps_left <= 0) {
            start_step_x = random(9) - 4;
            if (start_step_x == 0)
                ++start_step_x;
            end_step_x = random(9) - 4;
            start_step_y = random(9) - 4;
            if (start_step_y == 0)
                --start_step_y;
            end_step_y = random(9) - 4;
            steps_left = (random(8) + 1) * 4;
        }

        if (!xor_trail) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
        start_x += start_step_x;
        if (abs(start_x) >= screen_extent)
            start_step_x = -start_step_x;
        end_x += end_step_x;
        if (abs(end_x) >= screen_extent)
            end_step_x = -end_step_x;
        start_y += start_step_y;
        if (abs(start_y) >= screen_extent)
            start_step_y = -start_step_y;
        end_y += end_step_y;
        if (abs(end_y) >= screen_extent)
            end_step_y = -end_step_y;
        --steps_left;

        saved_color = plot_color;
        if (!xor_trail)
            plot_color = 0;
        draw_x = ((struct LineWipePoint far *)linewipe_history)[history_index].x0;
        draw_y = ((struct LineWipePoint far *)linewipe_history)[history_index].y0;
        line_done = 1;
        do {
            line_done = line_step(((struct LineWipePoint far *)linewipe_history)[history_index].x1,
                               ((struct LineWipePoint far *)linewipe_history)[history_index].y1, line_done);
            plot_callback(effect_parameter);
        } while (line_done == 0);
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        plot_color = saved_color & color_index_mask;
        ((struct LineWipePoint far *)linewipe_history)[history_index].x0 = start_x;
        ((struct LineWipePoint far *)linewipe_history)[history_index].x1 = end_x;
        ((struct LineWipePoint far *)linewipe_history)[history_index].y0 = start_y;
        ((struct LineWipePoint far *)linewipe_history)[history_index].y1 = end_y;
        ++history_index;
        if (history_index >= trail_length)
            history_index = 0;
        draw_x = start_x;
        draw_y = start_y;
        do {
            line_done = line_step(end_x, end_y, line_done);
            plot_callback(effect_parameter);
        } while (line_done == 0);
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* Coordinates and velocities use signed 16.16 values. */

/* Animates outward points with signed 16.16 positions and velocities, restarting points
 * at the center after they leave the bounds. A delayed second set erases their trails.
 * Original 0CCD:37F5, 1625 bytes. */

void fade_starburst(void)
{
    register int particle_count;
    register unsigned int update_index, tail_index;
    unsigned int velocity_limit, tail_delay;
    struct StarburstState far *heads;
    struct StarburstState far *tails;

    particle_count = random(75) + 75;
    if ((heads = (struct StarburstState far *)calloc(particle_count, sizeof(*heads))) == 0 ||
        (tails = (struct StarburstState far *)calloc(particle_count, sizeof(*tails))) == 0) {
        exit_requested = 1;
        fatal_selfcheck_error(0, "Memory exhausted in star_burst routine!");
    }
    set_ega_raster_xor(0);
    effect_parameter = random(3);
    fade_timer_ticks = random(300) + 90;
    if (video_adapter == ADAPTER_VGA)
        velocity_limit = 0x3ffe;
    else
        velocity_limit = 0x7eee;
    if (effect_parameter == 0) {
        velocity_limit >>= 1;
        fade_timer_ticks >>= 1;
    }

    for (update_index = 0; update_index < particle_count; ++update_index) {
        tails[update_index].x.value = heads[update_index].x.value = 0;
        tails[update_index].y.value = heads[update_index].y.value = 0;
        heads[update_index].dx.value = random(velocity_limit) + 1;
        tails[update_index].dx.value = heads[update_index].dx.value <<= 4;
        heads[update_index].dy.value = random(velocity_limit) + 1;
        tails[update_index].dy.value = heads[update_index].dy.value <<= 4;
        tails[update_index].color = heads[update_index].color = random(palette_entries) + 1;
    }

    update_index = 0;
    if (effect_parameter == 0)
        tail_delay = particle_count / 2 + random(particle_count / 2);
    else
        tail_delay = random(4) + 3;

    /* Head passes precede delayed eraser passes; both keep per-particle 16.16 motion. */
    while (fade_timer_ticks != 0 && current_graphics_key != KEY_CLEAR) {
        if (update_index < particle_count) {
            plot_color = heads[update_index].color & color_index_mask;
            draw_x = heads[update_index].x.word.integer;
            draw_y = heads[update_index].y.word.integer;
            heads[update_index].x.value += heads[update_index].dx.value;
            heads[update_index].y.value += heads[update_index].dy.value;
            if (draw_y > screen_half_height || draw_x > screen_half_width) {
                heads[update_index].y.value = heads[update_index].x.value = 0;
                heads[update_index].dx.value = random(velocity_limit) + 1;
                heads[update_index].dx.value <<= 4;
                heads[update_index].dy.value = random(velocity_limit) + 1;
                heads[update_index].dy.value <<= 4;
            }
            ++update_index;
        } else if (tail_delay < 1) {
            tail_index = update_index - particle_count;
            plot_color = 0;
            draw_x = tails[tail_index].x.word.integer;
            draw_y = tails[tail_index].y.word.integer;
            tails[tail_index].x.value += tails[tail_index].dx.value;
            tails[tail_index].y.value += tails[tail_index].dy.value;
            if (draw_x > screen_half_width || draw_y > screen_half_height) {
                tails[tail_index].y.value = tails[tail_index].x.value = 0;
                tails[tail_index].dx.value = heads[tail_index].dx.value;
                tails[tail_index].dy.value = heads[tail_index].dy.value;
            }
            ++update_index;
        } else {
            update_index = 0;
            --tail_delay;
        }

        if (update_index >= particle_count * 2)
            update_index = 0;
        plot_callback(1);
        if (input_pending != 0 && poll_graphics_keys() != 0)
            break;
    }

    free(heads);
    free(tails);
}

/* Traverses a quadrant with reflected plotting: paint_columns != 0 paints colored columns;
 * zero erases rows. display_cleared records whether the chosen pass clears the display.
 * Original 0CCD:3E4E, 344 bytes. */
void fade_wipe(int paint_columns)
{
    plot_color = 0;
    if (paint_columns) {
        ++plot_color;
        if (plot_color >= color_count)
            plot_color = FIRST_CYCLING_COLOR;
        draw_x = 0;
        while (draw_x <= screen_half_width) {
            draw_y = 0;
            while (draw_y <= screen_half_height) {
                plot_callback(1);
                if (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) &&
                    FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700))
                    ++draw_y;
                ++draw_y;
            }
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if (video_adapter == ADAPTER_EGA) {
                if ((draw_x & (color_count - 1)) == 0) {
                    ++plot_color;
                    if (plot_color >= color_count)
                        plot_color = FIRST_CYCLING_COLOR;
                }
            } else {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = FIRST_CYCLING_COLOR;
            }
            ++draw_x;
        }
        display_cleared = 0;
        return;
    }

    draw_y = 0;
    while (draw_y <= screen_half_height) {
        draw_x = 0;
        while (draw_x <= screen_half_width) {
            plot_callback(1);
            ++draw_x;
        }
        if (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) &&
            FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700))
            ++draw_y;
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == KEY_CLEAR)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        ++draw_y;
    }
    display_cleared = 1;
}

/* Runs complete expanding hexagon passes with quadrant symmetry and optional EGA XOR,
 * restarting the image generator after its coordinates pass the drawing extent.
 * Original 0CCD:3FA6, 171 bytes. */

void hexagon_transition(void)
{
    int passes_left;

    set_ega_raster_xor(!random(3));
    passes_left = random(4) + 1;
    while (passes_left >= 0) {
        hexagon(1);
        plot_callback(1);
        do {
            hexagon(0);
            plot_callback(1);
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
        } while (draw_y <= screen_extent || draw_x <= screen_extent);
        --passes_left;
    }
}

/* Scrolls the physical display start one row at a time from from_row to to_row,
 * servicing palette timing and interruption commands after each retrace-synchronized step.
 * Original 0CCD:4051, 105 bytes. */

void animate_vertical_display_start(int from_row, int to_row)
{
    int row_step, row;

    row = from_row;
    if (from_row < to_row)
        row_step = 1;
    else
        row_step = -1;
    while (row != to_row) {
        if (row != to_row)
            row += row_step;
        display_start_callback(0, row);
        if (input_pending != 0 || exit_requested != 0) {
            if (poll_graphics_keys() != 0)
                return;
        }
        if (current_graphics_key == KEY_CLEAR)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* Moves the EGA/VGA line-compare split through the physical display in either direction,
 * with palette/input service and BIOS-resolution adjustment; skips TARGA+64 mode.
 * Original 0CCD:40BA, 328 bytes. */

void animate_line_compare_split(int bottom_to_top)
{
    int scanline;

    if (targa64_mode != 0)
        return;
    if (video_adapter == ADAPTER_EGA) {
        if (bottom_to_top) {
            for (scanline = EGA_PHYSICAL_HEIGHT; scanline >= 0; scanline--) {
                ega_set_line_compare(scanline);
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == KEY_CLEAR)
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        } else {
            for (scanline = 0; scanline < EGA_PHYSICAL_HEIGHT; scanline++) {
                ega_set_line_compare(scanline);
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == KEY_CLEAR)
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        }
    } else {
        if (bottom_to_top) {
            for (scanline = VGA_ENHANCED_PHYSICAL_HEIGHT; scanline >= 0; scanline--) {
                vga_set_line_compare(scanline);
                if (bios_resolution_mode != 0 && (scanline & 1) != 0)
                    scanline++;
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == KEY_CLEAR)
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        } else {
            for (scanline = 0; scanline < VGA_ENHANCED_PHYSICAL_HEIGHT; scanline++) {
                vga_set_line_compare(scanline);
                if (bios_resolution_mode != 0 && (scanline & 1) != 0)
                    scanline++;
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == KEY_CLEAR)
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        }
    }
}

/* Stamps nonzero samples from the shared shaded-ball bitmap at random positions for a
 * timed interval, varying sample stride, symmetry, color offsets and palette timing.
 * Original 0CCD:4202, 649 bytes. */
void fade_blister(void)
{
    int color_offset, keep_color_offset, skip_palette_cycle;
    unsigned char far *sample;
    int x_origin, y_origin;
    int sample_stride;
    int flat_color;
    int bitmap_y, bitmap_x;

    color_offset = 0;
    sample_stride = 1;
    fade_timer_ticks = random(180) + 90;
    keep_color_offset = random(3);
    flat_color = random(5) == 0;
    skip_palette_cycle = random(5) == 0;
    if (random(3) != 0)
        effect_parameter = 1;
    else
        effect_parameter = 3;
    if (random(3) == 0) {
        sample_stride = 2;
        fade_timer_ticks >>= 1;
    }
    while (fade_timer_ticks != 0) {
        sample = ball_bitmap_pixels;
        if (keep_color_offset == 0)
            color_offset = random(color_count);
        if (flat_color) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = FIRST_CYCLING_COLOR;
        }
        y_origin = random(screen_half_height + ball_bitmap_height) - ball_bitmap_height;
        x_origin = random(screen_half_width + ball_bitmap_width) - ball_bitmap_width;
        for (bitmap_x = 0; bitmap_x < ball_bitmap_width; bitmap_x += sample_stride) {
            for (bitmap_y = 0; bitmap_y < ball_bitmap_height; bitmap_y += sample_stride) {
                int sample_color = *sample;
                sample += sample_stride;
                if (sample_color != 0) {
                    if (!flat_color)
                        plot_color = (sample_color + color_offset) & color_index_mask;
                    draw_x = x_origin + bitmap_x;
                    draw_y = y_origin + bitmap_y;
                    plot_callback(effect_parameter);
                }
            }
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                return;
            if (!skip_palette_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    }

    wait_fade_tick(60, skip_palette_cycle);
}

/* Decrements nonzero palette bytes to zero and uploads intermediate palettes.
 * mask_high_bits selects six-bit masking versus setting the high bits before the fade.
 * Original 0CCD:448B, 248 bytes. */
void fade_dac_to_black(int mask_high_bits)
{
    volatile int upload_delay_mode;
    unsigned char far *component;
    int component_count, component_index;
    register int nonzero_bits;

    component_count = (color_count + palette_entries) * RGB_COMPONENTS;
    component = palette_buffer;
    if (targa64_mode != 0) {
        upload_delay_mode = 0;
    } else {
        upload_delay_mode = random(4);
        for (component_index = 0; component_index < component_count; ++component_index) {
            if (mask_high_bits)
                *component &= DAC_COMPONENT_MASK;
            else
                *component |= DAC_HIGH_BITS_MASK;
            ++component;
        }
    }

    do {
        nonzero_bits = 0;
        component = palette_buffer;
        for (component_index = 0; component_index < component_count; ++component_index) {
            if (*component > 0)
                (*component)--;
            nonzero_bits |= *component;
            ++component;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        upload_delay_mode ^= 1;
        switch (upload_delay_mode) {
        case 0:
            break;
        default:
            palette_upload_callback(palette_buffer + palette_cycle_offset);
        }
    } while (nonzero_bits != 0);
    display_cleared = 1;
}

/* Overwrites two moving palette windows with the background value, wrapping within the
 * shared buffer; a VGA alternative delegates to the DAC decrement fade.
 * Original 0CCD:4583, 402 bytes. */

void fade_palette_to_background(void)
{
    unsigned int window_bytes;
    register int half_window_bytes, bytes_written;
    unsigned int background_component;
    unsigned char far *first_window, far *second_window, far *buffer_end;

    background_component = background_fill_word & 0xff;
    if (video_adapter == ADAPTER_EGA)
        window_bytes = color_count;
    else
        window_bytes = color_count * RGB_COMPONENTS;
    half_window_bytes = window_bytes >> 1;

    /* Replace two halves in parallel, wrapping each pointer at the buffer end. */
    first_window = palette_buffer + palette_cycle_offset;
    second_window = palette_buffer + (palette_cycle_offset + (window_bytes >> 1)) % palette_buffer_bytes;
    buffer_end = palette_buffer + palette_buffer_bytes;

    if (video_adapter == ADAPTER_EGA || random(2) != 0) {
        bytes_written = 0;
        while (bytes_written < half_window_bytes) {
            *second_window++ = *first_window++ = (unsigned char)background_component;
            if (video_adapter == ADAPTER_VGA) {
                *second_window++ = *first_window++ = (unsigned char)background_component;
                ++bytes_written;
                *second_window++ = *first_window++ = (unsigned char)background_component;
                ++bytes_written;
            } else {
                delay(75);
            }
            if (FP_OFF(first_window) >= FP_OFF(buffer_end))
                first_window = palette_buffer;
            if (FP_OFF(second_window) >= FP_OFF(buffer_end))
                second_window = palette_buffer;
            palette_upload_callback(palette_buffer + palette_cycle_offset);
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            ++bytes_written;
        }
    } else {
        fade_dac_to_black(fade_juggle_palette_enabled == 0 || random(35) != 0 || current_graphics_key == KEY_CLEAR);
    }
}

/* Chooses a keyed or random fade index, checks enable/adapter constraints and dispatches it.
 * Returns whether another transition should follow; also prepares the Fireworks background.
 * Original 0CCD:4715, 877 bytes. */

int dispatch_fade_key(void)
{
    unsigned int fade_index;
    register int repeat_fade = 0, key_selected;

    for (;;) {
        key_selected = 0;
        fade_index = random(35);
        if (current_graphics_key >= FIRST_FADE_KEY && current_graphics_key <= LAST_FADE_KEY) {
            fade_index = current_graphics_key - FIRST_FADE_KEY;
            key_selected = 1;
        }
        current_graphics_key = 0;

        /* Cases are lowercase key offsets from 'a'; spare random indices select no effect. */
        switch (fade_index) {
        case 0:
            if (fade_randomize_enabled == 0)
                continue;
            randomize_transition();
            break;
        case 1:
            if (fade_blister_enabled == 0)
                continue;
            fade_blister();
            break;
        case 2:
            if (fade_countdown_enabled == 0)
                continue;
            if (!key_selected && random(3) != 0)
                continue;
            fade_countdown();
            repeat_fade = 1;
            break;
        case 3:
            if (fade_drip_enabled == 0)
                continue;
            drip_transition();
            break;
        case 4:
            if (fade_exclusive_or_enabled == 0)
                continue;
            fade_exclusive_or();
            break;
        case 5:
            if (fade_firefly_enabled == 0)
                continue;
            firefly_transition(1);
            break;
        case 6:
            if (fade_gas_cloud_enabled == 0 || video_adapter == ADAPTER_EGA)
                continue;
            gas_cloud();
            repeat_fade = !bios_resolution_mode;
            break;
        case 7:
            continue;
        case 8:
            if (fade_spiral_enabled == 0)
                continue;
            spiral_transition();
            break;
        case 9:
            if (fade_juggle_palette_enabled == 0 || video_adapter == ADAPTER_EGA)
                continue;
            if (!key_selected && random(4) != 0)
                continue;
            fade_dac_to_black(0);
            break;
        case 10:
            if (fade_kleenup_enabled == 0)
                continue;
            cleanup_transition();
            break;
        case 11:
            if (fade_linewipe_enabled == 0)
                continue;
            fade_linewipe();
            break;
        case 12:
            if (fade_mesh_enabled == 0)
                continue;
            mesh_transition();
            break;
        case 14:
            if (fade_fireworks_enabled == 0)
                continue;
            {
                fade_palette_to_background();
                if (video_adapter == ADAPTER_VGA)
                    randomize_palette_state(0);
                *(unsigned char *)&background_fill_word = background_alt_color = (unsigned char)random(4);
                page_paragraph = 0;
                mode_setup_callback(1);
                palette_build_callback(0);
                palette_upload_callback(palette_buffer + palette_cycle_offset);
                random_delay_transition();
                display_cleared = 0;
            }
            break;
        case 16:
            if (fade_quilt_enabled == 0)
                continue;
            quilt_transition();
            break;
        case 17:
            if (fade_rectangles_enabled == 0)
                continue;
            fade_rectangles();
            break;
        case 19:
            if (fade_twist_enabled == 0)
                continue;
            fade_twist();
            break;
        case 20:
            if (fade_umbra_enabled == 0 || video_adapter == ADAPTER_EGA)
                continue;
            fade_umbra();
            repeat_fade = 1;
            break;
        case 21:
            if (fade_moving_bars_enabled == 0 || video_adapter == ADAPTER_EGA)
                continue;
            fade_moving_bars();
            repeat_fade = !display_cleared;
            break;
        case 22:
            if (fade_wipe_enabled == 0)
                continue;
            fade_wipe(random(2));
            repeat_fade = !display_cleared;
            break;
        case 23:
            if (fade_hexagon_enabled == 0)
                continue;
            hexagon_transition();
            if (random(2) != 0)
                break;
        /* X may continue with an enabled Starburst. */
        case 18:
            if (fade_starburst_enabled != 0)
                fade_starburst();
            break;
        case 24:
            if (fade_tetris_enabled == 0)
                continue;
            fade_tetris();
            break;
        case 25:
            if (fade_ziti_enabled == 0 || video_adapter == ADAPTER_EGA)
                continue;
            fade_ziti();
            repeat_fade = !display_cleared;
            break;
        default:
            break;
        }

        return repeat_fade && (random(5) != 0 || key_selected);
    }
}

/* Coordinates hidden-page drawing, vertical scrolling, Auto pause and fade dispatch.
 * Finishes palette clearing as needed, chooses the next background/border and resets state.
 * Original 0CCD:4A82, 1224 bytes. */

void display_boundary_fade_and_pan(void)
{
    struct REGPACK idle_args;
    register int scroll_led_on = 0, repeat_fade = 0;

    set_ega_raster_xor(0);
    if (display_pan_state == 0 && (random(6) == 0 || current_graphics_key == KEY_PAN_SCROLL))
        display_pan_state = 1;
    if ((current_graphics_key != 0 && current_graphics_key != KEY_PAN_SCROLL && display_pan_state == 1) ||
        semi_auto_mode != 0 || fade_pan_scroll_enabled == 0 ||
        (video_adapter == ADAPTER_VGA && bios_resolution_mode != 0))
        display_pan_state = 0;

    /* State 1 starts hidden-page drawing; later states reveal the completed page. */
    switch (display_pan_state) {
        case 0:
            break;
        case 1:
            if (page_paragraph != 0) {
                display_pan_state = 0;
            } else {
                saved_pan_draw_delay = draw_delay_count;
                draw_delay_count = 1;
                ++display_pan_state;
                if (current_graphics_key == KEY_PAN_SCROLL)
                    previous_graphics_key = current_graphics_key = 0;
                if (video_adapter == ADAPTER_EGA)
                    page_paragraph = EGA_SECOND_PAGE_PARAGRAPH;
                else
                    page_paragraph = VGA_SECOND_PAGE_PARAGRAPH;
                keyboard_led_flags(LED_NUM_LOCK, 0);
                return;
            }
            break;
        default:
            draw_delay_count = saved_pan_draw_delay;
            animate_vertical_display_start(0, video_adapter == ADAPTER_EGA ?
                                           EGA_PHYSICAL_HEIGHT : screen_half_height * 2);
            if ((random(3) == 0 || current_graphics_key == KEY_PAN_SCROLL) &&
                (current_graphics_key == 0 || current_graphics_key == KEY_PAN_SCROLL)) {
                animate_line_compare_split(1);
                animate_line_compare_split(0);
            }
    }
    keyboard_led_flags(LED_NUM_AND_CAPS_LOCK, 1);
    fade_timer_ticks = auto_fade_pause_ticks;
    if (video_adapter == ADAPTER_EGA)
        fade_timer_ticks /= 2;
    if (palette_alternate_mode != 0)
        fade_timer_ticks /= 2;
    if (palette_random_mode != 0)
        fade_timer_ticks /= 3;
    if (semi_auto_mode == 0) {
        if (show_mode != 0 && fade_timer_ticks > show_duration_ticks)
            fade_timer_ticks = show_duration_ticks;
        while (fade_timer_ticks != 0) {
            geninterrupt(DOS_IDLE_INTERRUPT);
            if (_osmajor >= 3) {
                idle_args.r_ax = DOS_RELEASE_TIME_SLICE;
                intr(DOS_MULTIPLEX_INTERRUPT, &idle_args);
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == KEY_CLEAR)
                fade_timer_ticks = 0;
            if (current_graphics_key == KEY_HOLD)
                fade_timer_ticks = 7;
            if ((fade_timer_ticks & 15) < 8) {
                if (!scroll_led_on) {
                    scroll_led_on = 1;
                    keyboard_led_flags(LED_SCROLL_LOCK, 0);
                    fprintf(stdout, "\r");
                }
            } else if (scroll_led_on) {
                scroll_led_on = 0;
                keyboard_led_flags(LED_SCROLL_LOCK, 1);
            }
        }
    }
    keyboard_led_flags(LED_SCROLL_LOCK, 1);
    if (current_graphics_key != KEY_CLEAR)
        repeat_fade = dispatch_fade_key();
    if (repeat_fade && current_graphics_key != KEY_CLEAR) {
        display_cleared = 0;
    } else {
        repeat_fade = 0;
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (display_cleared == 0)
            fade_palette_to_background();
        setmem(palette_buffer, palette_buffer_bytes, 0);
        ega_border_color = background_fill_word = 0;
        palette_upload_callback(palette_buffer);
        fprintf(stdout, "\r");
        ega_border_color = random(color_count);
        if (video_adapter == ADAPTER_EGA) {
            if (random(3) != 0)
                background_fill_word = 8 & ~ega_border_color;
            else
                background_fill_word = random(4);
        } else {
            if (random(3) != 0) {
                background_fill_word = 0;
            } else {
                background_fill_word = random(4);
                background_alt_color = random(3) != 0 ? (unsigned char)background_fill_word : random(4) + 1;
            }
        }
        if (hide_ega_border != 0)
            ega_border_color = 0;
    }
    if (show_mode != 0 && show_duration_ticks == 0)
        exit_requested = 1;
    if (exit_requested == 0 && repeat_fade == 0) {
        if (video_adapter == ADAPTER_VGA)
            randomize_palette_state(0);
        mode_setup_callback(1);
        palette_build_callback(0);
        palette_upload_callback(palette_buffer + palette_cycle_offset);
        if (video_adapter == ADAPTER_EGA)
            delay(350);
        page_paragraph = 0;
        display_cleared = 1;
    }
    display_pan_state = palette_elapsed_ticks = 0;
    previous_graphics_key = current_graphics_key = 0;
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
int saved_pan_draw_delay; /* DS:7436 */
int tetris_source_y[21]; /* DS:740C */
unsigned int tetris_block_colors[21]; /* DS:73E2 */
int tetris_block_y[21]; /* DS:73B8 */
unsigned int tetris_column_active[21]; /* DS:738E */
int firefly_path_state[17]; /* DS:736C */
int firefly_y_history[5]; /* DS:7362 */
int firefly_x_history[5]; /* DS:7358 */
int quilt_tile_height; /* DS:7356 */
int quilt_tile_width; /* DS:7354 */
int quilt_second_y; /* DS:7352 */
int quilt_second_x; /* DS:7350 */
int quilt_first_y; /* DS:734E */
int quilt_first_x; /* DS:734C */
int quilt_flip_mode; /* DS:734A */
int quilt_grid_divisions; /* DS:7348 */
float fireworks_aspect_ratio; /* DS:7344 */
transition_particle_t fireworks_particles[4][64]; /* DS:6144 */
int fireworks_displaced_origins; /* DS:6142 */
int fireworks_background_x_mask; /* DS:6140 */
int fireworks_trail_mode; /* DS:613E */
int fireworks_background_b; /* DS:613C */
int fireworks_background_a; /* DS:613A */
int fireworks_acceleration; /* DS:6138 */
int fireworks_gravity[4]; /* DS:6130 */
int fireworks_active[4]; /* DS:6128 */
int fireworks_launch_delay[4]; /* DS:6120 */
int fireworks_colors[4]; /* DS:6118 */
int fireworks_finished_particles[4]; /* DS:6110 */
int fireworks_age[4]; /* DS:6108 */
int fireworks_origin_y[4]; /* DS:6100 */
int fireworks_origin_x[4]; /* DS:60F8 */

/* Rectangle, countdown, sweep and moving-bar pattern tables. */
int countdown_digit_segments[5][6] = {
    1, 4, 2, 7, 3, 0, 4, 2,
    5, 7, 0, 0, 1, 5, 2, 7,
    3, 0, 1, 5, 2, 6, 3, 0,
    8, 0, 0, 0, 0, 0
};
int countdown_segment_bounds[8][4] = {
    -24, -36, 24, -44, -24, 4, 24, -4,
    -24, 44, 24, 36, -24, 4, -16, -44,
    16, 4, 24, -44, -24, 44, -16, -4,
    16, 44, 24, -4, -4, 44, 4, -44
};
int ziti_sweep_patterns[8][8] = {
    1, -1, 1, -1, 1, -1, 1, 0,
    1, 0, 1, 0, 1, -1, 1, 1,
    -1, 1, -1, 1, -1, 1, -1, 0,
    0, 1, 0, 1, -1, 1, -1, 1,
    1, -1, -1, 1, 1, 1, 1, 0,
    1, 0, 0, 1, 1, 1, 1, 1,
    -1, 1, 1, -1, -1, -1, -1, 0,
    0, 1, 1, 0, -1, -1, -1, 1
};
int moving_bars_patterns[8][7] = {
    1, -1, 1, 1, 1, -1, 0, 1,
    0, 1, 1, 1, 0, 1, -1, 1,
    -1, -1, 1, -1, 0, 0, 1, -1,
    -1, 1, 0, 1, 1, -1, 1, 1,
    1, -1, 0, 1, 0, 1, 1, 1,
    0, 1, -1, 1, -1, -1, 1, -1,
    0, 0, 1, -1, -1, 1, -1, 1
};
