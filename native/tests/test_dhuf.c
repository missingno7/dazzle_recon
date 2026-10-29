/* Manual extraction against the immutable distribution files when present. */
#include "dazzle.h"

#include <assert.h>
#include <stdio.h>

#define ORIGINAL_EXE "D:\\Prog\\dazzle_recon\\assets\\DAZZLE.EXE"
#define ORIGINAL_DOC "D:\\Prog\\dazzle_recon\\assets\\DAZZLE.DOC"

int main(void)
{
    FILE *expected = fopen(ORIGINAL_DOC, "rb");
    FILE *executable = fopen(ORIGINAL_EXE, "rb");
    if (expected == NULL || executable == NULL) {
        if (expected != NULL) fclose(expected);
        if (executable != NULL) fclose(executable);
        puts("manual extraction skipped: distribution oracle is absent");
        return 0;
    }
    fclose(executable);

    dz_options.original_exe = ORIGINAL_EXE;
    exit_requested = 0;
    extract_manual();
    assert(exit_requested != 0);

    FILE *actual = fopen("DAZZLE.DOC", "rb");
    assert(actual != NULL);
    size_t compared = 0;
    for (;;) {
        int left = fgetc(actual);
        int right = fgetc(expected);
        assert(left == right);
        if (left == EOF)
            break;
        ++compared;
    }
    assert(compared == 84927);
    fclose(actual);
    fclose(expected);
    assert(remove("DAZZLE.DOC") == 0);
    printf("manual extraction matched %zu oracle bytes\n", compared);
    return 0;
}
