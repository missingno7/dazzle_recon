/* Fade and transition routines from code segment 0CCD. */
#include <dos.h>
#include <stdlib.h>
#include <math.h>

extern int g_5DA8;
extern unsigned int g_5D78;
extern unsigned int g_5D7A;
extern int g_5D7E;
extern int g_5D80;
extern int g_5DB0, g_5DB2;
extern int g_5DEE, g_5DF0;
extern unsigned int g_5DA6;
extern unsigned int g_5DB6;
extern unsigned int g_5D92;
extern unsigned char g_007D;
extern int g_7348, g_734A, g_734C, g_734E, g_7350, g_7352, g_7354, g_7356;
extern int g_04D4, g_5DC6, g_5DE4, g_7436;
extern unsigned int g_7490;
extern int g_5DAE, g_5CB1, g_7484;
extern int g_5CBB;
extern unsigned int g_5D8A;
extern int g_5DAA;
extern unsigned int g_7488;
extern unsigned int g_5DC0;
extern int g_7358[5], g_7362[5];
extern int g_736C[17];
extern int g_60F8[4];
extern int g_6108[4];
extern int g_6142;
typedef struct {
    int head_x, head_y;
    int tail_x, tail_y;
    int velocity_x, velocity_y;
    int tail_start, tail_end;
    int unknown_10; /* Not accessed by this routine. */
} transition_particle_t;
extern transition_particle_t g_6144[4][64];
extern int g_6100[4], g_6110[4], g_6118[4];
extern int g_6120[4], g_6128[4], g_612A[4], g_6130[4];
extern int g_6138, g_613A, g_613C, g_613E, g_6140;

int poll_palette_keys(void);
void cycle_palette(void);
int line_step(int, int, int);
void rtl_32d8(unsigned int, void *);
void swap_quilt_cells(unsigned int, unsigned int, unsigned int, unsigned int,
                      unsigned int, unsigned int, unsigned int);
void clear_vram_words(unsigned int, unsigned int, unsigned int);
void restore_vram_page(void);
void animate_vertical_display_start(unsigned int, unsigned int);
void set_xor_mode(int);
void fill_square(int, unsigned int);
void randomize_helper(int);
void choose_color(int);
extern const float transition_aspect_scale;
extern const float transition_fixed_scale;
extern const double transition_angle_step;
/* Existing literal-pool cells at DS:0BBC/0BC0/0BC4. */
extern float g_7344;
/* Same storage as initialize_math_tables' g_trig_table[128]. */
typedef struct { float cosine, sine; } transition_direction_t;
extern transition_direction_t g_5DF2[64];
typedef union {
    long value;
    struct { unsigned int fraction; int whole; } part;
} transition_point_t;
typedef void (far *plot_callback_t)(unsigned int);
extern plot_callback_t plot_callback;

/* 0ccd:0000..000a holds five words (13d8, 13e2, 13f4, 13fe, 1408). They are not a switch table for wait_fade_tick at 000d; 13e2 lands mid-instruction. Ownership remains unassigned, so the words are treated as a data island. */

/* objcmp: {"function":"near_cs","address":"0ccd:000a","size":3,"level":"PROVISIONAL","blocker":"static-no-pubdef"} */
static unsigned int near_cs(void)
{
    return _CS;
}

/* objcmp: {"function":"wait_fade_tick","address":"0ccd:000d","size":128,"symbols":{"_g_5DA8":"DS:5da8","_g_5D78":"DS:5d78","_g_5D7A":"DS:5d7a","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5DA6":"DS:5da6","_g_5D92":"DS:5d92","_g_007D":"DS:007d","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rtl_32d8":"0000:32d8"}} */
void wait_fade_tick(int requested, int skip_palette)
{
    unsigned int dos_args[10];

    if (requested > g_5DA8 / 2)
        requested = g_5DA8 / 2;
    g_5D78 = requested;

    while (g_5D78 != 0) {
        if (g_5D7E != 0 || g_5D80 != 0) {
            if (poll_palette_keys() != 0)
                break;
        }
        if (g_5D92 == 'n')
            break;
        if (skip_palette == 0) {
            if (g_5D7A >= g_5DA6)
                cycle_palette();
        }
        geninterrupt(0x28);
        if (g_007D >= 3) {
            dos_args[0] = 0x1680;
            rtl_32d8(0x2f, dos_args);
        }
    }
}
/* objcmp: {"function":"randomize_helper","address":"0ccd:008d","size":2792,"absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_7488":"DS:7488","_g_7484":"DS:7484","_g_5D8A":"DS:5d8a","_g_5DE4":"DS:5de4","_g_5c3f":"DS:5c3f","_g_60f8":"DS:60f8","_g_6100":"DS:6100","_g_6108":"DS:6108","_g_6110":"DS:6110","_g_6118":"DS:6118","_g_6120":"DS:6120","_g_6128":"DS:6128","_g_6130":"DS:6130","_g_6138":"DS:6138","_g_613A":"DS:613a","_g_613C":"DS:613c","_g_613c":"DS:613c","_g_613e":"DS:613e","_g_6140":"DS:6140","_g_6142":"DS:6142","_g_6144":"DS:6144","_g_5db6":"DS:5db6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_set_xor_mode":"11c1:0e2a","_fill_square":"066a:026b","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7","_line_step":"066a:000e","_plot_callback":"DS:5c3f","_g_5dae":"DS:5dae","_g_5cbb":"DS:5cbb","_g_7344":"DS:7344","_g_5DF2":"DS:5df2","_g_613E":"DS:613e"}} */
void randomize_helper(int steps)
{
    register int mode = steps;
    int lane, point, tail_step, min_x, min_y, speed, direction;

    if (mode != 0) {
        g_7344 = (float)g_5DB0 / (float)g_5DB2 * transition_aspect_scale;
        g_6142 = 1;
        if (mode == 1 && random(4) == 0)
            g_6142 = 0;
        if (g_6142 != 0)
            g_6138 = 2;
        else
            g_6138 = 0;
        if (random(3) != 0)
            g_613E = 0;
        else
            g_613E = 1;
        for (lane = 0; lane <= 3; ++lane) {
            for (point = 0; point <= 63; ++point) {
                speed = random(65) + 64;
                direction = random(64);
                g_6144[lane][point].velocity_x = speed * g_5DF2[direction].sine * g_7344;
                g_6144[lane][point].velocity_y = -(speed * g_5DF2[direction].cosine);
            }
            if (g_5D7A >= g_5DA6)
                cycle_palette();
        }
        if (mode > 1) {
            if (random(3) == 0)
                g_613E = 3;
            else {
                g_613E = 2;
                g_613A = 0;
                if (g_7484 == 8) {
                    g_613C = g_5D8A >> 8;
                    g_613A = g_5D8A & 0xff;
                    if (g_613A != g_613C)
                        g_613E = 4;
                    if (g_5DE4 == 0)
                        g_6140 = 4;
                    else
                        g_6140 = 1;
                }
            }
        }
        return;
    }

    {
        min_x = random(g_5DB0) - g_5DB0 / 2;
        min_y = -random(g_5DB2 / 2);

        for (lane = 0; lane <= 3; ++lane) {
            if (g_6142 != 0) {
                g_60F8[lane] = min_x + random(g_5DB0) - g_5DB0 / 2;
                g_6100[lane] = min_y + random(g_5DB2) - g_5DB2 / 2;
            } else
                g_60F8[lane] = g_6100[lane] = 0;
            g_6118[lane] = random(g_5DB6 - 3) + 3;
            g_6108[lane] = g_6110[lane] = 0;
            g_6130[lane] = 0;
            g_6128[lane] = 0;
            g_6120[lane] = g_5DB2 / 8 + random(g_5DB2 / 2);

            for (point = 0; point <= 63; ++point) {
                g_6144[lane][point].head_x =
                    (g_60F8[lane] + random(9) - 4) * 64;
                g_6144[lane][point].head_y =
                    (g_6100[lane] + random(9) - 4) * 64;
                g_6144[lane][point].tail_x = g_6144[lane][point].head_x;
                g_6144[lane][point].tail_y = g_6144[lane][point].head_y;
                g_6144[lane][point].tail_start = g_5DB2 / 8 + random(g_5DB2 / 4);
                g_6144[lane][point].tail_end = g_6144[lane][point].tail_start * 2
                                        + random(g_5DB2 / 4);
            }
            if (g_5D7A >= g_5DA6)
                cycle_palette();
        }

        if (g_613E == 0)
            set_xor_mode(1);
    }

    lane = 0;
    g_6128[0] = 1;
    while (g_6128[0] != 0 || g_6128[1] != 0 ||
           g_6128[2] != 0 || g_6128[3] != 0) {
        if (g_6128[lane] != 0) {
            for (point = 0; point <= 63; ++point) {
                int span;
                g_7488 = g_6118[lane];
                if (g_613E == 1)
                    span = g_6144[lane][point].tail_start;
                else
                    span = g_6144[lane][point].tail_end;
                if (g_6108[lane] < span) {
                    g_5DF0 = g_6144[lane][point].head_x >> 6;
                    g_5DEE = g_6144[lane][point].head_y >> 6;
                    if (abs(g_5DF0) <= g_5DB0 && abs(g_5DEE) <= g_5DB2) {
                        plot_callback(0);
                        g_6144[lane][point].head_x += g_6144[lane][point].velocity_x;
                        g_6144[lane][point].head_y += g_6144[lane][point].velocity_y
                                                  + g_6130[lane];
                    } else if (g_613E == 1)
                        g_6144[lane][point].tail_start = g_6108[lane];
                }

                if ((g_613E & 1) == 0 &&
                    g_6144[lane][point].tail_start <= g_6108[lane] &&
                    g_6144[lane][point].tail_end >= g_6108[lane]) {
                    g_7488 = g_6118[lane];
                    g_5DF0 = g_6144[lane][point].tail_x >> 6;
                    g_5DEE = g_6144[lane][point].tail_y >> 6;
                    if (g_613E == 2)
                        g_7488 = g_613A;
                    if (g_613E == 4 && (g_5DF0 & g_6140) != 0)
                        g_7488 = g_613C;
                    if (abs(g_5DF0) <= g_5DB0 && abs(g_5DEE) <= g_5DB2)
                        plot_callback(0);
                    else
                        g_6144[lane][point].tail_end = g_6108[lane];

                    tail_step = g_6144[lane][point].tail_start * 2;
                    if (g_6108[lane] < tail_step - 1) {
                        tail_step = g_6130[lane] * 2 - tail_step * g_6138;
                        g_6144[lane][point].tail_x += g_6144[lane][point].velocity_x;
                        g_6144[lane][point].tail_y += g_6144[lane][point].velocity_y + tail_step;
                        g_5DF0 = g_6144[lane][point].tail_x >> 6;
                        g_5DEE = g_6144[lane][point].tail_y >> 6;
                        if (g_613E == 2)
                            g_7488 = g_613A;
                        if (g_613E == 4 && (g_5DF0 & g_6140) != 0)
                            g_7488 = g_613C;
                        plot_callback(0);
                        g_6144[lane][point].tail_x += g_6144[lane][point].velocity_x;
                        g_6144[lane][point].tail_y +=
                            g_6144[lane][point].velocity_y + tail_step + g_6138;
                    } else {
                        g_6144[lane][point].tail_x += g_6144[lane][point].velocity_x;
                        g_6144[lane][point].tail_y +=
                            g_6144[lane][point].velocity_y + g_6130[lane] - g_6138;
                    }
                }

                if ((g_613E == 1 &&
                     g_6144[lane][point].tail_start == g_6108[lane]) ||
                    g_6144[lane][point].tail_end == g_6108[lane])
                    ++g_6110[lane];
            }

            if ((g_613E & 1) != 0) {
                ++g_7488;
                if (g_7488 >= g_5DB6)
                    g_7488 = 3;
                g_6118[lane] = g_7488;
            }
            g_6130[lane] += g_6138;
            ++g_6108[lane];
            if ((g_6108[lane] == g_6120[lane] ||
                 (g_6110[lane] > 63 && g_6108[lane] < g_6120[lane])) && lane < 3)
                g_6128[lane + 1] = 1;
            if (g_6110[lane] > 63)
                g_6128[lane] = 0;
        }
        if (++lane > 3)
            lane = 0;
        if (!(g_5D7A < g_5DA6))
            cycle_palette();
    }

    g_5DF0 = g_5DB0 + 1;
    g_5DEE = g_5DB2 + 1;
    g_5CBB += g_5DAE * 8;
}

/* objcmp: {"function":"random_delay_transition","address":"0ccd:0b75","size":133,"symbols":{"_g_5D78":"DS:5d78","_g_5D7A":"DS:5d7a","_g_5DA6":"DS:5da6","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_randomize_helper":"0ccd:008d","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7"}} */
void random_delay_transition(void)
{
    randomize_helper(random(2) + 1);
    g_5D78 = random(200) + 200;
    while (g_5D78 > 0) {
        randomize_helper(0);
        if (g_5D7A >= g_5DA6)
            cycle_palette();
        if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5D92 == 'n')
            return;
    }
}
/* objcmp: {"function":"quilt_transition","address":"0ccd:0bfa","size":369,"symbols":{"_g_5D78":"DS:5d78","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5DA6":"DS:5da6","_g_5D7A":"DS:5d7a","_g_5D92":"DS:5d92","_g_5DB0":"DS:5db0","_g_5DB2":"DS:5db2","_g_7354":"DS:7354","_g_7356":"DS:7356","_g_7348":"DS:7348","_g_734A":"DS:734a","_g_734C":"DS:734c","_g_734E":"DS:734e","_g_7350":"DS:7350","_g_7352":"DS:7352","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_swap_quilt_cells":"066a:0ba5","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515"}} */
void quilt_transition(void)
{
    g_5D78 = random(180) + 180;
    g_7348 = random(7) + 2;
    while (g_5D78 != 0) {
        g_7354 = g_5DB0 / g_7348;
        g_7356 = g_5DB2 / g_7348;
        g_734C = random(g_7348) * g_7354;
        g_734E = random(g_7348) * g_7356;
        g_7350 = random(g_7348) * g_7354;
        g_7352 = random(g_7348) * g_7356;
        g_734A = random(4);
        swap_quilt_cells(g_734A, g_734C, g_734E, g_7350, g_7352,
                         g_7354, g_7356);
        if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5D92 == 'n')
            return;
    }
}
/* objcmp: {"function":"gas_cloud","address":"0ccd:0d6b","size":198,"symbols":{"_g_5D78":"DS:5d78","_g_5D7A":"DS:5d7a","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5DA6":"DS:5da6","_g_5D92":"DS:5d92","_g_5DB2":"DS:5db2","_g_5DC6":"DS:5dc6","_g_5DE4":"DS:5de4","_g_7436":"DS:7436","_g_04D4":"DS:04d4","_g_7490":"DS:7490","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_clear_vram_words":"1899:0acd","_restore_vram_page":"1899:133d","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_animate_vertical_display_start":"0ccd:4051"}} */
void gas_cloud(void)
{
    int vertical = 0;

    g_7436 = g_5DC6;
    g_5DC6 = 1;
    if (g_5DE4 == 0 && g_04D4 != 0 && random(5) != 0) {
        vertical = g_5DB2 * 2 + 1;
        if (g_7490 == 0)
            g_7490 = 2000;
        else
            g_7490 = 0;
        clear_vram_words(0, 16000, g_7490);
    }
    restore_vram_page();
    g_5DC6 = g_7436;

    if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
        return;
    if (g_5D92 != 'n') {
        if (vertical == 0) {
            wait_fade_tick(0x78, 0);
        } else if (g_7490 == 0) {
            animate_vertical_display_start(vertical, 0);
        } else {
            animate_vertical_display_start(0, vertical);
        }
    }
}
/* objcmp: {"function":"randomize_transition","address":"0ccd:0e31","size":330,"symbols":{"_g_5D7A":"DS:5d7a","_g_5DA6":"DS:5da6","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_g_5DAE":"DS:5dae","_g_5DB2":"DS:5db2","_g_5DB6":"DS:5db6","_g_5DF0":"DS:5df0","_g_5DEE":"DS:5dee","_g_5CB1":"DS:5cb1","_g_7484":"DS:7484","_g_7488":"DS:7488","_g_5DC0":"DS:5dc0","_set_xor_mode":"11c1:0e2a","_fill_square":"066a:026b","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7"}} */
void randomize_transition(void)
{
    unsigned int state;
    signed char scale;
    signed char minimum;
    unsigned int polynomial;

    set_xor_mode(1);
    polynomial = 0x6000;
    scale = 0x40;
    minimum = 2;
    if (g_7484 == 4)
        minimum = 4;
    do {
        g_7488 = 0 & g_5DC0;
        state = 1;
        do {
            g_5DEE = state / ((g_5DAE + 1) / scale);
            if (g_5DEE <= g_5DB2 / scale) {
                g_5DF0 = state % ((g_5DAE + 1) / scale);
                g_5DF0 *= scale;
                g_5DEE *= scale;
                if (scale > 1) {
                    g_7488++;
                    if (g_7488 >= g_5DB6)
                        g_7488 = 3;
                }
                fill_square(scale, 1);
            }
            if (state & 1)
                state = (state >> 1) ^ polynomial;
            else
                state >>= 1;
            if (g_5D7A >= g_5DA6)
                cycle_palette();
            if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5D92 == 'n')
                return;
        } while (state != 1);

        g_5DEE = g_5DF0 = 0;
        if (scale > 1) {
            g_7488++;
            if (g_7488 >= g_5DB6)
                g_7488 = 3;
        }
        fill_square(scale, 1);
        scale >>= 1;
        if (scale < minimum) {
            g_5CB1 = 0;
            return;
        }
    } while (1);
}
/* objcmp: {"function":"mesh_transition","address":"0ccd:0f7b","size":677,"symbols":{"_G_7484":"DS:7484","_G_7488":"DS:7488","_G_5DC0":"DS:5dc0","_G_5DF0":"DS:5df0","_G_5DEE":"DS:5dee","_G_5DB0":"DS:5db0","_G_5DB2":"DS:5db2","_G_5DA6":"DS:5da6","_G_5D7A":"DS:5d7a","_G_5D7E":"DS:5d7e","_G_5D80":"DS:5d80","_G_5D92":"DS:5d92","_plot_callback":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_choose_color":"066a:011b","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7","_g_5D78":"DS:5d78"}} */
void mesh_transition(void)
{
    int limit;
    register int stride;
    register int next;
    int direction;
    int color_a;
    int color_b;

    if (g_7484 == 8)
        limit = 0x100;
    else
        limit = 0x200;
    next = limit;
    direction = random(2);
    color_a = random(3);
    color_b = random(7) + 2;
    g_7488 = 3 & g_5DC0;
    g_5DF0 = 0;
    g_5DEE = 0;
    plot_callback(1);
    do {
        next >>= 1;
        stride = next;
        do {
            g_5DF0 = stride;
            g_5DEE = stride;
            if (g_5DEE <= g_5DB2) {
                if (direction != 0)
                    choose_color(color_b);
                else
                    choose_color(color_a);
                plot_callback(1);
                do {
                    g_5DF0 -= next;
                    if (g_5DF0 <= g_5DB0) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (g_5DF0 > 0);
            }

            g_5DF0 = stride;
            if (g_5DF0 <= g_5DB0) {
                if (direction == 0)
                    choose_color(color_a);
                g_5DEE = stride;
                do {
                    g_5DEE -= next;
                    if (g_5DEE <= g_5DB2) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (g_5DEE > 0);
            }

            stride += next;
            g_5DF0 = stride;
            g_5DEE = stride;
            if (g_5DEE <= g_5DB2) {
                if (direction == 0)
                    choose_color(color_a);
                g_5DF0 += next;
                do {
                    g_5DF0 -= next * 2;
                    if (g_5DF0 <= g_5DB0) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (g_5DF0 > next);
            }

            g_5DF0 = stride;
            if (g_5DF0 <= g_5DB0) {
                if (direction == 0)
                    choose_color(color_a);
                g_5DEE += next;
                do {
                    g_5DEE -= next * 2;
                    if (g_5DEE <= g_5DB2) {
                        if (direction != 0)
                            choose_color(color_b);
                        plot_callback(1);
                    }
                } while (g_5DEE > next);
            }

            stride += next;
            if (g_5D7A >= g_5DA6)
                cycle_palette();
            if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5D92 == 'n')
                return;
        } while ((stride <= g_5DB0 || stride <= g_5DB2) && stride <= limit);

        g_5D78 = 5;
        while (g_5D78 > 0 && g_5D92 != 'n') {
            if (g_5D7A >= g_5DA6)
                cycle_palette();
        }
        if (next <= 1) {
            g_5D78 = 0x3c;
            while (g_5D78 > 0 && g_5D92 != 'n') {
                if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5D7A >= g_5DA6)
                    cycle_palette();
            }
            return;
        }
    } while (1);
}

/* objcmp: {"function":"firefly_transition","address":"0ccd:1220","size":1210,"symbols":{"_g_5DF0":"DS:5df0","_g_5DEE":"DS:5dee","_g_5DB0":"DS:5db0","_g_5DB2":"DS:5db2","_g_5DA6":"DS:5da6","_g_5D7A":"DS:5d7a","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_g_7488":"DS:7488","_g_5C3F":"DS:5c3f","_g_736C":"DS:736c","_g_7358":"DS:7358","_g_7362":"DS:7362","_g_60F8":"DS:60f8","_g_6108":"DS:6108","_g_6142":"DS:6142","_g_6144":"DS:6144","_g_5CBB":"DS:5cbb","_g_5D8A":"DS:5d8a","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_fill_square":"066a:026b","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7","_firefly_transition":"0ccd:1220","_g_5DB6":"DS:5db6","_g_7484":"DS:7484","_g_5DE4":"DS:5de4","_g_5DAE":"DS:5dae","_plot_callback":"DS:5c3f"}} */
#define FF_STATE(i)       (g_736C[(i)])
#define FF_LONG(i)        (*(long *)&g_736C[(i)])
#define FF_PART(l,p,f)    (g_6144[(l)][(p)][(f)])
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
        if (g_7484 == 8) {
            FF_STATE(6) = g_5D8A >> 8;
            FF_STATE(5) = g_5D8A & 0xff;
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
                if (g_5DE4 == 0)
                    FF_STATE(8) = 4;
                else
                    FF_STATE(8) = 1;
            }
        } else {
            FF_STATE(5) = 0;
            FF_STATE(3) <<= 1;
            FF_STATE(4) = 1;
        }

        FF_LONG(9) = g_5DB2;
        FF_LONG(13) = -g_5DB0;
        FF_LONG(15) = g_5DB2;
        FF_STATE(1) = 0;
        while (FF_STATE(1) <= 4) {
            g_7358[FF_STATE(1)] = -g_5DB0;
            g_7362[FF_STATE(1)] = g_5DB2;
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

        g_7488 = FF_STATE(4) < 2 ? FF_STATE(5) : FF_STATE(2);
        particle = 0;
        do {
            g_5DF0 = g_7358[FF_STATE(1)];
            g_5DEE = g_7362[FF_STATE(1)];
            if (FF_STATE(4) == 2 && FF_STATE(1) == 0) {
                ++g_7488;
                if (g_7488 >= g_5DB6)
                    g_7488 = 3;
            }
            fill_square(FF_STATE(3), 0);
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

            g_5DF0 = g_7358[FF_STATE(1)];
            g_5DEE = g_7362[FF_STATE(1)];
            switch (FF_STATE(4)) {
            case 0:
                g_7488 = (g_5DF0 & FF_STATE(8)) ? FF_STATE(6) : FF_STATE(5);
                plot_callback(0);
                break;
            case 1:
                g_7488 = FF_STATE(5);
                fill_square(FF_STATE(3), 0);
                break;
            case 2:
                g_7488 = FF_STATE(2);
                if (FF_STATE(1) == 0) {
                    ++g_7488;
                    if (g_7488 >= g_5DB6)
                        g_7488 = 3;
                }
                FF_STATE(2) = g_7488;
                fill_square(FF_STATE(3), 0);
                break;
            }

            FF_LONG(13) += ((long)g_5DB0 * x_step) / g_5DB2;
            FF_LONG(15) -= y_step;
            g_5DF0 = FF_LONG(13) >> 16;
            g_5DEE = FF_LONG(15) >> 16;
            if ((FF_LONG(13) & 0x8000L) != 0)
                ++g_5DF0;
            if ((FF_LONG(15) & 0x8000L) != 0)
                ++g_5DEE;
            g_7358[FF_STATE(1)] = g_5DF0;
            g_7362[FF_STATE(1)] = g_5DEE;
            ++FF_STATE(1);
            FF_STATE(1) %= 5;

            if (g_5D7A >= g_5DA6)
                cycle_palette();
            if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5D92 == 'n')
                return;

            g_7488 = FF_STATE(7);
            if (FF_STATE(4) == 0)
                plot_callback(0);
            else
                fill_square(FF_STATE(3), 0);
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

/* objcmp: {"function":"spiral_transition","address":"0ccd:16da","size":734,"absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5d78":"DS:5d78","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_5c3f":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7","_plot_callback":"DS:5c3f","_transition_aspect_scale":"DS:0bbc","_transition_fixed_scale":"DS:0bc0","_transition_angle_step":"DS:0bc4","_sin":"0000:0e75","_cos":"0000:0f6e","F_FTOL@":"0000:0fbe"}} */
void spiral_transition(void)
{
    int color_counter, limit_y, limit_x, phase;
    float radius, growth, angle, aspect;
    transition_point_t next_x, next_y;
    transition_point_t x_history[360], y_history[360];
    register int i;
    register int done;

    growth = 0.001127f;
    aspect = (float)g_5DB0 / (float)g_5DB2 * transition_aspect_scale;
    limit_y = (g_5DB2 * 3) >> 1;
    limit_x = (g_5DB0 * 3) >> 1;
    color_counter = 0;
    g_7488 = 3;
    phase = random(3);
    if (phase == 2)
        phase = 0;
    radius = 1.0f;
    angle = 0.0f;

    for (i = 0; i < 360; ++i) {
        radius *= 1 + growth;
        x_history[i].value = radius * cos(angle) * aspect * transition_fixed_scale;
        y_history[i].value = radius * sin(angle) * transition_fixed_scale;
        angle += transition_angle_step;
    }

    i = 0;
    do {
        next_x.value = x_history[i].value + (x_history[i].value >> 1);
        next_y.value = y_history[i].value + (y_history[i].value >> 1);
        g_5DF0 = x_history[i].part.whole;
        g_5DEE = y_history[i].part.whole;
        done = 1;
        do {
            done = line_step(next_x.part.whole, next_y.part.whole, done);
            plot_callback(phase);
        } while (done == 0);

        x_history[i].value = next_x.value;
        y_history[i].value = next_y.value;
        ++i;
        i = i % 360;
        if (g_5D7A >= g_5DA6)
            cycle_palette();
        if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5D92 == 'n')
            return;

        ++color_counter;
        if ((color_counter = color_counter & 0x0f) == 0) {
            ++g_7488;
            if (g_7488 >= g_5DB6)
                g_7488 = 3;
        }
    } while (next_x.part.whole <= limit_x || next_y.part.whole <= limit_y);
    g_5D78 = 60;
    while (g_5D78 > 0 && g_5D92 != 'n') {
        if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5D7A >= g_5DA6)
            cycle_palette();
    }
}

/* objcmp: {"function":"spiral_midpoint","address":"0ccd:19b8","size":233,"symbols":{"_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
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
/* objcmp: {"function":"drip_transition","address":"0ccd:1aa1","size":544,"symbols":{"_g_5DB0":"DS:5db0","_g_5DB2":"DS:5db2","_g_5DA6":"DS:5da6","_g_5D7A":"DS:5d7a","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5D92":"DS:5d92","_g_7488":"DS:7488","_g_7484":"DS:7484","_g_5DAA":"DS:5daa","_g_5DF0":"DS:5df0","_g_5DEE":"DS:5dee","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_spiral_midpoint":"0ccd:19b8","F_LXLSH@":"0000:1395","_g_5DB6":"DS:5db6","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7","_plot_callback":"DS:5c3f"}} */
typedef union {
    unsigned long value;
    struct { unsigned int fraction; int whole; } part;
} spiral_point_t;

void drip_transition(void)
{
    spiral_point_t x_path[641];
    spiral_point_t y_path[641];
    int finished = 0;
    int max_x;
    register int step;
    register int i;

    g_7488 = 3;
    max_x = g_5DB0 << 1;
    step = 8 << random(4);
    for (i = 0; i <= 0x280; ++i) {
        y_path[i].part.whole = -g_5DB2;
        y_path[i].part.fraction = (x_path[i].part.whole = 0);
    }
    for (i = 0; i <= max_x; i += step)
        x_path[i].part.fraction = random(0xa000) + 0x4000;
    for (i = 0; i <= max_x - step; i += step)
        spiral_midpoint(i, i + step,
                        (unsigned long far *)&x_path[0].value);

    if (finished < g_5DB0) do {
        g_5DF0 = -g_5DB0;
        for (i = 1; i <= max_x; ++i) {
            if (y_path[i].part.whole <= g_5DB2) {
                g_5DEE = y_path[i].part.whole;
                y_path[i].value = y_path[i].value + x_path[i].value;
                if (g_5DEE < y_path[i].part.whole)
                    plot_callback(0);
                if (g_5DB2 <= y_path[i].part.whole) {
                    ++y_path[i].part.whole;
                    ++finished;
                }
            }
            ++g_5DF0;
            if (g_5D7A >= g_5DA6)
                cycle_palette();
            if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                return;
            if (g_5D92 == 'n')
                return;
        }
        if (g_7484 == 8) {
            ++g_7488;
            if (g_7488 >= g_5DB6)
                g_7488 = 3;
        } else {
            ++g_5DAA;
            if ((g_5DAA &= 0x0f) == 0) {
                ++g_7488;
                if (g_7488 >= g_5DB6)
                    g_7488 = 3;
            }
        }
    } while (finished < g_5DB0);
}

/* objcmp: {"function":"cleanup_transition","address":"0ccd:1cc1","size":728,"symbols":{"_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5d78":"DS:5d78","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_set_xor_mode":"11c1:0e2a","_choose_color":"066a:011b","_line_step":"066a:000e","_cycle_palette":"11c1:2515","_poll_palette_keys":"11c1:18e7","_plot_callback":"DS:5c3f"}} */
void cleanup_transition(void)
{
    int y, x, pattern, choice, color_limit;
    int first_pattern, last_pattern, line_x, line_y;
    int start_x, start_y, bound_x, bound_y, done, pass;
    register int step_y, step_x;

    y = 0;
    x = 0;
    color_limit = random(8) + 2;
    choice = random(5);

    switch (choice) {
    case 0:
        first_pattern = 0;
        last_pattern = 7;
        break;
    case 1:
        y = g_5DB2;
        first_pattern = 0;
        last_pattern = 3;
        break;
    case 2:
        y = -g_5DB2;
        first_pattern = 4;
        last_pattern = 7;
        break;
    case 3:
        x = -g_5DB0;
        first_pattern = 2;
        last_pattern = 5;
        break;
    case 4:
        x = g_5DB0;
        first_pattern = 6;
        last_pattern = 9;
        break;
    }

    set_xor_mode(1);
    pattern = first_pattern;
    pass = 1;
    while (pass <= 2) {
        for (; pattern <= last_pattern; ++pattern) {
            switch (pattern) {
            case 0:
            case 8:
                start_x = bound_x = -g_5DB0;
                step_x = 0;
                start_y = y;
                bound_y = -g_5DB2;
                step_y = -1;
                break;
            case 1:
            case 9:
                start_x = -g_5DB0;
                bound_x = x;
                step_x = 1;
                start_y = bound_y = -g_5DB2;
                step_y = 0;
                break;
            case 2:
                start_x = x;
                bound_x = g_5DB0;
                step_x = 1;
                start_y = bound_y = -g_5DB2;
                step_y = 0;
                break;
            case 3:
                start_x = bound_x = g_5DB0;
                step_x = 0;
                start_y = -g_5DB2;
                bound_y = y;
                step_y = 1;
                break;
            case 4:
                start_x = bound_x = g_5DB0;
                step_x = 0;
                start_y = y;
                bound_y = g_5DB2;
                step_y = 1;
                break;
            case 5:
                start_x = g_5DB0;
                bound_x = x;
                step_x = -1;
                start_y = bound_y = g_5DB2;
                step_y = 0;
                break;
            case 6:
                start_x = x;
                bound_x = -g_5DB0;
                step_x = -1;
                start_y = bound_y = g_5DB2;
                step_y = 0;
                break;
            case 7:
                start_x = bound_x = -g_5DB0;
                step_x = 0;
                start_y = g_5DB2;
                bound_y = y;
                step_y = -1;
                break;
            }

            line_x = start_x;
            line_y = start_y;
            while (step_x * line_x <= step_x * bound_x &&
                   step_y * line_y <= step_y * bound_y) {
                g_5DF0 = x;
                g_5DEE = y;
                g_7488 = 3;
                done = 1;
                do {
                    done = line_step(line_x, line_y, done);
                    if (color_limit < 9)
                        choose_color(color_limit);
                    else {
                        ++g_7488;
                        if (g_7488 >= g_5DB6)
                            g_7488 = 3;
                    }
                    plot_callback(0);
                } while (done == 0);

                if (g_5D7A >= g_5DA6)
                    cycle_palette();
                if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
                    return;
                if (g_5D92 == 'n')
                    return;

                line_x += step_x;
                line_y += step_y;
            }
        }
        ++pass;
        pattern = first_pattern;
        set_xor_mode(0);
    }
}
