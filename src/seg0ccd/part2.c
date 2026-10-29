/* Fade effects, pan/scroll helpers, and the fade dispatcher from segment 0CCD. */
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
extern int abs(int);

extern unsigned int colors, g_5dba;
extern int g_5dc4, g_5d94, g_5d96, g_5dec, g_5cb1;
extern int g_5D7E, g_5D80;
extern unsigned int g_5D92, g_5D7A, g_5DA6;
extern void (*g_5c37)(unsigned, unsigned);
extern int x, y;
extern int line_step(int, int, int);
extern void (*plot)(int);
extern int g_5db0, g_5db2, g_6025, g_7484;
extern int g_5d78;
extern int g_5df0, g_5dee;
extern unsigned int g_7488;
extern unsigned int g_5c3f, g_5c41;
extern unsigned int g_5db6, g_5d7a, g_5da6, g_5d7e, g_5d80, g_5d92;
extern unsigned int g_5dc0;
extern int g_5dae;
extern unsigned int far *g_5c57;
extern int g_0b4c[8][7];
extern unsigned int (*g_5c3b)(int, int);
extern unsigned int g_738e[64];
extern int g_73b8[64];
extern unsigned int g_73e2[64];
extern int g_740c[64];
extern void far fill_square(int, unsigned int);
extern unsigned int g_5c4f, g_5c51, g_5c53, g_5c55;
extern void wait_fade_tick(int, int);
extern int poll_palette_keys(void);
extern void cycle_palette(void);

struct LineWipePoint {
    unsigned int x0, y0, x1, y1;
};

/* objcmp: {"function":"fade_countdown","address":"0ccd:1f99","size":587,"symbols":{"_plot":"DS:5c3f","_g_5c3f":"DS:5c3f","_g_5c41":"DS:5c41","_ega_plot_backend":"1899:012d","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_x":"DS:5df0","_y":"DS:5dee","_g_7488":"DS:7488","_g_5dc0":"DS:5dc0","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_0a50":"DS:0a50","_g_0a8c":"DS:0a8c","_set_ega_raster_xor":"11c1:0e2a","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515"}} */
extern unsigned int g_5c3f, g_5c41;
extern int g_5db0, g_5db2, g_5dee, g_5df0;
extern int x, y;
extern unsigned int g_7488, g_5dc0, g_5da6, g_5d7a, g_5d7e, g_5d80, g_5d92;
extern int g_0a50[6][6];
extern int g_0a8c[8][4];
extern void far ega_plot_backend(void);
extern void set_ega_raster_xor(int);
extern void (*plot)(int);

void fade_countdown(void)
{
    int outer_x, outer_y, inner_x, inner_y;
    int pattern, row, rectangle, xor_enabled;
    int j, i;

    outer_x = 40;
    outer_y = 60;
    inner_x = 24;
    inner_y = 44;
    if (g_5c41 == FP_SEG(ega_plot_backend) &&
        g_5c3f == FP_OFF(ega_plot_backend)) {
        for (i = 0; i < 8; ++i)
            for (j = 0; j < 4; ++j)
                g_0a8c[i][j] <<= 1;
        outer_x <<= 1;
        outer_y <<= 1;
        inner_x <<= 1;
        inner_y <<= 1;
    }

    set_ega_raster_xor(1);
    xor_enabled = 1;
    pattern = 0;
    while (pattern <= 5) {
        y = -g_5db2;
        while (y <= g_5db2) {
            x = -g_5db0;
            while (x <= g_5db0) {
                if (abs(x) >= abs(y))
                    g_7488 = abs(x) & g_5dc0;
                else
                    g_7488 = abs(y) & g_5dc0;
                if (g_7488 < 3)
                    g_7488 = 3;

                if (pattern < 5 && x <= outer_x && x >= -outer_x &&
                    y <= outer_y && y >= -outer_y) {
                    if (xor_enabled) {
                        set_ega_raster_xor(0);
                        xor_enabled = 0;
                    }
                    if (x <= inner_x && x >= -inner_x &&
                        y <= inner_y && y >= -inner_y) {
                        row = 0;
                        while (g_0a50[pattern][row] > 0) {
                            rectangle = g_0a50[pattern][row] - 1;
                            if (g_0a8c[rectangle][0] <= x && x <= g_0a8c[rectangle][2] &&
                                y <= g_0a8c[rectangle][1] && g_0a8c[rectangle][3] <= y) {
                                g_7488 = 1;
                                break;
                            }
                            ++row;
                        }
                    }
                } else if (!xor_enabled) {
                    set_ega_raster_xor(1);
                    xor_enabled = 1;
                }

                plot(0);
                ++x;
            }
            if (g_5c41 == FP_SEG(ega_plot_backend) &&
                g_5c3f == FP_OFF(ega_plot_backend))
                ++y;
            if (g_5d7a >= g_5da6)
                cycle_palette();
            if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5d92 == 'n')
                return;
            ++y;
        }
        ++pattern;
        if (pattern == 5)
            set_ega_raster_xor(0);
    }
}

/* objcmp: {"function":"fade_ziti","address":"0ccd:21e4","size":745,"symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5dba":"DS:5dba","_g_0acc":"DS:0acc","_x":"DS:5df0","_y":"DS:5dee","_g_7488":"DS:7488","_g_5cb1":"DS:5cb1","_g_7484":"DS:7484","_g_5db6":"DS:5db6","_g_5dec":"DS:5dec","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_plot":"DS:5c3f","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
extern int g_7484;
extern unsigned int g_5db6;
extern int g_0acc[8][8];

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
    initial_y = ((g_5db0 * 3) / 8) + random((g_5db0 * 3) / 8);
    pattern = random(8);
    saved_colors = g_5dba;
    color = initial_y + 3;
    max_x = g_0acc[pattern][0] * g_5db0;
    start_x = g_0acc[pattern][1] * g_5db0;
    base_y = g_0acc[pattern][2] * g_5db2;
    table_y = g_0acc[pattern][3] * g_5db2;
    step_x = g_0acc[pattern][4];
    step_y = g_0acc[pattern][5];
    horizontal_offset = g_0acc[pattern][6] * initial_y;
    pattern_color = g_0acc[pattern][7];
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
            x = first_x;
            y = base_y;
            x_step_count = vertical_offset;
            g_7488 = color;
            if (line_origin < 0 && first_x < start_x) {
                y = base_y + (start_x - first_x) * step_y;
                x = start_x;
                x_step_count -= start_x - first_x;
            }
            if (line_origin > 0 && first_x > start_x) {
                y = base_y + (first_x - start_x) * step_y;
                x = start_x;
                x_step_count -= first_x - start_x;
            }
            i = 0;
            while (i <= x_step_count) {
                plot(pattern_color);
                if (x == max_x)
                    break;
                x += step_x;
                y += step_y;
                ++i;
            }
            if (g_7484 == 8) {
                --color;
                if (color < 3)
                    color = g_5db6 - 1;
            }
        }

        if (second_line) {
            x = second_x;
            y = base_y;
            x_step_count = vertical_offset;
            g_7488 = 0;
            if (line_origin < 0 && second_x < start_x) {
                y = base_y + (start_x - second_x) * step_y;
                x = start_x;
                x_step_count -= start_x - second_x;
            }
            if (line_origin > 0 && second_x > start_x) {
                y = base_y + (second_x - start_x) * step_y;
                x = start_x;
                x_step_count -= second_x - start_x;
            }
            i = 0;
            while (i <= x_step_count) {
                plot(pattern_color);
                if (x == max_x)
                    break;
                x += step_x;
                y += step_y;
                ++i;
            }
        }

        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d92 == 'n')
            return;
        if ((phase ^ 1) != 0) {
            cycle_palette();
        } else {
            ++g_5dec;
            if (g_7484 == 8)
                g_5dec += 2;
        }
        first_x += step_x;
        second_x += step_x;
        if (first_x == max_x)
            first_line = 0;
        if (second_x == line_origin)
            second_line = 1;
    }
    g_5cb1 = 1;
    g_5dba = saved_colors;
}

/* objcmp: {"function":"fade_umbra","address":"0ccd:24cd","size":413,"level":"PROVISIONAL","blocker":"switch layout and paired line endpoints at 0CCD:2514","symbols":{"_g_5dba":"DS:5dba","_g_7488":"DS:7488","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5dec":"DS:5dec","_g_7484":"DS:7484","_g_5db6":"DS:5db6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5cb1":"DS:5cb1","_x":"DS:5df0","_y":"DS:5dee","_line_step":"066a:000e","_plot":"DS:5c3f","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void fade_umbra(void)
{
    volatile int span;
    int pair, saved_palette, y_finish;
    volatile int x_at;
    volatile int palette_step;
    volatile int x_start;
    int y_start;
    volatile unsigned int pattern;
    register int complete;
    volatile int skip_cycle;

    skip_cycle = 0;
    span = 160;
    pattern = random(4);
    palette_step = 0;
    saved_palette = g_5dba;
    g_7488 = span + 3;
    switch (pattern) {
    case 0:
        palette_step = 0;
        x_start = -g_5db0;
        y_start = -g_5db2;
        y_finish = g_5db2;
        break;
    case 1:
        palette_step = 0;
        x_start = -g_5db0;
        y_start = g_5db2;
        y_finish = -g_5db2;
        break;
    case 2:
        palette_step = 1;
        x_start = 0;
        y_start = 0;
        y_finish = g_5db2;
        break;
    case 3:
        palette_step = 1;
        x_start = 0;
        y_start = g_5db2;
        y_finish = 0;
        break;
    default:
        break;
    }

    x_at = x_start;
    while (x_at != g_5db0) {
        for (pair = 0; pair <= 1; ++pair) {
            x = x_start;
            y = y_finish;
            complete = 1;
            do {
                complete = line_step(y_start, x_at, complete);
                plot(palette_step);
            } while (complete == 0);

            x = g_5db0;
            y = y_start;
            complete = 1;
            do {
                complete = line_step(y_finish, g_5db0 + x_start - x_at, complete);
                plot(palette_step);
            } while (complete == 0);

            ++x_at;
        }

        if (g_7484 == 8) {
            --g_7488;
            if (g_7488 < 3)
                g_7488 = colors - 1;
        }
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d92 == 'n')
            return;
        if ((skip_cycle ^ 1) != 0) {
            cycle_palette();
        } else {
            ++g_5dec;
            if (g_7484 == 8)
                g_5dec += 2;
        }
    }

    g_5dba = saved_palette;
    g_5cb1 = 0;
}

/* objcmp: {"function":"fade_moving_bars","address":"0ccd:2672","size":804,"level":"PROVISIONAL","blocker":"pattern table and bar-state layout at 0CCD:26A0","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5dba":"DS:5dba","_g_5db6":"DS:5db6","_g_7484":"DS:7484","_g_7488":"DS:7488","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5cb1":"DS:5cb1","_g_5c3f":"DS:5c3f","_g_0b4c":"DS:0b4c","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void fade_moving_bars(void)
{
    int pattern, mode8_adjust, strip, start_x, start_y;
    int offset, end_x, end_y, step, color, original_color;
    int vertical, horizontal, line, first_active, second_active;

    mode8_adjust = 0;
    pattern = random(8);
    if ((pattern & 1) != 0 && g_7484 == 8)
        mode8_adjust = random(2);

    if (pattern <= 3) {
        strip = g_5db2 / 4 + random(g_5db2 / 4);
        start_x = g_0b4c[pattern][0] * g_5db2;
        start_y = g_0b4c[pattern][1] * g_5db2;
        offset = g_0b4c[pattern][3] * strip;
        vertical = g_0b4c[pattern][4] * g_5db0;
        horizontal = g_0b4c[pattern][5] * g_5db0;
    } else {
        strip = g_5db0 / 4 + random(g_5db0 / 4);
        start_x = g_0b4c[pattern][0] * g_5db0;
        start_y = g_0b4c[pattern][1] * g_5db0;
        offset = g_0b4c[pattern][3] * strip;
        vertical = g_0b4c[pattern][4] * g_5db2;
        horizontal = g_0b4c[pattern][5] * g_5db2;
    }
    step = g_0b4c[pattern][2];
    color = g_0b4c[pattern][6];
    if (pattern & 1)
        offset >>= 1;
    original_color = g_5dba;
    strip += 3;
    first_active = 1;
    second_active = 0;
    line = start_y;

    for (;;) {
        if (start_x + offset == line) {
            if (second_active)
                g_5cb1 = 1;
            g_5dba = original_color;
            return;
        }
        if (start_x + step == line)
            first_active = 0;
        if (start_y + offset == line && mode8_adjust == 0)
            second_active = 1;

        if (first_active) {
            g_7488 = strip;
            if (pattern <= 3) {
                y = line;
                x = horizontal;
                while (x <= vertical) {
                    plot(color);
                    ++x;
                }
            } else {
                x = line;
                y = horizontal;
                while (y <= vertical) {
                    plot(color);
                    ++y;
                }
            }
        }
        if (second_active) {
            g_7488 = 0;
            if (pattern <= 3) {
                y = line - offset;
                x = horizontal;
                while (x <= vertical) {
                    plot(color);
                    ++x;
                }
            } else {
                x = line - offset;
                y = horizontal;
                while (y <= vertical) {
                    plot(color);
                    ++y;
                }
            }
        }

        if (g_7484 == 8) {
            --strip;
            if (strip < 3)
                strip = g_5db6 - 1;
        }
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d92 == 'n')
            return;
        cycle_palette();
        line += step;
    }
}

/* objcmp: {"function":"fade_tetris","address":"0ccd:2996","size":881,"level":"PROVISIONAL","blocker":"falling-block state arrays at 0CCD:2A79","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5db6":"DS:5db6","_g_7484":"DS:7484","_g_7488":"DS:7488","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5cb1":"DS:5cb1","_g_5c3b":"DS:5c3b","_g_5c3f":"DS:5c3f","_g_738e":"DS:738e","_g_73b8":"DS:73b8","_g_73e2":"DS:73e2","_g_740c":"DS:740c","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_fill_square":"066a:026b","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void fade_tetris(void)
{
    int levels, step, tile_width, x_pos, y_pos, level, block, block_x;
    int initial_y, finished, value;
    unsigned char pixel;

    levels = 4;
    if (g_7484 == 4)
        levels = 5;
    step = 1 << levels;
    tile_width = g_5db0 / step;

    for (level = 1; level <= levels; ++level) {
        step = 1 << level;
        y_pos = 0;
        x_pos = 0;
        while (x_pos <= g_5db0 || y_pos <= g_5db2) {
            x = x_pos;
            y = y_pos;
            while (y >= 0 && x <= g_5db0) {
                pixel = (unsigned char)g_5c3b(x, y);
                g_7488 = pixel;
                fill_square(step, 1);
                y -= step;
                x += step;
                if (g_5d7a >= g_5da6)
                    cycle_palette();
                if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5d92 == 'n')
                    return;
            }
            if (g_5db2 < y_pos)
                x_pos += step;
            else
                y_pos += step;
        }
    }

    finished = 0;
    block = 0;
    step = 1 << levels;
    do {
        if (tile_width < block) {
            for (;;) {
                if (tile_width <= finished) {
                    g_5cb1 = 1;
                    return;
                }
                for (block = 0; block <= tile_width; ++block) {
                    block_x = step * block;
                    if (g_738e[block] != 0) {
                        while (g_73e2[block] == 0) {
                            g_73b8[block] += step;
                            if (g_5db2 < g_73b8[block]) {
                                g_738e[block] = 0;
                                ++finished;
                                break;
                            }
                            pixel = (unsigned char)g_5c3b(block_x, g_73b8[block]);
                            g_73e2[block] = pixel;
                            g_740c[block] = g_73b8[block];
                        }

                        g_5dee = g_73b8[block] - 1;
                        if (g_5dee >= 0) {
                            g_7488 = g_73e2[block];
                            g_5df0 = block_x;
                            while (g_5df0 <= block_x + step - 1) {
                                plot(1);
                                ++g_5df0;
                            }
                            g_5df0 = block_x;
                        }
                        value = g_73b8[block] + step - 1;
                        if (value > g_5db2)
                            value = g_5db2;
                        g_7488 = 0;
                        g_5dee = value;
                        g_5df0 = block_x;
                        while (g_5df0 <= block_x + step - 1) {
                            plot(1);
                            ++g_5df0;
                        }
                        g_5df0 = block_x;
                        --g_73b8[block];
                        if (g_73b8[block] <= -step) {
                            g_73b8[block] = g_740c[block];
                            g_73e2[block] = 0;
                        }
                    }
                    if (g_5d7a >= g_5da6)
                        cycle_palette();
                    if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
                        return;
                    if (g_5d92 == 'n')
                        return;
                }
            }
        }

        g_738e[block] = 1;
        y_pos = random(g_5db2 / step) * step;
        initial_y = y_pos;
        do {
            pixel = (unsigned char)g_5c3b(step * block, y_pos);
            g_73e2[block] = pixel;
            if (pixel != 0)
                break;
            y_pos += step;
            if (g_5db2 < y_pos)
                y_pos = 0;
        } while (y_pos != initial_y);
        if (pixel == 0) {
            g_738e[block] = 0;
            ++finished;
        }
        g_73b8[block] = y_pos;
        g_740c[block] = y_pos;
        ++block;
    } while (1);
}

/* objcmp: {"function":"fade_exclusive_or","address":"0ccd:2d07","size":281,"level":"PROVISIONAL","blocker":"relocation-coverage: 0CCD:2D8D emits lcall 11C1:18E7; oracle has conditional branch","symbols":{"_g_7488":"DS:7488","_g_5DB6":"DS:5db6","_g_5DB0":"DS:5db0","_g_5DB2":"DS:5db2","_g_5DEE":"DS:5dee","_g_5DF0":"DS:5df0","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_g_5D7A":"DS:5d7a","_g_5DA6":"DS:5da6","_set_ega_raster_xor":"11c1:0e2a","_line_step":"066a:000e","_plot":"DS:5c3f","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
extern unsigned int g_7488, g_5DB6;
extern int g_5DB0, g_5DB2, g_5DEE, g_5DF0;
extern int line_step(int, int, int);
extern void set_ega_raster_xor(int);
extern void (*plot)(int);

void fade_exclusive_or(void)
{
    unsigned int layers;
    int width, height, edge, side, end_x, end_y, result;

    set_ega_raster_xor(1);
    ++g_7488;
    if (g_7488 >= g_5DB6)
        g_7488 = 3;

    layers = random(4) + 4;
    layers &= 0xfe;
    do {
        unsigned int next_layers = layers - 1;

        if (layers == 0)
            return;
        width = g_5DB0;
        if (width <= g_5DB2)
            width = g_5DB2;
        edge = width;
        side = 1;
        layers = next_layers;
        while (width > 0) {
            if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5D92 == 'n')
                return;
            if (g_5D7A >= g_5DA6)
                cycle_palette();

            if (side == 1) {
                g_5DF0 = 1;
                g_5DEE = edge;
                end_x = width;
                end_y = edge;
                side++;
            } else if (side == 2) {
                g_5DF0 = width;
                g_5DEE = edge;
                end_x = edge;
                end_y = 1;
                side++;
            } else {
                side = 1;
                --width;
                --edge;
            }

            result = 1;
            do {
                result = line_step(end_x, end_y, result);
                plot(1);
            } while (result == 0);
        }
    } while (1);
}

/* objcmp: {"function":"fade_twist","address":"0ccd:2e20","size":626,"level":"PROVISIONAL","blocker":"fixed point rotation setup and four point rings at 0CCD:2E61 (INT 3Bh emulator sequence)","symbols":{"_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_7488":"DS:7488","_g_5dc0":"DS:5dc0","_colors":"DS:5db6","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_x":"DS:5df0","_y":"DS:5dee","_line_step":"066a:000e","_plot":"DS:5c3f","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_wait_fade_tick":"0ccd:000d","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void fade_twist(void)
{
    int edge, direction, color, n, radius, completed;
    int old_x[4], old_y[4], next_x[4], next_y[4];
    int stride;

    g_6025 = random(3) == 2;
    radius = g_5dae;
    stride = random(6) + 2;
    g_7488 = (random(6) + 2) & g_5dc0;
    ++g_7488;
    if (g_7488 >= colors)
        g_7488 = 3;

    old_x[0] = -radius;
    old_y[0] = -radius;
    old_x[1] = radius;
    old_y[1] = -radius;
    old_x[2] = radius;
    old_y[2] = radius;
    old_x[3] = -radius;
    old_y[3] = radius;
    for (n = 0; n < 4; ++n) {
        next_x[n] = old_x[n];
        next_y[n] = old_y[n];
    }

    direction = 1;
    do {
        for (edge = 0; edge < 4; ++edge) {
            int next = (edge + 1) & 3;

            x = old_x[edge];
            y = old_y[edge];
            completed = 1;
            do {
                completed = line_step(old_x[next], old_y[next], completed);
                plot(1);
            } while (completed == 0);

            x = old_x[next];
            y = old_y[next];
            completed = 1;
            do {
                completed = line_step(next_x[edge], next_y[edge], completed);
                plot(1);
            } while (completed == 0);
        }

        if (g_5d7a >= g_5da6)
            cycle_palette();
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d92 == 'n')
            return;

        for (n = 0; n < 4; ++n) {
            old_x[n] = next_x[n];
            old_y[n] = next_y[n];
            next_x[n] += direction * stride;
            next_y[n] += direction * stride;
        }
        if (g_5df0 == 0 && g_5dee == 0)
            break;
        color = (abs(g_5df0) + abs(g_5dee)) >> 2;
        if (color == 0)
            break;
        if (--radius <= 0)
            direction = -direction;
    } while (radius > 0);

    wait_fade_tick(0, 120);
}

struct RotationPair {
    float cosine;
    float sine;
};
extern struct RotationPair g_5df2[65];

/* objcmp: {"function":"swap_rectangle","address":"0ccd:3092","size":660,"symbols":{"_g_5dac":"DS:5dac","_g_5df2":"DS:5df2","_x":"DS:5df0","_y":"DS:5dee","_plot":"DS:5c3f","_line_step":"066a:000e","F_FTOL@":"0000:0fbe","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
extern int g_5dac;

void swap_rectangle(int x1, int y1, int x2, int y2)
{
    int ax, cx, bx, ay;
    volatile int cy;
    int by, dx, dy, done;

    if (g_5dac > 64)
        return;

    ax = (int)(x1 * g_5df2[g_5dac].cosine - y1 * g_5df2[g_5dac].sine);
    ay = (int)(x1 * g_5df2[g_5dac].sine + y1 * g_5df2[g_5dac].cosine);
    bx = (int)(x2 * g_5df2[g_5dac].cosine - y2 * g_5df2[g_5dac].sine);
    by = (int)(x2 * g_5df2[g_5dac].sine + y2 * g_5df2[g_5dac].cosine);
    cx = (int)(x2 * g_5df2[g_5dac].cosine - y1 * g_5df2[g_5dac].sine);
    cy = (int)(x2 * g_5df2[g_5dac].sine + y1 * g_5df2[g_5dac].cosine);
    dx = (int)(x1 * g_5df2[g_5dac].cosine - y2 * g_5df2[g_5dac].sine);
    dy = (int)(x1 * g_5df2[g_5dac].sine + y2 * g_5df2[g_5dac].cosine);

    x = ax;
    y = ay;
    done = 1;
    do {
        done = line_step(cx, cy, done);
        plot(1);
    } while (!done);
    x = cx;
    y = cy;
    do {
        done = line_step(bx, by, done);
        plot(1);
    } while (!done);
    x = bx;
    y = by;
    do {
        done = line_step(dx, dy, done);
        plot(1);
    } while (!done);
    x = dx;
    y = dy;
    do {
        done = line_step(ax, ay, done);
        plot(1);
    } while (!done);
}

/* objcmp: {"function":"fade_rectangles","address":"0ccd:3326","size":273,"symbols":{"_g_5dac":"DS:5dac","_g_5dae":"DS:5dae","_g_7488":"DS:7488","_colors":"DS:5db6","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_poll_palette_keys":"11c1:18e7","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_swap_rectangle":"0ccd:3092"}} */
extern int g_5dae, g_6025;

void fade_rectangles(void)
{
    int limit, i, saved_color;

    g_5dac = 0;
    g_6025 = random(3);
    limit = g_5dae / 2 + random(g_5dae / 2);
    for (i = 0; i < limit; ++i) {
        g_5dac += 4;
        g_5dac %= 64;
        ++g_7488;
        if (g_7488 >= colors)
            g_7488 = 3;
        swap_rectangle(0, 0, i, i);
        if (g_6025 == 0) {
            g_5dac -= 3;
            if (g_5dac < 0)
                g_5dac += 64;
            saved_color = g_7488;
            g_7488 = 0;
            swap_rectangle(0, 0, i - 3, i - 3);
            g_7488 = saved_color;
        }
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d92 == 'n')
            return;
    }
}

/* objcmp: {"function":"fade_linewipe","address":"0ccd:3437","size":958,"level":"PROVISIONAL","blocker":"far point ring and Bresenham walk at 0CCD:34D1","symbols":{"_g_5c57":"DS:5c57","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5dae":"DS:5dae","_g_5d78":"DS:5d78","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5db6":"DS:5db6","_g_5dc0":"DS:5dc0","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5c3f":"DS:5c3f","_g_7488":"DS:7488","_g_6025":"DS:6025","_g_7484":"DS:7484","_set_ega_raster_xor":"11c1:0e2a","_line_step":"066a:000e","_plot":"DS:5c3f","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
extern void set_ega_raster_xor(int);

void fade_linewipe(void)
{
    register int i;
    int count, mode, cycle_color, result;
    int dx0, dy1, dx2, dy3;
    unsigned int pos0, pos1, pos2, pos3, old_color;

    for (i = 0; i < 250; ++i) {
        ((struct LineWipePoint far *)g_5c57)[i].x0 = 0;
        ((struct LineWipePoint far *)g_5c57)[i].x1 = 0;
        ((struct LineWipePoint far *)g_5c57)[i].y0 = 0;
        ((struct LineWipePoint far *)g_5c57)[i].y1 = 0;
    }
    i = 0;
    count = i;
    pos0 = i;
    pos1 = i;
    pos2 = i;
    pos3 = i;
    old_color = g_7488 + 1;
    if (old_color >= g_5db6)
        old_color = 3;
    g_7488 = old_color;
    if (g_7484 == 4)
        mode = random(125) + 82;
    else
        mode = random(125) + 124;
    cycle_color = random(3);
    if (cycle_color == 0 || cycle_color == 1)
        g_6025 = 1;
    else
        g_6025 = 3;
    cycle_color = random(3) == 0;
    set_ega_raster_xor(cycle_color);
    g_5d78 = random(300) + 90;

    while (g_5d78 != 0 && g_5d92 != 'n') {
        if (count < 1) {
            dx0 = random(9) - 4;
            if (dx0 == 0)
                dx0 = 1;
            dy1 = random(9) - 4;
            dx2 = random(9) - 4;
            if (dx2 == 0)
                dx2 = -1;
            dy3 = random(9) - 4;
            count = (random(4) + 1) * 4;
        }

        if (!cycle_color) {
            ++g_7488;
            if (g_7488 >= g_5db6)
                g_7488 = 3;
        }
        pos0 += dx0;
        if (abs(pos0) >= g_5dae)
            dx0 = -dx0;
        pos1 += dy1;
        if (abs(pos1) >= g_5dae)
            dy1 = -dy1;
        pos2 += dx2;
        if (abs(pos2) >= g_5dae)
            dx2 = -dx2;
        pos3 += dy3;
        if (abs(pos3) >= g_5dae)
            dy3 = -dy3;
        --count;

        old_color = g_7488;
        if (!cycle_color)
            g_7488 = 0;
        x = ((struct LineWipePoint far *)g_5c57)[i].x0;
        y = ((struct LineWipePoint far *)g_5c57)[i].y0;
        result = 1;
        do {
            result = line_step(((struct LineWipePoint far *)g_5c57)[i].x1,
                               ((struct LineWipePoint far *)g_5c57)[i].y1, result);
            plot(g_6025);
        } while (result == 0);
        if (g_5d7a >= g_5da6)
            cycle_palette();
        g_7488 = old_color & g_5dc0;
        ((struct LineWipePoint far *)g_5c57)[i].x0 = pos0;
        ((struct LineWipePoint far *)g_5c57)[i].x1 = pos1;
        ((struct LineWipePoint far *)g_5c57)[i].y0 = pos2;
        ((struct LineWipePoint far *)g_5c57)[i].y1 = pos3;
        i = (i + 1) % mode;
        x = pos0;
        y = pos1;
        do {
            result = line_step(pos2, pos3, result);
            plot(g_6025);
        } while (result == 0);
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d7a >= g_5da6)
            cycle_palette();
    }
}

/* Coordinates and velocities use signed 16.16 values. */
union StarburstFixed {
    long value;
    struct {
        unsigned int fraction;
        int integer;
    } word;
};

struct StarburstState {
    union StarburstFixed x;
    union StarburstFixed y;
    union StarburstFixed dx;
    union StarburstFixed dy;
    unsigned int color;
};

/* objcmp: {"function":"fade_starburst","address":"0ccd:37f5","size":1625,"level":"PROVISIONAL","blocker":"source layout and long fixed-point stores at 0CCD:3870","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5d80":"DS:5d80","_g_6025":"DS:6025","_g_5d78":"DS:5d78","_g_5d92":"DS:5d92","_g_5dc0":"DS:5dc0","_g_7488":"DS:7488","_g_5db6":"DS:5db6","_g_5d7e":"DS:5d7e","_g_5da6":"DS:5da6","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_x":"DS:5df0","_y":"DS:5dee","_plot":"DS:5c3f","_set_ega_raster_xor":"11c1:0e2a","_poll_palette_keys":"11c1:18e7","_calloc":"0000:259d","_free":"0000:2b47","_fatal_selfcheck_error":"11c1:052f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
extern void set_ega_raster_xor(int enabled);

extern void far fatal_selfcheck_error(int, const char far *);

void fade_starburst(void)
{
    register unsigned int count, i, previous_index;
    int velocity_limit, frame_limit;
    struct StarburstState far *current;
    struct StarburstState far *previous;

    count = (unsigned int)(random(75) + 75);
    current = (struct StarburstState far *)calloc(count, sizeof(*current));
    if (current != 0) {
        previous = (struct StarburstState far *)calloc(count, sizeof(*previous));
        if (previous != 0)
            goto allocated;
    }
    {
        g_5d80 = 1;
        fatal_selfcheck_error(0, (char far *)MK_FP(_DS, 0x0bd4));
    }
allocated:

    set_ega_raster_xor(0);
    g_6025 = random(3);
    g_5d78 = random(300) + 90;
    if (g_6025 == 0)
        g_5d78 >>= 1;

    if (g_7484 == 8)
        velocity_limit = 0x3ffe;
    else
        velocity_limit = 0x7eee;
    if (g_6025 == 0)
        velocity_limit >>= 1;

    for (i = 0; i < count; ++i) {
        current[i].x.value = 0;
        current[i].y.value = 0;
        previous[i].x.value = 0;
        previous[i].y.value = 0;
        current[i].dx.value = 0;
        previous[i].dx.value = 0;
        current[i].dy.value = 0;
        previous[i].dy.value = 0;
        current[i].dx.value = random(velocity_limit) + 1;
        previous[i].dx.value = current[i].dx.value;
        current[i].dy.value = random(velocity_limit) + 1;
        previous[i].dy.value = current[i].dy.value;
        current[i].color = random(g_5dba) + 1;
        previous[i].color = current[i].color;
    }

    i = 0;
    if (g_6025 == 0)
        frame_limit = (count >> 1) + random(count >> 1);
    else
        frame_limit = random(2) + 3;

    for (;;) {
        if (i < count) {
            g_7488 = current[i].color & g_5dc0;
            x = current[i].x.word.integer;
            y = current[i].y.word.integer;
            current[i].x.value += current[i].dx.value;
            current[i].y.value += current[i].dy.value;
            if (current[i].y.word.integer > g_5db2 ||
                current[i].x.word.integer > g_5db0) {
                current[i].x.value = 0;
                current[i].y.value = 0;
                current[i].dx.value = random(velocity_limit) + 1;
                previous[i].dx.value = current[i].dx.value;
                current[i].dy.value = random(velocity_limit) + 1;
                previous[i].dy.value = current[i].dy.value;
            }
            ++i;
        } else if (frame_limit >= 1) {
            i = 0;
            --frame_limit;
        } else {
            previous_index = i - count;
            g_7488 = 0;
            x = previous[previous_index].x.word.integer;
            y = previous[previous_index].y.word.integer;
            previous[previous_index].x.value += previous[previous_index].dx.value;
            previous[previous_index].y.value += previous[previous_index].dy.value;
            if (previous[previous_index].x.word.integer > g_5db0 ||
                previous[previous_index].y.word.integer > g_5db2) {
                previous[previous_index].x.value = 0;
                previous[previous_index].y.value = 0;
                previous[previous_index].dx.value = current[previous_index].dx.value;
                previous[previous_index].dy.value = current[previous_index].dy.value;
            }
            ++i;
        }

        if (i >= count * 2)
            i = 0;
        plot(1);
        if (g_5d7e != 0 && poll_palette_keys() != 0)
            break;
        if (g_5d78 == 0 || g_5d92 == 'n')
            break;
    }

    free(current);
    free(previous);
}

/* objcmp: {"function":"fade_wipe","address":"0ccd:3e4e","size":344,"symbols":{"_g_7488":"DS:7488","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5db6":"DS:5db6","_g_5cb1":"DS:5cb1","_g_7484":"DS:7484","_g_5c41":"DS:5c41","_g_5c3f":"DS:5c3f","_plot":"DS:5c3f","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_ega_plot_backend":"1899:012d","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515"}} */
void fade_wipe(int horizontal)
{
    g_7488 = 0;
    if (horizontal) {
        ++g_7488;
        if (g_7488 >= g_5db6)
            g_7488 = 3;
        g_5df0 = 0;
        while (g_5df0 <= g_5db0) {
            g_5dee = 0;
            while (g_5dee <= g_5db2) {
                plot(1);
                if (g_5c41 == FP_SEG(ega_plot_backend) &&
                    g_5c3f == FP_OFF(ega_plot_backend))
                    ++g_5dee;
                ++g_5dee;
            }
            if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5d92 == 'n')
                return;
            if (g_5d7a >= g_5da6)
                cycle_palette();
            if (g_7484 == 4) {
                if ((g_5df0 & (g_5db6 - 1)) == 0) {
                    ++g_7488;
                    if (g_7488 >= g_5db6)
                        g_7488 = 3;
                }
            } else {
                ++g_7488;
                if (g_7488 >= g_5db6)
                    g_7488 = 3;
            }
            ++g_5df0;
        }
        g_5cb1 = 0;
        return;
    }

    g_5dee = 0;
    while (g_5dee <= g_5db2) {
        g_5df0 = 0;
        while (g_5df0 <= g_5db0) {
            plot(1);
            ++g_5df0;
        }
        if (g_5c41 == FP_SEG(ega_plot_backend) &&
            g_5c3f == FP_OFF(ega_plot_backend))
            ++g_5dee;
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d92 == 'n')
            return;
        if (g_5d7a >= g_5da6)
            cycle_palette();
        ++g_5dee;
    }
    g_5cb1 = 1;
}

/* objcmp: {"function":"hexagon_transition","address":"0ccd:3fa6","size":171,"symbols":{"_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_set_ega_raster_xor":"11c1:0e2a","_hexagon":"066a:2f53","_plot":"DS:5c3f","_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_poll_palette_keys":"11c1:18e7"}} */
extern int x, y, g_5dae;
extern void set_ega_raster_xor(int);
extern void hexagon(int);
extern void (*plot)(int);

void hexagon_transition(void)
{
    int loops;

    set_ega_raster_xor(!random(3));
    loops = random(4) + 1;
    while (loops >= 0) {
        hexagon(1);
        plot(1);
        do {
            hexagon(0);
            plot(1);
            if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5D92 == 'n')
                return;
        } while (y <= g_5dae || x <= g_5dae);
        --loops;
    }
}

/* objcmp: {"function":"animate_vertical_display_start","address":"0ccd:4051","size":105,"symbols":{"_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_g_5D7A":"DS:5d7a","_g_5DA6":"DS:5da6","_set_display_start":"DS:5c33","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515"}} */
extern void (*set_display_start)(int, int);

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
        set_display_start(0, row);
        if (g_5D7E != 0 || g_5D80 != 0) {
            if (poll_palette_keys() != 0)
                return;
        }
        if (g_5D92 == 'n')
            return;
        if (g_5D7A >= g_5DA6)
            cycle_palette();
    }
}

/* objcmp: {"function":"animate_line_compare_split","address":"0ccd:40ba","size":328,"symbols":{"_g_5DC4":"DS:5dc4","_g_7484":"DS:7484","_g_5DE4":"DS:5de4","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_g_5D7A":"DS:5d7a","_g_5DA6":"DS:5da6","_ega_set_line_compare":"1899:0e40","_vga_set_line_compare":"1899:0e73","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515"}} */
extern int g_5DC4, g_7484, g_5DE4;
extern void ega_set_line_compare(unsigned int);
extern void vga_set_line_compare(unsigned int);

void animate_line_compare_split(int reverse)
{
    int line;

    if (g_5DC4 != 0)
        return;
    if (g_7484 == 4) {
        if (reverse) {
            for (line = 350; line >= 0; line--) {
                ega_set_line_compare(line);
                if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5D92 == 'n')
                    return;
                if (g_5D7A >= g_5DA6)
                    cycle_palette();
            }
        } else {
            for (line = 0; line < 350; line++) {
                ega_set_line_compare(line);
                if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5D92 == 'n')
                    return;
                if (g_5D7A >= g_5DA6)
                    cycle_palette();
            }
        }
    } else {
        if (reverse) {
            for (line = 400; line >= 0; line--) {
                vga_set_line_compare(line);
                if (g_5DE4 != 0 && (line & 1) != 0)
                    line++;
                if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5D92 == 'n')
                    return;
                if (g_5D7A >= g_5DA6)
                    cycle_palette();
            }
        } else {
            for (line = 0; line < 400; line++) {
                vga_set_line_compare(line);
                if (g_5DE4 != 0 && (line & 1) != 0)
                    line++;
                if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5D92 == 'n')
                    return;
                if (g_5D7A >= g_5DA6)
                    cycle_palette();
            }
        }
    }
}

/* objcmp: {"function":"fade_blister","address":"0ccd:4202","size":649,"level":"PROVISIONAL","blocker":"pixel stepping and local flag placement at 0CCD:43D5","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5db6":"DS:5db6","_g_5d78":"DS:5d78","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_5c4f":"DS:5c4f","_g_5c51":"DS:5c51","_g_5c53":"DS:5c53","_g_5c55":"DS:5c55","_g_5dc0":"DS:5dc0","_g_5c3f":"DS:5c3f","_plot":"DS:5c3f","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_wait_fade_tick":"0ccd:000d"}} */
void fade_blister(void)
{
    int color, delay_palette, use_random_color, advance_color;
    int x_origin, y_origin;
    int step;
    int x_index, y_index;
    unsigned char far *pixels;

    color = 0;
    step = 1;
    g_5d78 = random(180) + 90;
    delay_palette = random(3);
    advance_color = random(5) == 0;
    use_random_color = random(5) == 0;
    g_6025 = random(3) == 0 ? 1 : 3;
    if (random(3) != 0) {
        step = 2;
        g_5d78 >>= 1;
    }
    pixels = (unsigned char far *)MK_FP(g_5c55, g_5c53);

    while (g_5d78 != 0) {
        if (delay_palette == 0)
            color = random(colors);
        if (advance_color) {
            ++g_7488;
            if (g_7488 >= colors)
                g_7488 = 3;
        }
        y_origin = random(g_5db2 + g_5c4f) - g_5c4f;
        x_origin = random(g_5db0 + g_5c51) - g_5c51;
        for (x_index = 0; x_index < g_5c51; x_index += step) {
            for (y_index = 0; y_index < g_5c4f; y_index += step) {
                unsigned char value = *pixels;
                pixels += step;
                if (value != 0) {
                    if (!advance_color)
                        g_7488 = (value + color) & g_5dc0;
                    x = x_origin + x_index;
                    y = y_origin + y_index;
                    plot(g_6025);
                }
            }
            if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
                goto done;
            if (g_5d92 == 'n')
                goto done;
            if (!use_random_color && g_5d7a >= g_5da6)
                cycle_palette();
        }
    }

done:
    if (g_5d78 == 0)
        wait_fade_tick(use_random_color, 60);
}

/* objcmp: {"function":"fade_dac_to_black","address":"0ccd:448b","size":248,"level":"PROVISIONAL","blocker":"control-flow: object 247 vs 248; first mismatch 0CCD:455A conditional-test register and one-byte extent shift","symbols":{"_colors":"DS:5db6","_g_5dba":"DS:5dba","_g_5dc4":"DS:5dc4","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5dec":"DS:5dec","_g_5de8":"DS:5de8","_g_5cb1":"DS:5cb1","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_interval":"DS:5da6","_g_5c37":"DS:5c37","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void fade_dac_to_black(int mask_high_bits)
{
    volatile int draw_delay;
    unsigned char far *palette;
    int count, i;
    register int remaining;
    register int delay_state;

    count = (colors + g_5dba) * 3;
    palette = (unsigned char far *)MK_FP(g_5d96, g_5d94);
    if (g_5dc4 != 0) {
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
        palette = (unsigned char far *)MK_FP(g_5d96, g_5d94);
        for (i = 0; i < count; ++i) {
            if (*palette > 0)
                (*palette)--;
            remaining |= *palette;
            ++palette;
        }
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5d7a >= g_5da6)
            cycle_palette();
        draw_delay ^= 1;
        delay_state = draw_delay;
        if (delay_state != 0) {
            g_5c37(g_5d94 + g_5dec, g_5d96);
        }
    } while (remaining != 0);
    g_5cb1 = 1;
}

/* objcmp: {"function":"fade_palette_to_background","address":"0ccd:4583","size":402,"symbols":{"_g_5d8a":"DS:5d8a","_colors":"DS:5db6","_g_5db6":"DS:5db6","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5dec":"DS:5dec","_g_5de8":"DS:5de8","_g_7484":"DS:7484","_g_5c37":"DS:5c37","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_poll_palette_keys":"11c1:18e7","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_delay":"0000:29a6","_g_04b1":"DS:04b1","_fade_dac_to_black":"0ccd:448b"}} */
extern unsigned int g_5d8a, g_5de8, g_04b1;
extern void delay(unsigned int);

void fade_palette_to_background(void)
{
    register unsigned int width, half_width, i;
    unsigned int color;
    unsigned int end, middle;
    unsigned char far *first, far *second;

    color = g_5d8a & 0xff;
    if (g_7484 == 4)
        width = colors;
    else
        width = colors * 3;
    half_width = width >> 1;

    first = (unsigned char far *)MK_FP(g_5d96, g_5d94 + g_5dec);
    middle = (g_5dec + half_width) % g_5de8;
    second = (unsigned char far *)MK_FP(g_5d96, g_5d94 + middle);
    end = g_5d94 + g_5de8;

    if (g_7484 != 4 && random(2) == 0)
        goto black_fade;

    i = 0;
    while (i < half_width) {
        *first++ = (unsigned char)color;
        *second++ = (unsigned char)color;
        if (g_7484 == 8) {
            *first++ = (unsigned char)color;
            *second++ = (unsigned char)color;
            ++i;
            *first++ = (unsigned char)color;
            *second++ = (unsigned char)color;
            ++i;
        } else {
            delay(75);
        }
        if (FP_OFF(first) >= end)
            first = (unsigned char far *)MK_FP(g_5d96, g_5d94);
        if (FP_OFF(second) >= end)
            second = (unsigned char far *)MK_FP(g_5d96, g_5d94);
        g_5c37(g_5d94 + g_5dec, g_5d96);
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            return;
        ++i;
    }

    return;

black_fade:
    if (g_04b1 != 0)
        i = random(35) != 0 || g_5d92 == 'n';
    else
        i = 1;
    fade_dac_to_black(i);
}

/* objcmp: {"function":"dispatch_fade_key","address":"0ccd:4715","size":877,"level":"PROVISIONAL","blocker":"switch dispatch and mixed panel cases at 0CCD:4767 (jump table offsets differ)","symbols":{"_g_5d92":"DS:5d92","_g_0479":"DS:0479","_g_0480":"DS:0480","_g_0487":"DS:0487","_g_048e":"DS:048e","_g_0495":"DS:0495","_g_049c":"DS:049c","_g_04a3":"DS:04a3","_g_04aa":"DS:04aa","_g_04b1":"DS:04b1","_g_04b8":"DS:04b8","_g_04bf":"DS:04bf","_g_04c6":"DS:04c6","_g_04cd":"DS:04cd","_g_04d4":"DS:04d4","_g_04db":"DS:04db","_g_04e2":"DS:04e2","_g_04e9":"DS:04e9","_g_04f0":"DS:04f0","_g_04f7":"DS:04f7","_g_04fe":"DS:04fe","_g_0505":"DS:0505","_g_050c":"DS:050c","_g_0513":"DS:0513","_g_051a":"DS:051a","_g_7484":"DS:7484","_g_5de4":"DS:5de4","_g_5cb1":"DS:5cb1","_g_5d78":"DS:5d78","_g_5d8a":"DS:5d8a","_g_5d8b":"DS:5d8b","_g_5d90":"DS:5d90","_g_5c37":"DS:5c37","_g_5c43":"DS:5c43","_g_5c47":"DS:5c47","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5dec":"DS:5dec","_g_7490":"DS:7490","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_effect_0e31":"0ccd:0e31","_effect_1aa1":"0ccd:1aa1","_effect_1220":"0ccd:1220","_effect_0d6b":"0ccd:0d6b","_effect_16da":"0ccd:16da","_effect_0bfa":"0ccd:0bfa","_effect_0f7b":"0ccd:0f7b","_effect_1cc1":"0ccd:1cc1","_panel_setup_0b75":"0ccd:0b75","_palette_update_mode8":"11c1:11b9"}} */
extern int g_0479, g_0480, g_0487, g_048e, g_0495, g_049c, g_04a3;
extern int g_04aa, g_04b8, g_04bf, g_04c6, g_04cd, g_04d4;
extern int g_04db, g_04e2, g_04e9, g_04f0, g_04f7, g_04fe, g_0505;
extern int g_050c, g_0513, g_051a, g_5de4, g_5d90, g_5d8b, g_7490;
extern void (*g_5c43)(int);
extern void (*g_5c47)(int);
extern void palette_update_mode8(int mode);
extern void effect_0e31(void);
extern void effect_1aa1(void);
extern void effect_1220(int mode);
extern void effect_0d6b(void);
extern void effect_16da(void);
extern void effect_0bfa(void);
extern void effect_0f7b(void);
extern void effect_1cc1(void);
extern void panel_setup_0b75(void);

int dispatch_fade_key(void)
{
    int choice, keyed, repeat;

    for (;;) {
        choice = random(35);
        keyed = 0;
        if (g_5d92 >= 'a' && g_5d92 <= 'z') {
            choice = g_5d92 - 'a';
            keyed = 1;
        }
        g_5d92 = 0;
        repeat = 0;

        switch (choice) {
    case 0:
        if (g_0479 == 0)
            continue;
            effect_0e31();
        break;
    case 1:
        if (g_0480 == 0)
            continue;
        fade_blister();
        break;
    case 2:
        if (g_0487 == 0)
            continue;
        if (!keyed && random(3) != 0)
            continue;
        fade_countdown();
        repeat = 1;
        break;
    case 3:
        if (g_048e == 0)
            continue;
        effect_1aa1();
        break;
    case 4:
        if (g_0495 == 0)
            continue;
        fade_exclusive_or();
        break;
    case 5:
        if (g_049c == 0)
            continue;
        effect_1220(1);
        break;
    case 6:
        if (g_04a3 == 0 || g_7484 == 4)
            continue;
        effect_0d6b();
        repeat = g_5cb1 == 0;
        break;
    case 7:
        break;
    case 8:
        if (g_04aa == 0)
            continue;
        effect_16da();
        break;
    case 9:
        if (g_04b1 == 0 || g_7484 == 4)
            continue;
        if (!keyed && random(2) != 0)
            continue;
        fade_dac_to_black(0);
        break;
    case 10:
        if (g_04b8 == 0)
            continue;
        effect_1cc1();
        break;
    case 11:
        if (g_04bf == 0)
            continue;
        fade_linewipe();
        break;
    case 12:
        if (g_04c6 == 0)
            continue;
        effect_0f7b();
        break;
    case 13:
        if (g_04cd == 0)
            continue;
        {
            fade_palette_to_background();
            if (g_7484 == 8)
                palette_update_mode8(0);
            g_5d8a = (unsigned char)random(2);
            g_5d8b = (unsigned char)g_5d8a;
            g_7490 = 0;
            g_5c43(1);
            g_5c47(0);
            g_5c37(g_5d94 + g_5dec, g_5d96);
            panel_setup_0b75();
            g_5cb1 = 0;
        }
        break;
    case 14:
        if (g_04d4 == 0)
            continue;
        effect_0f7b();
        break;
    case 15:
        break;
    case 16:
        if (g_04db == 0)
            continue;
        effect_0bfa();
        break;
    case 17:
        if (g_04e2 == 0)
            continue;
        fade_rectangles();
        break;
    case 18:
        if (g_04e9 == 0)
            continue;
        if (!keyed && random(2) != 0)
            continue;
        fade_starburst();
        break;
    case 19:
        if (g_04f0 == 0)
            continue;
        fade_twist();
        break;
    case 20:
        if (g_04f7 == 0 || g_7484 == 4)
            continue;
        fade_umbra();
        break;
    case 21:
        if (g_04fe == 0 || g_7484 == 4)
            continue;
        fade_moving_bars();
        repeat = g_5cb1 == 0;
        break;
    case 22:
        if (g_0505 == 0)
            continue;
        fade_wipe(random(2));
        repeat = g_5cb1 == 0;
        break;
    case 23:
        if (g_050c == 0)
            continue;
        hexagon_transition();
        if (keyed || random(2) == 0)
            fade_starburst();
        repeat = g_5cb1 == 0;
        break;
    case 24:
        if (g_0513 == 0)
            continue;
        fade_tetris();
        break;
    case 25:
        if (g_051a == 0 || g_7484 == 4)
            continue;
        fade_ziti();
        repeat = g_5cb1 == 0;
        break;
    default:
        return 0;
    }

        if (!repeat)
            return 0;
        if (random(5) != 0 || keyed)
            return 1;
        return 0;
    }
}

/* objcmp: {"function":"display_boundary_fade_and_pan","address":"0ccd:4a82","size":1224,"level":"PROVISIONAL","blocker":"panel polling and menu drawing flow at 0CCD:4C53 (INT 28h / interrupt-register sequence)","symbols":{"_g_5de6":"DS:5de6","_g_5d92":"DS:5d92","_g_5da0":"DS:5da0","_g_04d4":"DS:04d4","_g_7484":"DS:7484","_g_5de4":"DS:5de4","_g_5dc6":"DS:5dc6","_g_7436":"DS:7436","_g_7490":"DS:7490","_g_5db2":"DS:5db2","_g_5d78":"DS:5d78","_g_5de0":"DS:5de0","_g_5dda":"DS:5dda","_g_5dce":"DS:5dce","_g_5d86":"DS:5d86","_g_5da8":"DS:5da8","_g_5cb1":"DS:5cb1","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5db6":"DS:5db6","_g_5d8a":"DS:5d8a","_g_5d8b":"DS:5d8b","_g_5d8c":"DS:5d8c","_g_5d90":"DS:5d90","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5dec":"DS:5dec","_g_5de8":"DS:5de8","_g_5c37":"DS:5c37","_g_5c43":"DS:5c43","_g_5c47":"DS:5c47","_set_ega_raster_xor":"11c1:0e2a","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_dispatch_fade_key":"0ccd:4715","_fade_palette_to_background":"0ccd:4583","_animate_vertical_display_start":"0ccd:4051","_animate_line_compare_split":"0ccd:40ba","_panel_setup_0b75":"0ccd:0b75","_palette_update_mode8":"11c1:11b9","_panel_signal":"11c1:000a","_intr":"0000:32d8","_fprintf":"0000:45d4","_fsetmem":"0000:4d12","_delay":"0000:29a6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
extern int g_5de6, g_5da0, g_5dc6, g_7436, g_5de0, g_5dda;
extern int g_5dce, g_5d86, g_5da8, g_5d8c, g_5dde;
extern void (*panel_signal)(int, int);
extern void far fsetmem(void far *destination, int value, unsigned int count);
extern void panel_setup_0b75(void);
extern void palette_update_mode8(int mode);
extern int dispatch_fade_key(void);
extern void animate_vertical_display_start(int from, int to);
extern void animate_line_compare_split(int reverse);

void display_boundary_fade_and_pan(void)
{
    struct REGPACK registers;
    FILE far *panel_file;
    char far *panel_format;
    int row, key_result, panel_open, repeat;
    int palette_color, panel_count;

    panel_file = (FILE far *)MK_FP(_DS, 0x0bfc);
    panel_format = (char far *)MK_FP(_DS, 0x4408);
    set_ega_raster_xor(0);

    if (g_5de6 == 0 && (random(6) == 0 || g_5d92 == 'p'))
        g_5de6 = 1;
    if (g_5d92 != 0 && g_5d92 != 'p' && g_5de6 != 1) {
        if (g_5da0 == 0 && g_04d4 != 0 &&
            (g_7484 != 8 || g_5de4 == 0))
            g_5de6 = 0;
    }

    panel_open = g_5de6;
    if (panel_open == 1) {
        if (g_7490 != 0) {
            g_5de6 = 0;
        } else {
            g_7436 = g_5dc6;
            g_5dc6 = 1;
            ++g_5de6;
            if (g_5d92 == 'p') {
                g_5d92 = 0;
                g_5d90 = 0;
            }
            g_7490 = g_7484 == 4 ? 1734 : 2000;
            panel_signal(0, 32);
            goto finish;
        }
    } else if (panel_open > 1) {
        g_5dc6 = g_7436;
        row = g_7484 == 4 ? 350 : g_5db2 * 2;
        animate_vertical_display_start(row, 0);
        if (random(3) == 0 || g_5d92 == 0 || g_5d92 == 'p') {
            animate_line_compare_split(1);
            animate_line_compare_split(0);
        }
        delay(96);
    }

    g_5d78 = g_5da8;
    if (g_7484 == 4)
        g_5d78 /= 2;
    if (g_5de0 != 0)
        g_5d78 /= 2;
    if (g_5dda != 0)
        g_5d78 /= 3;
    if (g_5da0 != 0)
        g_5d78 = 0;
    if (g_5dce == 0)
        g_5d78 = 0;
    else if (g_5d78 > g_5d86)
        g_5d78 = g_5d86;

    repeat = 0;
    while (g_5d78 != 0 && g_5d92 != 'n') {
        intr(0x28, &registers);
        if ((unsigned char)*(unsigned char far *)MK_FP(_DS, 0x007d) >= 3) {
            registers.r_ax = 0x1680;
            intr(0x2f, &registers);
        }
        if (g_5d7a >= g_5da6)
            cycle_palette();
        if ((g_5d7e != 0 || g_5d80 != 0) && poll_palette_keys() != 0)
            break;
        if (g_5d92 == 'n') {
            g_5d78 = 0;
            break;
        }
        if (g_5d92 == 'h')
            g_5d78 = 7;
        if ((g_5d78 & 15) < 8) {
            if (g_5de6 == 0) {
                panel_signal(0, 16);
                fprintf(panel_file, panel_format);
                panel_open = 1;
            }
        } else if (panel_open != 0) {
            panel_signal(1, 16);
            fprintf(panel_file, panel_format);
            panel_open = 0;
        }

        if (g_5d78 == 0)
            break;
        key_result = dispatch_fade_key();
        if (key_result != 0 && g_5d92 != 'n') {
            g_5cb1 = 0;
            repeat = 1;
            break;
        }
        --g_5d78;
    }

    if (!repeat) {
        g_5d90 = 0;
        if (g_5d92 != 'n') {
            key_result = poll_palette_keys();
            if (key_result != 0)
                g_5d92 = key_result;
        }
        if (g_5cb1 == 0)
            fade_palette_to_background();
        fsetmem(MK_FP(g_5d96, g_5d94), 0, g_5de8);
        g_5d8a = 0;
        g_5d8b = 0;
        g_5c37(g_5d96, g_5d94);
        fprintf(panel_file, panel_format);

        panel_count = colors;
        g_5d8c = panel_count;
        if (g_7484 == 4) {
            palette_color = random(3);
            g_5d8a = palette_color == 0 ? (~g_5d8c & 8) : random(2);
        } else {
            palette_color = random(3);
            if (palette_color != 0)
                g_5d8a = 0;
            else
                g_5d8a = random(2);
            g_5d8b = random(2) + 1;
        }
        if (g_5dde != 0)
            g_5d8c = 0;
        if (g_5dce != 0 && g_5d86 == 0)
            g_5d80 = 1;
        if (g_5d80 == 0 && repeat == 0) {
            if (g_7484 == 8)
                palette_update_mode8(0);
            g_5c43(1);
            g_5c47(0);
            g_5c37(g_5d94 + g_5dec, g_5d96);
            if (g_7484 == 4)
                delay(350);
            g_7490 = 0;
            g_5cb1 = 1;
        }
    }

finish:
    g_5d7a = 0;
    g_5de6 = 0;
    g_5d92 = 0;
    g_5d90 = 0;
}
