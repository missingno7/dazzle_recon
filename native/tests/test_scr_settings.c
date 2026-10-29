#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scr.h"

static int same_portable_settings(const struct dz_scr_settings *a, const struct dz_scr_settings *b)
{
    return a->mode == b->mode && a->palette == b->palette && a->draw_delay == b->draw_delay &&
        a->sound == b->sound && a->presentation == b->presentation && a->renderer == b->renderer &&
        a->minimum_images == b->minimum_images && a->maximum_images == b->maximum_images &&
        a->fade_pause_seconds == b->fade_pause_seconds &&
        memcmp(a->image_size, b->image_size, sizeof a->image_size) == 0 &&
        memcmp(a->fade_enabled, b->fade_enabled, sizeof a->fade_enabled) == 0;
}

static void remove_test_files(const char *root)
{
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s\\import.dzl", root); DeleteFileA(path);
    snprintf(path, sizeof path, "%s\\DAZZLE\\MTDAZZLE.DZL", root); DeleteFileA(path);
    snprintf(path, sizeof path, "%s\\DAZZLE\\dazzle.ini", root); DeleteFileA(path);
    snprintf(path, sizeof path, "%s\\DAZZLE", root); RemoveDirectoryA(path);
    RemoveDirectoryA(root);
}

int main(void)
{
    char old_appdata[32768] = "";
    DWORD old_length = GetEnvironmentVariableA("APPDATA", old_appdata, sizeof old_appdata);
    char temp[MAX_PATH], root[MAX_PATH];
    assert(GetTempPathA(sizeof temp, temp));
    assert(GetTempFileNameA(temp, "dzs", 0, root));
    DeleteFileA(root);
    assert(CreateDirectoryA(root, NULL));
    assert(SetEnvironmentVariableA("APPDATA", root));

    struct dz_scr_settings settings, defaults, loaded;
    dz_scr_settings_defaults(&settings);
    defaults = settings;
    assert(settings.mode == DZ_SCR_VGA400);
    assert(settings.draw_delay == 1 && settings.sound == 0);
    assert(settings.minimum_images == 3 && settings.maximum_images == 9);
    assert(settings.image_size[0] == 40 && settings.image_size[41] == 10);
    assert(settings.fade_enabled[0] == 1);

    struct dz_scr_settings invalid = settings;
    invalid.mode = (enum dz_scr_mode)7;
    invalid.palette = (enum dz_scr_palette)-1;
    invalid.draw_delay = 0;
    invalid.minimum_images = 300;
    invalid.maximum_images = 2;
    invalid.fade_pause_seconds = 999;
    invalid.image_size[0] = UINT16_MAX;
    invalid.fade_enabled[0] = 3;
    assert(!dz_scr_settings_validate(&invalid));
    assert(invalid.mode == settings.mode && invalid.palette == settings.palette);
    assert(invalid.draw_delay == 1 && invalid.minimum_images == 256 && invalid.maximum_images == 256);
    assert(invalid.fade_pause_seconds == 256 && invalid.image_size[0] < UINT16_MAX);
    assert(invalid.fade_enabled[0] == settings.fade_enabled[0]);
    invalid = settings;
    invalid.minimum_images = 0;
    invalid.maximum_images = 0;
    assert(!dz_scr_settings_validate(&invalid));
    assert(invalid.minimum_images == 1 && invalid.maximum_images == 1);

    settings.mode = DZ_SCR_EGA350;
    settings.palette = DZ_SCR_PALETTE_RANDOM;
    settings.draw_delay = 27;
    settings.sound = 1;
    settings.presentation = DZ_PRESENT_CROP;
    settings.renderer = DZ_RENDERER_RGBA;
    settings.minimum_images = 4;
    settings.maximum_images = 11;
    settings.fade_pause_seconds = 12;
    settings.image_size[0] = 37;
    settings.image_size[41] = 21;
    settings.fade_enabled[0] = 0;
    assert(dz_scr_settings_validate(&settings));
    assert(dz_scr_settings_save(&settings));

    dz_scr_settings_defaults(&loaded);
    char dzl_path[4096];
    assert(dz_scr_settings_load(&loaded, dzl_path, sizeof dzl_path));
    assert(dzl_path[0]);
    assert(same_portable_settings(&settings, &loaded));

    char import_path[MAX_PATH];
    snprintf(import_path, sizeof import_path, "%s\\import.dzl", root);
    FILE *file = fopen(import_path, "wt");
    assert(file);
    fputs("#10500;DO NOT MODIFY THIS LINE!\n~1 [077]\n^a [021]\na [n]\n$I [004]\n$X [009]\n$F [012]\n", file);
    assert(fclose(file) == 0);
    assert(dz_scr_settings_import(&loaded, import_path));
    assert(loaded.image_size[0] == 77);
    assert(loaded.image_size[36] == 21);
    assert(loaded.fade_enabled[0] == 0);
    assert(loaded.minimum_images == 4 && loaded.maximum_images == 9 && loaded.fade_pause_seconds == 12);

    struct dz_scr_settings before_bad_import = loaded;
    file = fopen(import_path, "wt");
    assert(file);
    for (int i = 0; i < 100; ++i) fputc('X', file);
    fputc('\n', file);
    assert(fclose(file) == 0);
    assert(!dz_scr_settings_import(&loaded, import_path));
    assert(same_portable_settings(&loaded, &before_bad_import));

    if (old_length) assert(SetEnvironmentVariableA("APPDATA", old_appdata));
    else assert(SetEnvironmentVariableA("APPDATA", NULL));
    remove_test_files(root);
    (void)defaults;
    return 0;
}
