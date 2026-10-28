/* DAZZLE 5.0 image callbacks, source-order slice 066A:25EB..51A0. */
#include <stdlib.h>
#include <dos.h>
#include <math.h>
#include <stdio.h>
#include <conio.h>
#include <bios.h>
#include <dir.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>

extern char executable_path[80], startup_directory[80];
extern struct text_info saved_text_info;
extern union REGS keyboard_regs;
extern unsigned char far *palette_buffer;
extern char startup_directory_error[], assistance_message[], parser_error[];
extern char loading_box_top[], loading_box_left[], loading_label[];
extern char loading_box_right[], loading_box_bottom[];
extern char restore_directory_error[], startup_config_error[];
extern void restore_text_attributes(void);
extern void restore_runtime_callbacks(void);
extern void signal_diagnostic(int);
extern void initialize_program_defaults(void);
extern void save_video_state(void);
extern void detect_video_adapter(void);
extern int parse_command_line(int, char far * far *, int far *);
extern void print_startup_information(void);
extern void keyboard_led_flags(int, int);
extern void fatal_selfcheck_error(int, const char far *);
extern void check_executable_integrity(void);
extern void configure_video_backend(int);
extern int resolve_startup_configuration(int);
extern void install_runtime_callbacks(void);
extern void beep(unsigned, unsigned);
extern void park_disk_heads(void);
extern void prepare_logo_image_resources(void);
extern void semi_auto_loop(void), auto_loop(void);
extern void set_ega_raster_xor(int);
extern void closing_graphic_and_music(void);
extern void restore_video_state(int);
extern void print_closing_text_and_exit(int);

extern int x, y;
/* References to the original TU's shared literal pool, not new data owners. */
extern const double pedals_phase_scale; /* DS:06B7: 0.0002 */
extern const double sine_phase_step;    /* DS:06F3: 0.01 */
extern float amplitude_x, amplitude_y, phase, mod_x, mod_y, step_x, step_y;
extern void ega_plot_symmetric_640x700(int);
extern const double squirals_start_angle, squirals_angle_step; /* DS:06FB/0703 */
extern int g_5dae, g_5dee, g_5df0, g_6025;
extern int g_5b24, g_5b26, g_5dc0, g_5dc4;
extern unsigned int g_5b66, g_5b68;
extern unsigned int g_5db6, g_7488;
extern int far *g_5c0f;
extern void (far *g_fn5c47)();
extern void (far *g_fn5c43)();
extern void (far *g_fn5c37)(unsigned char far *);
extern unsigned int g_5d9c;
extern unsigned int g_5d9a;
extern unsigned int g_5d98;
extern unsigned int g_5d8e;
extern unsigned int g_5d8a;
extern unsigned int g_5d7e;
extern unsigned int g_5d7c;
extern int g_5dec;
extern int g_5dd2;
extern int g_5dce;
extern int g_5dcc;
extern int g_5dca;
extern int g_5dc8;
extern int g_5dbc;
extern int g_5da0;
extern int g_5d84;
extern int g_5d80;
extern int g_5d78;
extern int g_5b5a, g_5b5c, g_5b5e;
extern int g_5b6a, g_5b6c;
extern int g_5bf3, g_5bf5, g_5bf7, g_5bf9, g_5bfb, g_5bfd;
extern int g_5c09, g_5c0b, g_5c0d;
extern int g_5bdb, g_5bdd, g_5bdf, g_5be1, g_5be3, g_5be5;
extern int g_00a6[4], g_00ae[4];
extern int g_01e6[4], g_01ee[4];
extern int g_01f6[2], g_01fa[2];
typedef struct { int x, y; } SierpinskiVertex;
extern SierpinskiVertex g_5ba5[3];
extern int g_5bb1;
extern int g_5bcf, g_5bd1, g_5bd3, g_5bd5, g_5bd7, g_5bd9;
extern int g_5be7, g_5be9, g_5beb, g_5bed, g_5bef, g_5bf1;
extern int g_5b82, g_5b84, g_5b86, g_5b88;
extern int g_5bff, g_5c01, g_5c03, g_5c05, g_5c07;
extern int g_5c13, g_5c15, g_5c17, g_5c19, g_5c1b, g_5c1d, g_5c1f, g_5c21, g_5c23, g_5c25;
extern int g_5c27, g_5c29, g_5c2b, g_5c2d, g_5c2f, g_5c31;
extern int g_5b60, g_5b62, g_5b64;
extern int g_5c3f, g_5c41;
extern unsigned char g_00e6[64];
extern int g_5b92, g_5b94, g_5b96, g_5b98, g_5b9a, g_5b9c;
extern char g_5b9e;
extern int g_5b9f, g_5ba1, g_5ba3;
extern int g_5b2a, g_5b2c, g_5b2e, g_5b30, g_5b32, g_5b34;
extern long g_long5b36, g_long5b3a, g_long5b3e, g_long5b42, g_long5b46;
extern long g_long5b4a, g_long5b4e, g_long5b52, g_long5b56;
extern int g_5b28;
extern int g_5b6e, g_5b70, g_5b72, g_5b74, g_5b76, g_5b78, g_5b7a, g_5b7c, g_5b7e, g_5b80;
extern int g_5b8a, g_5b8c, g_5b8e, g_5cbb;
extern int g_5b90;
extern void choose_color(int kind);
extern int line_step(int endx, int endy, int initialize);
extern void fill_square(int color, int solid);
extern void far cycle_palette(void);
extern int g_5da6, g_5d7a, g_5daa, g_7484;
extern void (far *g_plot_pixel)(int);
extern int rand(void);
extern void far segment1899(void);


/* objcmp: {"function":"diag1","address":"066a:25eb","size":172,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_choose_color":"066a:011b","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void diag1(int size) {
    --y;
    if (size || x++ > g_5dae - 1) {
        x = 0;
        ++y;
        if (size) {
            g_6025 = (int)((long)rand() * 9L / 32768L);
            if (g_6025 == 1)
                --g_6025;
            if ((int)((long)rand() * 2L / 32768L) != 0)
                g_6025 = 0;
        }
        if (g_6025 == 0)
            choose_color(0);
    }
    if (y < 0)
        y = g_5dae;
    if (g_6025 != 0)
        choose_color(g_6025);
}

/* objcmp: {"function":"diag2","address":"066a:2697","size":185,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b24":"DS:5b24","_g_5b26":"DS:5b26","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_g_7488":"DS:7488","_choose_color":"066a:011b","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void diag2(int size) {
    register int mode = size;
    if (mode) {
        g_5b24 = g_5b26 = 0;
        g_6025 = (int)((long)rand() * 2L / 32768L);
    }
    ++y;
    if (g_6025 != 0) {
        if (x++ >= g_5dae) {
            x = 0;
            --y;
            g_7488 = 4 & g_5dc0;
            if (++g_5b26 > 20)
                g_5b26 = 0;
        }
        if (g_5b24++ <= g_5b26)
            goto clamp;
        g_5b24 = 0;
    } else if (!mode && x++ <= g_5dae - 1) {
        goto clamp;
    } else {
        x = 0;
        --y;
    }
color:
    choose_color(0);
clamp:
    if (y > g_5dae)
        y = 0;
}

/* objcmp: {"function":"ellipse","address":"066a:2750","size":1548,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b2a":"DS:5b2a","_g_5b2c":"DS:5b2c","_g_5b2e":"DS:5b2e","_g_5b30":"DS:5b30","_g_5b32":"DS:5b32","_g_5b34":"DS:5b34","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_choose_color":"066a:011b","_g_long5b36":"DS:5b36","_g_long5b3a":"DS:5b3a","_g_long5b3e":"DS:5b3e","_g_long5b42":"DS:5b42","_g_long5b46":"DS:5b46","_g_long5b4a":"DS:5b4a","_g_long5b4e":"DS:5b4e","_g_long5b52":"DS:5b52","_g_long5b56":"DS:5b56","_g_5b28":"DS:5b28","_g_5cbb":"DS:5cbb","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_plot_pixel":"DS:5c3f","_cycle_palette":"11c1:2515","_line_step":"066a:000e"}} */
void ellipse(int size) {
    if (size || (unsigned)g_5b2a >= 2) {
        g_5b2a = 0;
        g_6025 = (int)((long)rand() * 2L / 32768L);
        g_5b2c = (int)((long)rand() * 3L / 32768L);
        g_5b2e = 0;
        if (g_5b2c == 0) {
            g_5b32 = 0;
            g_5b34 = 0;
        } else {
            g_5b32 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 4;
            g_5b34 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 4;
        }
        g_long5b42 = (int)((long)rand() * (g_5dae >> 1) / 32768L) + (g_5dae >> 4);
        g_5b30 = (int)((long)rand() * (g_5dae >> 1) / 32768L) + (g_5dae >> 4);
        g_long5b46 = g_5b30;
        g_long5b4a = g_long5b42 * g_long5b42;
        g_long5b4e = g_long5b4a << 1;
        g_long5b52 = g_long5b46 * g_long5b46;
        g_long5b56 = g_long5b52 << 1;
        g_long5b36 = g_long5b52 - g_long5b4a * g_long5b46 + g_long5b4a / 4L;
        g_long5b3a = 0L;
        g_long5b3e = g_long5b4e * g_long5b46;
        g_7488 = 3 & g_5dc0;
    }
    switch (g_5b2a) {
    case 0:
        if (g_long5b3a < g_long5b3e) {
            if (g_6025) {
                if (++g_7488 >= g_5db6)
                    g_7488 = 3;
            } else {
                g_7488 = (g_5b2e + g_5b30) & g_5dc0;
            }
            if ((unsigned)g_5b2c < 2) {
                g_5df0 = g_5b32;
                g_5dee = g_5b34;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    g_plot_pixel(1);
                    if (!g_6025 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                } while (!g_5b28);
            } else {
                g_5df0 = g_5b32 - g_5b2e;
                g_5dee = g_5b34 - g_5b30;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    g_plot_pixel(1);
                    if (!g_6025 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                } while (!g_5b28);
                g_5df0 = g_5b32 - g_5b2e;
                g_5dee = g_5b34 + g_5b30;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 - g_5b30, g_5b28);
                    g_plot_pixel(1);
                    if (!g_6025 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                } while (!g_5b28);
            }
            if (g_long5b36 > 0L) {
                --g_5b30;
                g_long5b3e -= g_long5b4e;
                g_long5b36 -= g_long5b3e;
            }
            ++g_5b2e;
            g_long5b3a += g_long5b56;
            g_long5b36 += g_long5b52 + g_long5b3a;
            break;
        }
        ++g_5b2a;
        g_long5b36 += (3L * (g_long5b4a - g_long5b52) / 2L
                      - (g_long5b3a + g_long5b3e)) / 2L;
        /* Continue directly with the second region of the ellipse. */
    case 1:
        if (g_5b30 >= 0) {
            if (g_6025) {
                if (++g_7488 >= g_5db6)
                    g_7488 = 3;
            } else {
                g_7488 = (g_5b2e + g_5b30) & g_5dc0;
            }
            if ((unsigned)g_5b2c < 2) {
                g_5df0 = g_5b32;
                g_5dee = g_5b34;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    g_plot_pixel(1);
                    if (!g_6025 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                } while (!g_5b28);
            } else {
                g_5df0 = g_5b32 - g_5b2e;
                g_5dee = g_5b34 - g_5b30;
                g_5b28 = 1;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 + g_5b30, g_5b28);
                    ++g_5cbb;
                    g_plot_pixel(1);
                    if (!g_6025 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                } while (!g_5b28);
                g_5df0 = g_5b32 - g_5b2e;
                g_5dee = g_5b34 + g_5b30;
                do {
                    g_5b28 = line_step(g_5b32 + g_5b2e, g_5b34 - g_5b30, g_5b28);
                    g_plot_pixel(1);
                    if (!g_6025 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                } while (!g_5b28);
            }
            if (g_long5b36 < 0L) {
                ++g_5b2e;
                g_long5b3a += g_long5b56;
                g_long5b36 += g_long5b3a;
            }
            --g_5b30;
            g_long5b3e -= g_long5b4e;
            g_long5b36 += g_long5b4a - g_long5b3e;
        } else {
            ++g_5b2a;
        }
    }
}

/* objcmp: {"function":"formula1","address":"066a:2d5c","size":190,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b5a":"DS:5b5a","_g_5b5c":"DS:5b5c","_g_5b5e":"DS:5b5e","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void formula1(int size) {
    if (!size && x <= g_5dae)
        goto update;
    x = 0;
    y = -1;
    g_5b5a = (int)((long)rand() * 2L / 32768L) + 1;
    g_5b5c = (int)((long)rand() * 3L / 32768L) + 1;
    g_5b5e = (int)((long)rand() * 4L / 32768L) + 2;
update:
    if (y++ > x) {
        ++x;
        y = 0;
    }
    g_7488 = (y * y * g_5b5c - (x << g_5b5a)) >> g_5b5e & g_5dc0;
}

/* objcmp: {"function":"grid","address":"066a:2e1a","size":313,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b60":"DS:5b60","_g_5b62":"DS:5b62","_g_5b64":"DS:5b64","_g_5b66":"DS:5b66","_g_5b68":"DS:5b68","_g_00e6":"DS:00e6","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_5c3f":"DS:5c3f","_g_5c41":"DS:5c41","_rand":"0000:1694","_segment1899":"1899:0000","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void grid(int size) {
    if (size) {
        g_7488 = (int)(((long)rand() << 2) / 32768L) & g_5dc0;
        g_5b64 = g_5b66 = g_5b68 = 0;
        g_6025 = (int)((long)rand() * 3L / 32768L);
    }
    do {
        if ((g_5b64 & 0x3f) == 0) {
            if (g_6025 == 0 && ++g_7488 >= g_5db6)
                g_7488 = 3;
            g_5b62 = g_5b66;
            x = g_5b66 - 1;
            if (g_5c41 == FP_SEG(segment1899) && g_5c3f == 0x012d)
                y = g_5b68 << 1;
            else
                y = g_5b68;
            g_5b60 = g_5b62 + 7;
            g_5b64 = ((g_5b66 + g_5b68) & 0x18) << 3;
            g_5b66 += 8;
            if (g_5b66 >= 0x60) {
                if (++g_7488 >= g_5db6)
                    g_7488 = 3;
                g_5b66 = 0;
                g_5b68 += 8;
                if (g_5b68 > 0x60)
                    g_5b68 = 0;
            }
        }
        if (++x > g_5b60) {
            x = g_5b62;
            ++y;
            if (g_5c41 == FP_SEG(segment1899) && g_5c3f == 0x012d)
                ++y;
        }
    } while (g_00e6[g_5b64++] != 0);
}

/* objcmp: {"function":"hexagon","address":"066a:2f53","size":119,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b6a":"DS:5b6a","_g_5b6c":"DS:5b6c","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488","_choose_color":"066a:011b"}} */
void hexagon(int size) {
    if (!size && (x <= g_5dae || y <= g_5dae))
        goto paint;
    g_5b6c = g_5b6a = 1;
    x = y = 0;
    if (++g_7488 >= g_5db6)
        g_7488 = 3;
paint:
    if (x < 0) {
        y = 0;
        ++g_5b6c;
        x = g_5b6c;
        ++g_5b6a;
        choose_color(0);
    } else {
        ++y;
        if (y > g_5b6a)
            --x;
    }
}

/* objcmp: {"function":"hilbert","address":"066a:2fca","size":715,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b6e":"DS:5b6e","_g_5b70":"DS:5b70","_g_5b72":"DS:5b72","_g_5b74":"DS:5b74","_g_5b76":"DS:5b76","_g_5b78":"DS:5b78","_g_5b7a":"DS:5b7a","_g_5b7c":"DS:5b7c","_g_5b7e":"DS:5b7e","_g_5b80":"DS:5b80","_g_5dae":"DS:5dae","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5cbb":"DS:5cbb","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_hilbert":"066a:2fca","_cycle_palette":"11c1:2515","_fill_square":"066a:026b","_g_plot_pixel":"DS:5c3f"}} */
void hilbert(int size) {
    int initialize, direction, column, mode, color, step;
    register int dx, dy;
    if ((unsigned)g_5d7a >= (unsigned)g_5da6)
        cycle_palette();
    if (size || g_5b80) {
        g_5b80 = 0;
        dx = g_5dae >> 1;
        step = random(3);
        g_5b76 = step + 3;
        g_5b7e = 6 >> step;
        g_5b78 = dx / (2 << (g_5b76 - 1));
        g_5b7a = 0;
        g_5b6e = g_5b70 = g_5b72 = g_5b74 = 0;
        g_5b7c = random(2);
        if (size && random(2)) {
            x = y = 0;
            mode = random(2);
            if (mode) {
                if (++g_7488 >= g_5db6)
                    g_7488 = 3;
            } else {
                color = random(3) + 2;
            }
            for (dy = 1; dy <= dx; ++dy) {
                for (column = 1; column <= dx; ++column) {
                    if (!mode)
                        choose_color(color);
                    g_plot_pixel(1);
                    ++y;
                }
                ++x;
                y -= dx;
                if ((unsigned)g_5d7a >= (unsigned)g_5da6)
                    cycle_palette();
            }
            set_ega_raster_xor(0);
        }
        hilbert(0);
        g_5cbb = 32000;
        g_5b80 = 1;
    } else {
        --g_5b76;
        if (g_5b76) {
            step = g_5b78;
            g_5b78 = g_5b7a;
            g_5b7a = step;
            hilbert(0);
            step = g_5b7a;
            g_5b7a = g_5b78;
            g_5b78 = step;
        }
        for (direction = 1; direction <= 3; ++direction) {
            switch (direction) {
            case 1: dx = g_5b78; dy = g_5b7a; break;
            case 2: dx = g_5b7a; dy = g_5b78; break;
            case 3:
                dx = -g_5b78;
                dy = -g_5b7a;
                g_5b78 = dy;
                g_5b7a = dx;
                break;
            }
            g_5b72 += dx;
            g_5b74 += dy;
            g_5df0 = g_5b6e;
            g_5dee = g_5b70;
            initialize = 1;
            do {
                initialize = line_step(g_5b72, g_5b74, initialize);
                fill_square(g_5b7e, 1);
                if (g_5b7c && ++g_7488 >= g_5db6)
                    g_7488 = 3;
            } while (!initialize);
            if (!g_5b7c && ++g_7488 >= g_5db6)
                g_7488 = 3;
            g_5b6e = g_5b72;
            g_5b70 = g_5b74;
            if (g_5b76)
                hilbert(0);
        }
        g_5b78 = -dx;
        g_5b7a = -dy;
        ++g_5b76;
    }
}

/* objcmp: {"function":"lines","address":"066a:3295","size":266,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b82":"DS:5b82","_g_5b84":"DS:5b84","_g_5b86":"DS:5b86","_g_5b88":"DS:5b88","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void lines(int size) {
    if (size) {
        g_5b82 = 255;
        g_5b88 = 0;
        g_5b84 = g_5b86 = 1;
        g_7488 = (int)(((long)rand() << 2) / 32768L) & g_5dc0;
    }
    ++g_5b82;
    if ((g_5b82 = g_5b82 & 3) == 0) {
        x += g_5b84;
        g_5b88 += g_5b86;
        if (x < 0) {
            g_5b84 = 1;
            x = 0;
            if (++g_7488 >= g_5db6)
                g_7488 = 3;
        }
        if (g_5b88 < 0) {
            g_5b86 = (int)(((long)rand() << 2) / 32768L) + 1;
            g_5b88 = 0;
        }
        if (x > g_5dae) {
            g_5b84 = -1;
            x = g_5dae;
        }
        if (g_5b88 > g_5dae) {
            g_5b86 = 0 - ((int)(((long)rand() << 2) / 32768L) + 1);
            g_5b88 = g_5dae;
        }
        y = g_5b88;
    } else {
        ++y;
    }
}

/* objcmp: {"function":"littleStars","address":"066a:339f","size":869,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5cbb":"DS:5cbb","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_5dc0":"DS:5dc0","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5daa":"DS:5daa","_g_7484":"DS:7484","_g_plot_pixel":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_choose_color":"066a:011b","_cycle_palette":"11c1:2515"}} */
void littleStars(void) {
    int length, start, spacing, dx, dy, minimum, initial_color, inset;
    int span, sweep_y, sweep_x, end_x, end_y, line_done, swap, row, column;
    register int mode, point;
    length = random(3 * g_5dae / 8) + g_5dae / 8;
    if (random(2))
        start = 0;
    else
        start = (random(3) + 1) * (length / 4);
    spacing = ((random(4) + 2) * length) / 20 & 0xfffe;
    dx = random(g_5dae);
    dy = random(g_5dae);
    if (random(2))
        minimum = random(spacing);
    else
        minimum = 0;
    if (g_7484 == 8)
        mode = random(3);
    else
        mode = 3;
    initial_color = g_7488 = random(g_5db6 - 1) & g_5dc0;
    for (row = 0; row <= 1; ++row) {
        sweep_x = (sweep_y = spacing) << 1;
        g_7488 = initial_color & g_5dc0;
        if (row) {
            end_y = end_x = start;
            inset = 1;
        } else {
            end_x = 0;
            end_y = length;
            inset = 0;
        }
        do {
            x = sweep_y;
            y = sweep_x;
            if (mode == 0 || mode == 2)
                g_7488 = initial_color & g_5dc0;
            line_done = 1;
            do {
                line_done = line_step(end_x, end_y, line_done);
                point = x;
                span = y;
                for (column = 0; column <= 7; ++column) {
                    x = dx + point;
                    y = dy + span;
                    if (x < g_5dae && y < g_5dae)
                        g_plot_pixel(1);
                    span = -span;
                    if (column & 1)
                        point = -point;
                    if (column == 3) {
                        swap = point;
                        point = span;
                        span = swap;
                    }
                    if (mode == 2 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                }
                x = span;
                y = point;
                if (mode == 0 && ++g_7488 >= g_5db6)
                    g_7488 = 3;
                ++g_5cbb;
                if ((unsigned)g_5d7a >= (unsigned)g_5da6)
                    cycle_palette();
            } while (!line_done);
            if (mode == 1 && ++g_7488 >= g_5db6)
                g_7488 = 3;
            if (mode == 3) {
                ++g_5daa;
                if ((g_5daa &= 3) == 0 && ++g_7488 >= g_5db6)
                    g_7488 = 3;
            }
            end_x -= inset;
            --end_y;
            sweep_y -= sweep_x & 1;
            --sweep_x;
        } while (sweep_y >= minimum);
    }
    x = dx;
    y = dy;
}

/* objcmp: {"function":"lotsadots","address":"066a:3704","size":129,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_choose_color":"066a:011b"}} */
void lotsadots(int size) {
    if (size)
        g_6025 = (int)((long)rand() * 9L / 32768L);
    x = (int)((long)rand() * g_5dae / 32768L);
    y = (int)((long)rand() * g_5dae / 32768L);
    choose_color(g_6025);
}

/* objcmp: {"function":"pedals","address":"066a:3785","size":302,"absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b8a":"DS:5b8a","_g_5b8c":"DS:5b8c","_g_5b8e":"DS:5b8e","_g_5b90":"DS:5b90","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","F_LXRSH@":"0000:13b6","F_LXURSH@":"0000:13d6","_cos":"0000:0f6e","_sin":"0000:0e75","F_FTOL@":"0000:0fbe","_pedals_phase_scale":"DS:06b7"}} */
void pedals(int size) {
    float phase;
    if (size || g_5b8c <= 0) {
        g_5b90 = 255;
        g_5b8e = 0;
        g_5b8c = g_5b8a = 3 * (g_5dae / 4);
        if (size)
            g_7488 = 3 & g_5dc0;
        else if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
    if (g_5b8a <= 0) {
        g_5b8c -= 16;
        g_5b8a = g_5b8c;
        g_5b8e = 0;
    }
    ++g_5b90;
    if ((g_5b90 = g_5b90 & 7) != 0)
        goto pedals_bounds;
    g_5b8e += 31;
    phase = g_5b8e * pedals_phase_scale;
    x = g_5b8a * sin(phase);
    y = g_5b8a * cos(phase);
    if ((--g_5b8a & 7) == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
    return;
pedals_bounds:
    ++x;
    ++y;
}

/* objcmp: {"function":"puffs","address":"066a:38b3","size":534,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b92":"DS:5b92","_g_5b94":"DS:5b94","_g_5b96":"DS:5b96","_g_5b98":"DS:5b98","_g_5b9a":"DS:5b9a","_g_5b9c":"DS:5b9c","_g_5b9e":"DS:5b9e","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e","_choose_color":"066a:011b"}} */
void puffs(int size) {
    if (!size && abs(x) < g_5dae && abs(y) < g_5dae)
        goto draw;
    x = g_5b92 = (int)((long)rand() * g_5dae / 32768L);
    y = g_5b94 = (int)((long)rand() * g_5dae / 32768L);
    g_5b9a = (int)((long)rand() * (g_5dae / 6) / 32768L) + g_5dae / 6;
    g_5b9c = 1;
    g_5b96 = g_5b92 + g_5b9a;
    g_5b98 = g_5b94 + g_5b9a;
    if (size)
        g_6025 = (int)((long)rand() * 2L / 32768L);
    g_5b9e = (int)((long)rand() * 2L / 32768L) + 1;
draw:
    g_5b9c = line_step(g_5b96, g_5b98, g_5b9c);
    choose_color(0);
    if (g_5b9c) {
        if (g_6025) {
            x = g_5b92;
            y = g_5b94;
            g_5b96 = g_5b92 + (int)((long)rand() * (g_5b9a * 2) / 32768L) - g_5b9a;
            g_5b98 = g_5b94 + (int)((long)rand() * (g_5b9a * 2) / 32768L) - g_5b9a;
        } else {
            g_5b92 += g_5b9e;
            x = g_5b92;
            g_5b94 += g_5b9e;
            y = g_5b94;
            g_5b96 = g_5b92 + (int)((long)rand() * g_5b9a / 32768L);
            g_5b98 = g_5b94 + (int)((long)rand() * g_5b9a / 32768L);
        }
    }
}

/* objcmp: {"function":"ribbons","address":"066a:3ac9","size":338,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b9f":"DS:5b9f","_g_5ba1":"DS:5ba1","_g_5ba3":"DS:5ba3","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void ribbons(int size) {
    if (size) {
        g_5b9f = g_5ba1 = 0;
        g_5ba3 = 24;
        g_6025 = (int)((long)rand() * 2L / 32768L);
    }
    if (g_5b9f++ > g_5ba1) {
        x -= g_5ba1;
        g_5b9f = 0;
        if (++g_5ba1 > g_5ba3) {
            g_5ba1 = 0;
            if ((g_5ba3 += (int)((long)rand() * 4L / 32768L)) > 44)
                g_5ba3 = 0;
        }
        if (g_6025 && ++g_7488 >= g_5db6)
            g_7488 = 3;
        if (++y > g_5dae)
            y = (int)((long)rand() * (g_5dae / 2) / 32768L);
    }
    if (++x > g_5dae)
        x = (int)((long)rand() * (g_5dae / 2) / 32768L);
    if ((int)((long)rand() * 1000L / 32768L) == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
}

/* objcmp: {"function":"sierpinski","address":"066a:3c1b","size":311,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5ba5":"DS:5ba5","_g_5bb1":"DS:5bb1","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void sierpinski(int size) {
    int choice;
    register int center;
    if (size) {
        center = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 3;
        x = y = center;
        g_5ba5[1].x = g_5ba5[1].y = g_5ba5[2].y = center;
        if ((int)((long)rand() * 2L / 32768L))
            g_5ba5[0].x = g_5ba5[0].y = g_5ba5[2].x = center / 2;
        else
            g_5ba5[0].x = g_5ba5[0].y = g_5ba5[2].x = 0;
        g_5bb1 = (int)((long)rand() * 2L / 32768L);
        g_7488 = (int)(((long)rand() << 2) / 32768L) & g_5dc0;
    }
    choice = (int)((long)rand() * 3L / 32768L);
    if (choice == 0 && g_5bb1 == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
    x += (g_5ba5[choice].x - x) / 2;
    y += (g_5ba5[choice].y - y) / 2;
}


/* objcmp: {"function":"sine","address":"066a:3d52","size":505,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_amplitude_x":"DS:5bb3","_amplitude_y":"DS:5bb7","_phase":"DS:5bbb","_mod_x":"DS:5bbf","_mod_y":"DS:5bc3","_step_x":"DS:5bc7","_step_y":"DS:5bcb","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_FTOL@":"0000:0fbe","_sin":"0000:0e75","_cos":"0000:0f6e","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_plot_pixel":"DS:5c3f","_ega_plot_symmetric_640x700":"1899:012d","_sine_phase_step":"DS:06f3"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
void sine(int rebuild)
{
    if (rebuild) {
        phase = mod_x = mod_y = 0;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
        amplitude_x = g_5dae / 8 + random(g_5dae / 8);
        amplitude_y = g_5dae / 8 + random(g_5dae / 8);
        step_x = 1.0 / (1 + amplitude_x);
        step_y = 1.0 / (1 + amplitude_y);
    }
    x = amplitude_x * sin(phase);
    y = amplitude_y * cos(phase);
    phase += sine_phase_step;
    mod_x += step_x;
    mod_y += step_y;
    amplitude_x += sin(mod_x);
    amplitude_y += cos(mod_y);
    if (x == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
    if (g_plot_pixel == ega_plot_symmetric_640x700) {
        x &= ~1;
        y &= ~1;
    }
}

/* objcmp: {"function":"squares","address":"066a:3f4b","size":354,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bcf":"DS:5bcf","_g_5bd1":"DS:5bd1","_g_5bd3":"DS:5bd3","_g_5bd5":"DS:5bd5","_g_5bd7":"DS:5bd7","_g_5bd9":"DS:5bd9","_g_5dae":"DS:5dae","_g_5dc4":"DS:5dc4","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_00a6":"DS:00a6","_g_00ae":"DS:00ae","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void squares(int size) {
    if (size || g_5bd1 >= g_5bd9 || x > g_5dae || y > g_5dae) {
        g_5bd5 = g_5bd7 = g_5bcf = g_5bd1 = g_5bd3 = 0;
        x = (int)((long)rand() * g_5dae / 32768L);
        y = (int)((long)rand() * g_5dae / 32768L);
        g_6025 = (int)((long)rand() * 2L / 32768L);
        if (g_5dc4)
            g_6025 = 0;
        g_5bd9 = (int)((long)rand() * (g_5dae / 4) / 32768L) + g_5dae / 4;
        if (g_6025)
            g_5bd9 >>= 1;
    }
    if (g_5bcf++ >= g_5bd1) {
        g_5bcf = 0;
        if (++g_5bd3 > 3) {
            ++g_5bd1;
            if (g_6025)
                --x;
            else
                ++x;
            if (++g_7488 >= g_5db6)
                g_7488 = 3;
            g_5bd3 = 0;
        }
        g_5bd5 = g_00a6[g_5bd3];
        g_5bd7 = g_00ae[g_5bd3];
    }
    x += g_5bd5;
    y += g_5bd7;
}

/* objcmp: {"function":"squirals","address":"066a:40ad","size":778,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_5cbb":"DS:5cbb","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_sin":"0000:0e75","_cos":"0000:0f6e","F_FTOL@":"0000:0fbe","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_plot_pixel":"DS:5c3f","_line_step":"066a:000e","_cycle_palette":"11c1:2515","F_LXLSH@":"0000:1395","_squirals_start_angle":"DS:06fb","_squirals_angle_step":"DS:0703"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
void squirals(int size) {
    int end_x, end_y, center_x, center_y, radius, symmetry, line_done;
    float scale = 1.0, decay, angle_step, cosine, sine_value, angle;
    register int saved_x, saved_y;

    angle = random(2) * squirals_start_angle;
    angle_step = (random(3) + 1) * squirals_angle_step;
    decay = 1.0 / (sin(angle_step) + cos(angle_step));
    radius = random(3 * g_5dae / 8) + g_5dae / 8;
    center_x = random(3 * g_5dae / 4);
    center_y = random(3 * g_5dae / 4);
    symmetry = random(4);
    do {
        cosine = cos(angle);
        sine_value = sin(angle);
        end_x = radius * scale * (cosine - sine_value);
        end_y = radius * scale * (cosine + sine_value);
        x = end_y;
        y = -end_x;
        line_done = 1;
        do {
            line_done = line_step(end_x, end_y, line_done);
            saved_x = x;
            saved_y = y;
            switch (symmetry) {
            case 0:
            case 1:
            case 2:
                x = center_x + saved_x;
                y = center_y + saved_y;
                g_plot_pixel(1);
                x = center_x - saved_x;
                y = center_y - saved_y;
                g_plot_pixel(1);
                if (symmetry == 2)
                    break;
            case 3:
                x = center_x - saved_y;
                y = center_y + saved_x;
                g_plot_pixel(1);
                x = center_x + saved_y;
                y = center_y - saved_x;
                g_plot_pixel(1);
            }
            x = saved_x;
            y = saved_y;
            if ((unsigned)g_5d7a >= (unsigned)g_5da6)
                cycle_palette();
            ++g_5cbb;
        } while (!line_done);
        angle += angle_step;
        scale *= decay;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    } while (abs(end_x) >= 1 || abs(end_y) >= 1);
    x = center_x;
    y = center_y;
}

/* objcmp: {"function":"spiral","address":"066a:43b7","size":235,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bdb":"DS:5bdb","_g_5bdd":"DS:5bdd","_g_5bdf":"DS:5bdf","_g_5be1":"DS:5be1","_g_5be3":"DS:5be3","_g_5be5":"DS:5be5","_g_00a6":"DS:00a6","_g_00ae":"DS:00ae","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void spiral(int size) {
    if (size || x > g_5dae || y > g_5dae) {
        g_5bdb = g_5bdd = g_5be1 = g_5bdf = 0;
        x = (int)((long)rand() * g_5dae / 32768L);
        y = (int)((long)rand() * g_5dae / 32768L);
    }
    if (g_5bdb++ >= g_5bdd) {
        g_5bdb = 0;
        ++g_5be1;
        g_5be1 = g_5be1 & 3;
        if ((g_5be1 & 1) == 0)
            ++g_5bdd;
    g_5be3 = g_00a6[g_5be1];
    g_5be5 = g_00ae[g_5be1];
    }
    x += g_5be3;
    y += g_5be5;
    ++g_5bdf;
    if ((g_5bdf = g_5bdf & 7) == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
}

/* objcmp: {"function":"spiral1","address":"066a:44a2","size":345,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5be7":"DS:5be7","_g_5be9":"DS:5be9","_g_5beb":"DS:5beb","_g_5bed":"DS:5bed","_g_5bef":"DS:5bef","_g_5bf1":"DS:5bf1","_g_01e6":"DS:01e6","_g_01ee":"DS:01ee","_g_5dae":"DS:5dae","_g_5dc4":"DS:5dc4","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void spiral1(int size) {
    if (size || g_5be9 >= g_5bf1 || x > g_5dae || y > g_5dae) {
        g_5bed = g_5bef = g_5be7 = g_5be9 = g_5beb = 0;
        x = (int)((long)rand() * g_5dae / 32768L);
        y = (int)((long)rand() * g_5dae / 32768L);
        g_6025 = (int)((long)rand() * 2L / 32768L);
        if (g_5dc4)
            g_6025 = 1;
        g_5bf1 = (int)((long)rand() * (3 * (g_5dae / 4)) / 32768L);
        if (g_6025)
            g_5bf1 >>= 1;
    }
    if (g_5be7++ >= g_5be9) {
        g_5be7 = 0;
        if (++g_5beb > 3) {
            ++g_5be9;
            if (g_6025)
                --x;
            else
                ++x;
            g_5beb = 0;
            if (++g_7488 >= g_5db6)
                g_7488 = 3;
        }
        g_5bed = g_01e6[g_5beb];
        g_5bef = g_01ee[g_5beb];
    }
    x += g_5bed;
    y += g_5bef;
}

/* objcmp: {"function":"sweep","address":"066a:45fb","size":107,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bf3":"DS:5bf3","_g_5bf5":"DS:5bf5","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488"}} */
void sweep(int size) {
    if (size || g_5bf5 > g_5dae / 2)
        g_5bf5 = g_5bf3 = x = y = 0;
    if (g_5bf3++ > g_5bf5) {
        ++y;
        g_5bf3 = 0;
    }
    if (x++ > g_5dae) {
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
        ++g_5bf5;
        x = y = 0;
    }
}

/* objcmp: {"function":"sweep2","address":"066a:4666","size":202,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bf7":"DS:5bf7","_g_5bf9":"DS:5bf9","_g_5bfb":"DS:5bfb","_g_5bfd":"DS:5bfd","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dc4":"DS:5dc4","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_g_7488":"DS:7488","_line_step":"066a:000e","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void sweep2(int size) {
    register int mode = size;
    if (!mode && g_5bfd == 0)
        goto draw;
    x = y = 0;
    if (mode) {
        g_6025 = (int)((long)rand() * 2L / 32768L);
        if (g_5dc4)
            g_6025 = 0;
        if (g_6025) {
            g_5bf9 = g_5bfb = 0;
        } else {
            g_5bf9 = -1;
        }
    }
    if (g_6025) {
        g_5bf9 += g_5bfb;
        if (++g_5bfb > g_5dae)
            g_5bfb = -g_5dae;
    } else {
        ++g_5bf9;
        g_5bf9 %= (g_5dae + 2);
    }
    g_5bf7 = g_5dae;
    if (++g_7488 >= g_5db6)
        g_7488 = 3;
    mode = 1;
draw:
    g_5bfd = line_step(g_5bf9, g_5bf7, mode);
}

/* objcmp: {"function":"triangles","address":"066a:4730","size":417,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bff":"DS:5bff","_g_5c01":"DS:5c01","_g_5c03":"DS:5c03","_g_5c05":"DS:5c05","_g_5c07":"DS:5c07","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void triangles(int size) {
    if (size) {
        g_5c03 = 0;
        g_6025 = (int)((long)rand() * 3L / 32768L);
        g_5c07 = (int)((long)rand() * 3L / 32768L);
        g_7488 = (int)(((long)rand() << 2) / 32768L) & g_5dc0;
    }
    if (g_5c03) {
        if (++x > g_5bff) {
            if (g_6025 && ++g_7488 >= g_5db6)
                g_7488 = 3;
            if (g_5c07)
                --g_5c01;
            if (g_5bff++ >= g_5c05) {
                g_5c03 = 0;
            } else {
                x = g_5c01;
                if (++y > g_5dae) {
                    g_5c03 = 0;
                    y = g_5dae;
                }
            }
        }
    }
    if (!g_5c03) {
        g_5c03 = 1;
        g_6025 = !g_6025;
        g_5c07 = !g_5c07;
        g_5c01 = (int)((long)rand() * (g_5dae / 2) / 32768L);
        g_5bff = g_5c01;
        x = g_5c01;
        g_5c05 = (int)((long)rand() * (g_5dae / 3) / 32768L) + g_5c01 + 8;
        if (g_5c05 > g_5dae)
            g_5c05 = g_5dae;
        y = (int)((long)rand() * g_5dae / 32768L);
    }
}

/* objcmp: {"function":"tunnel","address":"066a:48d1","size":181,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5c09":"DS:5c09","_g_5c0b":"DS:5c0b","_g_5c0d":"DS:5c0d","_g_5c0f":"DS:5c0f","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dc0":"DS:5dc0","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void tunnel(int size) {
    if (size) {
        g_5c0b = 0;
        if ((int)((long)rand() * 2L / 32768L)) {
            g_5c0f = &g_5c0d;
        } else {
            g_5c0f = &y;
        }
        g_7488 = 3 & g_5dc0;
    }
    if (g_5c0b) {
        if (++x > *g_5c0f) {
            x = g_5c09;
            ++y;
            --g_5c0b;
            if (y > g_5dae) {
                g_5c0b = 0;
                y = g_5dae;
            }
            if (++g_7488 >= g_5db6)
                g_7488 = 3;
        }
    }
    if (g_5c0b <= 0) {
        g_5c0d = g_5c0b = g_5dae;
        x = g_5c09 = y = 0;
    }
}

/* objcmp: {"function":"wandering","address":"066a:4986","size":456,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5c13":"DS:5c13","_g_5c15":"DS:5c15","_g_5c17":"DS:5c17","_g_5c19":"DS:5c19","_g_5c1b":"DS:5c1b","_g_5c1d":"DS:5c1d","_g_5c1f":"DS:5c1f","_g_5c21":"DS:5c21","_g_5c23":"DS:5c23","_g_5c25":"DS:5c25","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e"}} */
int wandering(int size) {
    if (size) {
        g_5c13 = g_5c15 = g_5c17 = g_5c19 = g_5c1b = 0;
        g_5c25 = 0;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
    if (!size && !g_5c25)
        return g_5c25 = line_step(g_5c17, g_5c19, 0);
    if (g_5c1b <= 0) {
        g_5c1d = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c1f = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c21 = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c23 = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c1b = ((int)((long)rand() * 8L / 32768L) + 1) << 2;
    }
    if (++g_7488 >= g_5db6)
        g_7488 = 3;
    g_5c13 += g_5c1d;
    if (g_5c13 < 1 || g_5c13 > g_5dae)
        g_5c1d = -g_5c1d;
    g_5c17 += g_5c1f;
    if (g_5c17 < 1 || g_5c17 > g_5dae)
        g_5c1f = -g_5c1f;
    g_5c15 += g_5c21;
    if (g_5c15 < 1 || g_5c15 > g_5dae)
        g_5c21 = -g_5c21;
    g_5c19 += g_5c23;
    if (g_5c19 < 1 || g_5c19 > g_5dae)
        g_5c23 = -g_5c23;
    --g_5c1b;
    x = g_5c13;
    y = g_5c15;
    return g_5c25 = line_step(g_5c17, g_5c19, 1);
}

/* objcmp: {"function":"zigstream","address":"066a:4b4e","size":495,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5c27":"DS:5c27","_g_5c29":"DS:5c29","_g_5c2b":"DS:5c2b","_g_5c2d":"DS:5c2d","_g_5c2f":"DS:5c2f","_g_5c31":"DS:5c31","_g_01f6":"DS:01f6","_g_01fa":"DS:01fa","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e"}} */
void zigstream(int size) {
    int mode;
    if (!size && x <= g_5dae && y <= g_5dae)
        goto step;
    if (size)
        g_6025 = (int)((long)rand() * 2L / 32768L);
    g_5c27 = g_5c29 = g_5c2d = g_5c2b = 0;
    x = (int)((long)rand() * g_5dae / 32768L);
    y = (int)((long)rand() * g_5dae / 32768L);
    if (++g_7488 >= g_5db6)
        g_7488 = 3;
step:
    if ((mode = g_6025) == 0) {
        if (g_5c27++ >= g_5c29) {
            g_5c27 = 0;
            if ((++g_5c2d & 1) == 0) {
                ++g_5c29;
                g_5c2d = 0;
            }
            g_5c2f = g_01f6[g_5c2d];
            g_5c31 = g_01fa[g_5c2d];
        }
        ++g_5c2b;
        if ((g_5c2b = g_5c2b & 7) == 0) {
            ++g_7488;
            if (g_7488 >= g_5db6)
                g_7488 = 3;
        }
    } else {
        if (--g_5c27 <= 0) {
            g_5c2f = (int)((long)rand() * 2L / 32768L) ? -1 : 1;
            g_5c31 = (int)((long)rand() * 3L / 32768L) - 1;
            g_5c27 = (int)((long)rand() * 12L / 32768L) + 12;
            if (++g_7488 >= g_5db6)
                g_7488 = 3;
        }
        if (x <= 0 || x >= g_5dae)
            g_5c2f = -g_5c2f;
        if (y <= 0 || y >= g_5dae)
            g_5c31 = -g_5c31;
    }
    x += g_5c2f;
    y += g_5c31;
}

/* objcmp: {"function":"main","address":"066a:4d3d","size":1123,"symbols":{"_g_5d78":"DS:5d78","_g_5d80":"DS:5d80","_g_5d84":"DS:5d84","_g_5da0":"DS:5da0","_g_5dbc":"DS:5dbc","_g_5dc8":"DS:5dc8","_g_5dca":"DS:5dca","_g_5dcc":"DS:5dcc","_g_5dce":"DS:5dce","_g_5dd2":"DS:5dd2","_g_5dec":"DS:5dec","_g_5d7c":"DS:5d7c","_g_5d7e":"DS:5d7e","_g_5d8a":"DS:5d8a","_g_5d8e":"DS:5d8e","_g_5d98":"DS:5d98","_g_5d9a":"DS:5d9a","_g_5d9c":"DS:5d9c","_g_fn5c37":"DS:5c37","_g_fn5c43":"DS:5c43","_g_fn5c47":"DS:5c47","_atexit":"0000:0fea","_chdir":"0000:1017","_exit":"0000:11a8","_setdisk":"0000:128e","_srand":"0000:1683","_time":"0000:1ef6","_tolower":"0000:1f66","_bioskey":"0000:2553","_textcolor":"0000:25fc","_cprintf":"0000:277a","_int86":"0000:320e","_signal":"0000:3bcb","_nosound":"0000:3db4","_gettextinfo":"0000:3dbb","_clrscr":"0000:4332","_fprintf":"0000:45d4","_getcwd":"0000:4b1f","_strchr":"0000:598f","_strncpy":"0000:5a79","_executable_path":"DS:5d17","_startup_directory":"DS:5cc7","_saved_text_info":"DS:6016","_keyboard_regs":"DS:6006","_palette_buffer":"DS:5d94","_startup_directory_error":"DS:091d","_assistance_message":"DS:0941","_parser_error":"DS:097a","_loading_box_top":"DS:098c","_loading_box_left":"DS:09a4","_loading_label":"DS:09a8","_loading_box_right":"DS:09b8","_loading_box_bottom":"DS:09be","_restore_directory_error":"DS:09d7","_startup_config_error":"DS:09eb","_restore_text_attributes":"11c1:01b5","_restore_runtime_callbacks":"11c1:01c7","_signal_diagnostic":"11c1:02d8","_initialize_program_defaults":"11c1:1f29","_save_video_state":"1899:0932","_detect_video_adapter":"1899:0959","_parse_command_line":"11c1:2eed","_print_startup_information":"11c1:339f","_keyboard_led_flags":"11c1:000a","_fatal_selfcheck_error":"11c1:052f","_check_executable_integrity":"11c1:0949","_configure_video_backend":"11c1:21a6","_resolve_startup_configuration":"11c1:2d26","_install_runtime_callbacks":"11c1:00c9","_beep":"11c1:0e07","_park_disk_heads":"1899:0efb","_prepare_logo_image_resources":"17f5:06ef","_semi_auto_loop":"11c1:3539","_auto_loop":"11c1:38c3","_set_ega_raster_xor":"11c1:0e2a","_closing_graphic_and_music":"11c1:2161","_restore_video_state":"1899:0bcd","_print_closing_text_and_exit":"11c1:0d46","__streams":"DS:43f4"}} */
void main(int argc, char far * far *argv)
{
    int config_option, insecure_character;
    char far *drive_separator;

    strncpy(executable_path, argv[0], 80);
    gettextinfo(&saved_text_info);
    atexit(restore_text_attributes);
    nosound();
    g_5d8e = 1;
    initialize_program_defaults();
    save_video_state();
    g_5d9c = 36;
    g_5d9a = 6;
    g_5d98 = 24;
    if (getcwd(startup_directory, 80) == NULL) {
        fprintf(stderr, startup_directory_error);
        fprintf(stderr, assistance_message);
        bioskey(0);
    }
    config_option = parse_command_line(argc, argv, &insecure_character);
    detect_video_adapter();
    print_startup_information();
    keyboard_led_flags(0x70, 1);
    if (g_5dc8 && tolower(argv[g_5dc8][insecure_character]) != 'i') {
        g_5d80 = 1;
        fatal_selfcheck_error(1, parser_error);
    }
    if (!g_5dce && !g_5dbc)
        check_executable_integrity();
    else
        g_5d80 = 0;
    configure_video_backend(1);
    resolve_startup_configuration(config_option);
    if (g_5dce || g_5dca || g_5dbc)
        g_5dd2 = 1;
    if (!g_5dd2) {
        keyboard_regs.h.ah = 3;
        keyboard_regs.h.al = 5;
        keyboard_regs.h.bh = 3;
        keyboard_regs.h.bl = 31;
        int86(0x16, &keyboard_regs, &keyboard_regs);
    }
    install_runtime_callbacks();
    atexit(restore_runtime_callbacks);
    signal(0x16, signal_diagnostic);
    signal(8, signal_diagnostic);
    signal(4, signal_diagnostic);
    signal(0x0b, signal_diagnostic);
    if (!g_5dce && !g_5d80 && !g_5dbc) {
        g_5d78 = 40;
        if (!g_5dca || !g_5dcc)
            beep(880, 40);
        if (!g_5dcc)
            park_disk_heads();
        if (!g_5dc4 && (!g_5dca || !g_5dcc)) {
            textcolor(14);
            cprintf(loading_box_top);
            cprintf(loading_box_left);
            textcolor(140);
            cprintf(loading_label);
            textcolor(14);
            cprintf(loading_box_right);
            cprintf(loading_box_bottom);
            textcolor(3);
        }
    }
    prepare_logo_image_resources();
    g_fn5c47(0);
    while (g_5d78)
        ;
    g_5d8a = 0;
    g_fn5c43(1);
    clrscr();
    g_fn5c37(palette_buffer + g_5dec);
    srand((unsigned)time(NULL));
    while (bioskey(1))
        bioskey(0);
    g_5d8e = g_5d7e = g_5d7c = 0;
    while (!g_5d80) {
        if (g_5da0)
            semi_auto_loop();
        else
            auto_loop();
    }
    keyboard_led_flags(0x70, 1);
    set_ega_raster_xor(0);
    if (!g_5dce && !g_5dca && !g_5d84)
        closing_graphic_and_music();
    if (!g_5dca)
        while (bioskey(1))
            bioskey(0);
    restore_video_state(1);
    if (!g_5dca && !g_5dce && startup_directory != NULL) {
        if (chdir(startup_directory)) {
            fprintf(stderr, restore_directory_error);
        } else {
            drive_separator = strchr(startup_directory, ':');
            if (drive_separator != NULL)
                setdisk(tolower(drive_separator[-1]) - 'a');
        }
    }
    if (g_5d84) {
        fprintf(stderr, startup_config_error);
        exit(14);
    }
    nosound();
    if (!g_5dce && !g_5dca && !g_5dbc)
        print_closing_text_and_exit(0);
    else
        exit(0);
}
