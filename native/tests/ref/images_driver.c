#include "imgmodel.h"

#ifndef REF_NATIVE
void main(int argc, char far * far *argv)
{
    FILE *scenarios;
    unsigned i;

    full_log = fopen("PLOT.BIN", "wb");
    first_log = fopen("PLOT.LOG", "w");
    summary = fopen("IMAGE.TXT", "w");
    scenarios = fopen("IMAGES.TSV", "r");
    (void)argc;
    (void)argv;
    if (!full_log || !first_log || !summary || !scenarios) exit(2);
    fprintf(summary, "# name,seed,steps,tick,symmetry,adapter,plot_calls,stream_crc,rand_calls,cycle_calls,fb_init,fb_half,fb_final,palette_ticks\n");
    fflush(summary);
    make_crc_table();
    framebuffer = (unsigned char huge *)farmalloc(224000UL);
    if (!framebuffer) exit(3);
    initialize_math_tables();
    i = 0;
    while (!run_one(scenarios, i)) ++i;
    fclose(scenarios);
    fclose(full_log);
    fclose(first_log);
    fclose(summary);
    printf("wrote IMAGE.TXT, PLOT.LOG, PLOT.BIN\n");
    exit(0);
}

#endif
