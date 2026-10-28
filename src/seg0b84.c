#include <stdlib.h>
#include <dos.h>
#include <dos.h>

extern int x;
extern int y;
extern int half_width;
extern int half_height;
extern unsigned colors;
extern unsigned color;
extern unsigned video_adapter;
extern unsigned palette_tick;
extern unsigned palette_cycle_ticks;
extern unsigned draw_symmetry;
extern void (far *plot_callback)(unsigned);
extern void far ega_plot_symmetric_640x700(unsigned);
extern void draw_polygon(int, int, int far *, int far *, unsigned, unsigned, unsigned);
extern void draw_ellipse(int, int, int, int, int, unsigned, unsigned, unsigned);
extern void cycle_palette(void);

extern int g_6032;
extern int g_6034[5];
extern int g_6036;
extern int g_6038;
extern int g_603a;
extern int g_603c;
extern int g_6040[5];
extern int g_604c[5];
extern int g_604e;
extern int g_6050;
extern int g_6052;
extern int g_6054;
extern int g_6058;
extern int g_605a;
extern int g_605c;
extern int g_605e;
extern int g_6060;
extern int g_6062;
extern int g_6064;
extern int g_6066;

extern int g_6068;
extern int g_606a;
extern int g_606c;
extern int g_606e;
extern int g_6070;
extern int g_6072;
extern int g_6074;
extern int g_6076;
extern int g_6078;
extern int g_607a;
extern int g_607c;
extern int g_607e;
extern int g_6080;
extern int g_6082;
extern int g_6084;
extern int g_6086;
extern int g_6088;
extern int g_608a;
extern int g_608c;
extern int g_608e;
extern int g_6090;
extern int g_6092;
extern int g_6094;
extern int g_6096;
extern int g_6098;
extern int g_609a;
extern int g_609c;
extern int g_609e;
extern int g_60a0;
extern int g_60a2;
extern int g_60a4;
extern int g_60a6;
extern int g_60a8;
extern int g_60aa;
extern int g_60ac;
extern int g_60ae;
extern int g_60b0;
extern int g_60b2;
extern int g_60b4;
extern int g_60b6;
extern int g_60b8;
extern int g_60ba;
extern int g_60bc[3];
extern int g_60c2[3];
extern int g_60c8[3];
extern int g_60ce[3];
extern int g_60d4;
extern int g_60d6;
extern int g_60d8;
extern int g_60da;
extern int g_60dc;
extern int g_60de;
extern int g_60e0;
extern int g_60e2;
extern int g_60e4;
extern int g_60e6;
extern int g_60e8;
extern int g_60ea;
extern float g_60ec;
extern float g_60f0;
extern float g_60f4;
extern float g_0a4c;
extern int g_5cbb;
extern int g_0a38[4];
extern int g_0a40[4];
extern int g_0a48[2];

#define EGA_PLOT_CALLBACK (FP_SEG(plot_callback) == FP_SEG(ega_plot_symmetric_640x700) && FP_OFF(plot_callback) == FP_OFF(ega_plot_symmetric_640x700))

/* objcmp: {"function":"get_code_segment","address":"0b84:0000","size":3,"symbols":{}} */
unsigned get_code_segment(void)
{
    return (unsigned)_CS;
}

/* objcmp: {"function":"butterflies","address":"0b84:0003","size":748,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_half_width":"DS:5db0","_half_height":"DS:5db2","_colors":"DS:5db6","_color":"DS:7488","_video_adapter":"DS:7484","_palette_tick":"DS:5d7a","_plot_callback":"DS:5c3f","_g_6032":"DS:6032","_g_6034":"DS:6034","_g_6036":"DS:6036","_g_6038":"DS:6038","_g_603a":"DS:603a","_g_603c":"DS:603c","_g_6040":"DS:6040","_g_604c":"DS:604c","_g_604e":"DS:604e","_g_6050":"DS:6050","_g_6052":"DS:6052","_g_6054":"DS:6054","_g_6058":"DS:6058","_g_605a":"DS:605a","_g_605c":"DS:605c","_g_605e":"DS:605e","_g_6060":"DS:6060","_g_6062":"DS:6062","_g_6064":"DS:6064","_g_6066":"DS:6066","_g_6068":"DS:6068","_g_606a":"DS:606a","_g_606c":"DS:606c","_g_606e":"DS:606e","_g_6070":"DS:6070","_g_6072":"DS:6072","_g_6074":"DS:6074","_g_6076":"DS:6076","_g_6078":"DS:6078","_g_607c":"DS:607c","_g_607e":"DS:607e","_g_6080":"DS:6080","_g_6082":"DS:6082","_g_6084":"DS:6084","_g_6086":"DS:6086","_g_6088":"DS:6088","_g_608a":"DS:608a","_g_608c":"DS:608c","_g_608e":"DS:608e","_g_6090":"DS:6090","_g_6092":"DS:6092","_g_6094":"DS:6094","_g_6096":"DS:6096","_g_6098":"DS:6098","_g_609a":"DS:609a","_g_609c":"DS:609c","_g_609e":"DS:609e","_g_60a0":"DS:60a0","_g_60bc":"DS:60bc","_g_60c2":"DS:60c2","_g_60c8":"DS:60c8","_g_60ce":"DS:60ce","_g_60de":"DS:60de","_g_60e0":"DS:60e0","_g_60e2":"DS:60e2","_g_60e4":"DS:60e4","_g_60e6":"DS:60e6","_g_60e8":"DS:60e8","_g_60ea":"DS:60ea","_g_60ec":"DS:60ec","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_draw_polygon":"066a:02bb","_draw_ellipse":"066a:089b","_cycle_palette":"11c1:2515"}} */
void butterflies(int initialize)
{
    int i;

    if (initialize || g_6066) {
        g_6058 = random(half_width * 3 / 4) + half_width / 8;
        g_605a = random(half_height * 3 / 4) + half_height / 8;
        g_6036 = random(half_width / 8) + half_width / 16;
        g_604e = random(half_height / 16);
        g_6062 = random(half_height / 2) + half_height / 8;
        g_6064 = random(half_width / 4) + 8;
        g_605e = random(colors);
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
        if (g_604e > g_6062 || g_604e - g_6054 >= half_height - 1) {
            g_6066 = 1;
            return;
        } else {
            g_605c = random(colors);
            draw_polygon(1, 5, g_6034, g_604c, g_605c, g_605e, g_6060);
            draw_polygon(1, 5, g_6040, g_604c, g_605c, g_605e, g_6060);
        }
    }
    x = g_6058;
    y = g_605a;
}

/* objcmp: {"function":"diamonds","address":"0b84:02ef","size":1079,"symbols":{"_x":"DS:5df0","_half_width":"DS:5db0","_half_height":"DS:5db2","_colors":"DS:5db6","_color":"DS:7488","_video_adapter":"DS:7484","_g_6068":"DS:6068","_g_606a":"DS:606a","_g_606c":"DS:606c","_g_606e":"DS:606e","_g_6072":"DS:6072","_g_6074":"DS:6074","_g_6076":"DS:6076","_g_6078":"DS:6078","_g_607c":"DS:607c","_g_607e":"DS:607e","_g_6080":"DS:6080","_g_6082":"DS:6082","_g_6084":"DS:6084","_g_6086":"DS:6086","_g_6088":"DS:6088","_g_608a":"DS:608a","_g_608c":"DS:608c","_g_608e":"DS:608e","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_draw_polygon":"066a:02bb"}} */
void diamonds(int initialize)
{
    int i;
    int fill_color;
    int flags;
    int selector;

    if (initialize) {
        i = 0;
        g_608c = half_width / 2;
        g_608e = half_height / 2;
        g_6088 = half_width - half_width / 4;
        g_608a = half_height - half_height / 4;
        g_6068 = random(g_6088);
        g_6072 = random(g_608a);
        g_607c = random(g_608c - 8) + 8;
        g_607e = random(g_608e - 8) + 8;
    }

    if (i <= 0) {
        (void)random(4);
        if (random(2))
            g_6080 = random(half_width / 16) + 4;
        else
            g_6080 = -(random(half_width / 16) + 4);
        if (random(2))
            g_6082 = random(half_height / 16) + 4;
        else
            g_6082 = -(random(half_height / 16) + 4);
        g_6084 = random(half_width / 4) - half_width / 8;
        g_6086 = random(half_width / 4) - half_width / 8;
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

    fill_color = random(colors);
    if (++color >= colors)
        color = 3;
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
        case 0: color = random(colors); break;
        case 1: flags |= 4; break;
        case 2: flags |= 8; break;
        }
    }
    draw_polygon(1, 4, &g_6068, &g_6072, color, fill_color, flags);
}

/* objcmp: {"function":"gumballs","address":"0b84:0726","size":764,"symbols":{"_g_6090":"DS:6090","_g_6092":"DS:6092","_g_6094":"DS:6094","_g_6096":"DS:6096","_g_6098":"DS:6098","_g_609a":"DS:609a","_g_609c":"DS:609c","_g_609e":"DS:609e","_g_60a0":"DS:60a0","_half_width":"DS:5db0","_half_height":"DS:5db2","_colors":"DS:5db6","_color":"DS:7488","_video_adapter":"DS:7484","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_draw_ellipse":"066a:089b"}} */
void gumballs(int initialize)
{
    if (initialize) {
        g_6090 = random(half_height / 4 - 8) + 8;
        g_6092 = half_width / 4 + random(half_width / 2);
        g_6094 = half_height / 4 + random(half_height / 2);
    }
    g_6096 = random(half_height / 4) - half_height / 8;
    g_6098 = random(half_width / 4) - half_width / 8;
    g_609a = random(half_height / 4) - half_height / 8;
    g_6090 += g_6096;
    g_6092 += g_6098;
    g_6094 += g_609a;
    if (half_width / 8 > g_6092 || g_6092 > half_width) {
        g_6098 = -g_6098;
        g_6092 += g_6098;
    }
    if (half_height / 8 > g_6094 || g_6094 > half_height) {
        g_609a = -g_609a;
        g_6094 += g_609a;
    }
    if (g_6090 < 8 || g_6090 > half_height / 4) {
        g_6096 = -g_6096;
        g_6090 += g_6096;
    }
    g_609c = random(colors);
    if (++color >= colors) color = 3;
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
        case 0: color = random(colors); break;
        case 1: g_609e |= 4; break;
        case 2: g_609e |= 8; break;
        case 3: g_609e |= 0x10; break;
        }
    }
    draw_ellipse(1, g_6090, 0, g_6092, g_6094, color, g_609c, g_609e);
}

/* objcmp: {"function":"tiles","address":"0b84:0a22","size":1017,"symbols":{"_x":"DS:5df0","_y":"DS:5dee","_half_width":"DS:5db0","_half_height":"DS:5db2","_colors":"DS:5db6","_color":"DS:7488","_video_adapter":"DS:7484","_palette_tick":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_draw_symmetry":"DS:5caf","_g_5cbb":"DS:5cbb","_g_60a2":"DS:60a2","_g_60a4":"DS:60a4","_g_60a6":"DS:60a6","_g_60a8":"DS:60a8","_g_60aa":"DS:60aa","_g_60ac":"DS:60ac","_g_60ae":"DS:60ae","_g_60b0":"DS:60b0","_g_60b2":"DS:60b2","_g_60b4":"DS:60b4","_g_60b6":"DS:60b6","_g_60b8":"DS:60b8","_g_60ba":"DS:60ba","_g_0a38":"DS:0a38","_g_0a40":"DS:0a40","_g_0a48":"DS:0a48","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_cycle_palette":"11c1:2515","_plot_callback":"DS:5c3f","_ega_plot_symmetric_640x700":"1899:012d"}} */
void tiles(int initialize)
{
    if (EGA_PLOT_CALLBACK)
        y >>= 1;

    if (initialize) {
        if (EGA_PLOT_CALLBACK)
            g_60ba = half_height / 2;
        else
            g_60ba = half_height;
        g_60b2 = color;
        x = half_width / 4 + random(half_width / 2);
        y = g_60ba / 4 + random(g_60ba / 2);
    }

    g_60a8 = random(half_width / 4) - half_width / 8;
    g_60aa = random(g_60ba / 4) - g_60ba / 8;
    g_0a48[0] = random(half_width / 2);
    g_0a48[1] = random(g_60ba / 2);
    x += g_60a8;
    y += g_60aa;
    if (half_width / 4 > x || x >= half_width) {
        g_60a8 = -g_60a8;
        x += g_60a8;
    }
    if (g_60ba / 4 > y || y >= g_60ba) {
        g_60aa = -g_60aa;
        y += g_60aa;
    }

    if (random(2)) {
        if (video_adapter == 8)
            g_60b8 = random(2) + 1;
        else
            g_60b8 = 0;
        if (g_60b8 == 2)
            color = g_60b2;
        g_60b4 = x;
        g_60b6 = y;
        g_60b0 = g_0a48[1];
        if (EGA_PLOT_CALLBACK) {
            y <<= 1;
            g_60b0 <<= 1;
        }

        while (g_60b0 >= 0) {
            if (palette_tick >= palette_cycle_ticks)
                cycle_palette();
            if (g_60b8 == 1)
                color = g_60b2;
            g_60a2 = g_0a48[0];
            while (g_60a2 >= 0) {
                plot_callback(draw_symmetry);
                --x;
                --g_60a2;
                if (g_60b8 == 1 && ++color >= colors)
                    color = 3;
                ++g_5cbb;
            }
            x += g_0a48[0] + 1;
            --y;
            --g_60b0;
            if (g_60b8 == 2 && ++color >= colors)
                color = 3;
            if (EGA_PLOT_CALLBACK) {
                --y;
                --g_60b0;
            }
        }
        x = g_60b4;
        y = g_60b6;
        if (g_60b8 == 0 && ++color >= colors)
            color = 3;
        g_60b2 = color;
    }

    if (EGA_PLOT_CALLBACK) {
        y <<= 1;
        g_0a48[1] <<= 1;
    }
    color = random(colors);
    g_60a6 = -1;
    while (++g_60a6 <= 3) {
        g_60a2 = g_0a48[g_60a6 & 1];
        g_60ac = g_0a38[g_60a6];
        g_60ae = g_0a40[g_60a6];
        g_60a4 = 0;
        while (g_60a4++ < g_60a2) {
            plot_callback(draw_symmetry);
            x += g_60ac;
            y += g_60ae;
            if (g_60ae != 0 && EGA_PLOT_CALLBACK) {
                y += g_60ae;
                ++g_60a4;
            }
            ++g_5cbb;
        }
    }
}

/* objcmp: {"function":"wedges","address":"0b84:0e1b","size":804,"symbols":{"_half_width":"DS:5db0","_half_height":"DS:5db2","_colors":"DS:5db6","_color":"DS:7488","_video_adapter":"DS:7484","_g_60bc":"DS:60bc","_g_60c2":"DS:60c2","_g_60c8":"DS:60c8","_g_60ce":"DS:60ce","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395","_draw_polygon":"066a:02bb"}} */
void wedges(int initialize)
{
    int i, px[4], py[4], first, fill_color, selector, flags;

    if (initialize) {
        first = 0;
        for (i = 0; i < 3; ++i) {
            g_60bc[i] = half_width / 4 + random(half_width / 2);
            g_60c2[i] = half_height / 4 + random(half_height / 2);
        }
    }
    if (first <= 0) {
        (void)random(8);
        for (i = 0; i < 3; ++i) {
            g_60c8[i] = random(half_width / 4) - half_width / 8;
            g_60ce[i] = random(half_height / 4) - half_height / 8;
        }
    }

    for (i = 0; i < 3; ++i) {
        g_60bc[i] += g_60c8[i];
        g_60c2[i] += g_60ce[i];
        if (g_60bc[i] < 0 || g_60bc[i] >= half_width) {
            g_60c8[i] = -g_60c8[i];
            g_60bc[i] += g_60c8[i];
        }
        if (g_60c2[i] < 0 || g_60c2[i] >= half_height) {
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

    fill_color = random(colors);
    if (++color >= colors)
        color = 3;
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
        case 0: color = random(colors); break;
        case 1: flags |= 4; break;
        case 2: flags |= 8; break;
        }
    }
    draw_polygon(1, 3, px, py, color, fill_color, flags);
}

/* objcmp: {"function":"yarnball","address":"0b84:113f","size":859,"level":"SEMANTIC","blocker":"register-order at 0B84:1437: target loads CX from DS:60E8 before AX=2; BC emits AX=2 before loading CL","symbols":{"_x":"DS:5df0","_y":"DS:5dee","_half_width":"DS:5db0","_half_height":"DS:5db2","_colors":"DS:5db6","_color":"DS:7488","_g_0a4c":"DS:0a4c","_g_60d4":"DS:60d4","_g_60d6":"DS:60d6","_g_60d8":"DS:60d8","_g_60da":"DS:60da","_g_60dc":"DS:60dc","_g_60de":"DS:60de","_g_60e0":"DS:60e0","_g_60e2":"DS:60e2","_g_60e4":"DS:60e4","_g_60e6":"DS:60e6","_g_60e8":"DS:60e8","_g_60ea":"DS:60ea","_g_60ec":"DS:60ec","_g_60f0":"DS:60f0","_g_60f4":"DS:60f4","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_FTOL@":"0000:0fbe","_draw_ellipse":"066a:089b"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
void yarnball(int initialize)
{
    register int draw_x, draw_y;
    int i;

    if (initialize) {
        g_60ec = (float)half_width / (float)half_height * g_0a4c;
        g_60f0 = 0.866f;
        g_60f4 = 0.5f;
        g_60e4 = 1;
        g_60ea = 1;
        g_60e6 = random(3);
        if (g_60e6 != 0) {
            g_60de = half_height / 8 + random(half_height / 8);
            g_60e2 = g_60de * 2 + random(half_height / 4);
            g_60e0 = (int)((float)random(half_width / 4) + ((float)(g_60de * 2) * g_60ec));
        } else {
            g_60e8 = 1;
            g_60de = half_height / 4 + random(half_height * 3 / 4);
            g_60e0 = 0;
            g_60e2 = 0;
            g_60e4 |= 0x20;
        }
        return;
    }

    draw_ellipse(1, g_60de, 0, g_60e0, g_60e2, 0, color, g_60e4);

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
        draw_ellipse(0, g_60de, 0, draw_x, draw_y, 0, color, g_60e4);
        if (g_60e6 != 0)
            ++i;
        else {
            i = 2 << g_60e8;
            ++g_60e8;
        }
    } while (i < 12);

    g_60de += g_60ea;
    if (g_60e6 != 0)
        g_60e4 ^= 0x20;
    if (++color >= colors)
        color = 3;
    g_60e8 = 1;
}
