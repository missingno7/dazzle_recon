/* Fade effects, pan/scroll helpers, and the fade dispatcher from segment 0CCD. */
#include <dos.h>
#include <stdlib.h>

extern unsigned int colors, g_5dba;
extern int g_5dc4, g_5d94, g_5d96, g_5dec, g_5cb1;
extern int g_5D7E, g_5D80;
extern unsigned int g_5D92, g_5D7A, g_5DA6;
extern void (*g_5c37)(unsigned, unsigned);
extern int poll_palette_keys(void);
extern void cycle_palette(void);

/* objcmp: {"function":"fade_countdown","address":"0ccd:1f99","size":587,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* A timed/counting transition, possibly clearing the display in stages. */

/* objcmp: {"function":"fade_ziti","address":"0ccd:21e4","size":745,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Zigzag or folded-strip transition. */

/* objcmp: {"function":"fade_umbra","address":"0ccd:24cd","size":413,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Shadow-like wipe using the line iterator and plot callback. */

/* objcmp: {"function":"fade_moving_bars","address":"0ccd:2672","size":804,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Moving bars sweep across the image. */

/* objcmp: {"function":"fade_tetris","address":"0ccd:2996","size":881,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Falling-block transition, using the pixel read and plot callbacks. */

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

/* objcmp: {"function":"fade_twist","address":"0ccd:2e20","size":626,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Twisting screen transformation; this extent contains the INT 34h..3Dh FP emulator sequence. */

/* objcmp: {"function":"swap_rectangle","address":"0ccd:3092","size":660,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Rectangle swap helper called by Rectangles. */

/* objcmp: {"function":"fade_rectangles","address":"0ccd:3326","size":273,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Rectangle-based transition. */

/* objcmp: {"function":"fade_linewipe","address":"0ccd:3437","size":958,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Line-by-line wipe. */

/* objcmp: {"function":"fade_starburst","address":"0ccd:37f5","size":1625,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Starburst-shaped reveal or collapse. */

/* objcmp: {"function":"fade_wipe","address":"0ccd:3e4e","size":344,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* General pixel or region wipe. */

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

/* objcmp: {"function":"fade_blister","address":"0ccd:4202","size":649,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Spreading bubble-like transition; calls wait_fade_tick in the original TU. */

/* objcmp: {"function":"fade_dac_to_black","address":"0ccd:448b","size":248,"level":"PROVISIONAL","blocker":"control-flow: 4546 jne displacement 34 vs 35; object extent 247 vs 248","symbols":{"_colors":"DS:5db6","_g_5dba":"DS:5dba","_g_5dc4":"DS:5dc4","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_g_5dec":"DS:5dec","_g_5de8":"DS:5de8","_g_5cb1":"DS:5cb1","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_interval":"DS:5da6","_g_5c37":"DS:5c37","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void fade_dac_to_black(int mask_high_bits)
{
    volatile int draw_delay;
    unsigned char far *palette;
    int count, i;
    register int remaining;

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
        if ((g_5D7E != 0 || g_5D80 != 0) && poll_palette_keys() != 0)
            return;
        if (g_5D7A >= g_5DA6)
            cycle_palette();
        draw_delay ^= 1;
        if (draw_delay != 0)
            g_5c37(g_5d94 + g_5dec, g_5d96);
    } while (remaining != 0);
    g_5cb1 = 1;
}

/* objcmp: {"function":"fade_palette_to_background","address":"0ccd:4583","size":402,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Replace palette buffer entries with the low background byte and upload each step. */

/* objcmp: {"function":"dispatch_fade_key","address":"0ccd:4715","size":877,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Key switch plus its 26-word table at 0CCD:4A4E. */

/* objcmp: {"function":"display_boundary_fade_and_pan","address":"0ccd:4a82","size":1224,"level":"UNKNOWN","blocker":"source-unreconstructed"} */
/* Boundary, pause, fade-key, and Pan/scroll orchestration; RETF at 0CCD:4F49. */
