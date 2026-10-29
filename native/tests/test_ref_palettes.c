#define REF_NATIVE
#include "ref/palette_driver.c"
#include "ref/native_util.h"

int main(void)
{
    FILE *scenarios = fopen(ref_path("scenarios/palettes.tsv"), "r");
    unsigned cases = 0, failures;
    assert(scenarios);
    output = fopen(ref_path("../../../build/workers/n-diff/native-palettes.txt"), "w+");
    buffer = malloc(MAX_BUFFER);
    assert(output && buffer);
    while (!run_case(scenarios)) ++cases;
    assert(cases >= 11);
    failures = ref_compare_text(output, ref_path("data/palettes.txt"));
    fclose(scenarios); fclose(output); free(buffer);
    return failures ? 1 : 0;
}
