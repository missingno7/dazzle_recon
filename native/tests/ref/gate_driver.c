/* The diagnostic shim controls the exact stack slot read by unmodified DZL2. */
#include "imgmodel.h"
extern int diamonds_origin_x, diamonds_origin_y, diamonds_step_x, diamonds_step_y;
extern int wedges_vertex_x[3], wedges_vertex_y[3], wedges_step_x[3], wedges_step_y[3];

static void diamond_call(int poison)
{
    asm mov ax, poison
    asm mov bx, sp
    asm mov ss:[bx-10], ax
    diamonds(0);
}
static void wedge_call(int poison)
{
    asm mov ax, poison
    asm mov bx, sp
    asm mov ss:[bx-26], ax
    wedges(0);
}

void main(int argc, char far * far *argv)
{
    static int poisons[] = {0, 1, -1, 32767, -32768};
    unsigned i, n;
    unsigned long before;
    FILE *out = fopen("GATES.TXT", "w");
    (void)argc; (void)argv;
    full_log = fopen("PLOT.BIN", "wb");
    first_log = fopen("PLOT.LOG", "w");
    if (!out || !full_log || !first_log) exit(2);
    framebuffer = (unsigned char huge *)farmalloc(224000UL);
    if (!framebuffer) exit(3);
    make_crc_table(); initialize_math_tables(); setup_video_model(ADAPTER_VGA);
    tick_period = 37; draw_symmetry = 3;
    for (i = 0; i < 5; ++i) {
        ref_srand(0x4125); plot_calls = cycle_calls = 0; plot_crc = 0xffffffffUL;
        plot_color = 3; reset_frame(); diamonds(1);
        for (n = 0; n < 3; ++n) {
            before = rng_calls;
            diamond_call(poisons[i]);
            fprintf(out, "diamonds,%d,%u,%lu,%d,%d,%d,%d\n", poisons[i], n,
                rng_calls-before, diamonds_origin_x, diamonds_origin_y, diamonds_step_x, diamonds_step_y);
        }
        ref_srand(0x4128); plot_calls = cycle_calls = 0; plot_crc = 0xffffffffUL;
        plot_color = 3; reset_frame(); wedges(1);
        for (n = 0; n < 3; ++n) {
            before = rng_calls;
            wedge_call(poisons[i]);
            fprintf(out, "wedges,%d,%u,%lu,%d,%d,%d,%d\n", poisons[i], n,
                rng_calls-before, wedges_vertex_x[0], wedges_vertex_y[0], wedges_step_x[0], wedges_step_y[0]);
        }
    }
    fclose(out); fclose(full_log); fclose(first_log);
    exit(0);
}
