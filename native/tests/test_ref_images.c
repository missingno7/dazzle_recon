/* DOS/native differential: the driver model is deliberately shared. */
#define REF_NATIVE
#include "ref/images_driver.c"
#include "ref/native_util.h"

int main(void)
{
    FILE *scenarios = fopen(ref_path("scenarios/images.tsv"), "r");
    unsigned cases = 0, failures;
    const char *log_path = ref_path("../../../build/workers/n-diff/native-plot.bin");
    assert(scenarios);
    summary = fopen(ref_path("../../../build/workers/n-diff/native-images.txt"), "w+");
    first_log = fopen(ref_path("../../../build/workers/n-diff/native-first.log"), "w+");
    full_log = fopen(log_path, "wb");
    assert(summary && first_log && full_log);
    framebuffer = malloc(448000);
    assert(framebuffer);
    make_crc_table();
    initialize_math_tables();
    while (!run_one(scenarios, cases)) ++cases;
    assert(cases >= IMAGE_COUNT);
    fclose(scenarios); fclose(full_log); fclose(first_log);
    failures = ref_compare_text(summary, ref_path("data/images.txt"));
    fclose(summary);
    free(framebuffer); free(palette_buffer); free(linewipe_history); free(ball_bitmap_pixels);
    if (failures && !getenv("REF_NO_CAPTURE")) {
        ref_compare_plots("images", log_path);
    }
    return failures ? 1 : 0;
}
