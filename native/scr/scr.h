#ifndef DAZZLE_SCR_H
#define DAZZLE_SCR_H

#include <windows.h>
#include "platform/platform.h"

enum dz_scr_mode { DZ_SCR_VGA400, DZ_SCR_VGA200, DZ_SCR_EGA350, DZ_SCR_EGA700 };
enum dz_scr_palette { DZ_SCR_PALETTE_NORMAL, DZ_SCR_PALETTE_ALTERNATE,
    DZ_SCR_PALETTE_RANDOM, DZ_SCR_PALETTE_FIXED };

struct dz_scr_settings {
    enum dz_scr_mode mode;
    enum dz_scr_palette palette;
    unsigned draw_delay;
    int sound;
    unsigned presentation;
    unsigned renderer;
    unsigned minimum_images, maximum_images, fade_pause_seconds;
    uint16_t image_size[42];
    unsigned char fade_enabled[24];
};

enum dz_scr_command { DZ_SCR_SETTINGS, DZ_SCR_ANIMATE, DZ_SCR_PREVIEW };
struct dz_scr_launch { enum dz_scr_command command; HWND owner; };

void dz_scr_settings_defaults(struct dz_scr_settings *settings);
int dz_scr_settings_validate(struct dz_scr_settings *settings);
int dz_scr_settings_load(struct dz_scr_settings *settings, char *dzl_path, size_t capacity);
int dz_scr_settings_save(const struct dz_scr_settings *settings);
int dz_scr_settings_import(struct dz_scr_settings *settings, const char *path);
void dz_scr_settings_apply_globals(const struct dz_scr_settings *settings);
int dz_scr_parse_command_line(int argc, char **argv, struct dz_scr_launch *launch);
INT_PTR CALLBACK dz_scr_settings_dialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);
int dz_scr_run_animation(void);
int dz_scr_run_preview(HWND parent);

#endif
