#define REF_NATIVE
#include "ref/primitives_driver.c"
#include "ref/native_util.h"

int main(void)
{
    FILE *out = fopen(ref_path("../../../build/workers/n-diff/native-primitives.txt"), "w+");
    unsigned failures;
    assert(out);
    primitive_cases(out);
    failures = ref_compare_text(out, ref_path("data/primitives.txt"));
    fclose(out);
    return failures ? 1 : 0;
}
