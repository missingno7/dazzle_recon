#include "dazzle.h"
#include "dzconio.h"
#include "platform.h"
#include "dosi_support.h"
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>

/* cprintf currently writes the emulated console only. Echo in this module until
 * conio's owner provides a shared policy; do not echo config-file writes. */
int dz_dosi_printf(const char *format, ...)
{
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0) { va_end(args); return length; }
    char *text = malloc((size_t)length + 1);
    if (!text) { va_end(args); return -1; }
    vsnprintf(text, (size_t)length + 1, format, args);
    va_end(args);
    fputs(text, stdout);
    fflush(stdout);
    /* DOS stdio translated LF to CRLF; cprintf's existing CRs are harmless. */
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p == '\n') putch('\r');
        putch(*p);
    }
    free(text);
    return length;
}
char *dz_dosi_upper(char *text)
{
    for (unsigned char *p = (unsigned char *)text; *p; ++p) *p = (unsigned char)toupper(*p);
    return text;
}
char *dz_dosi_lower(char *text)
{
    for (unsigned char *p = (unsigned char *)text; *p; ++p) *p = (unsigned char)tolower(*p);
    return text;
}
uint16_t dz_dosi_le16(const uint8_t *bytes) { return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8)); }
int dz_dosi_file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    fclose(file); return 1;
}
int dz_dosi_find_config(const char *name, char *path, size_t capacity)
{
    /* First beside the resource executable, then through the native PATH. */
    const char *exe = executable_path;
    const char *slash = strrchr(exe, '/'), *backslash = strrchr(exe, '\\');
    if (backslash && (!slash || backslash > slash)) slash = backslash;
    if (slash) {
        size_t prefix = (size_t)(slash - exe) + 1;
        if (prefix + strlen(name) < capacity) {
            memcpy(path, exe, prefix); strcpy(path + prefix, name);
            if (dz_dosi_file_exists(path)) return 1;
        }
    }
    const char *search = getenv("PATH");
    while (search && *search) {
        const char *end = strchr(search, ';');
        size_t length = end ? (size_t)(end - search) : strlen(search);
        if (length + strlen(name) + 2 <= capacity) {
            memcpy(path, search, length); path[length] = '/';
            strcpy(path + length + 1, name);
            if (dz_dosi_file_exists(path)) return 1;
        }
        search = end ? end + 1 : NULL;
    }
    return 0;
}
int dz_dosi_close_configuration(void)
{
    int result = fclose(configuration_file_stream);
    configuration_file_stream = NULL;
    return result;
}

/* INT 33h's callback mask: motion, then press/release for left/right/middle.
 * dzplat's callback has no event payload, so obtain counters and retain button
 * state here before invoking the unchanged DVDT body. No SDL calls are needed. */
static uint16_t mouse_mask;
static unsigned previous_mouse_buttons;
static void masked_mouse_callback(void)
{
    int dx, dy;
    unsigned buttons;
    dz_mouse_counters(&dx, &dy, &buttons);
    unsigned changed = previous_mouse_buttons ^ buttons;
    uint16_t events = (uint16_t)((dx || dy) ? 1u : 0u);
    for (unsigned i = 0; i < 3; ++i) {
        unsigned bit = 1u << i;
        if (changed & bit) events |= (uint16_t)((buttons & bit ? 2u : 4u) << (i * 2u));
    }
    previous_mouse_buttons = buttons;
    if (events & mouse_mask) mouse_pending_callback();
}
void dz_dosi_register_mouse(uint16_t mask)
{
    mouse_mask = mask;
    dz_mouse_counters(NULL, NULL, &previous_mouse_buttons);
    dz_mouse_event_handler = mask ? masked_mouse_callback : NULL;
}

static const int diagnostic_signals[] = {SIGABRT, SIGFPE, SIGILL, SIGSEGV};
static void (*saved_signals[4])(int);
static int signals_installed;
static void native_signal(int number) { signal_diagnostic((int16_t)number, 0, NULL); }
void dz_dosi_install_signals(void)
{
    if (signals_installed) return;
    for (unsigned i = 0; i < 4; ++i) saved_signals[i] = signal(diagnostic_signals[i], native_signal);
    signals_installed = 1;
}
void dz_dosi_restore_signals(void)
{
    if (!signals_installed) return;
    for (unsigned i = 0; i < 4; ++i)
        if (saved_signals[i] != SIG_ERR) signal(diagnostic_signals[i], saved_signals[i]);
    signals_installed = 0;
}

static uint64_t limit_frames, limit_ns, first_frame, first_ns;
static const char *capture_path;
static int captured, output_failed;
void dz_dosi_set_limits(uint64_t frames, uint64_t nanoseconds, const char *screenshot)
{
    limit_frames = frames; limit_ns = nanoseconds;
    first_frame = dz_presented_frames(); first_ns = dz_virtual_ns();
    capture_path = screenshot; captured = output_failed = 0;
}
static void capture(void)
{
    if (capture_path && !captured) {
        captured = 1;
        if (!dz_save_screenshot(capture_path)) {
            fprintf(stderr, "Screenshot: %s\n", SDL_GetError()); output_failed = 1;
        }
    }
}
void dz_dosi_checkpoint(void)
{
    if ((limit_frames && dz_presented_frames() - first_frame >= limit_frames) ||
        (limit_ns && dz_virtual_ns() - first_ns >= limit_ns)) {
        capture();
        exit_requested = input_pending = 1;
    }
}
void dz_dosi_finish(void) { capture(); }
int dz_dosi_output_failed(void) { return output_failed; }
