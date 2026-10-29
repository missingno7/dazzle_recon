#include "dazzle.h"
#include "platform/platform.h"
#include "platform/dosi_support.h"
#include "platform/testctl.h"
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

/* DOSI interprets /? and bare * itself; MinGW must not expand them as paths. */
#ifdef __MINGW32__
int _dowildcard = 0;
#endif

static uint64_t number(const char *text, uint64_t maximum)
{
    char *end;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 0);
    if (!*text || *text == '-' || *end || errno || value > maximum) {
        fprintf(stderr, "Invalid numeric option: %s\n", text); exit(2);
    }
    return (uint64_t)value;
}
static double positive(const char *text)
{
    char *end; errno = 0;
    double value = strtod(text, &end);
    if (!*text || *end || errno || !(value > 0.0) || value > 1000000.0) {
        fprintf(stderr, "Invalid positive option: %s\n", text); exit(2);
    }
    return value;
}
static void window_size(const char *text)
{
    char *end;
    errno = 0;
    unsigned long w = strtoul(text, &end, 10), h = 0;
    if (*text >= '0' && *text <= '9' && *end == 'x' && end[1] >= '0' && end[1] <= '9')
        h = strtoul(end + 1, &end, 10);
    if (*end || errno || !w || !h || w > 32768 || h > 32768) {
        fprintf(stderr, "Invalid window size: %s\n", text); exit(2);
    }
    dz_display.window_width = (int)w; dz_display.window_height = (int)h;
}
static void report_rng(void)
{
    printf("rng=%" PRIu64 " ticks=%" PRIu64 "\n", dz_rand_calls(), dz_virtual_ticks());
}
static void usage(void)
{
    puts("DAZZLE 5.0 native\n"
         "  DOS options: /? help, /$ order form, /Q quiet, ...\n"
         "  --slice sierpinski|lines   --mode vga400|vga200|ega350|ega700\n"
         "  --deterministic --seed N   --headless --speed X\n"
         "  --renderer auto|indexed|rgba --present original|crop|stretch\n"
         "  --window WxH --fullscreen\n"
         "  --steps N --frames N      --exit-after SECONDS\n"
         "  --script FILE             --snapshot SECONDS FILE (repeatable)\n"
         "  --original PATH           --screenshot FILE.bmp --trace-rng\n"
         "Without --slice runs the original program. Escape closes it; Alt+Enter toggles fullscreen.");
}
static const char *default_original(const char *program)
{
    static char path[4096];
    const char *slash = strrchr(program, '/'), *backslash = strrchr(program, '\\');
    if (backslash && (!slash || backslash > slash)) slash = backslash;
    if (slash && (size_t)(slash - program) + 12 < sizeof path) {
        size_t length = (size_t)(slash - program) + 1;
        memcpy(path, program, length);
        strcpy(path + length, "DAZZLE.EXE");
        /* On Windows the native dazzle.exe has the same case-insensitive name;
         * accept the sibling only if it is actually the original MZ program. */
        FILE *file = fopen(path, "rb");
        if (file) {
            unsigned char header[64];
            size_t count = fread(header, 1, sizeof header, file); fclose(file);
            if (count == sizeof header && header[0] == 'M' && header[1] == 'Z' &&
                header[0x18] < 0x40) return path;
        }
    }
    return "assets/DAZZLE.EXE";
}
int main(int argc, char **argv)
{
    const char *slice = NULL, *mode = "vga400", *screenshot = NULL, *script = NULL;
    uint64_t steps = 0, frames = 0, exit_ns = 0;
    int explicit_seed = 0, trace_rng = 0;
    char **dos_argv = calloc((size_t)argc + 1, sizeof *dos_argv);
    if (!dos_argv) return 1;
    int dos_argc = 1;
    dos_argv[0] = argv[0];
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (!strcmp(arg, "--help")) { usage(); return 0; }
        if (strncmp(arg, "--", 2)) { dos_argv[dos_argc++] = argv[i]; continue; }
        if (!strcmp(arg, "--deterministic")) { dz_options.deterministic = 1; continue; }
        if (!strcmp(arg, "--headless")) { dz_options.headless = 1; continue; }
        if (!strcmp(arg, "--fullscreen")) { dz_display.fullscreen = 1; continue; }
        if (!strcmp(arg, "--trace-rng")) { trace_rng = 1; continue; }
        if (i + 1 >= argc) { fprintf(stderr, "Missing value: %s\n", arg); return 2; }
        const char *value = argv[++i];
        if (!strcmp(arg, "--slice")) slice = value;
        else if (!strcmp(arg, "--renderer")) {
            if (!strcmp(value, "auto")) dz_display.renderer = DZ_RENDERER_AUTO;
            else if (!strcmp(value, "indexed")) dz_display.renderer = DZ_RENDERER_INDEXED;
            else if (!strcmp(value, "rgba")) dz_display.renderer = DZ_RENDERER_RGBA;
            else { fprintf(stderr, "Unknown renderer: %s\n", value); return 2; }
        }
        else if (!strcmp(arg, "--present")) {
            if (!strcmp(value, "original")) dz_display.presentation = DZ_PRESENT_ORIGINAL;
            else if (!strcmp(value, "crop")) dz_display.presentation = DZ_PRESENT_CROP;
            else if (!strcmp(value, "stretch")) dz_display.presentation = DZ_PRESENT_STRETCH;
            else { fprintf(stderr, "Unknown presentation: %s\n", value); return 2; }
        }
        else if (!strcmp(arg, "--window")) window_size(value);
        else if (!strcmp(arg, "--mode")) mode = value;
        else if (!strcmp(arg, "--seed")) { dz_options.seed = (uint32_t)number(value, UINT32_MAX); explicit_seed = 1; }
        else if (!strcmp(arg, "--speed")) dz_options.speed = positive(value);
        else if (!strcmp(arg, "--steps")) steps = number(value, UINT64_MAX);
        else if (!strcmp(arg, "--frames")) frames = number(value, UINT64_MAX);
        else if (!strcmp(arg, "--exit-after")) exit_ns = (uint64_t)llround(positive(value) * (double)DZ_NS_PER_SECOND);
        else if (!strcmp(arg, "--original")) dz_options.original_exe = value;
        else if (!strcmp(arg, "--script")) script = value;
        else if (!strcmp(arg, "--snapshot")) {
            if (i + 1 >= argc) { fprintf(stderr, "Missing snapshot filename\n"); return 2; }
            uint64_t ns = !strcmp(value, "0") ? 0 : (uint64_t)llround(positive(value) * (double)DZ_NS_PER_SECOND);
            if (!dz_testctl_snapshot(ns, argv[++i])) return 2;
        }
        else if (!strcmp(arg, "--screenshot")) screenshot = value;
        else { fprintf(stderr, "Unknown option: %s\n", arg); return 2; }
    }
    video_adapter = 8;
    if (!strcmp(mode, "vga200")) bios_resolution_mode = 1;
    else if (!strcmp(mode, "ega350")) { video_adapter = 4; bios_resolution_mode = 1; }
    else if (!strcmp(mode, "ega700")) video_adapter = 4;
    else if (strcmp(mode, "vga400")) { fprintf(stderr, "Unknown mode: %s\n", mode); return 2; }
    if (slice && strcmp(slice, "sierpinski") && strcmp(slice, "lines")) { fprintf(stderr, "Unknown slice: %s\n", slice); return 2; }
    if (!explicit_seed && !dz_options.deterministic) dz_options.seed = (uint16_t)time(NULL);
    if (!dz_options.original_exe) dz_options.original_exe = default_original(argv[0]);
    if (!dz_initialize()) { fprintf(stderr, "SDL initialization: %s\n", SDL_GetError()); return 1; }
    atexit(dz_shutdown);
    if (script && !dz_testctl_script(script)) return 2;
    if (!slice) {
        dz_dosi_set_limits(frames, exit_ns, screenshot);
        if (trace_rng) atexit(report_rng);
        atexit(dz_dosi_finish);
        int result = dazzle_main(dos_argc, dos_argv);
        dz_dosi_finish();
        free(dos_argv);
        return dz_dosi_output_failed() ? 1 : result;
    }
    free(dos_argv);
    if (!dz_slice_begin(slice)) return 1;
    uint64_t first_frame = dz_presented_frames(), start = dz_virtual_ns(), done = 0;
    while (!exit_requested && (!steps || done < steps) &&
        (!frames || dz_presented_frames() - first_frame < frames) &&
        (!exit_ns || dz_virtual_ns() - start < exit_ns)) {
        dz_slice_step(); ++done;
    }
    int result = 0;
    if (screenshot && !dz_save_screenshot(screenshot)) {
        fprintf(stderr, "Screenshot: %s\n", SDL_GetError()); result = 1;
    }
    printf("slice=%s seed=%" PRIu32 " steps=%" PRIu64 " ticks=%" PRIu64
        " rng=%" PRIu64 " framebuffer=%08" PRIx32 " palette=%08" PRIx32 "\n",
        slice, dz_options.seed, done, dz_virtual_ticks(), dz_rand_calls(), dz_framebuffer_hash(), dz_palette_hash());
    dz_slice_end();
    return result;
}
