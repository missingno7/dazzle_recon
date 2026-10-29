#include "dazzle.h"
#include "dzconst.h"
#include "dzconio.h"
#include "platform/platform.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

static void keep_text_mode(uint16_t unused)
{
    (void)unused;
    dz_video_set_mode(DZ_MODE_TEXT, 0);
}

static void keep_text_palette(uint8_t *unused)
{
    (void)unused;
}

static void reset_menu_keys(void)
{
    dz_input_reset();
    input_pending = 0;
    mouse_abort_pending = 0;
    exit_requested = 0;
    current_graphics_key = previous_graphics_key = 0;
    help_menu_active = configuration_active = menu_active = 0;
    mode_setup_callback = keep_text_mode;
    palette_upload_callback = keep_text_palette;
    muted = 1;
    dz_video_set_mode(DZ_MODE_TEXT, 1);
    dz_conio_reset();
}

static void set_image(ImageConfigRow *row, char key, char *name, uint16_t size)
{
    row->draw = NULL;
    row->size = size;
    row->name = name;
    row->enabled = 1;
    row->continuous = 0;
    row->key = (uint8_t)key;
}

static void setup_configuration(void)
{
    alt_image_count = 2;
    ctrl_image_count = 1;
    fade_count = 2;
    set_image(&alt_image_records[0], '1', "alpha", 40);
    set_image(&alt_image_records[1], '2', "beta", 50);
    set_image(&ctrl_image_records[0], 'a', "gamma", 60);
    fade_records[0].key = 'b';
    fade_records[0].name = "blister";
    fade_records[0].enabled = 1;
    fade_records[1].key = 'd';
    fade_records[1].name = "drip";
    fade_records[1].enabled = 1;
    maximum_image_size = 120;
    minimum_images_per_display = 3;
    maximum_images_per_display = 9;
    auto_fade_pause_ticks = 91;
    default_config_name = "menu_test.dzl";
    strcpy(configuration_filename, "menu_test.dzl");
    file_dialog_mode = 's';
}

static int text_has(const char *needle)
{
    for (unsigned row = 0; row < 25; ++row) {
        char line[81];
        for (unsigned column = 0; column < 80; ++column)
            line[column] = (char)dz_video.text[row][column][0];
        line[80] = 0;
        if (strstr(line, needle)) return 1;
    }
    return 0;
}

static void push(uint16_t key)
{
    assert(dz_input_push(key));
}

static void test_conio(void)
{
    uint8_t saved[12];
    dz_video_set_mode(DZ_MODE_TEXT, 1);
    dz_conio_reset();
    window(10, 10, 12, 11);
    textattr(0x87);
    textcolor(0x8c);
    struct text_info info;
    gettextinfo(&info);
    assert(info.attribute == 0x8c);
    textbackground(3);
    gettextinfo(&info);
    assert(info.attribute == 0xbc);
    cputs("ABCDEF");
    assert(dz_video.text[9][9][0] == 'D');
    assert(dz_video.text[9][10][0] == 'E');
    assert(dz_video.text[9][11][0] == 'F');
    assert(dz_video.text[10][9][0] == ' ');
    assert(dz_video.text[8][9][0] == ' ');
    assert(gettext(10, 10, 12, 10, saved));
    assert(saved[0] == 'D' && saved[2] == 'E' && saved[4] == 'F');
    puttext(10, 11, 12, 11, saved);
    assert(dz_video.text[10][9][0] == 'D');
    _setcursortype(_NOCURSOR);
    assert(dz_video.cursor_visible == 0);
    _setcursortype(_SOLIDCURSOR);
    int start, end;
    dzconio_cursor_rows(&start, &end);
    assert(start == 0 && end == 15 && dz_video.cursor_visible == 1);
    _setcursortype(_NORMALCURSOR);
    dzconio_cursor_rows(&start, &end);
    assert(start == 14 && end == 15);

    dz_video_set_mode(DZ_MODE_TEXT, 1);
    dz_conio_reset();
    window(5, 4, 14, 5);
    assert(cprintf("X\nY") == 3);
    assert(dz_video.text[3][4][0] == 'X');
    assert(dz_video.text[4][5][0] == 'Y');
}

static void test_help(const char *screenshot)
{
    reset_menu_keys();
    push(BIOS_KEY_F1);
    help_screen();
    assert(!help_menu_active && !menu_active);
    assert(text_has("DAZZLE Help Screen Subsystem"));
    if (screenshot) assert(dz_save_screenshot(screenshot));
}

/* Original DOSBox-X acceptance: F1 returns to drawing; ESC quits the program.
 * F10 returns from every configuration editor, while ESC quits from each. */
static void test_reference_menu_exits(void)
{
    reset_menu_keys();
    push(0x011b);
    help_screen();
    assert(exit_requested && !help_menu_active && !menu_active);
    for (unsigned panel = 0; panel < 5; ++panel) {
        for (unsigned escape = 0; escape < 2; ++escape) {
            reset_menu_keys();
            setup_configuration();
            for (unsigned page = 0; page < panel; ++page) push(BIOS_KEY_PAGE_DOWN);
            push(escape ? 0x011b : BIOS_KEY_F10);
            configuration_screen();
            assert(exit_requested == escape);
            assert(!configuration_active && !menu_active);
        }
    }
}

static void test_image_size_edit(void)
{
    reset_menu_keys();
    setup_configuration();
    push('5');
    push(0x1c0d);
    push(BIOS_KEY_F2);
    configuration_screen();
    assert(alt_image_records[0].size == 5);
    assert(!configuration_active && !menu_active);
}

static void test_error_dialog_restore(void)
{
    reset_menu_keys();
    toggle_menu_mode(1);
    render_menu_text(3, 22, 'X', MENU_ATTR_NORMAL);
    push(0x011b);
    menu_error_message("test");
    assert(dz_video.text[22][3][0] == 'X');
    assert(exit_requested == 1);
    toggle_menu_mode(0);
}

static void test_file_save(void)
{
    char original_directory[MAX_PATH];
    char temporary_base[MAX_PATH], temporary_directory[MAX_PATH];
    char config_path[MAX_PATH];
    assert(_getcwd(original_directory, sizeof original_directory));
    DWORD length = GetTempPathA(MAX_PATH, temporary_base);
    assert(length > 0 && length < MAX_PATH);
    assert(GetTempFileNameA(temporary_base, "dzl", 0, temporary_directory));
    assert(DeleteFileA(temporary_directory));
    assert(CreateDirectoryA(temporary_directory, NULL));
    assert(_chdir(temporary_directory) == 0);

    reset_menu_keys();
    setup_configuration();
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_F4);
    push(BIOS_KEY_F2);
    configuration_screen();

    snprintf(config_path, sizeof config_path, "%s\\%s", temporary_directory, "menu_test.dzl");
    FILE *stream = fopen(config_path, "rt");
    assert(stream != NULL);
    char contents[4096];
    size_t bytes = fread(contents, 1, sizeof contents - 1u, stream);
    contents[bytes] = 0;
    assert(fclose(stream) == 0);
    assert(strstr(contents, "#10500;DO NOT MODIFY THIS LINE!"));
    assert(strstr(contents, "~1 [040]"));
    assert(strstr(contents, "$I [003]"));

    alt_image_records[0].size = 1;
    fade_records[0].enabled = 0;
    minimum_images_per_display = 8;
    reset_menu_keys();
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_PAGE_DOWN);
    push(BIOS_KEY_PAGE_DOWN);
    push('l');
    push(0x1c0d);
    push(BIOS_KEY_F4);
    push(BIOS_KEY_F2);
    configuration_screen();
    assert(alt_image_records[0].size == 40);
    assert(fade_records[0].enabled == 1);
    assert(minimum_images_per_display == 3);

    FILE *wildcard_file = fopen("WILD.DZL", "wt");
    assert(wildcard_file != NULL);
    assert(fclose(wildcard_file) == 0);
    strcpy(readonly_search_spec, "*.DZL");
    reset_menu_keys();
    push(0x011b);
    char selected_path[80] = {0};
    configuration_directory_menu(selected_path);
    assert(text_has("WILD.DZL"));
    assert(head == NULL);
    assert(DeleteFileA("WILD.DZL"));

    assert(_chdir(original_directory) == 0);
    assert(DeleteFileA(config_path));
    assert(RemoveDirectoryA(temporary_directory));
}

int main(int argc, char **argv)
{
    const char *help_screenshot = NULL, *menu_screenshot = NULL;
    if (argc == 3 && strcmp(argv[1], "--screenshots") == 0) {
        help_screenshot = argv[2];
        static char path[MAX_PATH];
        char *separator = strrchr(argv[2], '\\');
        if (!separator) separator = strrchr(argv[2], '/');
        if (separator) {
            size_t length = (size_t)(separator - argv[2]);
            memcpy(path, argv[2], length);
            path[length] = 0;
        } else {
            strcpy(path, ".");
        }
        strncat(path, "\\menu-f2.bmp", sizeof path - strlen(path) - 1u);
        menu_screenshot = path;
    }
    dz_options.deterministic = 1;
    dz_options.seed = 1;
    dz_options.headless = help_screenshot == NULL;
    assert(dz_initialize());

    test_conio();
    test_help(help_screenshot);
    test_reference_menu_exits();

    reset_menu_keys();
    setup_configuration();
    push(BIOS_KEY_F2);
    configuration_screen();
    assert(text_has("Enter the IMAGE SIZE values"));
    if (menu_screenshot) assert(dz_save_screenshot(menu_screenshot));

    test_image_size_edit();
    test_error_dialog_restore();
    test_file_save();
    dz_shutdown();
    return 0;
}
