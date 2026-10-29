#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dazzle.h"
#include "scr.h"

static struct dz_scr_settings default_settings;
static int defaults_captured;

static unsigned image_limit(enum dz_scr_mode mode)
{
    unsigned extent = mode == DZ_SCR_VGA400 ? 201u : mode == DZ_SCR_VGA200 ? 161u :
        mode == DZ_SCR_EGA350 ? 321u : 351u;
    return 0xffffu / (extent + extent / 4u) - 1u;
}

static void prepare_dosi_state(void)
{
    alt_image_count = 36;
    ctrl_image_count = 6;
    fade_count = 24;
}

void dz_scr_settings_defaults(struct dz_scr_settings *settings)
{
    prepare_dosi_state();
    if (!defaults_captured) {
        memset(&default_settings, 0, sizeof default_settings);
        default_settings.mode = DZ_SCR_VGA400;
        default_settings.palette = DZ_SCR_PALETTE_NORMAL;
        default_settings.draw_delay = 1;
        default_settings.presentation = DZ_PRESENT_ORIGINAL;
        default_settings.renderer = DZ_RENDERER_AUTO;
        default_settings.minimum_images = 3;
        default_settings.maximum_images = 9;
        default_settings.fade_pause_seconds = 5;
        for (unsigned i = 0; i < 36; ++i) default_settings.image_size[i] = alt_image_records[i].size;
        for (unsigned i = 0; i < 6; ++i) default_settings.image_size[36 + i] = ctrl_image_records[i].size;
        for (unsigned i = 0; i < 24; ++i) default_settings.fade_enabled[i] = (unsigned char)!!fade_records[i].enabled;
        defaults_captured = 1;
    }
    *settings = default_settings;
    dz_scr_settings_apply_globals(settings);
}

int dz_scr_settings_validate(struct dz_scr_settings *settings)
{
    int valid = 1;
    if (settings->mode < DZ_SCR_VGA400 || settings->mode > DZ_SCR_EGA700) {
        settings->mode = default_settings.mode; valid = 0;
    }
    if (settings->palette < DZ_SCR_PALETTE_NORMAL || settings->palette > DZ_SCR_PALETTE_FIXED) {
        settings->palette = default_settings.palette; valid = 0;
    }
    if (settings->draw_delay < 1 || settings->draw_delay > 8191) {
        settings->draw_delay = default_settings.draw_delay; valid = 0;
    }
    if (settings->sound != 0 && settings->sound != 1) {
        settings->sound = default_settings.sound; valid = 0;
    }
    if (settings->presentation > DZ_PRESENT_STRETCH) {
        settings->presentation = default_settings.presentation; valid = 0;
    }
    if (settings->renderer != DZ_RENDERER_AUTO && settings->renderer != DZ_RENDERER_RGBA) {
        settings->renderer = default_settings.renderer; valid = 0;
    }
    if (settings->minimum_images < 1) {
        settings->minimum_images = 1; valid = 0;
    } else if (settings->minimum_images > 256) {
        settings->minimum_images = 256; valid = 0;
    }
    if (settings->maximum_images > 256) {
        settings->maximum_images = 256; valid = 0;
    }
    if (settings->maximum_images < settings->minimum_images) {
        settings->maximum_images = settings->minimum_images; valid = 0;
    }
    if (settings->fade_pause_seconds > 256) {
        settings->fade_pause_seconds = 256; valid = 0;
    }
    unsigned limit = image_limit(settings->mode);
    for (unsigned i = 0; i < 42; ++i) {
        if (settings->image_size[i] > limit) {
            settings->image_size[i] = (uint16_t)limit;
            valid = 0;
        }
    }
    for (unsigned i = 0; i < 24; ++i) {
        if (settings->fade_enabled[i] > 1) {
            settings->fade_enabled[i] = default_settings.fade_enabled[i];
            valid = 0;
        }
    }
    return valid;
}

void dz_scr_settings_apply_globals(const struct dz_scr_settings *settings)
{
    prepare_dosi_state();
    for (unsigned i = 0; i < 36; ++i) alt_image_records[i].size = settings->image_size[i];
    for (unsigned i = 0; i < 6; ++i) ctrl_image_records[i].size = settings->image_size[36 + i];
    for (unsigned i = 0; i < 24; ++i) fade_records[i].enabled = settings->fade_enabled[i];
    minimum_images_per_display = (uint16_t)settings->minimum_images;
    maximum_images_per_display = (uint16_t)settings->maximum_images;
    auto_fade_pause_ticks = (uint16_t)(settings->fade_pause_seconds * 18u);
    draw_delay_count = (uint16_t)settings->draw_delay;
    muted = (int16_t)!settings->sound;
    palette_alternate_mode = settings->palette == DZ_SCR_PALETTE_ALTERNATE;
    palette_random_mode = settings->palette == DZ_SCR_PALETTE_RANDOM;
    palette_fixed_mode = settings->palette == DZ_SCR_PALETTE_FIXED;
    force_ega = settings->mode == DZ_SCR_EGA350 || settings->mode == DZ_SCR_EGA700;
    force_vga = !force_ega;
    video_adapter = force_ega ? 4u : 8u;
    bios_resolution_mode = settings->mode == DZ_SCR_VGA200 || settings->mode == DZ_SCR_EGA350;
    configure_video_backend(0);
    dz_display.presentation = (enum dz_presentation)settings->presentation;
    dz_display.renderer = (enum dz_renderer)settings->renderer;
}

static int config_paths(char *folder, size_t folder_capacity, char *dzl, size_t dzl_capacity,
                        char *ini, size_t ini_capacity, int create_folder)
{
    char appdata[32768];
    DWORD length = GetEnvironmentVariableA("APPDATA", appdata, (DWORD)sizeof appdata);
    if (!length || length >= sizeof appdata) return 0;
    int wrote = snprintf(folder, folder_capacity, "%s\\DAZZLE", appdata);
    if (wrote < 0 || (size_t)wrote >= folder_capacity) return 0;
    if (create_folder && !CreateDirectoryA(folder, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return 0;
    wrote = snprintf(dzl, dzl_capacity, "%s\\MTDAZZLE.DZL", folder);
    if (wrote < 0 || (size_t)wrote >= dzl_capacity) return 0;
    wrote = snprintf(ini, ini_capacity, "%s\\dazzle.ini", folder);
    return wrote >= 0 && (size_t)wrote < ini_capacity;
}

static int valid_dzl_text(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    int okay = 1, has_version = 0;
    if (fseek(file, 0, SEEK_END) != 0) okay = 0;
    long file_size = okay ? ftell(file) : -1;
    if (file_size < 0 || file_size > 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) okay = 0;
    char *contents = okay ? malloc((size_t)file_size + 1) : NULL;
    if (okay && !contents) okay = 0;
    if (okay && fread(contents, 1, (size_t)file_size, file) != (size_t)file_size) okay = 0;
    if (okay && memchr(contents, 0, (size_t)file_size)) okay = 0;
    if (okay) contents[file_size] = 0;
    for (size_t start = 0; okay && start < (size_t)file_size;) {
        size_t end = start;
        while (end < (size_t)file_size && contents[end] != '\n') ++end;
        size_t length = end - start + (end < (size_t)file_size);
        if (length > 79) { okay = 0; break; }
        for (size_t i = start; i < end; ++i) {
            unsigned char byte = (unsigned char)contents[i];
            if (byte < 32 && byte != '\t' && byte != '\r') { okay = 0; break; }
        }
        if (!okay) break;
        char line[80];
        memcpy(line, contents + start, end - start);
        line[end - start] = 0;
        char *cursor = line;
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        if (*cursor == '#') {
            char *version_end;
            errno = 0;
            long version = strtol(cursor + 1, &version_end, 10);
            if (version_end == cursor + 1 || errno || version < 10410 || version > 65535) {
                okay = 0; break;
            }
            has_version = 1;
        }
        start += length;
    }
    free(contents);
    if (ferror(file)) okay = 0;
    fclose(file);
    (void)has_version; /* DOSI permits versionless legacy text files. */
    return okay;
}

static void read_settings_globals(struct dz_scr_settings *settings)
{
    for (unsigned i = 0; i < 36; ++i) settings->image_size[i] = alt_image_records[i].size;
    for (unsigned i = 0; i < 6; ++i) settings->image_size[36 + i] = ctrl_image_records[i].size;
    for (unsigned i = 0; i < 24; ++i) settings->fade_enabled[i] = (unsigned char)!!fade_records[i].enabled;
    settings->minimum_images = minimum_images_per_display;
    settings->maximum_images = maximum_images_per_display;
    settings->fade_pause_seconds = auto_fade_pause_ticks / 18u;
}

int dz_scr_settings_import(struct dz_scr_settings *settings, const char *path)
{
    if (!path || !valid_dzl_text(path)) return 0;
    struct dz_scr_settings candidate = *settings;
    dz_scr_settings_apply_globals(&candidate);
    char filename[4096];
    int wrote = snprintf(filename, sizeof filename, "%s", path);
    if (wrote < 0 || (size_t)wrote >= sizeof filename || load_configuration_file(filename)) return 0;
    read_settings_globals(&candidate);
    if (!dz_scr_settings_validate(&candidate)) {
        dz_scr_settings_apply_globals(&candidate);
    }
    *settings = candidate;
    return 1;
}

static int read_ini_number(const char *ini, const char *key, unsigned minimum,
                           unsigned maximum, unsigned *value)
{
    char text[40] = "";
    GetPrivateProfileStringA("Display", key, "", text, sizeof text, ini);
    if (!text[0]) return 0;
    char *end;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 10);
    if (end == text || *end || errno || parsed < minimum || parsed > maximum) return 0;
    *value = (unsigned)parsed;
    return 1;
}

int dz_scr_settings_load(struct dz_scr_settings *settings, char *dzl_path, size_t capacity)
{
    char folder[32768], ini[32768], dzl[32768];
    dz_scr_settings_defaults(settings);
    if (!config_paths(folder, sizeof folder, dzl, sizeof dzl, ini, sizeof ini, 0)) return 0;
    unsigned value;
    if (read_ini_number(ini, "Mode", 0, 3, &value)) settings->mode = (enum dz_scr_mode)value;
    if (read_ini_number(ini, "Palette", 0, 3, &value)) settings->palette = (enum dz_scr_palette)value;
    if (read_ini_number(ini, "Delay", 1, 8191, &value)) settings->draw_delay = value;
    if (read_ini_number(ini, "Sound", 0, 1, &value)) settings->sound = (int)value;
    if (read_ini_number(ini, "Presentation", 0, DZ_PRESENT_STRETCH, &value)) settings->presentation = value;
    if (read_ini_number(ini, "Renderer", 0, DZ_RENDERER_RGBA, &value) && value != DZ_RENDERER_INDEXED)
        settings->renderer = value;
    dz_scr_settings_validate(settings);
    dz_scr_settings_apply_globals(settings);
    int have_dzl = valid_dzl_text(dzl) && dz_scr_settings_import(settings, dzl);
    if (dzl_path && capacity) {
        size_t length = have_dzl ? strlen(dzl) : 0;
        if (length >= capacity) return 0;
        if (have_dzl) memcpy(dzl_path, dzl, length + 1);
        else dzl_path[0] = 0;
    }
    return 1;
}

int dz_scr_settings_save(const struct dz_scr_settings *settings)
{
    struct dz_scr_settings checked = *settings;
    dz_scr_settings_validate(&checked);
    char folder[32768], ini[32768], dzl[32768];
    if (!config_paths(folder, sizeof folder, dzl, sizeof dzl, ini, sizeof ini, 1)) return 0;
    dz_scr_settings_apply_globals(&checked);
    char filename[4096];
    if (snprintf(filename, sizeof filename, "%s", dzl) < 0 || save_configuration_file(filename)) return 0;
    FILE *file = fopen(ini, "wt");
    if (!file) return 0;
    int result = fprintf(file, "[Display]\nMode=%u\nPalette=%u\nDelay=%u\nSound=%d\nPresentation=%u\nRenderer=%u\n",
        (unsigned)checked.mode, (unsigned)checked.palette, checked.draw_delay, checked.sound,
        checked.presentation, checked.renderer) > 0;
    if (fclose(file) != 0) result = 0;
    return result;
}
