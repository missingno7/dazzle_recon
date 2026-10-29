/* Bounded geometry iterators: negative radii need not ever finish. */
#ifdef REF_NATIVE
#include "compat.h"
#define ref_srand dz_srand
#define ref_calls() ((unsigned long)dz_rand_calls())
#else
#include "imgmodel.h"
#define ref_calls() rng_calls
#endif
#include <stdio.h>
#include <stdlib.h>


static void primitive_cases(FILE *out)
{
    static RefInt coordinates[][2] = {
        {0,0}, {-1,1}, {181,-181}, {-32768,32767}, {32767,-32768}, {-20000,-30000}, {7,11}
    };
    static RefInt lines_to_test[][4] = {
        {0,0,0,0}, {0,0,10,0}, {10,0,-10,-7}, {-5,-7,9,11},
        {-32768,32767,32767,-32768}, {0,0,20000,30000}, {32760,-32760,-32760,32760}
    };
    static RefInt radii[][2] = {
        {0,0}, {1,0}, {-1,0}, {-10,5}, {181,181}, {300,0},
        {30000,20000}, {-30000,32767}, {32767,32767}, {200,1}
    };
    unsigned adapter, mode, i, step;
    RefInt done;
    for (adapter = 4; adapter <= 8; adapter += 4) {
        video_adapter = adapter;
        screen_half_width = adapter == 4 ? 320 : 160;
        screen_half_height = adapter == 4 ? 350 : 200;
        color_count = adapter == 4 ? 16 : 256;
        color_index_mask = color_count - 1;
        for (mode = 0; mode <= 8; ++mode) {
            ref_srand(0x5eed);
            plot_color = 254; ega_color_substep = 0;
            for (i = 0; i < sizeof(coordinates)/sizeof(coordinates[0]); ++i) {
                draw_x = coordinates[i][0]; draw_y = coordinates[i][1];
                choose_color(mode);
                fprintf(out, "color,%u,%u,%u,%u,%u,%lu\n", adapter, mode, i,
                        plot_color, ega_color_substep, ref_calls());
            }
        }
        for (i = 0; i < sizeof(lines_to_test)/sizeof(lines_to_test[0]); ++i) {
            draw_x = lines_to_test[i][0]; draw_y = lines_to_test[i][1];
            for (step = 0; step < 256; ++step) {
                done = line_step(lines_to_test[i][2], lines_to_test[i][3], step == 0);
                fprintf(out, "line,%u,%u,%u,%d,%d,%d\n", adapter, i, step, draw_x, draw_y, done);
                if (done) break;
            }
        }
        for (i = 0; i < sizeof(radii)/sizeof(radii[0]); ++i) {
            for (step = 0; step < 512; ++step) {
                done = ellipse_step(radii[i][0], radii[i][1], step == 0);
                fprintf(out, "ellipse,%u,%u,%u,%d,%d,%d,%ld,%ld,%ld\n", adapter, i, step,
                        draw_x, draw_y, done, (long)ellipse_step_error,
                        (long)ellipse_step_gradient_x, (long)ellipse_step_gradient_y);
                if (done) break;
            }
        }
    }
}

#ifndef REF_NATIVE
void main(int argc, char far * far *argv)
{
    FILE *out = fopen("PRIMS.TXT", "w");
    (void)argc; (void)argv;
    if (!out) exit(2);
    primitive_cases(out);
    fclose(out);
    exit(0);
}
#endif
