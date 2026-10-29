#define REF_NATIVE
#include "ref/pcx_driver.c"
#include "ref/native_util.h"

int main(void)
{
    FILE *out = fopen(ref_path("../../../build/workers/n-diff/native-pcx.txt"), "w+");
    unsigned failures;
    assert(out);
    pcx_cases(out, ref_path("../../../build/workers/n-diff/row.bin"));
    failures = ref_compare_text(out, ref_path("data/pcx.txt"));
    fclose(out);
    return failures ? 1 : 0;
}
