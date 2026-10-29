#include <stdlib.h>
#include <dos.h>
#include <dos.h>

extern int draw_x;
extern int draw_y;
extern int screen_half_width;
extern int screen_half_height;
extern unsigned color_count;
extern unsigned plot_color;
extern unsigned video_adapter;
extern unsigned palette_elapsed_ticks;
extern unsigned palette_cycle_ticks;
extern unsigned draw_symmetry;
extern void (far *plot_callback)(unsigned);
extern void far ega_plot_symmetric_640x700(unsigned);
extern void draw_polygon(int, int, int far *, int far *, unsigned, unsigned, unsigned);
extern void draw_ellipse(int, int, int, int, int, unsigned, unsigned, unsigned);
extern void cycle_palette(void);

extern int g_6036;
extern int g_6038;
extern int g_603a;
extern int g_603c;
extern int g_604e;
extern int g_6050;
extern int g_6052;
extern int g_6054;

extern float g_0a4c;
extern int g_5cbb;
extern int g_0a38[4];
extern int g_0a40[4];
extern int g_0a48[2];

#define EGA_PLOT_CALLBACK (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) && FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700))

extern float g_60f4;
extern float g_60f0;
extern float g_60ec;
extern int g_60ea;
extern int g_60e8;
extern int g_60e6;
extern int g_60e4;
extern int g_60e2;
extern int g_60e0;
extern int g_60de;
extern int g_60dc;
extern int g_60da;
extern int g_60d8;
extern int g_60d6;
extern int g_60d4;
extern int g_60ce[3];
extern int g_60c8[3];
extern int g_60c2[3];
extern int g_60bc[3];
extern int g_60ba;
extern int g_60b8;
extern int g_60b6;
extern int g_60b4;
extern int g_60b2;
extern int g_60b0;
extern int g_60ae;
extern int g_60ac;
extern int g_60aa;
extern int g_60a8;
extern int g_60a6;
extern int g_60a4;
extern int g_60a2;
extern int g_60a0;
extern int g_609e;
extern int g_609c;
extern int g_609a;
extern int g_6098;
extern int g_6096;
extern int g_6094;
extern int g_6092;
extern int g_6090;
extern int g_608e;
extern int g_608c;
extern int g_608a;
extern int g_6088;
extern int g_6086;
extern int g_6084;
extern int g_6082;
extern int g_6080;
extern int g_607e;
extern int g_607c;
extern int g_607a;
extern int g_6078;
extern int g_6076;
extern int g_6074;
extern int g_6072;
extern int g_6070;
extern int g_606e;
extern int g_606c;
extern int g_606a;
extern int g_6068;
extern int g_6066;
extern int g_6064;
extern int g_6062;
extern int g_6060;
extern int g_605e;
extern int g_605c;
extern int g_605a;
extern int g_6058;
extern int g_604c[6];
extern int g_6040[6];
extern int g_6034[6];
extern int g_6032;

/* Scalar views of the closed polygon vertex arrays. */
#define g_6036 (*(int near *)((unsigned)g_6034+2))
#define g_6038 (*(int near *)((unsigned)g_6034+4))
#define g_603a (*(int near *)((unsigned)g_6034+6))
#define g_603c (*(int near *)((unsigned)g_6034+8))
#define g_604e (*(int near *)((unsigned)g_604c+2))
#define g_6050 (*(int near *)((unsigned)g_604c+4))
#define g_6052 (*(int near *)((unsigned)g_604c+6))
#define g_6054 (*(int near *)((unsigned)g_604c+8))

/* 0B84:0000, 3 bytes */
unsigned dzl2_seg(void)
{
    return (unsigned)_CS;
}

/* 0B84:0003, 748 bytes */
void butterflies(int initialize)
{
    int i;

    if (initialize || g_6066) {
        g_6058 = random(screen_half_width * 3 / 4) + screen_half_width / 8;
        g_605a = random(screen_half_height * 3 / 4) + screen_half_height / 8;
        g_6036 = random(screen_half_width / 8) + screen_half_width / 16;
        g_604e = random(screen_half_height / 16);
        g_6062 = random(screen_half_height / 2) + screen_half_height / 8;
        g_6064 = random(screen_half_width / 4) + 8;
        g_605e = random(color_count);
        g_6060 = random(3) + 1;
        if (video_adapter == 8)
            g_6060 += 8;
        if (random(2))
            g_6060 += 0x10;
        g_6040[0] = g_6058;
        g_6034[0] = g_6058;
        g_604c[0] = g_605a;
        g_6066 = 0;
    } else {
        g_6036 += 4;
        g_604e += 16;
        g_6038 = g_6036 + g_6064;
        g_6050 = g_604e;
        g_603a = -g_6038 / 2;
        g_6052 = -g_6050 / 2;
        g_603c = g_603a;
        g_6054 = g_603a * g_604e / g_6036;
        for (g_6032 = 1; g_6032 <= 4; ++g_6032)
            g_6040[g_6032] = -g_6034[g_6032];
        if (g_604e > g_6062 || g_604e - g_6054 >= screen_half_height - 1) {
            g_6066 = 1;
            return;
        } else {
            g_605c = random(color_count);
            draw_polygon(1, 5, g_6034, g_604c, g_605c, g_605e, g_6060);
            draw_polygon(1, 5, g_6040, g_604c, g_605c, g_605e, g_6060);
        }
    }
    draw_x = g_6058;
    draw_y = g_605a;
}

/* 0B84:02EF, 1079 bytes */
void diamonds(int initialize)
{
    int i;
    int fill_color;
    int flags;
    int selector;

    if (initialize) {
        i = 0;
        g_608c = screen_half_width / 2;
        g_608e = screen_half_height / 2;
        g_6088 = screen_half_width - screen_half_width / 4;
        g_608a = screen_half_height - screen_half_height / 4;
        g_6068 = random(g_6088);
        g_6072 = random(g_608a);
        g_607c = random(g_608c - 8) + 8;
        g_607e = random(g_608e - 8) + 8;
    }

    if (i <= 0) {
        (void)random(4);
        if (random(2))
            g_6080 = random(screen_half_width / 16) + 4;
        else
            g_6080 = -(random(screen_half_width / 16) + 4);
        if (random(2))
            g_6082 = random(screen_half_height / 16) + 4;
        else
            g_6082 = -(random(screen_half_height / 16) + 4);
        g_6084 = random(screen_half_width / 4) - screen_half_width / 8;
        g_6086 = random(screen_half_width / 4) - screen_half_width / 8;
    }

    g_6068 += g_6080;
    g_6072 += g_6082;
    g_607c += g_6084;
    g_607e += g_6086;
    if (g_6068 < 0 || g_6068 > g_6088) {
        g_6080 = -g_6080;
        g_6068 += g_6080;
    }
    if (g_6072 < 0 || g_6072 > g_608a) {
        g_6082 = -g_6082;
        g_6072 += g_6082;
    }
    if (g_607c < 8 || g_607c > g_608c) {
        g_6084 = -g_6084;
        g_607c += g_6084;
    }
    if (g_607e < 8 || g_607e > g_608e) {
        g_6086 = -g_6086;
        g_607e += g_6086;
    }

    g_606a = g_607c >> 1;
    g_6074 = g_607e >> 1;
    g_606c = g_607c;
    g_6076 = 0;
    g_606e = g_606a;
    g_6078 = -g_6074;

    fill_color = random(color_count);
    if (++plot_color >= color_count)
        plot_color = 3;
    flags = 0;
    if (random(2))
        flags |= 3;
    else {
        flags |= 1;
        if (video_adapter == 8 && random(2))
            flags |= 0x10;
    }
    if (video_adapter == 8) {
        selector = random(3);
        switch (selector) {
        case 0: plot_color = random(color_count); break;
        case 1: flags |= 4; break;
        case 2: flags |= 8; break;
        }
    }
    draw_polygon(1, 4, &g_6068, &g_6072, plot_color, fill_color, flags);
}

/* 0B84:0726, 764 bytes */
void gumballs(int initialize)
{
    if (initialize) {
        g_6090 = random(screen_half_height / 4 - 8) + 8;
        g_6092 = screen_half_width / 4 + random(screen_half_width / 2);
        g_6094 = screen_half_height / 4 + random(screen_half_height / 2);
    }
    g_6096 = random(screen_half_height / 4) - screen_half_height / 8;
    g_6098 = random(screen_half_width / 4) - screen_half_width / 8;
    g_609a = random(screen_half_height / 4) - screen_half_height / 8;
    g_6090 += g_6096;
    g_6092 += g_6098;
    g_6094 += g_609a;
    if (screen_half_width / 8 > g_6092 || g_6092 > screen_half_width) {
        g_6098 = -g_6098;
        g_6092 += g_6098;
    }
    if (screen_half_height / 8 > g_6094 || g_6094 > screen_half_height) {
        g_609a = -g_609a;
        g_6094 += g_609a;
    }
    if (g_6090 < 8 || g_6090 > screen_half_height / 4) {
        g_6096 = -g_6096;
        g_6090 += g_6096;
    }
    g_609c = random(color_count);
    if (++plot_color >= color_count) plot_color = 3;
    g_609e = 0;
    if (random(2))
        g_609e |= 2;
    else {
        g_609e |= 1;
        if (video_adapter == 8 && random(2))
            g_609e |= 0x20;
    }
    if (video_adapter == 8) {
        g_60a0 = random(4);
        switch (g_60a0) {
        case 0: plot_color = random(color_count); break;
        case 1: g_609e |= 4; break;
        case 2: g_609e |= 8; break;
        case 3: g_609e |= 0x10; break;
        }
    }
    draw_ellipse(1, g_6090, 0, g_6092, g_6094, plot_color, g_609c, g_609e);
}

/* 0B84:0A22, 1017 bytes */
void tiles(int initialize)
{
    if (EGA_PLOT_CALLBACK)
        draw_y >>= 1;

    if (initialize) {
        if (EGA_PLOT_CALLBACK)
            g_60ba = screen_half_height / 2;
        else
            g_60ba = screen_half_height;
        g_60b2 = plot_color;
        draw_x = screen_half_width / 4 + random(screen_half_width / 2);
        draw_y = g_60ba / 4 + random(g_60ba / 2);
    }

    g_60a8 = random(screen_half_width / 4) - screen_half_width / 8;
    g_60aa = random(g_60ba / 4) - g_60ba / 8;
    g_0a48[0] = random(screen_half_width / 2);
    g_0a48[1] = random(g_60ba / 2);
    draw_x += g_60a8;
    draw_y += g_60aa;
    if (screen_half_width / 4 > draw_x || draw_x >= screen_half_width) {
        g_60a8 = -g_60a8;
        draw_x += g_60a8;
    }
    if (g_60ba / 4 > draw_y || draw_y >= g_60ba) {
        g_60aa = -g_60aa;
        draw_y += g_60aa;
    }

    if (random(2)) {
        if (video_adapter == 8)
            g_60b8 = random(2) + 1;
        else
            g_60b8 = 0;
        if (g_60b8 == 2)
            plot_color = g_60b2;
        g_60b4 = draw_x;
        g_60b6 = draw_y;
        g_60b0 = g_0a48[1];
        if (EGA_PLOT_CALLBACK) {
            draw_y <<= 1;
            g_60b0 <<= 1;
        }

        while (g_60b0 >= 0) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
            if (g_60b8 == 1)
                plot_color = g_60b2;
            g_60a2 = g_0a48[0];
            while (g_60a2 >= 0) {
                plot_callback(draw_symmetry);
                --draw_x;
                --g_60a2;
                if (g_60b8 == 1 && ++plot_color >= color_count)
                    plot_color = 3;
                ++g_5cbb;
            }
            draw_x += g_0a48[0] + 1;
            --draw_y;
            --g_60b0;
            if (g_60b8 == 2 && ++plot_color >= color_count)
                plot_color = 3;
            if (EGA_PLOT_CALLBACK) {
                --draw_y;
                --g_60b0;
            }
        }
        draw_x = g_60b4;
        draw_y = g_60b6;
        if (g_60b8 == 0 && ++plot_color >= color_count)
            plot_color = 3;
        g_60b2 = plot_color;
    }

    if (EGA_PLOT_CALLBACK) {
        draw_y <<= 1;
        g_0a48[1] <<= 1;
    }
    plot_color = random(color_count);
    g_60a6 = -1;
    while (++g_60a6 <= 3) {
        g_60a2 = g_0a48[g_60a6 & 1];
        g_60ac = g_0a38[g_60a6];
        g_60ae = g_0a40[g_60a6];
        g_60a4 = 0;
        while (g_60a4++ < g_60a2) {
            plot_callback(draw_symmetry);
            draw_x += g_60ac;
            draw_y += g_60ae;
            if (g_60ae != 0 && EGA_PLOT_CALLBACK) {
                draw_y += g_60ae;
                ++g_60a4;
            }
            ++g_5cbb;
        }
    }
}

/* 0B84:0E1B, 804 bytes */
void wedges(int initialize)
{
    int i, px[4], py[4], first, fill_color, selector, flags;

    if (initialize) {
        first = 0;
        for (i = 0; i < 3; ++i) {
            g_60bc[i] = screen_half_width / 4 + random(screen_half_width / 2);
            g_60c2[i] = screen_half_height / 4 + random(screen_half_height / 2);
        }
    }
    if (first <= 0) {
        (void)random(8);
        for (i = 0; i < 3; ++i) {
            g_60c8[i] = random(screen_half_width / 4) - screen_half_width / 8;
            g_60ce[i] = random(screen_half_height / 4) - screen_half_height / 8;
        }
    }

    for (i = 0; i < 3; ++i) {
        g_60bc[i] += g_60c8[i];
        g_60c2[i] += g_60ce[i];
        if (g_60bc[i] < 0 || g_60bc[i] >= screen_half_width) {
            g_60c8[i] = -g_60c8[i];
            g_60bc[i] += g_60c8[i];
        }
        if (g_60c2[i] < 0 || g_60c2[i] >= screen_half_height) {
            g_60ce[i] = -g_60ce[i];
            g_60c2[i] += g_60ce[i];
        }
    }

    px[0] = g_60bc[0];
    py[0] = g_60c2[0];
    px[1] = g_60bc[1] - g_60bc[0];
    py[1] = g_60c2[1] - g_60c2[0];
    px[2] = g_60bc[2] - g_60bc[0];
    py[2] = g_60c2[2] - g_60c2[0];

    fill_color = random(color_count);
    if (++plot_color >= color_count)
        plot_color = 3;
    flags = 0;
    if (random(2))
        flags |= 3;
    else {
        flags |= 1;
        if (video_adapter == 8 && random(2))
            flags |= 0x10;
    }
    if (video_adapter == 8) {
        selector = random(3);
        switch (selector) {
        case 0: plot_color = random(color_count); break;
        case 1: flags |= 4; break;
        case 2: flags |= 8; break;
        }
    }
    draw_polygon(1, 3, px, py, plot_color, fill_color, flags);
}

/* 0B84:113F, 859 bytes */
void yarnball(int initialize)
{
    register int draw_x, draw_y;
    int i;

    if (initialize) {
        g_60ec = (float)screen_half_width / (float)screen_half_height * g_0a4c;
        g_60f0 = 0.866f;
        g_60f4 = 0.5f;
        g_60e4 = 1;
        g_60ea = 1;
        g_60e6 = random(3);
        if (g_60e6 != 0) {
            g_60de = screen_half_height / 8 + random(screen_half_height / 8);
            g_60e2 = g_60de * 2 + random(screen_half_height / 4);
            g_60e0 = (int)((float)random(screen_half_width / 4) + ((float)(g_60de * 2) * g_60ec));
        } else {
            g_60e8 = 1;
            g_60de = screen_half_height / 4 + random(screen_half_height * 3 / 4);
            g_60e0 = 0;
            g_60e2 = 0;
            g_60e4 |= 0x20;
        }
        return;
    }

    draw_ellipse(1, g_60de, 0, g_60e0, g_60e2, 0, plot_color, g_60e4);

    g_60d4 = (int)((float)g_60de * g_60f0);
    g_60d6 = (int)((float)g_60de * g_60f4);
    g_60d8 = (int)((float)g_60d4 * g_60ec);
    g_60da = (int)((float)g_60d6 * g_60ec);
    g_60dc = (int)((float)g_60de * g_60ec);

    i = 0;
    do {
        switch (i) {
        case 0: draw_x = g_60e0 + g_60dc; draw_y = g_60e2; break;
        case 1: draw_x = g_60e0 - g_60dc; break;
        case 2: draw_x = g_60e0; draw_y = g_60e2 + g_60de; break;
        case 3: draw_y = g_60e2 - g_60de; break;
        case 4: draw_x = g_60e0 + g_60d8; draw_y = g_60e2 + g_60d6; break;
        case 5: draw_x = g_60e0 - g_60d8; break;
        case 6: draw_y = g_60e2 - g_60d6; break;
        case 7: draw_x = g_60e0 + g_60d8; break;
        case 8: draw_x = g_60e0 + g_60da; draw_y = g_60e2 + g_60d4; break;
        case 9: draw_x = g_60e0 - g_60da; break;
        case 10: draw_y = g_60e2 - g_60d4; break;
        case 11: draw_x = g_60e0 + g_60da; break;
        }
        draw_ellipse(0, g_60de, 0, draw_x, draw_y, 0, plot_color, g_60e4);
        if (g_60e6 != 0)
            ++i;
        else {
            i = 2 << g_60e8++;
        }
    } while (i < 12);

    g_60de += g_60ea;
    if (g_60e6 != 0)
        g_60e4 ^= 0x20;
    if (++plot_color >= color_count)
        plot_color = 3;
    g_60e8 = 1;
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
float g_60f4; /* DS:60F4 */
float g_60f0; /* DS:60F0 */
float g_60ec; /* DS:60EC */
int g_60ea; /* DS:60EA */
int g_60e8; /* DS:60E8 */
int g_60e6; /* DS:60E6 */
int g_60e4; /* DS:60E4 */
int g_60e2; /* DS:60E2 */
int g_60e0; /* DS:60E0 */
int g_60de; /* DS:60DE */
int g_60dc; /* DS:60DC */
int g_60da; /* DS:60DA */
int g_60d8; /* DS:60D8 */
int g_60d6; /* DS:60D6 */
int g_60d4; /* DS:60D4 */
int g_60ce[3]; /* DS:60CE */
int g_60c8[3]; /* DS:60C8 */
int g_60c2[3]; /* DS:60C2 */
int g_60bc[3]; /* DS:60BC */
int g_60ba; /* DS:60BA */
int g_60b8; /* DS:60B8 */
int g_60b6; /* DS:60B6 */
int g_60b4; /* DS:60B4 */
int g_60b2; /* DS:60B2 */
int g_60b0; /* DS:60B0 */
int g_60ae; /* DS:60AE */
int g_60ac; /* DS:60AC */
int g_60aa; /* DS:60AA */
int g_60a8; /* DS:60A8 */
int g_60a6; /* DS:60A6 */
int g_60a4; /* DS:60A4 */
int g_60a2; /* DS:60A2 */
int g_60a0; /* DS:60A0 */
int g_609e; /* DS:609E */
int g_609c; /* DS:609C */
int g_609a; /* DS:609A */
int g_6098; /* DS:6098 */
int g_6096; /* DS:6096 */
int g_6094; /* DS:6094 */
int g_6092; /* DS:6092 */
int g_6090; /* DS:6090 */
int g_608e; /* DS:608E */
int g_608c; /* DS:608C */
int g_608a; /* DS:608A */
int g_6088; /* DS:6088 */
int g_6086; /* DS:6086 */
int g_6084; /* DS:6084 */
int g_6082; /* DS:6082 */
int g_6080; /* DS:6080 */
int g_607e; /* DS:607E */
int g_607c; /* DS:607C */
int g_607a; /* DS:607A */
int g_6078; /* DS:6078 */
int g_6076; /* DS:6076 */
int g_6074; /* DS:6074 */
int g_6072; /* DS:6072 */
int g_6070; /* DS:6070 */
int g_606e; /* DS:606E */
int g_606c; /* DS:606C */
int g_606a; /* DS:606A */
int g_6068; /* DS:6068 */
int g_6066; /* DS:6066 */
int g_6064; /* DS:6064 */
int g_6062; /* DS:6062 */
int g_6060; /* DS:6060 */
int g_605e; /* DS:605E */
int g_605c; /* DS:605C */
int g_605a; /* DS:605A */
int g_6058; /* DS:6058 */
int g_604c[6]; /* DS:604C */
int g_6040[6]; /* DS:6040 */
int g_6034[6]; /* DS:6034 */
int g_6032; /* DS:6032 */

/* Control-image geometry tables. */
int g_0a38[4] = {
    -1, 0, 1, 0
};
int g_0a40[4] = {
    0, -1, 0, 1
};
int g_0a48[2] = {
    0, 0
};
float g_0a4c = 0.75f;
