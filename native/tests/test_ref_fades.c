#define REF_NATIVE
#include "ref/fade_driver.c"
#include "ref/native_util.h"

int main(void)
{
    unsigned failures;
    summary = fopen(ref_path("../../../build/workers/n-diff/native-fades.txt"), "w+");
    full_log = fopen(ref_path("../../../build/workers/n-diff/native-fades.bin"), "wb");
    first_log = fopen(ref_path("../../../build/workers/n-diff/native-fades.log"), "w+");
    assert(summary && full_log && first_log);
    framebuffer = malloc(224000); assert(framebuffer);
    make_crc_table(); initialize_math_tables(); fade_cases();
    failures = ref_compare_text(summary, ref_path("data/fades.txt"));
    fclose(summary); fclose(full_log); fclose(first_log);
    free(framebuffer); free(palette_buffer); free(ball_bitmap_pixels); free(linewipe_history);
    if (failures && !getenv("REF_NO_CAPTURE")) {
        ref_compare_plots("fades", ref_path("../../../build/workers/n-diff/native-fades.bin"));
    }
    return failures ? 1 : 0;
}
