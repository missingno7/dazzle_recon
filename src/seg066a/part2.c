/* DAZZLE 5.0 image callbacks, source-order slice 066A:25EB..51A0. */
#include <stdlib.h>
#include <dos.h>
#include <math.h>

extern int x, y;
extern int g_5dae, g_5dee, g_5df0, g_6025;
extern int g_5b24, g_5b26, g_5dc0, g_5dc4;
extern unsigned int g_5db6, g_7488;
extern int far *g_5c0f;
extern int g_5b5a, g_5b5c, g_5b5e;
extern int g_5b6a, g_5b6c;
extern int g_5bf3, g_5bf5, g_5bf7, g_5bf9, g_5bfb, g_5bfd;
extern int g_5c09, g_5c0b, g_5c0d;
extern int g_5bdb, g_5bdd, g_5bdf, g_5be1, g_5be3, g_5be5;
extern int g_00a6[4], g_00ae[4];
extern int g_01e6[4], g_01ee[4];
typedef struct { int x, y; } SierpinskiVertex;
extern SierpinskiVertex g_5ba5[3];
extern int g_5ba9, g_5bab, g_5bad, g_5baf, g_5bb1;
extern int g_5bcf, g_5bd1, g_5bd3, g_5bd5, g_5bd7, g_5bd9, g_5bd3;
extern int g_5be7, g_5be9, g_5beb, g_5bed, g_5bef, g_5bf1;
extern int g_5b82, g_5b84, g_5b86, g_5b88;
extern int g_5bff, g_5c01, g_5c03, g_5c05, g_5c07;
extern int g_5c13, g_5c15, g_5c17, g_5c19, g_5c1b, g_5c1d, g_5c1f, g_5c21, g_5c23, g_5c25;
extern int g_5c27, g_5c29, g_5c2b, g_5c2d, g_5c2f, g_5c31;
extern int g_5b60, g_5b62, g_5b64, g_5b66, g_5b68;
extern int g_5c3f, g_5c41;
extern unsigned char g_00e6[64];
extern int g_5b92, g_5b94, g_5b96, g_5b98, g_5b9a, g_5b9c, g_5b9e;
extern int g_5b9f, g_5ba1, g_5ba3;
extern int g_5b2a, g_5b2c, g_5b2e, g_5b30, g_5b32, g_5b34;
extern int g_5b36, g_5b38, g_5b3a, g_5b3c, g_5b3e, g_5b40, g_5b42, g_5b44;
extern int g_5b46, g_5b48, g_5b4a, g_5b4c, g_5b4e, g_5b50, g_5b52, g_5b54, g_5b56, g_5b58;
extern int g_5b6e, g_5b70, g_5b72, g_5b74, g_5b76, g_5b78, g_5b7a, g_5b7c, g_5b7e, g_5b80;
extern int g_5b8a, g_5b8c, g_5b8e, g_5b90, g_5cbb;
extern void choose_color(int kind);
extern int line_step(int endx, int endy, int initialize);
extern int rand(void);

/* objcmp: {"function":"diag1","address":"066a:25eb","size":172,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_choose_color":"066a:011b","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"conditional branch now aligns; random expression differs at +0x2b"} */
void diag1(int size) {
    --y;
    if (size || x++ > g_5dae - 1) {
        x = 0;
        ++y;
        if (size) {
            g_6025 = (int)((long)rand() * 9L / 32768L);
            if (g_6025 == 1)
                --g_6025;
            if ((long)rand() * 2L / 32768L != 0)
                g_6025 = 0;
            if (g_6025 == 0)
                choose_color(0);
        }
    }
    if (y < 0)
        y = g_5dae;
    if (g_6025 != 0)
        choose_color(g_6025);
}

/* objcmp: {"function":"diag2","address":"066a:2697","size":185,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b24":"DS:5b24","_g_5b26":"DS:5b26","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_g_7488":"DS:7488","_choose_color":"066a:011b","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"cross-worker near-call to choose_color at 066a:273a"} */
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

/* objcmp: {"function":"ellipse","address":"066a:2750","size":1548,"level":"PROVISIONAL","blocker":"ellipse-rasterizer-and-pointer-callback-not-converged","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b2a":"DS:5b2a","_g_5b2c":"DS:5b2c","_g_5b2e":"DS:5b2e","_g_5b30":"DS:5b30","_g_5b32":"DS:5b32","_g_5b34":"DS:5b34","_g_5b36":"DS:5b36","_g_5b38":"DS:5b38","_g_5b3a":"DS:5b3a","_g_5b3c":"DS:5b3c","_g_5b3e":"DS:5b3e","_g_5b40":"DS:5b40","_g_5b42":"DS:5b42","_g_5b44":"DS:5b44","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_choose_color":"066a:011b"}} */
void ellipse(int size) {
    int dx, dy;
    if (size || g_5b2a >= 2) {
        g_5b2a = 0;
        g_6025 = (int)((long)rand() * 2L / 32768L);
        g_5b2c = (int)((long)rand() * 3L / 32768L);
        g_5b2e = 0;
        if (g_5b2c == 0) {
            g_5b32 = g_5b34 = 0;
        } else {
            g_5b32 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 4;
            g_5b34 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 4;
        }
        g_5b42 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 2;
        g_5b44 = (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 2;
        g_5b36 = g_5b38 = 0;
    }
    dx = g_5b42 - g_5b32;
    dy = g_5b44 - g_5b34;
    if (g_5b2e++ < dx) {
        x = g_5b32 + g_5b2e;
        y = g_5b34 + (int)((long)g_5b2e * dy / (dx ? dx : 1));
        choose_color(g_6025);
        if (g_5b2e >= dx)
            ++g_5b2a;
    } else {
        g_5b2e = 0;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
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

/* objcmp: {"function":"grid","address":"066a:2e1a","size":313,"level":"PROVISIONAL","blocker":"bitmap-loop-shape-unverified","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b60":"DS:5b60","_g_5b62":"DS:5b62","_g_5b64":"DS:5b64","_g_5b66":"DS:5b66","_g_5b68":"DS:5b68","_g_00e6":"DS:00e6","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_5c3f":"DS:5c3f","_g_5c41":"DS:5c41","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void grid(int size) {
    if (size) {
        g_7488 = (int)((long)rand() * 2L / 32768L) & g_5dc0;
        g_5b68 = g_5b66 = g_5b64 = 0;
        g_6025 = (int)((long)rand() * 3L / 32768L);
    }
    do {
        if ((g_5b64 & 0x3f) == 0) {
            if (g_6025 == 0 && ++g_7488 >= g_5db6)
                g_7488 = 3;
            g_5b62 = g_5b66;
            x = g_5b66 - 1;
            y = g_5b68;
            if (g_5c41 == 0x1899 && g_5c3f == 0x012d)
                y <<= 1;
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
            if (g_5c41 == 0x1899 && g_5c3f == 0x012d)
                ++y;
        }
    } while (g_00e6[g_5b64++] != 0);
}

/* objcmp: {"function":"hexagon","address":"066a:2f53","size":119,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b6a":"DS:5b6a","_g_5b6c":"DS:5b6c","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488","_choose_color":"066a:011b"},"level":"PROVISIONAL","blocker":"cross-worker near-call to choose_color at 066a:2fb1"} */
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

/* objcmp: {"function":"hilbert","address":"066a:2fca","size":715,"level":"PROVISIONAL","blocker":"recursive-space-filling-curve-control-flow","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b6e":"DS:5b6e","_g_5b70":"DS:5b70","_g_5b72":"DS:5b72","_g_5b74":"DS:5b74","_g_5b76":"DS:5b76","_g_5b78":"DS:5b78","_g_5b7a":"DS:5b7a","_g_5b7c":"DS:5b7c","_g_5b7e":"DS:5b7e","_g_5b80":"DS:5b80","_g_5dae":"DS:5dae","_g_5da6":"DS:5da6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_line_step":"066a:000e","_hilbert":"066a:2fca"}} */
void hilbert(int size) {
    int direction;
    if (size) {
        g_5b80 = 0;
        g_5b76 = (int)((long)rand() * 3L / 32768L) + 3;
        g_5b7e = 6 >> (int)((long)rand() * 2L / 32768L);
        g_5b78 = (g_5dae / (2 << (g_5b76 - 1)));
        g_5b7a = 0;
        g_5b72 = g_5b74 = 0;
        g_5b6e = g_5b70 = 0;
        g_5b7c = (int)((long)rand() * 2L / 32768L);
    }
    if (g_5b7c)
        ++g_7488;
    direction = g_5b80++ & 3;
    switch (direction) {
    case 0: x += g_5b78; break;
    case 1: y += g_5b78; break;
    case 2: x -= g_5b78; break;
    default: y -= g_5b78; break;
    }
    if (g_5b80 < g_5b76 * 4) {
        hilbert(0);
    } else {
        g_5b80 = 0;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
    line_step(x + g_5b78, y, 1);
}

/* objcmp: {"function":"lines","address":"066a:3295","size":266,"level":"PROVISIONAL","blocker":"branch-shape-unverified","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b82":"DS:5b82","_g_5b84":"DS:5b84","_g_5b86":"DS:5b86","_g_5b88":"DS:5b88","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void lines(int size) {
    if (size) {
        g_5b82 = -1;
        g_5b88 = 0;
        g_5b86 = g_5b84 = 1;
        g_7488 = (int)((long)rand() * 4L / 32768L) & g_5dc0;
    }
    if ((g_5b82 = (g_5b82 + 1) & 3) == 0) {
        x += g_5b84;
        g_5b88 += g_5b86;
        if (x < 0) {
            g_5b84 = 1;
            x = 0;
            if (++g_7488 >= g_5db6)
                g_7488 = 3;
        }
        if (g_5b88 < 0) {
            g_5b86 = (int)((long)rand() * 2L / 32768L) + 1;
            g_5b88 = 0;
        }
        if (x > g_5dae) {
            g_5b84 = -1;
            x = g_5dae;
        }
        if (g_5b88 > g_5dae) {
            g_5b86 = -((int)((long)rand() * 2L / 32768L) + 1);
            g_5b88 = g_5dae;
        }
        y = g_5b88;
    } else {
        ++y;
    }
}

/* objcmp: {"function":"littleStars","address":"066a:339f","size":869,"level":"PROVISIONAL","blocker":"local-star-outline-and-recursion-not-converged","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5cbb":"DS:5cbb","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e","_choose_color":"066a:011b"}} */
void littleStars(int size) {
    int cx, cy, radius, i, ox, oy;
    cx = (int)((long)rand() * g_5dae / 32768L);
    cy = (int)((long)rand() * g_5dae / 32768L);
    radius = (int)((long)rand() * (3L * g_5dae / 8L) / 32768L) + g_5dae / 8;
    if (size)
        g_5cbb = 0;
    for (i = 0; i < 5; ++i) {
        switch (i) {
        case 0: ox = radius; oy = 0; break;
        case 1: ox = radius / 3; oy = radius; break;
        case 2: ox = -radius; oy = radius / 2; break;
        case 3: ox = -radius / 2; oy = -radius; break;
        default: ox = radius / 2; oy = -radius / 2; break;
        }
        x = cx;
        y = cy;
        line_step(cx + ox, cy + oy, 1);
        choose_color(0);
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
    ++g_5cbb;
}

/* objcmp: {"function":"lotsadots","address":"066a:3704","size":129,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_choose_color":"066a:011b"},"level":"PROVISIONAL","blocker":"cross-worker near-call to choose_color at 066a:377e"} */
void lotsadots(int size) {
    if (size)
        g_6025 = (int)((long)rand() * 9L / 32768L);
    x = (int)((long)rand() * g_5dae / 32768L);
    y = (int)((long)rand() * g_5dae / 32768L);
    choose_color(g_6025);
}

/* objcmp: {"function":"pedals","address":"066a:3785","size":302,"level":"PROVISIONAL","blocker":"floating-point-emulator-expression-and-loop","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b8a":"DS:5b8a","_g_5b8c":"DS:5b8c","_g_5b8e":"DS:5b8e","_g_5b90":"DS:5b90","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","F_LXRSH@":"0000:13b6","F_LXURSH@":"0000:13d6"}} */
void pedals(int size) {
    float phase, radius;
    if (size || g_5b8c <= 0) {
        g_5b90 = -1;
        g_5b8e = 0;
        g_5b8a = g_5b8c = 3 * (g_5dae / 4);
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
    if ((g_5b90 = (g_5b90 + 1) & 7) == 0)
        g_5b8e += 31;
    phase = (float)g_5b8e / 128.0f;
    radius = (float)g_5b8a;
    x = (int)(g_5dae / 2 + radius * (float)cos(phase));
    y = (int)(g_5dae / 2 + radius * (float)sin(phase));
    if (x < 0 || x > g_5dae || y < 0 || y > g_5dae) {
        g_5b8e = 0;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
}

/* objcmp: {"function":"puffs","address":"066a:38b3","size":534,"level":"PROVISIONAL","blocker":"far-helper-call-placement-and-state-flow","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b92":"DS:5b92","_g_5b94":"DS:5b94","_g_5b96":"DS:5b96","_g_5b98":"DS:5b98","_g_5b9a":"DS:5b9a","_g_5b9c":"DS:5b9c","_g_5b9e":"DS:5b9e","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e","_choose_color":"066a:011b"}} */
void puffs(int size) {
    if (!size && abs(x) < g_5dae && abs(y) < g_5dae)
        goto draw;
    g_5b92 = x = (int)((long)rand() * g_5dae / 32768L);
    g_5b94 = y = (int)((long)rand() * g_5dae / 32768L);
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
            x += (int)((long)rand() * (g_5b9a * 2L) / 32768L) - g_5b9a;
            y += (int)((long)rand() * (g_5b9a * 2L) / 32768L) - g_5b9a;
        } else {
            x += g_5b9e;
            y += g_5b9e;
            g_5b96 = x + g_5b9a;
            g_5b98 = y + g_5b9a;
        }
    }
}

/* objcmp: {"function":"ribbons","address":"066a:3ac9","size":338,"level":"PROVISIONAL","blocker":"arithmetic-and-branch-shape","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5b9f":"DS:5b9f","_g_5ba1":"DS:5ba1","_g_5ba3":"DS:5ba3","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void ribbons(int size) {
    if (size) {
        g_5ba1 = g_5b9f = 0;
        g_5ba3 = 24;
        g_6025 = (int)((long)rand() * 2L / 32768L);
    }
    if (g_5b9f++ > g_5ba1) {
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
    ++y;
    if (y > g_5dae)
        y = (int)((long)rand() * (g_5dae / 2) / 32768L);
    if (++x > g_5dae)
        x = (int)((long)rand() * g_5dae / 32768L);
    if ((int)((long)rand() * 1000L / 32768L) == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
}

/* objcmp: {"function":"sierpinski","address":"066a:3c1b","size":311,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5ba5":"DS:5ba5","_g_5bb1":"DS:5bb1","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"vertex-table overlay and initialization order"} */
void sierpinski(int size) {
    int choice;
    if (size) {
        g_5ba5[2].y = g_5ba5[1].y = g_5ba5[1].x =
            (int)((long)rand() * (g_5dae / 2) / 32768L) + g_5dae / 3;
        x = y = g_5ba5[2].y;
        if ((int)((long)rand() * 2L / 32768L))
            g_5ba5[2].x = g_5ba5[0].y = g_5ba5[0].x = g_5ba5[2].y / 2;
        else
            g_5ba5[2].x = g_5ba5[0].y = g_5ba5[0].x = 0;
        g_5bb1 = (int)((long)rand() * 2L / 32768L);
        g_7488 = (int)((long)rand() * 3L / 32768L) & g_5dc0;
    }
    choice = (int)((long)rand() * 3L / 32768L);
    if (choice == 0 && g_5bb1 == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
    x += (g_5ba5[choice].x - x) / 2;
    y += (g_5ba5[choice].y - y) / 2;
}

/* objcmp: {"function":"sine","address":"066a:3d52","size":505,"level":"PROVISIONAL","blocker":"floating-point-callbacks-and-emulator-sequence","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_sin":"066a:72dc","_cos":"066a:78e4"}} */
void sine(int size) {
    float phase, step, center;
    if (!size)
        return;
    phase = (float)((long)rand() * 2L / 32768L) * 3.14159f;
    step = (float)(g_5dae / 8) / 32.0f;
    center = (float)g_5dae / 2.0f;
    x = (int)center;
    y = (int)(center + center * (float)sin(phase));
    phase += step;
    if (phase > 6.28318f) {
        phase = 0.0f;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
}

/* objcmp: {"function":"squares","address":"066a:3f4b","size":354,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bcf":"DS:5bcf","_g_5bd1":"DS:5bd1","_g_5bd3":"DS:5bd3","_g_5bd5":"DS:5bd5","_g_5bd7":"DS:5bd7","_g_5bd9":"DS:5bd9","_g_5dae":"DS:5dae","_g_5dc4":"DS:5dc4","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_g_00a6":"DS:00a6","_g_00ae":"DS:00ae","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"signed threshold branch differs at +0x2a"} */
void squares(int size) {
    if (size || g_5bd1 >= g_5bd9 || x > g_5dae || y > g_5dae) {
        g_5bd3 = g_5bd1 = g_5bcf = g_5bd7 = g_5bd5 = 0;
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

/* objcmp: {"function":"squirals","address":"066a:40ad","size":778,"level":"PROVISIONAL","blocker":"floating-point-switch-and-trailing-table","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_5cbb":"DS:5cbb","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_sin":"066a:72dc","_cos":"066a:78e4"}} */
void squirals(int size) {
    float a, b, radius;
    int branch;
    if (size)
        g_5cbb = 0;
    branch = (g_5cbb++ & 3);
    radius = (float)(int)((long)rand() * (3L * g_5dae / 8L) / 32768L) + 1.0f;
    a = (float)g_5cbb / 16.0f;
    switch (branch) {
    case 0: x = (int)(radius * (float)cos(a)); y = (int)(radius * (float)sin(a)); break;
    case 1: x = (int)(radius * (float)sin(a)); y = (int)(radius * (float)cos(a)); break;
    case 2: x = -(int)(radius * (float)cos(a)); y = (int)(radius * (float)sin(a)); break;
    default: x = (int)(radius * (float)cos(a)); y = -(int)(radius * (float)sin(a)); break;
    }
    b = a;
    (void)b;
    if (++g_7488 >= g_5db6)
        g_7488 = 3;
}

/* objcmp: {"function":"spiral","address":"066a:43b7","size":235,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bdb":"DS:5bdb","_g_5bdd":"DS:5bdd","_g_5bdf":"DS:5bdf","_g_5be1":"DS:5be1","_g_5be3":"DS:5be3","_g_5be5":"DS:5be5","_g_00a6":"DS:00a6","_g_00ae":"DS:00ae","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"counter mask/register allocation differs at +0x7d"} */
void spiral(int size) {
    int value;
    if (size || x > g_5dae || y > g_5dae) {
        g_5bdb = g_5bdd = g_5be1 = g_5bdf = 0;
        x = (int)((long)rand() * g_5dae / 32768L);
        y = (int)((long)rand() * g_5dae / 32768L);
    }
    if (g_5bdb++ >= g_5bdd) {
        g_5bdb = 0;
        ++g_5be1;
        value = g_5be1;
        value &= 3;
        g_5be1 = value;
        if ((g_5be1 & 1) == 0)
            ++g_5bdd;
    g_5be3 = g_00a6[g_5be1];
    g_5be5 = g_00ae[g_5be1];
    }
    x += g_5be3;
    y += g_5be5;
    ++g_5bdf;
    g_5bdf &= 7;
    if (g_5bdf == 0 && ++g_7488 >= g_5db6)
        g_7488 = 3;
}

/* objcmp: {"function":"spiral1","address":"066a:44a2","size":345,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5be7":"DS:5be7","_g_5be9":"DS:5be9","_g_5beb":"DS:5beb","_g_5bed":"DS:5bed","_g_5bef":"DS:5bef","_g_5bf1":"DS:5bf1","_g_5dae":"DS:5dae","_g_5dc4":"DS:5dc4","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"reset threshold and random range differ at +0x25"} */
void spiral1(int size) {
    if (size || g_5be9 >= g_5bf1 || x > g_5dae || y > g_5dae) {
        g_5beb = g_5be9 = g_5be7 = g_5bef = g_5bed = 0;
        x = (int)((long)rand() * g_5dae / 32768L);
        y = (int)((long)rand() * g_5dae / 32768L);
        g_6025 = (int)((long)rand() * 2L / 32768L);
        if (g_5dc4)
            g_6025 = 1;
        g_5bf1 = (int)((long)rand() * (3L * (g_5dae / 4)) / 32768L);
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

/* objcmp: {"function":"sweep2","address":"066a:4666","size":202,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bf7":"DS:5bf7","_g_5bf9":"DS:5bf9","_g_5bfb":"DS:5bfb","_g_5bfd":"DS:5bfd","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_5dc4":"DS:5dc4","_g_5dee":"DS:5dee","_g_5df0":"DS:5df0","_g_6025":"DS:6025","_g_7488":"DS:7488","_line_step":"066a:000e","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"level":"PROVISIONAL","blocker":"branch target and cross-worker near-call to line_step"} */
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
draw:
    g_5bfd = line_step(g_5bf9, g_5bf7, 1);
}

/* objcmp: {"function":"triangles","address":"066a:4730","size":417,"level":"PROVISIONAL","blocker":"multi-stage-state-machine-not-fully-matched","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5bff":"DS:5bff","_g_5c01":"DS:5c01","_g_5c03":"DS:5c03","_g_5c05":"DS:5c05","_g_5c07":"DS:5c07","_g_5dae":"DS:5dae","_g_5dc0":"DS:5dc0","_g_5db6":"DS:5db6","_g_6025":"DS:6025","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void triangles(int size) {
    if (size) {
        g_5c03 = 0;
        g_6025 = (int)((long)rand() * 3L / 32768L);
        g_5c07 = (int)((long)rand() * 3L / 32768L);
        g_7488 = (int)((long)rand() * 2L / 32768L) & g_5dc0;
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
        g_6025 = -g_6025;
        g_5c07 = -g_5c07;
        g_5c01 = (int)((long)rand() * (g_5dae / 2) / 32768L);
        g_5bff = g_5c01;
        x = g_5c01;
        g_5c05 = g_5c01 + (int)((long)rand() * (g_5dae / 3) / 32768L) + 8;
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

/* objcmp: {"function":"wandering","address":"066a:4986","size":456,"level":"PROVISIONAL","blocker":"multi-stage-state-machine-not-fully-matched","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5c13":"DS:5c13","_g_5c15":"DS:5c15","_g_5c17":"DS:5c17","_g_5c19":"DS:5c19","_g_5c1b":"DS:5c1b","_g_5c1d":"DS:5c1d","_g_5c1f":"DS:5c1f","_g_5c21":"DS:5c21","_g_5c23":"DS:5c23","_g_5c25":"DS:5c25","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_g_6025":"DS:6025","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e"}} */
void wandering(int size) {
    if (size) {
        g_5c13 = g_5c15 = g_5c17 = g_5c19 = g_5c1b = 0;
        g_5c25 = 0;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
    if (!size && !g_5c25)
        return;
    if (g_5c1b <= 0) {
        g_5c1d = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c1f = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c21 = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c23 = (int)((long)rand() * 9L / 32768L) - 4;
        g_5c1b = ((int)((long)rand() * 8L / 32768L) + 1) << 2;
        if (++g_7488 >= g_5db6)
            g_7488 = 3;
    }
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
    g_5c25 = line_step(g_5c17, g_5c19, 1);
}

/* objcmp: {"function":"zigstream","address":"066a:4b4e","size":495,"level":"PROVISIONAL","blocker":"state-machine-and-cross-worker-helper","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_g_5c27":"DS:5c27","_g_5c29":"DS:5c29","_g_5c2b":"DS:5c2b","_g_5c2d":"DS:5c2d","_g_5c2f":"DS:5c2f","_g_5c31":"DS:5c31","_g_6025":"DS:6025","_g_5dae":"DS:5dae","_g_5db6":"DS:5db6","_g_7488":"DS:7488","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_line_step":"066a:000e"}} */
void zigstream(int size) {
    if (!size && x <= g_5dae && y <= g_5dae)
        goto step;
    if (size)
        g_6025 = (int)((long)rand() * 2L / 32768L);
    g_5c2b = g_5c2d = g_5c29 = g_5c27 = 0;
    x = (int)((long)rand() * g_5dae / 32768L);
    y = (int)((long)rand() * g_5dae / 32768L);
    if (++g_7488 >= g_5db6)
        g_7488 = 3;
step:
    if (!g_6025) {
        if (g_5c27++ >= g_5c29) {
            g_5c27 = 0;
            ++g_5c2d;
            if ((g_5c2d & 1) == 0) {
                ++g_5c29;
                g_5c2d = 0;
                g_5c2f = g_5c2d;
                g_5c31 = g_5c2d;
            }
        }
        g_5c2b = (g_5c2b + 1) & 7;
        if (!g_5c2b && ++g_7488 >= g_5db6)
            g_7488 = 3;
    }
    --g_5c27;
    if (g_5c27 <= 0) {
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
    x += g_5c2f;
    y += g_5c31;
}

/* objcmp: {"function":"main","address":"066a:4d3d","size":1123,"level":"PROVISIONAL","blocker":"application-startup-and-menu-loop-not-reconstructed","symbols":{}} */
/* main is C code, but its setup, option parsing, video selection and
 * display/input loop are not reconstructed in this worker candidate. */
