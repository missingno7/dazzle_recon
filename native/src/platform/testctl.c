#include "dazzle.h"
#include "dvdt.h"
#include "platform.h"
#include "testctl.h"
#include <ctype.h>
#include <inttypes.h>
#include <math.h>

/* Bounded command-line fixtures, no scheduler/framework. Times are absolute
 * virtual nanoseconds from initialization, including the original startup. */
static struct { uint64_t ns; uint16_t key; } keys[256];
static struct { uint64_t ns; const char *path; int written; } snapshots[64];
static struct { uint64_t ns; const char *path; int written; } states[64];
static unsigned state_count;
static unsigned key_count, next_key, snapshot_count, last_fade;
static uint64_t limit_ns, work_count;
static const char *capture_path;
static int stopped, failed, captured, finished;

static uint32_t fnv(uint32_t h, const void *data, size_t size)
{
    const uint8_t *p = data;
    while (size--) h = (h ^ *p++) * UINT32_C(16777619);
    return h;
}

static void capture(void)
{
    if (!captured && capture_path) {
        captured = 1;
        if (!dz_save_screenshot(capture_path)) {
            fprintf(stderr, "Screenshot: %s\n", SDL_GetError());
            failed = 1;
        }
    }
}

void dz_testctl_limits(uint64_t frames, uint64_t ns, const char *screenshot)
{
    /* Frames are simulation retraces, independent of the monitor refresh rate. */
    uint64_t seconds = frames / 70u;
    uint64_t fraction = ((frames % 70u) * DZ_NS_PER_SECOND + 69u) / 70u;
    uint64_t frame_ns = seconds > (UINT64_MAX - fraction) / DZ_NS_PER_SECOND ? UINT64_MAX :
        seconds * DZ_NS_PER_SECOND + fraction;
    limit_ns = ns;
    stopped = failed = captured = finished = 0;
    if (frames && (!limit_ns || frame_ns < limit_ns)) limit_ns = frame_ns;
    capture_path = screenshot;
}

static uint16_t key_word(char *name)
{
    size_t length = strlen(name);
    if (length >= 3 && name[0] == '\'' && name[length - 1] == '\'') {
        name[length - 1] = 0; ++name;
    }
    for (char *p = name; *p; ++p) *p = (char)toupper((unsigned char)*p);
    SDL_Keymod mods = SDL_KMOD_NONE;
    for (;;) {
        if (!strncmp(name, "ALT+", 4)) { mods |= SDL_KMOD_ALT; name += 4; }
        else if (!strncmp(name, "CTRL+", 5)) { mods |= SDL_KMOD_CTRL; name += 5; }
        else if (!strncmp(name, "SHIFT+", 6)) { mods |= SDL_KMOD_SHIFT; name += 6; }
        else break;
    }
    SDL_Scancode code = !strcmp(name, "ESC") ? SDL_SCANCODE_ESCAPE :
        !strcmp(name, "ENTER") ? SDL_SCANCODE_RETURN : SDL_GetScancodeFromName(name);
    if (!strcmp(name, "+")) { code = SDL_SCANCODE_EQUALS; mods |= SDL_KMOD_SHIFT; }
    return dz_key_from_sdl(code, mods);
}

int dz_testctl_script(const char *path)
{
    FILE *file = fopen(path, "rt");
    if (!file) { fprintf(stderr, "Cannot open key script: %s\n", path); return 0; }
    char line[256], name[80], extra;
    unsigned line_number = 0;
    int ok = 1;
    while (fgets(line, sizeof line, file)) {
        ++line_number;
        char *comment = strchr(line, '#');
        if (comment) *comment = 0;
        char *p = line;
        while (isspace((unsigned char)*p)) ++p;
        if (!*p) continue;
        double seconds;
        if (sscanf(p, "%lf %79s %c", &seconds, name, &extra) != 2 ||
            !(seconds >= 0 && seconds <= 1e9) || key_count == 256) { ok = 0; break; }
        uint64_t ns = (uint64_t)llround(seconds * (double)DZ_NS_PER_SECOND);
        uint16_t key = key_word(name);
        if (!key || (key_count && ns < keys[key_count - 1].ns)) { ok = 0; break; }
        keys[key_count].ns = ns; keys[key_count++].key = key;
    }
    if (ferror(file)) ok = 0;
    fclose(file);
    if (!ok) fprintf(stderr, "Invalid key script %s line %u\n", path, line_number);
    return ok;
}

int dz_testctl_snapshot(uint64_t ns, const char *path)
{
    if (snapshot_count == 64) { fprintf(stderr, "Too many snapshots (maximum 64)\n"); return 0; }
    snapshots[snapshot_count].ns = ns;
    snapshots[snapshot_count++].path = path;
    return 1;
}

int dz_testctl_state(uint64_t ns, const char *path)
{
    if (state_count == 64) return 0;
    states[state_count].ns = ns; states[state_count++].path = path;
    return 1;
}

static void state(unsigned index)
{
    FILE *file = fopen(states[index].path, "wb");
    if (!file) { failed = 1; return; }
    /* Separate from the unchanged canonical snapshot format. Full aperture,
     * both palettes, RNG state/calls, virtual work/time and draw diagnostics. */
    if (fprintf(file, "DAZZLE-STATE 1\nns=%" PRIu64 "\nticks=%" PRIu64 "\nwork=%" PRIu64
        "\nrng=%" PRIu64 "\nrng_state=%" PRIu32 "\ndraw=%d,%d,%u,%u,%u\n"
        "image=%u,%d\nfade=%u\npage=%u\nbackend=%d\nedge=%u,%u\nEND\n",
        dz_virtual_ns(), dz_virtual_ticks(), work_count, dz_rand_calls(), dz_random_state(),
        draw_x, draw_y, plot_color, draw_symmetry, draw_delay_count, current_image_index,
        image_work_count, last_fade, page_paragraph, video_backend, symmetry_edge, symmetry_y_edge) < 0 ||
        fwrite(&dz_video, 1, sizeof dz_video, file) != sizeof dz_video) failed = 1;
    if (fclose(file)) failed = 1;
    states[index].written = 1;
}

static void snapshot(unsigned index)
{
    /* Palette is text hex; the indexed visible frame follows END\n verbatim.
     * Text mode records the canonical 80x25 character/attribute cells. */
    uint8_t frame[640 * 400];
    size_t size = (size_t)dz_video.width * (size_t)dz_video.height;
    if (dz_video.mode == DZ_MODE_TEXT) {
        size = sizeof dz_video.text;
        memcpy(frame, dz_video.text, size);
    } else {
        uint32_t capacity = DZ_VRAM_PLANE_BYTES * (uint32_t)dz_video.pixels_per_byte;
        for (unsigned y = 0; y < (unsigned)dz_video.height; ++y) {
            unsigned row = y > dz_video.line_compare ? y - dz_video.line_compare - 1u : dz_video.display_start_row + y;
            for (unsigned x = 0; x < (unsigned)dz_video.width; ++x)
                frame[y * (unsigned)dz_video.width + x] = dz_video.vram[(row * (unsigned)dz_video.width + x) % capacity];
        }
    }
    const uint8_t *palette = dz_video.mode == DZ_MODE_EGA_350 ? dz_video.ega_attr : &dz_video.dac[0][0];
    size_t palette_size = dz_video.mode == DZ_MODE_EGA_350 ? 16u : sizeof dz_video.dac;
    char hex[1539];
    for (size_t i = 0; i < palette_size; ++i) snprintf(hex + i * 2, 3, "%02x", palette[i]);
    if (dz_video.mode == DZ_MODE_EGA_350) snprintf(hex + 32, 3, "%02x", dz_video.ega_border);
    char header[2304];
    int length = snprintf(header, sizeof header,
        "DAZZLE-SNAPSHOT 1\nns=%" PRIu64 "\nmode=%d\nwidth=%d\nheight=%d\nbytes=%zu\n"
        "start=%u\nline_compare=%u\nimage=%u\nfade=%u\nrng=%" PRIu64 "\nticks=%" PRIu64
        "\nwork=%" PRIu64 "\nimage_work=%d\nsemi_auto=%u\npan=%u\npalette=%s\nEND\n",
        dz_virtual_ns(), dz_video.mode, dz_video.width, dz_video.height, size,
        dz_video.display_start_row, dz_video.line_compare, current_image_index, last_fade,
        dz_rand_calls(), dz_virtual_ticks(), work_count, image_work_count, semi_auto_mode, display_pan_state, hex);
    uint32_t hash = fnv(fnv(UINT32_C(2166136261), header, (size_t)length), frame, size);
    FILE *file = fopen(snapshots[index].path, "wb");
    if (!file) failed = 1;
    else {
        if (fwrite(header, 1, (size_t)length, file) != (size_t)length || fwrite(frame, 1, size, file) != size) failed = 1;
        if (fclose(file)) failed = 1;
    }
    if (failed) fprintf(stderr, "Snapshot write failed: %s\n", snapshots[index].path);
    printf("snapshot ns=%" PRIu64 " hash=%08" PRIx32 " image=%u fade=%u rng=%" PRIu64
        " ticks=%" PRIu64 " work=%" PRIu64 "\n", dz_virtual_ns(), hash, current_image_index,
        last_fade, dz_rand_calls(), dz_virtual_ticks(), work_count);
    fflush(stdout);
    snapshots[index].written = 1;
}

uint64_t dz_testctl_deadline(uint64_t target)
{
    if (!stopped && limit_ns && limit_ns < target) target = limit_ns;
    if (next_key < key_count && keys[next_key].ns < target) target = keys[next_key].ns;
    for (unsigned i = 0; i < snapshot_count; ++i)
        if (!snapshots[i].written && snapshots[i].ns < target) target = snapshots[i].ns;
    for (unsigned i = 0; i < state_count; ++i)
        if (!states[i].written && states[i].ns < target) target = states[i].ns;
    return target;
}
void dz_testctl_poll(void)
{
    uint64_t now = dz_virtual_ns();
    while (next_key < key_count && keys[next_key].ns <= now) {
        if (!dz_input_push(keys[next_key++].key)) { fprintf(stderr, "Script keyboard buffer full\n"); failed = 1; }
        if (dz_keyboard_irq_handler) dz_keyboard_irq_handler();
    }
    for (unsigned i = 0; i < snapshot_count; ++i)
        if (!snapshots[i].written && snapshots[i].ns <= now) snapshot(i);
    for (unsigned i = 0; i < state_count; ++i)
        if (!states[i].written && states[i].ns <= now) state(i);
    if (!stopped && limit_ns && now >= limit_ns) {
        capture();
        stopped = 1;
    }
    if (stopped) {
        exit_requested = input_pending = 1;
        countdown_ticks = 0;
    }
}
void dz_testctl_work(uint32_t units) { work_count += units; }
void dz_testctl_fade(unsigned key) { last_fade = key; }
int dz_testctl_stopped(void) { return stopped; }
int dz_testctl_failed(void) { return failed; }
void dz_testctl_finish(void)
{
    if (finished) return;
    finished = 1;
    capture();
    for (unsigned i = 0; i < snapshot_count; ++i) if (!snapshots[i].written) {
        fprintf(stderr, "Snapshot time not reached: %s\n", snapshots[i].path); failed = 1;
    }
    for (unsigned i = 0; i < state_count; ++i) if (!states[i].written) failed = 1;
}
