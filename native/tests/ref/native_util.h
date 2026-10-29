#ifndef REF_NATIVE_UTIL_H
#define REF_NATIVE_UTIL_H
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

/* __FILE__ is absolute in CMake builds; no dependency on CTest's working dir. */
static inline const char *ref_path(const char *suffix)
{
    static char paths[32][1024];
    static unsigned slot;
    char *path = paths[slot++ % 32];
    const char *file = __FILE__;
    const char *last = strrchr(file, '/'), *back = strrchr(file, '\\');
    size_t length;
    if (!last || (back && back > last)) last = back;
    assert(last);
    length = (size_t)(last - file + 1);
    assert(length + strlen(suffix) < 1024);
    memcpy(path, file, length);
    strcpy(path + length, suffix);
    return path;
}

__attribute__((constructor)) static void ref_prepare(void)
{
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
    (void)_mkdir(ref_path("../../../build"));
    (void)_mkdir(ref_path("../../../build/workers"));
    (void)_mkdir(ref_path("../../../build/workers/n-diff"));
#else
    (void)mkdir(ref_path("../../../build"), 0777);
    (void)mkdir(ref_path("../../../build/workers"), 0777);
    (void)mkdir(ref_path("../../../build/workers/n-diff"), 0777);
#endif
}

static inline int ref_line(FILE *file, char *line, size_t capacity)
{
    while (fgets(line, (int)capacity, file)) {
        line[strcspn(line, "\r\n")] = 0;
        if (*line && *line != '#') return 1;
    }
    return 0;
}

static inline unsigned ref_compare_text(FILE *actual, const char *expected_path)
{
    FILE *expected = fopen(expected_path, "r");
    char a[12000], b[12000];
    unsigned failures = 0, count = 0;
    if (!expected) { fprintf(stderr, "cannot read reference: %s\n", expected_path); return 1; }
    rewind(actual);
    while (ref_line(expected, b, sizeof b)) {
        ++count;
        if (!ref_line(actual, a, sizeof a)) strcpy(a, "<missing>");
        if (strcmp(a, b)) {
            size_t position = 0;
            while (a[position] && b[position] && a[position] == b[position]) ++position;
            if (failures >= 16) { ++failures; continue; }
            if (strlen(a) < 512 && strlen(b) < 512)
                fprintf(stderr, "reference row %u\n DOS: %s\n host: %s\n", count, b, a);
            else
                fprintf(stderr, "reference row %u first text difference at byte %zu\n DOS: %.100s\n host: %.100s\n",
                        count, position, b, a);
            ++failures;
        }
    }
    if (ref_line(actual, a, sizeof a)) {
        fprintf(stderr, "unexpected native row: %s\n", a);
        ++failures;
    }
    fclose(expected);
    printf("%u reference rows, %u mismatches\n", count, failures);
    return failures;
}

static inline void ref_compare_plots(const char *kind, const char *native_log)
{
    char receipt[160], command[4096];
    FILE *cached;
    int have_cache;
    snprintf(receipt, sizeof receipt, "../../../build/workers/n-diff/%s-capture.json", kind);
    cached = fopen(ref_path(receipt), "r");
    have_cache = cached != NULL;
    if (cached) fclose(cached);
    snprintf(command, sizeof command, "python -B -I \"%s\" %s --compare \"%s\"",
             ref_path("capture.py"), kind, native_log);
    if (!have_cache && !getenv("REF_REGENERATE_LOGS")) {
        /* A DOS capture can take minutes; do not start an orphaned DOS runner
         * when CTest will terminate this test after thirty seconds. */
        fprintf(stderr, "Full DOS log is not cached. Regenerate/compare on demand:\n%s\n", command);
        return;
    }
    fflush(stdout);
    (void)system(command);
}
#endif
