/* Fade and transition routines from code segment 0CCD. */
#include <dos.h>
#include <stdlib.h>

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
extern unsigned int g_7488;
extern unsigned int g_5DC0;

int poll_palette_keys(void);
void cycle_palette(void);
void rtl_32d8(unsigned int, void *);
void swap_quilt_cells(unsigned int, unsigned int, unsigned int, unsigned int,
                      unsigned int, unsigned int, unsigned int);
void clear_vram_words(unsigned int, unsigned int, unsigned int);
void restore_vram_page(void);
void animate_vertical_display_start(unsigned int, unsigned int);
void set_xor_mode(int);
void fill_square(int, unsigned int);
void randomize_helper(unsigned int);

/* 0ccd:0000..000a holds five words (13d8, 13e2, 13f4, 13fe, 1408). They are not a switch table for wait_fade_tick at 000d; 13e2 lands mid-instruction. Ownership remains unassigned, so the words are treated as a data island. */

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
/* objcmp: {"function":"randomize_helper","address":"0ccd:008d","size":2792,"level":"UNKNOWN","blocker":"fp-emulator-stream-at-0ccd:00a5; disjoint-CFG-through-0ccd:0b74","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5d7a":"DS:5d7a","_g_5da6":"DS:5da6","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_7488":"DS:7488","_g_5c3f":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
/* 0ccd:008d is a 2,792-byte particle/path routine. The Ghidra CFG has disjoint blocks and apparent software-FP escape bytes beginning near 0ccd:00a5; it also calls a DS-resident drawing callback at 5c3f. The C body is not reconstructed, so this entry has no PUBDEF candidate. */

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
/* objcmp: {"function":"gas_cloud","address":"0ccd:0d6b","size":198,"symbols":{"_g_5D78":"DS:5d78","_g_5D7A":"DS:5d7a","_g_5D7E":"DS:5d7e","_g_5D80":"DS:5d80","_g_5DA6":"DS:5da6","_g_5D92":"DS:5d92","_g_5DB2":"DS:5db2","_g_5DC6":"DS:5dc6","_g_5DE4":"DS:5de4","_g_7436":"DS:7436","_g_04D4":"DS:04d4","_g_7490":"DS:7490","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_clear_vram_words":"1899:0acd","_restore_vram_page":"1899:133d","_poll_palette_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_animate_vertical_display_start":"0ccd:4051"},"level":"PROVISIONAL","blocker":"linker-farcall-rewrite at 0ccd:0e28 (NOP; PUSH CS; CALL 0ccd:4051)"} */
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
            animate_vertical_display_start(0, vertical);
        } else {
            animate_vertical_display_start(vertical, 0);
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
/* objcmp: {"function":"mesh_transition","address":"0ccd:0f7b","size":677,"level":"UNKNOWN","blocker":"indirect-draw-callback-DS:5c3f; fixed-point-loop-control-not-rebuilt","symbols":{"_g_7484":"DS:7484","_g_7488":"DS:7488","_g_5dc0":"DS:5dc0","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5c3f":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
/* Ghidra identifies a multi-pass mesh drawing effect with nested horizontal and vertical sweeps, palette cycling, key polling, and indirect draw calls. The complete C control flow and callback signatures are not yet reconstructed. */

/* objcmp: {"function":"firefly_transition","address":"0ccd:1220","size":1210,"level":"UNKNOWN","blocker":"four-lane-fixed-point-state-and-indirect-draw-callbacks-not-rebuilt","symbols":{"_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_7488":"DS:7488","_g_5c3f":"DS:5c3f","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
/* Ghidra identifies a 1,210-byte particle animation with four active lanes, per-lane arrays, fixed-point coordinates, palette/key checks, and indirect draw callbacks. The source-level frame layout and arithmetic promotions remain unresolved. */

/* objcmp: {"function":"spiral_transition","address":"0ccd:16da","size":734,"level":"UNKNOWN","blocker":"fp-emulator-stream-at-0ccd:177c; later-block-through-0ccd:19b7","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
/* Ghidra analysis stops its first block at the Borland software floating-point emulator stream near 0ccd:177c; later blocks form one routine through 0ccd:19b7 and call the separately matched spiral_midpoint helper. */

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
/* objcmp: {"function":"drip_transition","address":"0ccd:1aa1","size":544,"level":"UNKNOWN","blocker":"0x140c-byte-stack-frame; local-array-and-fixed-point-layout-not-rebuilt","symbols":{"_g_5db0":"DS:5db0","_g_5db2":"DS:5db2","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92","_g_7488":"DS:7488","_g_7484":"DS:7484","_g_5daa":"DS:5daa","_g_5df0":"DS:5df0","_g_5dee":"DS:5dee","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_spiral_midpoint":"0ccd:19b8"}} */
/* Ghidra identifies the Drip effect using two large local path arrays (stack frame 0x140c), signed fixed-point updates, recursive midpoint generation, and palette/key checks. A readable C reconstruction preserving the original stack layout and 16-bit arithmetic is not yet available. */

/* objcmp: {"function":"cleanup_transition","address":"0ccd:1cc1","size":728,"level":"UNKNOWN","blocker":"switch-data-in-code-and-Ghidra-overlapping-instruction-flow","symbols":{"_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","_g_5da6":"DS:5da6","_g_5d7a":"DS:5d7a","_g_5d7e":"DS:5d7e","_g_5d80":"DS:5d80","_g_5d92":"DS:5d92"}} */
/* Ghidra reports overlapping instructions and an unstable type-propagation result for this cleanup routine. Its body ends at 0ccd:1f7b and has two associated CS switch tables at 1f7b (20 bytes) and 1f8f (10 bytes), so the complete extent is 728 bytes through 1f99. */

