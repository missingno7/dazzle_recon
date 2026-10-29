/* DAZZLE 5.0 fade module (DZFD), BC++ 3.00 -ml -O1. */
#include <dos.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "dazzle.h"


/* The two carriage-return uses share the original DS:0BFC pool entry. */
#pragma option -d

/* 0CCD:0000..000A is the preceding 0B84 TU's trailing switch table.
 * DOSI's signal diagnostic calls the far getter at this module's 000A start. */

/* 0CCD:000A, 3 bytes */
unsigned int dzfd_seg(void)
{
    return _CS;
}

/* 0CCD:000D, 128 bytes */
void wait_fade_tick(int requested, int skip_palette)
{
    struct REGPACK dos_args;

    if (requested > (int)auto_fade_pause_ticks / 2)
        requested = (int)auto_fade_pause_ticks / 2;
    fade_timer_ticks = requested;

    while (fade_timer_ticks != 0) {
        if (input_pending != 0 || exit_requested != 0) {
            if (poll_graphics_keys() != 0)
                break;
        }
        if (current_graphics_key == 'n')
            break;
        if (skip_palette == 0) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        geninterrupt(0x28);
        if (_osmajor >= 3) {
            dos_args.r_ax = 0x1680;
            intr(0x2f, &dos_args);
        }
    }
}
/* 0CCD:008D, 2792 bytes */
void randomize_helper(int steps)
{
    register int mode = steps;
    int lane, point, tail_step, min_x, min_y, speed, direction;

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
        for (lane = 0; lane <= 3; ++lane) {
            for (point = 0; point <= 63; ++point) {
                speed = random(65) + 64;
                direction = random(64);
                fireworks_particles[lane][point].velocity_x = speed * trig_table[direction].sine * fireworks_aspect_ratio;
                fireworks_particles[lane][point].velocity_y = -(speed * trig_table[direction].cosine);
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
                if (video_adapter == 8) {
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
        min_x = random(screen_half_width) - screen_half_width / 2;
        min_y = -random(screen_half_height / 2);

        for (lane = 0; lane <= 3; ++lane) {
            if (fireworks_displaced_origins != 0) {
                fireworks_origin_x[lane] = min_x + random(screen_half_width) - screen_half_width / 2;
                fireworks_origin_y[lane] = min_y + random(screen_half_height) - screen_half_height / 2;
            } else
                fireworks_origin_x[lane] = fireworks_origin_y[lane] = 0;
            fireworks_colors[lane] = random(color_count - 3) + 3;
            fireworks_age[lane] = fireworks_finished_particles[lane] = 0;
            fireworks_gravity[lane] = 0;
            fireworks_active[lane] = 0;
            fireworks_launch_delay[lane] = screen_half_height / 8 + random(screen_half_height / 2);

            for (point = 0; point <= 63; ++point) {
                fireworks_particles[lane][point].head_x =
                    (fireworks_origin_x[lane] + random(9) - 4) * 64;
                fireworks_particles[lane][point].head_y =
                    (fireworks_origin_y[lane] + random(9) - 4) * 64;
                fireworks_particles[lane][point].tail_x = fireworks_particles[lane][point].head_x;
                fireworks_particles[lane][point].tail_y = fireworks_particles[lane][point].head_y;
                fireworks_particles[lane][point].tail_start = screen_half_height / 8 + random(screen_half_height / 4);
                fireworks_particles[lane][point].tail_end = fireworks_particles[lane][point].tail_start * 2
                                        + random(screen_half_height / 4);
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }

        if (fireworks_trail_mode == 0)
            set_ega_raster_xor(1);
    }

    lane = 0;
    fireworks_active[0] = 1;
    while (fireworks_active[0] != 0 || fireworks_active[1] != 0 ||
           fireworks_active[2] != 0 || fireworks_active[3] != 0) {
        if (fireworks_active[lane] != 0) {
            for (point = 0; point <= 63; ++point) {
                int span;
                plot_color = fireworks_colors[lane];
                if (fireworks_trail_mode == 1)
                    span = fireworks_particles[lane][point].tail_start;
                else
                    span = fireworks_particles[lane][point].tail_end;
                if (fireworks_age[lane] < span) {
                    draw_x = fireworks_particles[lane][point].head_x >> 6;
                    draw_y = fireworks_particles[lane][point].head_y >> 6;
                    if (abs(draw_x) <= screen_half_width && abs(draw_y) <= screen_half_height) {
                        plot_callback(0);
                        fireworks_particles[lane][point].head_x += fireworks_particles[lane][point].velocity_x;
                        fireworks_particles[lane][point].head_y += fireworks_particles[lane][point].velocity_y
                                                  + fireworks_gravity[lane];
                    } else if (fireworks_trail_mode == 1)
                        fireworks_particles[lane][point].tail_start = fireworks_age[lane];
                }

                if ((fireworks_trail_mode & 1) == 0 &&
                    fireworks_particles[lane][point].tail_start <= fireworks_age[lane] &&
                    fireworks_particles[lane][point].tail_end >= fireworks_age[lane]) {
                    plot_color = fireworks_colors[lane];
                    draw_x = fireworks_particles[lane][point].tail_x >> 6;
                    draw_y = fireworks_particles[lane][point].tail_y >> 6;
                    if (fireworks_trail_mode == 2)
                        plot_color = fireworks_background_a;
                    if (fireworks_trail_mode == 4 && (draw_x & fireworks_background_x_mask) != 0)
                        plot_color = fireworks_background_b;
                    if (abs(draw_x) <= screen_half_width && abs(draw_y) <= screen_half_height)
                        plot_callback(0);
                    else
                        fireworks_particles[lane][point].tail_end = fireworks_age[lane];

                    tail_step = fireworks_particles[lane][point].tail_start * 2;
                    if (fireworks_age[lane] < tail_step - 1) {
                        tail_step = fireworks_gravity[lane] * 2 - tail_step * fireworks_acceleration;
                        fireworks_particles[lane][point].tail_x += fireworks_particles[lane][point].velocity_x;
                        fireworks_particles[lane][point].tail_y += fireworks_particles[lane][point].velocity_y + tail_step;
                        draw_x = fireworks_particles[lane][point].tail_x >> 6;
                        draw_y = fireworks_particles[lane][point].tail_y >> 6;
                        if (fireworks_trail_mode == 2)
                            plot_color = fireworks_background_a;
                        if (fireworks_trail_mode == 4 && (draw_x & fireworks_background_x_mask) != 0)
                            plot_color = fireworks_background_b;
                        plot_callback(0);
                        fireworks_particles[lane][point].tail_x += fireworks_particles[lane][point].velocity_x;
                        fireworks_particles[lane][point].tail_y +=
                            fireworks_particles[lane][point].velocity_y + tail_step + fireworks_acceleration;
                    } else {
                        fireworks_particles[lane][point].tail_x += fireworks_particles[lane][point].velocity_x;
                        fireworks_particles[lane][point].tail_y +=
                            fireworks_particles[lane][point].velocity_y + fireworks_gravity[lane] - fireworks_acceleration;
                    }
                }

                if ((fireworks_trail_mode == 1 &&
                     fireworks_particles[lane][point].tail_start == fireworks_age[lane]) ||
                    fireworks_particles[lane][point].tail_end == fireworks_age[lane])
                    ++fireworks_finished_particles[lane];
            }

            if ((fireworks_trail_mode & 1) != 0) {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = 3;
                fireworks_colors[lane] = plot_color;
            }
            fireworks_gravity[lane] += fireworks_acceleration;
            ++fireworks_age[lane];
            if ((fireworks_age[lane] == fireworks_launch_delay[lane] ||
                 (fireworks_finished_particles[lane] > 63 && fireworks_age[lane] < fireworks_launch_delay[lane])) && lane < 3)
                fireworks_active[lane + 1] = 1;
            if (fireworks_finished_particles[lane] > 63)
                fireworks_active[lane] = 0;
        }
        if (++lane > 3)
            lane = 0;
        if (!(palette_elapsed_ticks < palette_cycle_ticks))
            cycle_palette();
    }

    draw_x = screen_half_width + 1;
    draw_y = screen_half_height + 1;
    image_work_count += screen_extent * 8;
}

/* 0CCD:0B75, 133 bytes */
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
        if (current_graphics_key == 'n')
            return;
    }
}
/* 0CCD:0BFA, 369 bytes */
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
        if (current_graphics_key == 'n')
            return;
    }
}
/* 0CCD:0D6B, 198 bytes */
void gas_cloud(void)
{
    int vertical = 0;

    saved_pan_draw_delay = draw_delay_count;
    draw_delay_count = 1;
    if (bios_resolution_mode == 0 && fade_pan_scroll_enabled != 0 && random(5) != 0) {
        vertical = screen_half_height * 2 + 1;
        if (page_paragraph == 0)
            page_paragraph = 2000;
        else
            page_paragraph = 0;
        clear_vram_words(0, 16000, page_paragraph);
    }
    seed_plasma();
    draw_delay_count = saved_pan_draw_delay;

    if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
        return;
    if (current_graphics_key != 'n') {
        if (vertical == 0) {
            wait_fade_tick(0x78, 0);
        } else if (page_paragraph == 0) {
            animate_vertical_display_start(vertical, 0);
        } else {
            animate_vertical_display_start(0, vertical);
        }
    }
}
/* 0CCD:0E31, 330 bytes */
void randomize_transition(void)
{
    unsigned int state;
    signed char scale;
    signed char minimum;
    unsigned int polynomial;

    set_ega_raster_xor(1);
    polynomial = 0x6000;
    scale = 0x40;
    minimum = 2;
    if (video_adapter == 4)
        minimum = 4;
    do {
        plot_color = 0 & color_index_mask;
        state = 1;
        do {
            draw_y = state / ((screen_extent + 1) / scale);
            if (draw_y <= screen_half_height / scale) {
                draw_x = state % ((screen_extent + 1) / scale);
                draw_x *= scale;
                draw_y *= scale;
                if (scale > 1) {
                    plot_color++;
                    if (plot_color >= color_count)
                        plot_color = 3;
                }
                square(scale, 1);
            }
            if (state & 1)
                state = (state >> 1) ^ polynomial;
            else
                state >>= 1;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
        } while (state != 1);

        draw_y = draw_x = 0;
        if (scale > 1) {
            plot_color++;
            if (plot_color >= color_count)
                plot_color = 3;
        }
        square(scale, 1);
        scale >>= 1;
        if (scale < minimum) {
            display_cleared = 0;
            return;
        }
    } while (1);
}
/* 0CCD:0F7B, 677 bytes */
void mesh_transition(void)
{
    int limit;
    register int stride;
    register int next;
    int direction;
    int color_a;
    int color_b;

    if (video_adapter == 8)
        limit = 0x100;
    else
        limit = 0x200;
    next = limit;
    direction = random(2);
    color_a = random(3);
    color_b = random(7) + 2;
    plot_color = 3 & color_index_mask;
    draw_x = 0;
    draw_y = 0;
    plot_callback(1);
    do {
        next >>= 1;
        stride = next;
        do {
            draw_x = stride;
            draw_y = stride;
            if (draw_y <= screen_half_height) {
                if (direction != 0)
                    choose_color(color_b);
                else
                    choose_color(color_a);
                plot_callback(1);
                do {
                    draw_x -= next;
                    if (draw_x <= screen_half_width) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (draw_x > 0);
            }

            draw_x = stride;
            if (draw_x <= screen_half_width) {
                if (direction == 0)
                    choose_color(color_a);
                draw_y = stride;
                do {
                    draw_y -= next;
                    if (draw_y <= screen_half_height) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (draw_y > 0);
            }

            stride += next;
            draw_x = stride;
            draw_y = stride;
            if (draw_y <= screen_half_height) {
                if (direction == 0)
                    choose_color(color_a);
                draw_x += next;
                do {
                    draw_x -= next * 2;
                    if (draw_x <= screen_half_width) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (draw_x > next);
            }

            draw_x = stride;
            if (draw_x <= screen_half_width) {
                if (direction == 0)
                    choose_color(color_a);
                draw_y += next;
                do {
                    draw_y -= next * 2;
                    if (draw_y <= screen_half_height) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (draw_y > next);
            }

            stride += next;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
        } while ((stride <= screen_half_width || stride <= screen_half_height) && stride <= limit);

        fade_timer_ticks = 5;
        while (fade_timer_ticks > 0 && current_graphics_key != 'n') {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        if (next <= 1) {
            fade_timer_ticks = 0x3c;
            while (fade_timer_ticks > 0 && current_graphics_key != 'n') {
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
            return;
        }
    } while (1);
}

/* 0CCD:1220, 1210 bytes */
#define FF_STATE(i)       (firefly_path_state[(i)])
#define FF_LONG(i)        (*(long *)&firefly_path_state[(i)])
#define FF_PART(l,p,f)    (fireworks_particles[(l)][(p)][(f)])
void firefly_transition(int initialize)
{
    int i, particle;
    int depth;
    long x_step, y_step;
    volatile long saved;

    if (initialize != 0) {
        depth = random(3);
        FF_STATE(0) = depth + 6;
        FF_STATE(3) = 8 >> depth;
        FF_STATE(7) = 2;
        if (video_adapter == 8) {
            FF_STATE(6) = background_fill_word >> 8;
            FF_STATE(5) = background_fill_word & 0xff;
            if (FF_STATE(5) == FF_STATE(6)) {
                if (FF_STATE(7) == FF_STATE(5))
                    FF_STATE(7) = 1;
                FF_STATE(4) = 1;
                if (FF_STATE(5) == 0 && random(3) == 0)
                    FF_STATE(4) = 2;
            } else {
                FF_STATE(3) = 1;
                FF_STATE(0) = 9;
                FF_STATE(4) = 0;
                if (bios_resolution_mode == 0)
                    FF_STATE(8) = 4;
                else
                    FF_STATE(8) = 1;
            }
        } else {
            FF_STATE(5) = 0;
            FF_STATE(3) <<= 1;
            FF_STATE(4) = 1;
        }

        FF_LONG(9) = screen_half_height;
        FF_LONG(13) = -screen_half_width;
        FF_LONG(15) = screen_half_height;
        FF_STATE(1) = 0;
        while (FF_STATE(1) <= 4) {
            firefly_x_history[FF_STATE(1)] = -screen_half_width;
            firefly_y_history[FF_STATE(1)] = screen_half_height;
            ++FF_STATE(1);
        }
        FF_STATE(1) = 0;
        FF_STATE(14) = FF_STATE(13);
        FF_STATE(13) = 0;
        FF_STATE(16) = FF_STATE(15);
        FF_STATE(15) = 0;
        FF_LONG(9) = (FF_LONG(9) << 17) /
                     ((2 << (FF_STATE(0) - 1)) - 1);
        FF_LONG(11) = 0;
        FF_STATE(2) = 3;
        firefly_transition(0);

        plot_color = FF_STATE(4) < 2 ? FF_STATE(5) : FF_STATE(2);
        particle = 0;
        do {
            draw_x = firefly_x_history[FF_STATE(1)];
            draw_y = firefly_y_history[FF_STATE(1)];
            if (FF_STATE(4) == 2 && FF_STATE(1) == 0) {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = 3;
            }
            square(FF_STATE(3), 0);
            ++FF_STATE(1);
            FF_STATE(1) %= 5;
        } while (++particle <= 4);
    } else {
        --FF_STATE(0);
        if (FF_STATE(0) != 0) {
            saved = FF_LONG(9);
            FF_LONG(9) = FF_LONG(11);
            FF_LONG(11) = saved;
            firefly_transition(0);
            saved = FF_LONG(11);
            FF_LONG(11) = FF_LONG(9);
            FF_LONG(9) = saved;
        }

        for (i = 1; i <= 3; ++i) {
            switch (i) {
            case 1:
                x_step = FF_LONG(9);
                y_step = FF_LONG(11);
                break;
            case 2:
                x_step = FF_LONG(11);
                y_step = FF_LONG(9);
                break;
            case 3:
                x_step = -FF_LONG(9);
                y_step = -FF_LONG(11);
                FF_LONG(9) = y_step;
                FF_LONG(11) = x_step;
                break;
            default:
                break;
            }

            draw_x = firefly_x_history[FF_STATE(1)];
            draw_y = firefly_y_history[FF_STATE(1)];
            switch (FF_STATE(4)) {
            case 0:
                plot_color = (draw_x & FF_STATE(8)) ? FF_STATE(6) : FF_STATE(5);
                plot_callback(0);
                break;
            case 1:
                plot_color = FF_STATE(5);
                square(FF_STATE(3), 0);
                break;
            case 2:
                plot_color = FF_STATE(2);
                if (FF_STATE(1) == 0) {
                    ++plot_color;
                    if (plot_color >= color_count)
                        plot_color = 3;
                }
                FF_STATE(2) = plot_color;
                square(FF_STATE(3), 0);
                break;
            }

            FF_LONG(13) += ((long)screen_half_width * x_step) / screen_half_height;
            FF_LONG(15) -= y_step;
            draw_x = FF_LONG(13) >> 16;
            draw_y = FF_LONG(15) >> 16;
            if ((FF_LONG(13) & 0x8000L) != 0)
                ++draw_x;
            if ((FF_LONG(15) & 0x8000L) != 0)
                ++draw_y;
            firefly_x_history[FF_STATE(1)] = draw_x;
            firefly_y_history[FF_STATE(1)] = draw_y;
            ++FF_STATE(1);
            FF_STATE(1) %= 5;

            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;

            plot_color = FF_STATE(7);
            if (FF_STATE(4) == 0)
                plot_callback(0);
            else
                square(FF_STATE(3), 0);
            if (FF_STATE(0) != 0)
                firefly_transition(0);
        }

        FF_LONG(9) = -x_step;
        FF_LONG(11) = -y_step;
        ++FF_STATE(0);
    }
}
#undef FF_PART
#undef FF_LONG
#undef FF_STATE

/* 0CCD:16DA, 734 bytes */
void spiral_transition(void)
{
    int color_counter, limit_y, limit_x, phase;
    float radius, growth, angle, aspect;
    transition_point_t next_x, next_y;
    transition_point_t x_history[360], y_history[360];
    register int i;
    register int done;

    growth = 0.001127f;
    aspect = (float)screen_half_width / (float)screen_half_height * 0.75;
    limit_y = (screen_half_height * 3) >> 1;
    limit_x = (screen_half_width * 3) >> 1;
    color_counter = 0;
    plot_color = 3;
    phase = random(3);
    if (phase == 2)
        phase = 0;
    radius = 1.0f;
    angle = 0.0f;

    for (i = 0; i < 360; ++i) {
        radius *= 1 + growth;
        x_history[i].value = radius * cos(angle) * aspect * 65536.0;
        y_history[i].value = radius * sin(angle) * 65536.0;
        angle += 0.017453292519444445;
    }

    i = 0;
    do {
        next_x.value = x_history[i].value + (x_history[i].value >> 1);
        next_y.value = y_history[i].value + (y_history[i].value >> 1);
        draw_x = x_history[i].part.whole;
        draw_y = y_history[i].part.whole;
        done = 1;
        do {
            done = line_step(next_x.part.whole, next_y.part.whole, done);
            plot_callback(phase);
        } while (done == 0);

        x_history[i].value = next_x.value;
        y_history[i].value = next_y.value;
        ++i;
        i = i % 360;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == 'n')
            return;

        ++color_counter;
        if ((color_counter = color_counter & 0x0f) == 0) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = 3;
        }
    } while (next_x.part.whole <= limit_x || next_y.part.whole <= limit_y);
    fade_timer_ticks = 60;
    while (fade_timer_ticks > 0 && current_graphics_key != 'n') {
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* 0CCD:19B8, 233 bytes */
void spiral_midpoint(int first, int last, unsigned long far points[])
{
    int middle;
    int amplitude;

    amplitude = 0x800 << random(3);
    middle = (first + last) >> 1;
    points[middle] = ((points[first] + points[last]) >> 1) + amplitude
                   - random(amplitude * 2);
    if (middle - first > 1) {
        spiral_midpoint(first, middle, points);
        spiral_midpoint(middle, last, points);
    }
}
/* 0CCD:1AA1, 544 bytes */

void drip_transition(void)
{
    spiral_point_t x_path[641];
    spiral_point_t y_path[641];
    int finished = 0;
    int max_x;
    register int step;
    register int i;

    plot_color = 3;
    max_x = screen_half_width << 1;
    step = 8 << random(4);
    for (i = 0; i <= 0x280; ++i) {
        y_path[i].part.whole = -screen_half_height;
        y_path[i].part.fraction = (x_path[i].part.whole = 0);
    }
    for (i = 0; i <= max_x; i += step)
        x_path[i].part.fraction = random(0xa000) + 0x4000;
    for (i = 0; i <= max_x - step; i += step)
        spiral_midpoint(i, i + step,
                        (unsigned long far *)&x_path[0].value);

    if (finished < screen_half_width) do {
        draw_x = -screen_half_width;
        for (i = 1; i <= max_x; ++i) {
            if (y_path[i].part.whole <= screen_half_height) {
                draw_y = y_path[i].part.whole;
                y_path[i].value = y_path[i].value + x_path[i].value;
                if (draw_y < y_path[i].part.whole)
                    plot_callback(0);
                if (screen_half_height <= y_path[i].part.whole) {
                    ++y_path[i].part.whole;
                    ++finished;
                }
            }
            ++draw_x;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
        }
        if (video_adapter == 8) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = 3;
        } else {
            ++ega_color_substep;
            if ((ega_color_substep &= 0x0f) == 0) {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = 3;
            }
        }
    } while (finished < screen_half_width);
}

/* 0CCD:1CC1, 728 bytes */
void cleanup_transition(void)
{
    int origin_y, origin_x, pattern, choice, color_limit;
    int first_pattern, last_pattern, line_x, line_y;
    int start_x, start_y, bound_x, bound_y, done, pass;
    register int step_y, step_x;

    origin_y = 0;
    origin_x = 0;
    color_limit = random(8) + 2;
    choice = random(5);

    switch (choice) {
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
    pattern = first_pattern;
    pass = 1;
    while (pass <= 2) {
        for (; pattern <= last_pattern; ++pattern) {
            switch (pattern) {
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

            line_x = start_x;
            line_y = start_y;
            while (step_x * line_x <= step_x * bound_x &&
                   step_y * line_y <= step_y * bound_y) {
                draw_x = origin_x;
                draw_y = origin_y;
                plot_color = 3;
                done = 1;
                do {
                    done = line_step(line_x, line_y, done);
                    if (color_limit < 9)
                        choose_color(color_limit);
                    else {
                        ++plot_color;
                        if (plot_color >= color_count)
                            plot_color = 3;
                    }
                    plot_callback(0);
                } while (done == 0);

                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == 'n')
                    return;

                line_x += step_x;
                line_y += step_y;
            }
        }
        ++pass;
        pattern = first_pattern;
        set_ega_raster_xor(0);
    }
}

/* 0CCD:1F99, 587 bytes */

void fade_countdown(void)
{
    int outer_x, outer_y, inner_x, inner_y;
    int pattern, row, rectangle, xor_enabled;
    int j, i;

    outer_x = 40;
    outer_y = 60;
    inner_x = 24;
    inner_y = 44;
    if (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) &&
        FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700)) {
        for (i = 0; i < 8; ++i)
            for (j = 0; j < 4; ++j)
                countdown_segment_bounds[i][j] <<= 1;
        outer_x <<= 1;
        outer_y <<= 1;
        inner_x <<= 1;
        inner_y <<= 1;
    }

    set_ega_raster_xor(1);
    xor_enabled = 1;
    pattern = 0;
    while (pattern <= 5) {
        draw_y = -screen_half_height;
        while (draw_y <= screen_half_height) {
            draw_x = -screen_half_width;
            while (draw_x <= screen_half_width) {
                if (abs(draw_x) >= abs(draw_y))
                    plot_color = abs(draw_x) & color_index_mask;
                else
                    plot_color = abs(draw_y) & color_index_mask;
                if (plot_color < 3)
                    plot_color = 3;

                if (pattern < 5 && draw_x <= outer_x && draw_x >= -outer_x &&
                    draw_y <= outer_y && draw_y >= -outer_y) {
                    if (xor_enabled) {
                        set_ega_raster_xor(0);
                        xor_enabled = 0;
                    }
                    if (draw_x <= inner_x && draw_x >= -inner_x &&
                        draw_y <= inner_y && draw_y >= -inner_y) {
                        row = 0;
                        while (countdown_digit_segments[pattern][row] > 0) {
                            rectangle = countdown_digit_segments[pattern][row] - 1;
                            if (countdown_segment_bounds[rectangle][0] <= draw_x && draw_x <= countdown_segment_bounds[rectangle][2] &&
                                draw_y <= countdown_segment_bounds[rectangle][1] && countdown_segment_bounds[rectangle][3] <= draw_y) {
                                plot_color = 1;
                                break;
                            }
                            ++row;
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
            if (current_graphics_key == 'n')
                return;
            ++draw_y;
        }
        ++pattern;
        if (pattern == 5)
            set_ega_raster_xor(0);
    }
}

/* 0CCD:21E4, 745 bytes */

void fade_ziti(void)
{
    int initial_y, i, color, saved_colors, vertical_offset, line_origin;
    unsigned int pattern_color;
    int max_x, base_y, table_y, horizontal_offset, step_x, step_y;
    int first_line, second_line, first_x;
    volatile int second_x;
    volatile int phase;
    register int pattern, start_x;
    int x_step_count;

    phase = 0;
    initial_y = ((screen_half_width * 3) / 8) + random((screen_half_width * 3) / 8);
    pattern = random(8);
    saved_colors = palette_entries;
    color = initial_y + 3;
    max_x = ziti_sweep_patterns[pattern][0] * screen_half_width;
    start_x = ziti_sweep_patterns[pattern][1] * screen_half_width;
    base_y = ziti_sweep_patterns[pattern][2] * screen_half_height;
    table_y = ziti_sweep_patterns[pattern][3] * screen_half_height;
    step_x = ziti_sweep_patterns[pattern][4];
    step_y = ziti_sweep_patterns[pattern][5];
    horizontal_offset = ziti_sweep_patterns[pattern][6] * initial_y;
    pattern_color = ziti_sweep_patterns[pattern][7];
    if (pattern & 1)
        horizontal_offset >>= 1;
    vertical_offset = abs(base_y - table_y);
    line_origin = start_x - step_x * vertical_offset;
    first_x = line_origin;
    second_x = line_origin - horizontal_offset;
    first_line = 1;
    second_line = 0;

    while (second_x != max_x) {
        if (first_line) {
            draw_x = first_x;
            draw_y = base_y;
            x_step_count = vertical_offset;
            plot_color = color;
            if (line_origin < 0 && first_x < start_x) {
                draw_y = base_y + (start_x - first_x) * step_y;
                draw_x = start_x;
                x_step_count -= start_x - first_x;
            }
            if (line_origin > 0 && first_x > start_x) {
                draw_y = base_y + (first_x - start_x) * step_y;
                draw_x = start_x;
                x_step_count -= first_x - start_x;
            }
            i = 0;
            while (i <= x_step_count) {
                plot_callback(pattern_color);
                if (draw_x == max_x)
                    break;
                draw_x += step_x;
                draw_y += step_y;
                ++i;
            }
            if (video_adapter == 8) {
                --color;
                if (color < 3)
                    color = color_count - 1;
            }
        }

        if (second_line) {
            draw_x = second_x;
            draw_y = base_y;
            x_step_count = vertical_offset;
            plot_color = 0;
            if (line_origin < 0 && second_x < start_x) {
                draw_y = base_y + (start_x - second_x) * step_y;
                draw_x = start_x;
                x_step_count -= start_x - second_x;
            }
            if (line_origin > 0 && second_x > start_x) {
                draw_y = base_y + (second_x - start_x) * step_y;
                draw_x = start_x;
                x_step_count -= second_x - start_x;
            }
            i = 0;
            while (i <= x_step_count) {
                plot_callback(pattern_color);
                if (draw_x == max_x)
                    break;
                draw_x += step_x;
                draw_y += step_y;
                ++i;
            }
        }

        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == 'n')
            return;
        if ((phase ^ 1) != 0) {
            cycle_palette();
        } else {
            ++palette_cycle_offset;
            if (video_adapter == 8)
                palette_cycle_offset += 2;
        }
        first_x += step_x;
        second_x += step_x;
        if (first_x == max_x)
            first_line = 0;
        if (second_x == line_origin)
            second_line = 1;
    }
    display_cleared = 1;
    palette_entries = saved_colors;
}

/* 0CCD:24CD, 421 bytes */
void fade_umbra(void)
{
    volatile int span;
    int pair, saved_palette;
    volatile int palette_step;
    volatile int x_at;
    int y_finish;
    volatile int x_start;
    int y_start;
    volatile unsigned int pattern;
    volatile int skip_cycle;
    register int complete, x_finish;

    skip_cycle = 0;
    span = 160;
    pattern = random(4);
    saved_palette = palette_entries;
    plot_color = span + 3;
    switch (pattern) {
    case 0:
        palette_step = 0;
        x_start = -screen_half_width;
        x_finish = screen_half_width;
        y_start = -screen_half_height;
        y_finish = screen_half_height;
        break;
    case 1:
        palette_step = 0;
        x_start = -screen_half_width;
        x_finish = screen_half_width;
        y_start = screen_half_height;
        y_finish = -screen_half_height;
        break;
    case 2:
        palette_step = 1;
        x_start = 0;
        x_finish = screen_half_width;
        y_start = 0;
        y_finish = screen_half_height;
        break;
    case 3:
        palette_step = 1;
        x_start = 0;
        x_finish = screen_half_width;
        y_start = screen_half_height;
        y_finish = 0;
        break;
    default:
        break;
    }

    x_at = x_start;
    while (x_at != x_finish) {
        for (pair = 0; pair <= 1; ++pair) {
            draw_x = x_start;
            draw_y = y_finish;
            complete = 1;
            do {
                complete = line_step(x_at, y_start, complete);
                plot_callback(palette_step);
            } while (complete == 0);

            draw_x = x_finish;
            draw_y = y_start;
            do {
                complete = line_step(x_finish + x_start - x_at, y_finish, complete);
                plot_callback(palette_step);
            } while (complete == 0);

            ++x_at;
        }

        if (video_adapter == 8) {
            --plot_color;
            if (plot_color < 3)
                plot_color = color_count - 1;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == 'n')
            return;
        if ((skip_cycle ^ 1) != 0) {
            cycle_palette();
        } else {
            ++palette_cycle_offset;
            if (video_adapter == 8)
                palette_cycle_offset += 2;
        }
    }

    palette_entries = saved_palette;
    display_cleared = 0;
}

/* 0CCD:2672, 804 bytes */
void fade_moving_bars(void)
{
    int original_color, shade_color, mode8_adjust, first_active, second_active;
    int start_x, start_y, step, offset, vertical, horizontal, color;
    volatile int phase;
    register int pattern, line;
    int strip;

    phase = 0;
    pattern = random(8);
    mode8_adjust = 0;
    if ((pattern & 1) != 0 && video_adapter == 8)
        mode8_adjust = random(2);

    if (pattern <= 3) {
        strip = screen_half_height / 4 + random(screen_half_height / 4);
        start_x = moving_bars_patterns[pattern][0] * screen_half_height;
        start_y = moving_bars_patterns[pattern][1] * screen_half_height;
        offset = moving_bars_patterns[pattern][3] * strip;
        vertical = moving_bars_patterns[pattern][4] * screen_half_width;
        horizontal = moving_bars_patterns[pattern][5] * screen_half_width;
    } else {
        strip = screen_half_width / 4 + random(screen_half_width / 4);
        start_x = moving_bars_patterns[pattern][0] * screen_half_width;
        start_y = moving_bars_patterns[pattern][1] * screen_half_width;
        offset = moving_bars_patterns[pattern][3] * strip;
        vertical = moving_bars_patterns[pattern][4] * screen_half_height;
        horizontal = moving_bars_patterns[pattern][5] * screen_half_height;
    }
    step = moving_bars_patterns[pattern][2];
    color = moving_bars_patterns[pattern][6];
    if (pattern & 1)
        offset >>= 1;
    original_color = palette_entries;
    shade_color = strip + 3;
    first_active = 1;
    second_active = 0;
    line = start_y;

    while (start_x + offset != line) {
        if (start_x + step == line)
            first_active = 0;
        if (start_y + offset == line && mode8_adjust == 0)
            second_active = 1;

        if (first_active) {
            plot_color = shade_color;
            if (pattern <= 3) {
                draw_y = line;
                draw_x = horizontal;
                while (draw_x <= vertical) {
                    plot_callback(color);
                    ++draw_x;
                }
            } else {
                draw_x = line;
                draw_y = horizontal;
                while (draw_y <= vertical) {
                    plot_callback(color);
                    ++draw_y;
                }
            }
        }
        if (second_active) {
            plot_color = 0;
            if (pattern <= 3) {
                draw_y = line - offset;
                draw_x = horizontal;
                while (draw_x <= vertical) {
                    plot_callback(color);
                    ++draw_x;
                }
            } else {
                draw_x = line - offset;
                draw_y = horizontal;
                while (draw_y <= vertical) {
                    plot_callback(color);
                    ++draw_y;
                }
            }
        }

        if (video_adapter == 8) {
            --shade_color;
            if (shade_color < 3)
                shade_color = color_count - 1;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == 'n')
            return;
        if ((phase ^ 1) != 0) {
            cycle_palette();
        } else {
            ++palette_cycle_offset;
            if (video_adapter == 8)
                palette_cycle_offset += 2;
        }
        line += step;
    }
    if (second_active)
        display_cleared = 1;
    palette_entries = original_color;
}

/* 0CCD:2996, 881 bytes */
void fade_tetris(void)
{
    int y_pos, block, levels, finished, tile_width;
    register int step, block_x;

    levels = 4;
    if (video_adapter == 4)
        levels = 5;
    step = 1 << levels;
    tile_width = screen_half_width / step;
    for (block = 1; block <= levels; ++block) {
        step = 1 << block;
        block_x = y_pos = 0;
        while (block_x <= screen_half_width || y_pos <= screen_half_height) {
            draw_x = block_x;
            draw_y = y_pos;
            while (draw_y >= 0 && draw_x <= screen_half_width) {
                plot_color = (unsigned char)read_pixel_callback(draw_x, draw_y);
                square(step, 1);
                draw_y -= step;
                draw_x += step;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == 'n')
                    return;
            }
            if (y_pos <= screen_half_height)
                y_pos += step;
            else
                block_x += step;
        }
    }

    finished = 0;
    for (block = 0; block <= tile_width; ++block) {
        tetris_column_active[block] = 1;
        block_x = step * block;
        tetris_block_y[block] = random(screen_half_height / step + 1) * step;
        tetris_source_y[block] = tetris_block_y[block];
        while (!((tetris_block_colors[block] = (unsigned char)read_pixel_callback(block_x, tetris_block_y[block])),
                 tetris_block_colors[block])) {
            tetris_block_y[block] += step;
            if (tetris_block_y[block] > screen_half_height)
                tetris_block_y[block] = 0;
            if (tetris_block_y[block] == tetris_source_y[block]) {
                tetris_column_active[block] = 0;
                ++finished;
                break;
            }
        }
        tetris_source_y[block] = tetris_block_y[block];
    }

    while (finished < tile_width) {
        for (block = 0; block <= tile_width; ++block) {
            block_x = step * block;
            if (tetris_column_active[block] != 0) {
                while (tetris_block_colors[block] == 0) {
                    tetris_block_y[block] += step;
                    if (tetris_block_y[block] > screen_half_height) {
                        tetris_column_active[block] = 0;
                        ++finished;
                        break;
                    }
                    tetris_block_colors[block] = (unsigned char)read_pixel_callback(block_x, tetris_block_y[block]);
                    tetris_source_y[block] = tetris_block_y[block];
                }
                draw_y = tetris_block_y[block] - 1;
                if (draw_y >= 0) {
                    plot_color = tetris_block_colors[block];
                    draw_x = block_x;
                    while (step + block_x - 1 >= draw_x) {
                        plot_callback(1);
                        ++draw_x;
                    }
                    draw_x = block_x;
                }
                draw_y = step + tetris_block_y[block] - 1;
                if (draw_y > screen_half_height)
                    draw_y = screen_half_height;
                plot_color = 0;
                draw_x = block_x;
                while (step + block_x - 1 >= draw_x) {
                    plot_callback(1);
                    ++draw_x;
                }
                draw_x = block_x;
                if (--tetris_block_y[block] <= -step) {
                    tetris_block_y[block] = tetris_source_y[block];
                    tetris_block_colors[block] = 0;
                }
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
        }
    }
    display_cleared = 1;
}

/* 0CCD:2D07, 281 bytes */

void fade_exclusive_or(void)
{
    int result, end_x, edge, end_y;
    unsigned int layers;
    register int width, side;

    set_ega_raster_xor(1);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = 3;

    layers = random(4) + 4;
    layers &= 0xfe;
    while (layers--) {
        width = edge = screen_half_width > screen_half_height ? screen_half_width : screen_half_height;
        side = 1;
        while (width > 0) {
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();

            switch (side) {
            case 1:
                draw_x = 1;
                draw_y = edge;
                end_x = width;
                end_y = edge;
                side++;
                break;
            case 2:
                draw_x = width;
                draw_y = edge;
                end_x = width;
                end_y = 1;
                side++;
                break;
            default:
                side = 1;
                --width;
                --edge;
            }

            result = 1;
            do {
                result = line_step(end_x, end_y, result);
                plot_callback(1);
            } while (result == 0);
        }
    }
}

/* 0CCD:2E20, 626 bytes */
void fade_twist(void)
{
    float weight, remaining;
    float old_x[4], old_y[4], next_x[4], next_y[4];
    register int edge, completed;
    int next;

    effect_parameter = 0;
    if (random(3) == 2)
        effect_parameter = 1;
    old_x[0] = old_x[1] = screen_extent;
    old_x[2] = old_x[3] = -screen_extent;
    old_y[0] = old_y[3] = screen_extent;
    old_y[1] = old_y[2] = -screen_extent;
    weight = (random(6) + 2) * 0.005;
    remaining = 1.0 - weight;
    plot_color = 3 & color_index_mask;
    do {
        ++plot_color;
        if (plot_color >= color_count)
            plot_color = 3;
        draw_x = old_x[3];
        draw_y = old_y[3];
        for (edge = 0; edge < 4; ++edge) {
            completed = 1;
            do {
                completed = line_step(old_x[edge], old_y[edge], completed);
                plot_callback(effect_parameter);
            } while (!completed);
            next = (edge + 1) % 4;
            next_x[edge] = old_x[edge] * remaining + old_x[next] * weight;
            next_y[edge] = old_y[edge] * remaining + old_y[next] * weight;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        for (edge = 0; edge < 4; ++edge) {
            old_x[edge] = next_x[edge];
            old_y[edge] = next_y[edge];
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == 'n')
            return;
    } while ((abs(draw_x) + abs(draw_y)) >> 2);
    wait_fade_tick(120, 0);
}

/* 0CCD:3092, 660 bytes */

void swap_rectangle(int x1, int y1, int x2, int y2)
{
    int ax, cx, bx, ay;
    volatile int cy;
    int by, dx, dy, done;

    if (rectangles_angle_index > 64)
        return;

    ax = (int)(x1 * trig_table[rectangles_angle_index].cosine - y1 * trig_table[rectangles_angle_index].sine);
    ay = (int)(x1 * trig_table[rectangles_angle_index].sine + y1 * trig_table[rectangles_angle_index].cosine);
    bx = (int)(x2 * trig_table[rectangles_angle_index].cosine - y2 * trig_table[rectangles_angle_index].sine);
    by = (int)(x2 * trig_table[rectangles_angle_index].sine + y2 * trig_table[rectangles_angle_index].cosine);
    cx = (int)(x2 * trig_table[rectangles_angle_index].cosine - y1 * trig_table[rectangles_angle_index].sine);
    cy = (int)(x2 * trig_table[rectangles_angle_index].sine + y1 * trig_table[rectangles_angle_index].cosine);
    dx = (int)(x1 * trig_table[rectangles_angle_index].cosine - y2 * trig_table[rectangles_angle_index].sine);
    dy = (int)(x1 * trig_table[rectangles_angle_index].sine + y2 * trig_table[rectangles_angle_index].cosine);

    draw_x = ax;
    draw_y = ay;
    done = 1;
    do {
        done = line_step(cx, cy, done);
        plot_callback(1);
    } while (!done);
    draw_x = cx;
    draw_y = cy;
    do {
        done = line_step(bx, by, done);
        plot_callback(1);
    } while (!done);
    draw_x = bx;
    draw_y = by;
    do {
        done = line_step(dx, dy, done);
        plot_callback(1);
    } while (!done);
    draw_x = dx;
    draw_y = dy;
    do {
        done = line_step(ax, ay, done);
        plot_callback(1);
    } while (!done);
}

/* 0CCD:3326, 273 bytes */

void fade_rectangles(void)
{
    int limit, i, saved_color;

    rectangles_angle_index = 0;
    effect_parameter = random(3);
    limit = screen_extent / 2 + random(screen_extent / 2);
    for (i = 0; i < limit; ++i) {
        rectangles_angle_index += 4;
        rectangles_angle_index %= 64;
        ++plot_color;
        if (plot_color >= color_count)
            plot_color = 3;
        swap_rectangle(0, 0, i, i);
        if (effect_parameter == 0) {
            rectangles_angle_index -= 3;
            if (rectangles_angle_index < 0)
                rectangles_angle_index += 64;
            saved_color = plot_color;
            plot_color = 0;
            swap_rectangle(0, 0, i - 3, i - 3);
            plot_color = saved_color;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (current_graphics_key == 'n')
            return;
    }
}

/* 0CCD:3437, 958 bytes */

void fade_linewipe(void)
{
    int pos0, pos2, pos1, pos3;
    int count, dy1, dx2, dy3, result;
    unsigned int old_color;
    int mode, cycle_color;
    register int i, dx0;

    for (i = 0; i < 250; ++i) {
        ((struct LineWipePoint far *)linewipe_history)[i].x0 = 0;
        ((struct LineWipePoint far *)linewipe_history)[i].x1 = 0;
        ((struct LineWipePoint far *)linewipe_history)[i].y0 = 0;
        ((struct LineWipePoint far *)linewipe_history)[i].y1 = 0;
    }
    pos0 = pos2 = pos1 = pos3 = count = i = 0;
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = 3;
    if (video_adapter == 4)
        mode = random(125) + 82;
    else
        mode = random(125) + 124;
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
    cycle_color = !random(3);
    set_ega_raster_xor(cycle_color);
    fade_timer_ticks = random(300) + 90;

    while (fade_timer_ticks != 0 && current_graphics_key != 'n') {
        if (count <= 0) {
            dx0 = random(9) - 4;
            if (dx0 == 0)
                ++dx0;
            dy1 = random(9) - 4;
            dx2 = random(9) - 4;
            if (dx2 == 0)
                --dx2;
            dy3 = random(9) - 4;
            count = (random(8) + 1) * 4;
        }

        if (!cycle_color) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = 3;
        }
        pos0 += dx0;
        if (abs(pos0) >= screen_extent)
            dx0 = -dx0;
        pos1 += dy1;
        if (abs(pos1) >= screen_extent)
            dy1 = -dy1;
        pos2 += dx2;
        if (abs(pos2) >= screen_extent)
            dx2 = -dx2;
        pos3 += dy3;
        if (abs(pos3) >= screen_extent)
            dy3 = -dy3;
        --count;

        old_color = plot_color;
        if (!cycle_color)
            plot_color = 0;
        draw_x = ((struct LineWipePoint far *)linewipe_history)[i].x0;
        draw_y = ((struct LineWipePoint far *)linewipe_history)[i].y0;
        result = 1;
        do {
            result = line_step(((struct LineWipePoint far *)linewipe_history)[i].x1,
                               ((struct LineWipePoint far *)linewipe_history)[i].y1, result);
            plot_callback(effect_parameter);
        } while (result == 0);
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        plot_color = old_color & color_index_mask;
        ((struct LineWipePoint far *)linewipe_history)[i].x0 = pos0;
        ((struct LineWipePoint far *)linewipe_history)[i].x1 = pos1;
        ((struct LineWipePoint far *)linewipe_history)[i].y0 = pos2;
        ((struct LineWipePoint far *)linewipe_history)[i].y1 = pos3;
        ++i;
        if (i >= mode)
            i = 0;
        draw_x = pos0;
        draw_y = pos2;
        do {
            result = line_step(pos1, pos3, result);
            plot_callback(effect_parameter);
        } while (result == 0);
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* Coordinates and velocities use signed 16.16 values. */

/* 0CCD:37F5, 1625 bytes */

void fade_starburst(void)
{
    register int count;
    register unsigned int i, previous_index;
    unsigned int velocity_limit, frame_limit;
    struct StarburstState far *current;
    struct StarburstState far *previous;

    count = random(75) + 75;
    if ((current = (struct StarburstState far *)calloc(count, sizeof(*current))) == 0 ||
        (previous = (struct StarburstState far *)calloc(count, sizeof(*previous))) == 0) {
        exit_requested = 1;
        fatal_selfcheck_error(0, "Memory exhausted in star_burst routine!");
    }
    set_ega_raster_xor(0);
    effect_parameter = random(3);
    fade_timer_ticks = random(300) + 90;
    if (video_adapter == 8)
        velocity_limit = 0x3ffe;
    else
        velocity_limit = 0x7eee;
    if (effect_parameter == 0) {
        velocity_limit >>= 1;
        fade_timer_ticks >>= 1;
    }

    for (i = 0; i < count; ++i) {
        previous[i].x.value = current[i].x.value = 0;
        previous[i].y.value = current[i].y.value = 0;
        current[i].dx.value = random(velocity_limit) + 1;
        previous[i].dx.value = current[i].dx.value <<= 4;
        current[i].dy.value = random(velocity_limit) + 1;
        previous[i].dy.value = current[i].dy.value <<= 4;
        previous[i].color = current[i].color = random(palette_entries) + 1;
    }

    i = 0;
    if (effect_parameter == 0)
        frame_limit = count / 2 + random(count / 2);
    else
        frame_limit = random(4) + 3;

    while (fade_timer_ticks != 0 && current_graphics_key != 'n') {
        if (i < count) {
            plot_color = current[i].color & color_index_mask;
            draw_x = current[i].x.word.integer;
            draw_y = current[i].y.word.integer;
            current[i].x.value += current[i].dx.value;
            current[i].y.value += current[i].dy.value;
            if (draw_y > screen_half_height || draw_x > screen_half_width) {
                current[i].y.value = current[i].x.value = 0;
                current[i].dx.value = random(velocity_limit) + 1;
                current[i].dx.value <<= 4;
                current[i].dy.value = random(velocity_limit) + 1;
                current[i].dy.value <<= 4;
            }
            ++i;
        } else if (frame_limit < 1) {
            previous_index = i - count;
            plot_color = 0;
            draw_x = previous[previous_index].x.word.integer;
            draw_y = previous[previous_index].y.word.integer;
            previous[previous_index].x.value += previous[previous_index].dx.value;
            previous[previous_index].y.value += previous[previous_index].dy.value;
            if (draw_x > screen_half_width || draw_y > screen_half_height) {
                previous[previous_index].y.value = previous[previous_index].x.value = 0;
                previous[previous_index].dx.value = current[previous_index].dx.value;
                previous[previous_index].dy.value = current[previous_index].dy.value;
            }
            ++i;
        } else {
            i = 0;
            --frame_limit;
        }

        if (i >= count * 2)
            i = 0;
        plot_callback(1);
        if (input_pending != 0 && poll_graphics_keys() != 0)
            break;
    }

    free(current);
    free(previous);
}

/* 0CCD:3E4E, 344 bytes */
void fade_wipe(int horizontal)
{
    plot_color = 0;
    if (horizontal) {
        ++plot_color;
        if (plot_color >= color_count)
            plot_color = 3;
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
            if (current_graphics_key == 'n')
                return;
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if (video_adapter == 4) {
                if ((draw_x & (color_count - 1)) == 0) {
                    ++plot_color;
                    if (plot_color >= color_count)
                        plot_color = 3;
                }
            } else {
                ++plot_color;
                if (plot_color >= color_count)
                    plot_color = 3;
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
        if (current_graphics_key == 'n')
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        ++draw_y;
    }
    display_cleared = 1;
}

/* 0CCD:3FA6, 171 bytes */

void hexagon_transition(void)
{
    int loops;

    set_ega_raster_xor(!random(3));
    loops = random(4) + 1;
    while (loops >= 0) {
        hexagon(1);
        plot_callback(1);
        do {
            hexagon(0);
            plot_callback(1);
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
        } while (draw_y <= screen_extent || draw_x <= screen_extent);
        --loops;
    }
}

/* 0CCD:4051, 105 bytes */

void animate_vertical_display_start(int from_row, int to_row)
{
    int step, row;

    row = from_row;
    if (from_row < to_row)
        step = 1;
    else
        step = -1;
    while (row != to_row) {
        if (row != to_row)
            row += step;
        display_start_callback(0, row);
        if (input_pending != 0 || exit_requested != 0) {
            if (poll_graphics_keys() != 0)
                return;
        }
        if (current_graphics_key == 'n')
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* 0CCD:40BA, 328 bytes */

void animate_line_compare_split(int reverse)
{
    int line;

    if (targa64_mode != 0)
        return;
    if (video_adapter == 4) {
        if (reverse) {
            for (line = 350; line >= 0; line--) {
                ega_set_line_compare(line);
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == 'n')
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        } else {
            for (line = 0; line < 350; line++) {
                ega_set_line_compare(line);
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == 'n')
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        }
    } else {
        if (reverse) {
            for (line = 400; line >= 0; line--) {
                vga_set_line_compare(line);
                if (bios_resolution_mode != 0 && (line & 1) != 0)
                    line++;
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == 'n')
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        } else {
            for (line = 0; line < 400; line++) {
                vga_set_line_compare(line);
                if (bios_resolution_mode != 0 && (line & 1) != 0)
                    line++;
                if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                    return;
                if (current_graphics_key == 'n')
                    return;
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
        }
    }
}

/* 0CCD:4202, 649 bytes */
void fade_blister(void)
{
    int color, delay_palette, use_random_color;
    unsigned char far *pixels;
    int x_origin, y_origin;
    int step;
    int advance_color;
    int y_index, x_index;

    color = 0;
    step = 1;
    fade_timer_ticks = random(180) + 90;
    delay_palette = random(3);
    advance_color = random(5) == 0;
    use_random_color = random(5) == 0;
    if (random(3) != 0)
        effect_parameter = 1;
    else
        effect_parameter = 3;
    if (random(3) == 0) {
        step = 2;
        fade_timer_ticks >>= 1;
    }
    while (fade_timer_ticks != 0) {
        pixels = ball_bitmap_pixels;
        if (delay_palette == 0)
            color = random(color_count);
        if (advance_color) {
            ++plot_color;
            if (plot_color >= color_count)
                plot_color = 3;
        }
        y_origin = random(screen_half_height + ball_bitmap_height) - ball_bitmap_height;
        x_origin = random(screen_half_width + ball_bitmap_width) - ball_bitmap_width;
        for (x_index = 0; x_index < ball_bitmap_width; x_index += step) {
            for (y_index = 0; y_index < ball_bitmap_height; y_index += step) {
                int value = *pixels;
                pixels += step;
                if (value != 0) {
                    if (!advance_color)
                        plot_color = (value + color) & color_index_mask;
                    draw_x = x_origin + x_index;
                    draw_y = y_origin + y_index;
                    plot_callback(effect_parameter);
                }
            }
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                return;
            if (!use_random_color && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    }

    wait_fade_tick(60, use_random_color);
}

/* 0CCD:448B, 248 bytes */
void fade_dac_to_black(int mask_high_bits)
{
    volatile int draw_delay;
    unsigned char far *palette;
    int count, i;
    register int remaining;

    count = (color_count + palette_entries) * 3;
    palette = palette_buffer;
    if (targa64_mode != 0) {
        draw_delay = 0;
    } else {
        draw_delay = random(4);
        for (i = 0; i < count; ++i) {
            if (mask_high_bits)
                *palette &= 0x3f;
            else
                *palette |= 0xc0;
            ++palette;
        }
    }

    do {
        remaining = 0;
        palette = palette_buffer;
        for (i = 0; i < count; ++i) {
            if (*palette > 0)
                (*palette)--;
            remaining |= *palette;
            ++palette;
        }
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
        draw_delay ^= 1;
        switch (draw_delay) {
        case 0:
            break;
        default:
            palette_upload_callback(palette_buffer + palette_cycle_offset);
        }
    } while (remaining != 0);
    display_cleared = 1;
}

/* 0CCD:4583, 402 bytes */

void fade_palette_to_background(void)
{
    unsigned int width;
    register int half_width, i;
    unsigned int color;
    unsigned char far *first, far *second, far *end;

    color = background_fill_word & 0xff;
    if (video_adapter == 4)
        width = color_count;
    else
        width = color_count * 3;
    half_width = width >> 1;

    first = palette_buffer + palette_cycle_offset;
    second = palette_buffer + (palette_cycle_offset + (width >> 1)) % palette_buffer_bytes;
    end = palette_buffer + palette_buffer_bytes;

    if (video_adapter == 4 || random(2) != 0) {
        i = 0;
        while (i < half_width) {
            *second++ = *first++ = (unsigned char)color;
            if (video_adapter == 8) {
                *second++ = *first++ = (unsigned char)color;
                ++i;
                *second++ = *first++ = (unsigned char)color;
                ++i;
            } else {
                delay(75);
            }
            if (FP_OFF(first) >= FP_OFF(end))
                first = palette_buffer;
            if (FP_OFF(second) >= FP_OFF(end))
                second = palette_buffer;
            palette_upload_callback(palette_buffer + palette_cycle_offset);
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            ++i;
        }
    } else {
        fade_dac_to_black(fade_juggle_palette_enabled == 0 || random(35) != 0 || current_graphics_key == 'n');
    }
}

/* 0CCD:4715, 877 bytes */

int dispatch_fade_key(void)
{
    unsigned int choice;
    register int repeat = 0, keyed;

    for (;;) {
        keyed = 0;
        choice = random(35);
        if (current_graphics_key >= 'a' && current_graphics_key <= 'z') {
            choice = current_graphics_key - 'a';
            keyed = 1;
        }
        current_graphics_key = 0;

        switch (choice) {
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
            if (!keyed && random(3) != 0)
                continue;
            fade_countdown();
            repeat = 1;
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
            if (fade_gas_cloud_enabled == 0 || video_adapter == 4)
                continue;
            gas_cloud();
            repeat = !bios_resolution_mode;
            break;
        case 7:
            continue;
        case 8:
            if (fade_spiral_enabled == 0)
                continue;
            spiral_transition();
            break;
        case 9:
            if (fade_juggle_palette_enabled == 0 || video_adapter == 4)
                continue;
            if (!keyed && random(4) != 0)
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
                if (video_adapter == 8)
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
            if (fade_umbra_enabled == 0 || video_adapter == 4)
                continue;
            fade_umbra();
            repeat = 1;
            break;
        case 21:
            if (fade_moving_bars_enabled == 0 || video_adapter == 4)
                continue;
            fade_moving_bars();
            repeat = !display_cleared;
            break;
        case 22:
            if (fade_wipe_enabled == 0)
                continue;
            fade_wipe(random(2));
            repeat = !display_cleared;
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
            if (fade_ziti_enabled == 0 || video_adapter == 4)
                continue;
            fade_ziti();
            repeat = !display_cleared;
            break;
        default:
            break;
        }

        return repeat && (random(5) != 0 || keyed);
    }
}

/* 0CCD:4A82, 1224 bytes */

void display_boundary_fade_and_pan(void)
{
    struct REGPACK registers;
    register int led_on = 0, repeat = 0;

    set_ega_raster_xor(0);
    if (display_pan_state == 0 && (random(6) == 0 || current_graphics_key == 'p'))
        display_pan_state = 1;
    if ((current_graphics_key != 0 && current_graphics_key != 'p' && display_pan_state == 1) ||
        semi_auto_mode != 0 || fade_pan_scroll_enabled == 0 || (video_adapter == 8 && bios_resolution_mode != 0))
        display_pan_state = 0;

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
                if (current_graphics_key == 'p')
                    previous_graphics_key = current_graphics_key = 0;
                if (video_adapter == 4)
                    page_paragraph = 1750;
                else
                    page_paragraph = 2000;
                keyboard_led_flags(32, 0);
                return;
            }
            break;
        default:
            draw_delay_count = saved_pan_draw_delay;
            animate_vertical_display_start(0, video_adapter == 4 ? 350 : screen_half_height * 2);
            if ((random(3) == 0 || current_graphics_key == 'p') &&
                (current_graphics_key == 0 || current_graphics_key == 'p')) {
                animate_line_compare_split(1);
                animate_line_compare_split(0);
            }
    }
    keyboard_led_flags(96, 1);
    fade_timer_ticks = auto_fade_pause_ticks;
    if (video_adapter == 4)
        fade_timer_ticks /= 2;
    if (palette_alternate_mode != 0)
        fade_timer_ticks /= 2;
    if (palette_random_mode != 0)
        fade_timer_ticks /= 3;
    if (semi_auto_mode == 0) {
        if (show_mode != 0 && fade_timer_ticks > show_duration_ticks)
            fade_timer_ticks = show_duration_ticks;
        while (fade_timer_ticks != 0) {
            geninterrupt(0x28);
            if (_osmajor >= 3) {
                registers.r_ax = 0x1680;
                intr(0x2f, &registers);
            }
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
                return;
            if (current_graphics_key == 'n')
                fade_timer_ticks = 0;
            if (current_graphics_key == 'h')
                fade_timer_ticks = 7;
            if ((fade_timer_ticks & 15) < 8) {
                if (!led_on) {
                    led_on = 1;
                    keyboard_led_flags(16, 0);
                    fprintf(stdout, "\r");
                }
            } else if (led_on) {
                led_on = 0;
                keyboard_led_flags(16, 1);
            }
        }
    }
    keyboard_led_flags(16, 1);
    if (current_graphics_key != 'n')
        repeat = dispatch_fade_key();
    if (repeat && current_graphics_key != 'n') {
        display_cleared = 0;
    } else {
        repeat = 0;
        if ((input_pending != 0 || exit_requested != 0) && poll_graphics_keys() != 0)
            return;
        if (display_cleared == 0)
            fade_palette_to_background();
        setmem(palette_buffer, palette_buffer_bytes, 0);
        ega_border_color = background_fill_word = 0;
        palette_upload_callback(palette_buffer);
        fprintf(stdout, "\r");
        ega_border_color = random(color_count);
        if (video_adapter == 4) {
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
    if (exit_requested == 0 && repeat == 0) {
        if (video_adapter == 8)
            randomize_palette_state(0);
        mode_setup_callback(1);
        palette_build_callback(0);
        palette_upload_callback(palette_buffer + palette_cycle_offset);
        if (video_adapter == 4)
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
