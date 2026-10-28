/* DAZZLE 5.0 image callbacks, source-order slice 066A:25EB..51A0. */
#include <stdlib.h>
#include <dos.h>
#include <math.h>
typedef unsigned int undefined2;
typedef unsigned long undefined4;
typedef unsigned long ulong;

extern int x, y;
extern int g_5dae, g_5dee, g_5df0, g_6025;
extern int g_5b24, g_5b26, g_5dc0, g_5dc4;
extern unsigned int g_5b66, g_5b68;
extern unsigned int g_5db6, g_7488;
extern int far *g_5c0f;
extern void (far *g_fn5c47)();
extern void (far *g_fn5c43)();
extern void (far *g_fn5c37)();
extern unsigned char g_6009;
extern unsigned char g_6008;
extern unsigned char g_6007;
extern unsigned char g_6006;
extern unsigned int g_5d9c;
extern unsigned int g_5d9a;
extern unsigned int g_5d98;
extern unsigned int g_5d96;
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
extern int g_5d94;
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
extern int g_5ba9, g_5bab, g_5bad, g_5baf, g_5bb1;
extern int g_5bcf, g_5bd1, g_5bd3, g_5bd5, g_5bd7, g_5bd9, g_5bd3;
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
extern int g_5b36, g_5b38, g_5b3a, g_5b3c, g_5b3e, g_5b40, g_5b42, g_5b44;
extern int g_5b46, g_5b48, g_5b4a, g_5b4c, g_5b4e, g_5b50, g_5b52, g_5b54, g_5b56, g_5b58;
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
extern int far fn_00010fea();
extern int far fn_00011017();
extern int far fn_000111a8();
extern int far fn_0001128e();
extern int far fn_00011683();
extern int far fn_00011ef6();
extern int far fn_00011f66();
extern int far fn_00012553();
extern int far fn_000125fc();
extern int far fn_0001277a();
extern int far fn_0001320e();
extern int far fn_00013bcb();
extern int far fn_00013db4();
extern int far fn_00013dbb();
extern int far fn_00014332();
extern int far fn_000145d4();
extern long far fn_00014b1f();
extern long far fn_0001598f();
extern int far fn_00015a79();
extern int far fn_00021c1a();
extern int far fn_00021cd9();
extern int far fn_0002213f();
extern int far fn_00022559();
extern int far fn_00022956();
extern int far fn_00022a17();
extern int far fn_00022a3a();
extern int far fn_00023b39();
extern int far fn_00023d71();
extern int far fn_00023db6();
extern int far fn_00024936();
extern int far fn_00024afd();
extern int far fn_00024faf();
extern int far fn_00025149();
extern int far fn_000254d3();
extern int far fn_0002863f();
extern int far fn_000292c2();
extern int far fn_000292e9();
extern int far fn_0002955d();
extern int far fn_0002988b();


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

/* objcmp: {"function":"ellipse","address":"066a:2750","size":1548,"level":"PROVISIONAL","blocker":"full extent object 1459/oracle 1548; first diff 066a:2760 candidate JMP 2966 vs target JMP 2970; derivative update path still differs","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b2a":"DS:5b2a","_g_5b2c":"DS:5b2c","_g_5b2e":"DS:5b2e","_g_5b30":"DS:5b30","_g_5b32":"DS:5b32","_g_5b34":"DS:5b34","_g_5b36":"DS:5b36","_g_5b38":"DS:5b38","_g_5b3a":"DS:5b3a","_g_5b3c":"DS:5b3c","_g_5b3e":"DS:5b3e","_g_5b40":"DS:5b40","_g_5b42":"DS:5b42","_g_5b44":"DS:5b44","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_choose_color":"066a:011b","_g_long5b36":"DS:5b36","_g_long5b3a":"DS:5b3a","_g_long5b3e":"DS:5b3e","_g_long5b42":"DS:5b42","_g_long5b46":"DS:5b46","_g_long5b4a":"DS:5b4a","_g_long5b4e":"DS:5b4e","_g_long5b52":"DS:5b52","_g_long5b56":"DS:5b56","_g_5b28":"DS:5b28","_g_5cbb":"DS:5cbb","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_plot_pixel":"DS:5c3f","_cycle_palette":"11c1:2515","_line_step":"066a:000e"}} */
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
        g_long5b4a = (long)g_5b42 * g_5b42;
        g_long5b4e = g_long5b4a << 1;
        g_long5b52 = (long)g_5b46 * g_5b46;
        g_long5b56 = g_long5b52 << 1;
        g_long5b36 = g_long5b4a * g_long5b46 - g_long5b52 + g_long5b4a / 4L;
        g_long5b3a = 0L;
        g_long5b3e = g_long5b4e * g_long5b46;
        g_7488 = 3 & g_5dc0;
    }
    if (g_5b2a == 0) {
        if (g_long5b3a >= g_long5b3e) {
            g_5b2a = 1;
            g_long5b36 += (long)rand() * 2L + 1L;
            g_long5b3a += (long)rand() * 2L + 1L;
        } else {
            if (g_6025)
                ++g_7488;
            else
                g_7488 = (g_5b2e + g_5b30) & g_5dc0;
            if (g_5b2c < 2) {
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
            }
            ++g_5b2e;
            g_long5b3a += g_long5b56;
            g_long5b36 += g_long5b3a;
        }
    } else {
        if (g_5b30 < 0) {
            ++g_5b2a;
        } else {
            if (!g_6025)
                g_7488 = (g_5b2e + g_5b30) & g_5dc0;
            else if (++g_7488 >= g_5db6)
                g_7488 = 3;
            if (g_5b2c < 2) {
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

/* objcmp: {"function":"hilbert","address":"066a:2fca","size":715,"level":"PROVISIONAL","blocker":"full extent object 727/oracle 715; first diff 066a:2fcd candidate SUB SP,08 vs target SUB SP,0a","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b6e":"DS:5b6e","_g_5b70":"DS:5b70","_g_5b72":"DS:5b72","_g_5b74":"DS:5b74","_g_5b76":"DS:5b76","_g_5b78":"DS:5b78","_g_5b7a":"DS:5b7a","_g_5b7c":"DS:5b7c","_g_5b7e":"DS:5b7e","_g_5b80":"DS:5b80","_g_5dae":"DS:5dae","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5cbb":"DS:5cbb","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_hilbert":"066a:2fca","_cycle_palette":"11c1:2515","_fill_square":"066a:026b","_g_plot_pixel":"DS:5c3f"}} */
void hilbert(int size) {
    int direction, step, dx, dy, initialize, color;
    if ((unsigned)g_5d7a >= (unsigned)g_5da6)
        cycle_palette();
    if (!size && !g_5b80) {
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
        for (direction = 1; direction < 4; ++direction) {
            if (direction == 1) {
                dx = g_5b78;
                dy = g_5b7a;
            } else if (direction == 2) {
                dx = g_5b7a;
                dy = g_5b78;
            } else {
                dx = -g_5b78;
                g_5b78 = -g_5b7a;
                dy = g_5b78;
                g_5b7a = -dx;
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
        return;
    }
    g_5b80 = 0;
    step = (int)((long)rand() * 2L / 32768L);
    g_5b76 = (int)((long)rand() * 9L / 32768L) + 3;
    g_5b7e = 6 >> (int)((long)rand() * 2L / 32768L);
    g_5b78 = g_5dae / (2 << (g_5b76 - 1));
    g_5b7a = 0;
    g_5b72 = g_5b74 = 0;
    g_5b6e = g_5b70 = 0;
    g_5b7c = (int)((long)rand() * 2L / 32768L);
    if (size) {
        if ((int)((long)rand() * 2L / 32768L)) {
            g_5dee = 0;
            g_5df0 = 0;
            color = (int)((long)rand() * 9L / 32768L) + 2;
        } else {
            color = (int)((long)rand() * 9L / 32768L) + 2;
        }
        for (dy = 1; dy <= g_5dae / 2; ++dy) {
            for (dx = 1; dx <= g_5dae / 2; ++dx) {
                if (!step)
                    choose_color(color);
                g_plot_pixel(1);
                ++g_5dee;
            }
            ++g_5df0;
            g_5dee -= g_5dae / 2;
            if (g_5da6 <= g_5d7a)
                cycle_palette();
        }
    }
    hilbert(0);
    g_5cbb = 32000;
    g_5b80 = 1;
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

/* objcmp: {"function":"littleStars","address":"066a:339f","size":869,"level":"PROVISIONAL","blocker":"full extent object 850/oracle 869; first diff 066a:33a2 candidate SUB SP,20 vs target SUB SP,22","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5cbb":"DS:5cbb","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_5dc0":"DS:5dc0","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5daa":"DS:5daa","_g_7484":"DS:7484","_g_plot_pixel":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_choose_color":"066a:011b","_cycle_palette":"11c1:2515"}} */
void littleStars(void) {
    int length, start, spacing, step, dx, dy, mode, row, column, swap, initial_color;
    int save_x, save_y, line_done, initialize;
    unsigned int span, sweep_x, sweep_y, point;
    length = (int)((long)rand() * 3L / 32768L) + g_5dae / 8;
    if ((int)((long)rand() * 2L / 32768L) == 0)
        start = ((int)((long)rand() * 3L / 32768L) + 1) * (length / 4);
    else
        start = 0;
    spacing = (((int)((long)rand() * 4L / 32768L) + 2) * length) / 20 & 0xfffe;
    dx = (int)((long)rand() * g_5dae / 32768L);
    dy = (int)((long)rand() * g_5dae / 32768L);
    if ((int)((long)rand() * 2L / 32768L) == 0)
        step = 0;
    else
        step = (int)((long)rand() * spacing / 32768L);
    if (g_7484 == 8)
        mode = (int)((long)rand() * 3L / 32768L);
    else
        mode = 3;
    initial_color = (int)((long)rand() * (g_5db6 - 1) / 32768L) & g_5dc0;
    g_7488 = initial_color;
    for (row = 0; row <= 1; ++row) {
        sweep_x = spacing << 1;
        sweep_y = spacing;
        g_7488 = initial_color & g_5dc0;
        if (row == 0) {
            start = 0;
            step = 0;
            length = g_5dae / 8;
        } else {
            start = spacing;
            length = spacing;
            step = 1;
        }
        do {
            g_5df0 = sweep_y;
            g_5dee = sweep_x;
            if (mode == 0 || mode == 2)
                g_7488 = g_7488 & g_5dc0;
            initialize = 1;
            do {
                line_done = line_step(start, length, initialize);
                save_y = g_5dee;
                save_x = g_5df0;
                point = (unsigned int)save_x;
                span = (unsigned int)save_y;
                for (column = 0; column <= 7; ++column) {
                    g_5df0 = dx + (int)point;
                    g_5dee = dy + (int)span;
                    if (g_5df0 < g_5dae && g_5dee < g_5dae)
                        g_plot_pixel(1);
                    span = (unsigned int)-(int)span;
                    if (column & 1)
                        point = (unsigned int)-(int)point;
                    if (column == 3) {
                        swap = (int)point;
                        point = span;
                        span = (unsigned int)swap;
                    }
                    if (mode == 2 && ++g_7488 >= g_5db6)
                        g_7488 = 3;
                }
                g_5df0 = (int)span;
                if (mode == 0 && ++g_7488 >= g_5db6)
                    g_7488 = 3;
                ++g_5cbb;
                g_5dee = (int)point;
                if (g_5da6 <= g_5d7a)
                    cycle_palette();
                initialize = 0;
            } while (!line_done);
            if (mode == 1 && ++g_7488 >= g_5db6)
                g_7488 = 3;
            if (mode == 3 && ((g_5daa = (g_5daa + 1) & 3) == 0) && ++g_7488 >= g_5db6)
                g_7488 = 3;
            start -= step;
            --length;
            sweep_y -= sweep_x & 1;
            --sweep_x;
        } while ((int)step <= (int)sweep_y);
    }
    g_5df0 = dx;
    g_5dee = dy;
}

/* objcmp: {"function":"lotsadots","address":"066a:3704","size":129,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_choose_color":"066a:011b"}} */
void lotsadots(int size) {
    if (size)
        g_6025 = (int)((long)rand() * 9L / 32768L);
    x = (int)((long)rand() * g_5dae / 32768L);
    y = (int)((long)rand() * g_5dae / 32768L);
    choose_color(g_6025);
}

/* objcmp: {"function":"pedals","address":"066a:3785","size":302,"level":"PROVISIONAL","blocker":"full extent object 307/oracle 302; first diff 066a:3805 candidate JMP 38ac vs target JMP 38a7; floating-point update sequence remains longer","absolute_symbols":{"FIDRQQ":"0x5c32"},"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b8a":"DS:5b8a","_g_5b8c":"DS:5b8c","_g_5b8e":"DS:5b8e","_g_5b90":"DS:5b90","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","F_LXRSH@":"0000:13b6","F_LXURSH@":"0000:13d6","_cos":"0000:0f6e","_sin":"0000:0e75","F_FTOL@":"0000:0fbe"}} */
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
    phase = (float)g_5b8e / 128.0f;
    x = (int)(g_5dae / 2 + (float)g_5b8a * (float)sin(phase));
    y = (int)(g_5dae / 2 + (float)g_5b8a * (float)cos(phase));
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

/* objcmp: {"function":"ribbons","address":"066a:3ac9","size":338,"level":"PROVISIONAL","blocker":"full extent object 328/oracle 338; first diff 066a:3b06 candidate JLE 3b6e vs target JG 3b0b then JMP 3ba9","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b9f":"DS:5b9f","_g_5ba1":"DS:5ba1","_g_5ba3":"DS:5ba3","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void ribbons(int size) {
    if (size) {
        g_5b9f = g_5ba1 = 0;
        g_5ba3 = 24;
        g_6025 = (int)((long)rand() * 2L / 32768L);
    }
    if (g_5b9f++ > g_5ba1)
        goto advance_ribbon;
    goto move_ribbon;
advance_ribbon:
    {
        x -= g_5ba1;
        g_5b9f = 0;
        if (++g_5ba1 > g_5ba3) {
            g_5ba1 = 0;
            g_5ba3 += (int)((long)rand() * 2L / 32768L) + 16;
            if (g_5ba3 > 44)
                g_5ba3 = 0;
        }
        if (g_6025 && ++g_7488 >= g_5db6)
            g_7488 = 3;
    }
move_ribbon:
    ++y;
    if (y > g_5dae)
        y = (int)((long)rand() * (g_5dae / 2) / 32768L);
    if (++x > g_5dae)
        x = (int)((long)rand() * g_5dae / 32768L);
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

/* objcmp: {"function":"sine","address":"066a:3d52","size":505,"level":"PROVISIONAL","blocker":"full extent object 427/oracle 505; first diff 066a:3d55 candidate SUB SP,0a vs target SUB SP,0c; target size path has FP-emulator initialization through 3e2f","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_sin":"0000:0e75","_cos":"0000:0f6e","F_FTOL@":"0000:0fbe","_g_5c3f":"DS:5c3f","_g_5c41":"DS:5c41"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
void sine(int size) {
    float center, phase;
    if (size) {
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
        x = (int)((long)rand() * (3 * g_5dae / 8) / 32768L) + g_5dae / 8;
        y = (int)((long)rand() * (3 * g_5dae / 8) / 32768L) + g_5dae / 8;
    }
    center = (float)g_5dae / 2.0f;
    phase = (float)x / center;
    x = (int)(center + center * (float)sin(phase));
    phase = (float)y / center;
    y = (int)(center + center * (float)cos(phase));
    phase = (float)x / center;
    (void)sin(phase);
    phase = (float)y / center;
    (void)cos(phase);
    if (x == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
    if (FP_SEG(g_plot_pixel) == 0x1899 && FP_OFF(g_plot_pixel) == 0x012d) {
        x &= 0xfffe;
        y &= 0xfffe;
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

/* objcmp: {"function":"squirals","address":"066a:40ad","size":778,"level":"PROVISIONAL","blocker":"full extent object 804/oracle 778; first diff 066a:40b0 candidate SUB SP,28 vs target SUB SP,32; target initializes local words at BP-10 and BP-12","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_5cbb":"DS:5cbb","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_sin":"0000:0e75","_cos":"0000:0f6e","F_FTOL@":"0000:0fbe","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_plot_pixel":"DS:5c3f","_line_step":"066a:000e","_cycle_palette":"11c1:2515","F_LXLSH@":"0000:1395"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
void squirals(int size) {
    float angle, radius_x, radius_y;
    int start_x, start_y, extent_x, extent_y, offset_x, offset_y;
    int line_done, quadrant, saved_x, saved_y;

    (void)size;
    start_x = (int)((long)rand() * (3 * g_5dae / 4) / 32768L);
    start_y = (int)((long)rand() * (3 * g_5dae / 4) / 32768L);
    radius_x = (float)((int)((long)rand() * (3 * g_5dae / 8) / 32768L) + g_5dae / 8);
    radius_y = (float)((int)((long)rand() * (3 * g_5dae / 4) / 32768L));
    offset_x = (int)((long)rand() * (3 * g_5dae / 4) / 32768L);
    offset_y = (int)((long)rand() * (3 * g_5dae / 4) / 32768L);
    quadrant = (int)((long)rand() * 2L / 32768L);
    angle = (float)offset_y * 6.28318f;

    do {
        extent_x = (int)(radius_x * (float)cos(angle) + radius_y * (float)sin(angle));
        extent_y = (int)(radius_x * (float)sin(angle) - radius_y * (float)cos(angle));
        x = extent_y;
        y = -extent_x;
        line_done = 1;
        do {
            line_done = line_step(extent_x, extent_y, line_done);
            saved_x = x;
            saved_y = y;
            switch (quadrant) {
            case 0:
                x = saved_x + offset_x;
                y = saved_y + offset_y;
                g_plot_pixel(1);
                x = saved_x - offset_x;
                y = saved_y - offset_y;
                g_plot_pixel(1);
                x = saved_x - offset_x;
                y = saved_y + offset_y;
                g_plot_pixel(1);
                x = saved_x + offset_x;
                y = saved_y - offset_y;
                g_plot_pixel(1);
                break;
            case 2:
                x = saved_x + offset_x;
                y = saved_y + offset_y;
                g_plot_pixel(1);
                x = saved_x - offset_x;
                y = saved_y - offset_y;
                g_plot_pixel(1);
                break;
            default:
                x = saved_x + offset_x;
                y = saved_y + offset_y;
                g_plot_pixel(1);
                x = saved_x - offset_x;
                y = saved_y - offset_y;
                g_plot_pixel(1);
                x = saved_x - offset_x;
                y = saved_y + offset_y;
                g_plot_pixel(1);
                x = saved_x + offset_x;
                y = saved_y - offset_y;
                g_plot_pixel(1);
                break;
            }
            x = saved_x;
            y = saved_y;
            if (g_5da6 <= g_5d7a)
                cycle_palette();
            ++g_5cbb;
        } while (!line_done);
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    } while (abs(extent_x) > 1 || abs(extent_y) > 1);
    x = start_x;
    y = start_y;
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

/* objcmp: {"function":"zigstream","address":"066a:4b4e","size":495,"level":"PROVISIONAL","blocker":"full extent object 489/oracle 495; first diff 066a:4bfa candidate JNE 4c66 vs target JNE 4c6c","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5c27":"DS:5c27","_g_5c29":"DS:5c29","_g_5c2b":"DS:5c2b","_g_5c2d":"DS:5c2d","_g_5c2f":"DS:5c2f","_g_5c31":"DS:5c31","_g_01f6":"DS:01f6","_g_01fa":"DS:01fa","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e"}} */
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
            ++g_5c2d;
            if ((g_5c2d & 1) == 0) {
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
    }
    if (x <= 0 || x >= g_5dae)
        g_5c2f = -g_5c2f;
    if (y <= 0 || y >= g_5dae)
        g_5c31 = -g_5c31;
    x += g_5c2f;
    y += g_5c31;
}

/* objcmp: {"function":"main","address":"066a:4d3d","size":1123,"level":"PROVISIONAL","blocker":"full extent object 1594/oracle 1123; first diff 066a:4d40 candidate SUB SP,0c vs target SUB SP,08; next target op MOV AX,50 while candidate saves SI","symbols":{"_fn_00010fea":"0000:0fea","_fn_00011017":"0000:1017","_fn_000111a8":"0000:11a8","_fn_0001128e":"0000:128e","_fn_00011683":"0000:1683","_fn_00011ef6":"0000:1ef6","_fn_00011f66":"0000:1f66","_fn_00012553":"0000:2553","_fn_000125fc":"0000:25fc","_fn_0001277a":"0000:277a","_fn_0001320e":"0000:320e","_fn_00013bcb":"0000:3bcb","_fn_00013db4":"0000:3db4","_fn_00013dbb":"0000:3dbb","_fn_00014332":"0000:4332","_fn_000145d4":"0000:45d4","_fn_00014b1f":"0000:4b1f","_fn_0001598f":"0000:598f","_fn_00015a79":"0000:5a79","_fn_00021c1a":"11c1:000a","_fn_00021cd9":"11c1:00c9","_fn_0002213f":"11c1:052f","_fn_00022559":"11c1:0949","_fn_00022956":"11c1:0d46","_fn_00022a17":"11c1:0e07","_fn_00022a3a":"11c1:0e2a","_fn_00023b39":"11c1:1f29","_fn_00023d71":"11c1:2161","_fn_00023db6":"11c1:21a6","_fn_00024936":"11c1:2d26","_fn_00024afd":"11c1:2eed","_fn_00024faf":"11c1:339f","_fn_00025149":"11c1:3539","_fn_000254d3":"11c1:38c3","_fn_0002863f":"17f5:06ef","_fn_000292c2":"1899:0932","_fn_000292e9":"1899:0959","_fn_0002955d":"1899:0bcd","_fn_0002988b":"1899:0efb","_g_5d78":"DS:5d78","_g_5d80":"DS:5d80","_g_5d84":"DS:5d84","_g_5d94":"DS:5d94","_g_5da0":"DS:5da0","_g_5dbc":"DS:5dbc","_g_5dc8":"DS:5dc8","_g_5dca":"DS:5dca","_g_5dcc":"DS:5dcc","_g_5dce":"DS:5dce","_g_5dd2":"DS:5dd2","_g_5dec":"DS:5dec","_g_5d7c":"DS:5d7c","_g_5d7e":"DS:5d7e","_g_5d8a":"DS:5d8a","_g_5d8e":"DS:5d8e","_g_5d96":"DS:5d96","_g_5d98":"DS:5d98","_g_5d9a":"DS:5d9a","_g_5d9c":"DS:5d9c","_g_6006":"DS:6006","_g_6007":"DS:6007","_g_6008":"DS:6008","_g_6009":"DS:6009","_g_fn5c37":"DS:5c37","_g_fn5c43":"DS:5c43","_g_fn5c47":"DS:5c47"}} */
void main(undefined2 param_1, undefined2 param_2, undefined2 far *param_3)
{
  int iVar1;
  undefined2 far *puVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  long lVar5;
  int iStack_6;
  undefined2 uStack_4; uVar3 = (undefined2)((ulong)param_3 >> 0x10);
  puVar2 = (undefined2 far *)param_3;
  fn_00015a79(0x166a,0x5d17,0x39bf,*param_3,puVar2[1],0x50);
  fn_00013dbb(0x1000,0x6016,0x39bf);
  fn_00010fea(0x1000,0x1b5,0x21c1);
  fn_00013db4(0x1000);
  g_5d8e = 1;
  fn_00023b39(0x1000);
  fn_000292c2(0x21c1);
  g_5d9c = 0x24;
  g_5d9a = 6;
  g_5d98 = 0x18;
  lVar5 = fn_00014b1f(0x2899,0x5cc7,0x39bf,0x50);
  if (lVar5 == 0) {
    fn_000145d4(0x1000,0x441c,0x39bf,0x91d,0x39bf);
    fn_000145d4(0x1000,0x441c,0x39bf,0x941,0x39bf);
    fn_00012553(0x1000,0);
  }
  uStack_4 = fn_00024afd(0x1000,param_2,puVar2,uVar3,&iStack_6);
  fn_000292e9(0x21c1);
  fn_00024faf(0x2899);
  uVar4 = 0x21c1;
  fn_00021c1a(0x21c1,0x70,1);
  if (g_5dc8 != 0) {
    uVar4 = 0x1000;
    iVar1 = fn_00011f66(0x21c1,(int)*(char *)((int)*(undefined4 *)(puVar2 + g_5dc8 * 2) +
                                                 iStack_6));
    if (iVar1 != 0x69) {
      g_5d80 = 1;
      uVar4 = 0x21c1;
      fn_0002213f(0x1000,1,0x97a,0x39bf);
    }
  }
  if ((g_5dce == 0) && (g_5dbc == 0)) {
    fn_00022559(uVar4);
    uVar4 = 0x21c1;
  }
  else {
    g_5d80 = 0;
  }
  fn_00023db6(uVar4,1);
  uVar3 = 0x21c1;
  fn_00024936(0x21c1,uStack_4);
  if (((g_5dce != 0) || (g_5dca != 0)) || (g_5dbc != 0)) {
    g_5dd2 = 1;
  }
  if (g_5dd2 == 0) {
    g_6007 = 3;
    g_6006 = 5;
    g_6009 = 3;
    g_6008 = 0x1f;
    uVar3 = 0x1000;
    fn_0001320e(0x21c1,0x16,0x6006,0x39bf,0x6006,0x39bf);
  }
  fn_00021cd9(uVar3);
  fn_00010fea(0x21c1,0x1c7,0x21c1);
  fn_00013bcb(0x1000,0x16,0x2d8,0x21c1);
  fn_00013bcb(0x1000,8,0x2d8,0x21c1);
  fn_00013bcb(0x1000,4,0x2d8,0x21c1);
  uVar3 = 0x1000;
  fn_00013bcb(0x1000,0xb,0x2d8,0x21c1);
  if (((g_5dce == 0) && (g_5d80 == 0)) && (g_5dbc == 0)) {
    g_5d78 = 0x28;
    if ((g_5dca == 0) || (uVar4 = uVar3, g_5dcc == 0)) {
      uVar4 = 0x21c1;
      fn_00022a17(0x1000,0x370,0x28);
    }
    uVar3 = uVar4;
    if (g_5dcc == 0) {
      uVar3 = 0x2899;
      fn_0002988b(uVar4);
    }
    if ((g_5dc4 == 0) && ((g_5dca == 0 || (g_5dcc == 0)))) {
      fn_000125fc(uVar3,0xe);
      fn_0001277a(0x1000,0x98c,0x39bf);
      fn_0001277a(0x1000,0x9a4,0x39bf);
      fn_000125fc(0x1000,0x8c);
      fn_0001277a(0x1000,0x9a8,0x39bf);
      fn_000125fc(0x1000,0xe);
      fn_0001277a(0x1000,0x9b8,0x39bf);
      fn_0001277a(0x1000,0x9be,0x39bf);
      uVar3 = 0x1000;
      fn_000125fc(0x1000,3);
    }
  }
  fn_0002863f(uVar3);
  (*g_fn5c47)(0x27f5,0);
  do {
  } while (g_5d78 != 0);
  g_5d8a = 0;
  (*g_fn5c43)(0x27f5,1);
  fn_00014332(0x27f5);
  (*g_fn5c37)(0x1000,g_5d94 + g_5dec,g_5d96);
  uVar3 = fn_00011ef6(0x1000,0,0);
  fn_00011683(0x1000,uVar3);
  for (;;) {
    iVar1 = fn_00012553(0x1000,1);
    if (iVar1 == 0) break;
    fn_00012553(0x1000,0);
  }
  g_5d7c = 0;
  g_5d7e = 0;
  g_5d8e = 0;
  uVar3 = 0x1000;
  while (g_5d80 == 0) {
    if (g_5da0 == 0) {
      fn_000254d3(uVar3);
      uVar3 = 0x21c1;
    }
    else {
      fn_00025149(uVar3);
      uVar3 = 0x21c1;
    }
  }
  fn_00021c1a(uVar3,0x70,1);
  fn_00022a3a(0x21c1,0);
  if (((g_5dce == 0) && (g_5dca == 0)) && (g_5d84 == 0)) {
    fn_00023d71(0x21c1);
  }
  uVar4 = 0x21c1;
  uVar3 = uVar4;
  if (g_5dca == 0) {
    for (;;) {
      uVar3 = 0x1000;
      iVar1 = fn_00012553(uVar4,1);
      if (iVar1 == 0) break;
      uVar4 = 0x1000;
      fn_00012553(0x1000,0);
    }
  }
  uVar4 = 0x2899;
  fn_0002955d(uVar3,1);
  if ((g_5dca == 0) && (g_5dce == 0)) {
    iVar1 = fn_00011017(0x2899,0x5cc7,0x39bf);
    if (iVar1 == 0) {
      uVar4 = 0x1000;
      lVar5 = fn_0001598f(0x1000,0x5cc7,0x39bf,0x3a);
      if (lVar5 != 0) {
        iVar1 = fn_00011f66(0x1000,(int)*(char *)((int)lVar5 + -1));
        uVar4 = 0x1000;
        fn_0001128e(0x1000,iVar1 + -0x61);
      }
    }
    else {
      uVar4 = 0x1000;
      fn_000145d4(0x1000,0x441c,0x39bf,0x9d7,0x39bf);
    }
  }
  if (g_5d84 != 0) {
    fn_000145d4(uVar4,0x441c,0x39bf,0x9eb,0x39bf);
    uVar4 = 0x1000;
    fn_000111a8(0x1000,0xe);
  }
  fn_00013db4(uVar4);
  if (((g_5dce == 0) && (g_5dca == 0)) && (g_5dbc == 0)) {
    fn_00022956(0x1000,0);
  }
  else {
    fn_000111a8(0x1000,0);
  }
  return;

}
/* main is C code, but its setup, option parsing, video selection and
 * display/input loop are not reconstructed in this worker candidate. */
