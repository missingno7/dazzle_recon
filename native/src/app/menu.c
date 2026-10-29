/* DAZZLE 5.0 interactive menus/UI (158D; original module name unknown), native C17 port. */
#include "dazzle.h"
#include "dzconst.h"
#include "dzconio.h"
#include "platform/platform.h"
#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define TEXT_BYTES_PER_ROW 160
#define TEXT_SCREEN_BYTES 0x0fa0
#define TEXT_SCREEN_MODE 3
#define DOS_DIRECTORY_ATTRIBUTE 0x10
#define DOS_READ_ONLY_ATTRIBUTE 1
#define DOS_FIND_BLOCK_OFFSET 0x01d8
#define DOS_FIND_ATTRIBUTE_OFFSET 0x01ed
#define DOS_FIND_FILENAME_OFFSET 0x01f6
#define DRIVE_SEPARATOR 0x3a
#define MENU_COMMAND_N 0x6e
#define MENU_CELL_TEXT 0x7020
#define MENU_CELL_FRAME_VERTICAL 0x74ba
#define MENU_CELL_FRAME_HORIZONTAL 0x74cd
#define MENU_CELL_FRAME_TOP_LEFT 0x74c9
#define MENU_CELL_FRAME_TOP_RIGHT 0x74bb
#define MENU_CELL_FRAME_BOTTOM_LEFT 0x74c8
#define MENU_CELL_FRAME_BOTTOM_RIGHT 0x74bc
#define MENU_CELL_SHADOW 0x01b1
#define MENU_CELL_SHADOW_FILL 0x09b1
#define MENU_CELL_ERROR_VERTICAL 0x4eba
#define MENU_CELL_ERROR_HORIZONTAL 0x4ecd
#define MENU_CELL_ERROR_TOP_LEFT 0x4ec9
#define MENU_CELL_ERROR_TOP_RIGHT 0x4ebb
#define MENU_CELL_ERROR_BOTTOM_LEFT 0x4ec8
#define MENU_CELL_ERROR_BOTTOM_RIGHT 0x4ebc

#define menu_redraw_needed display_cleared
#define file_panel_height (configuration_panels[4].x)
#define ALT_IMAGE_SIZES alt_image_records
#define CTRL_IMAGE_SIZES ctrl_image_records
#define IMAGE_COUNT alt_image_count
#define OTHER_IMAGE_COUNT ctrl_image_count
#define MENU_REDRAW_NEEDED menu_redraw_needed
#define VIDEO_MODE_CALLBACK mode_setup_callback
#define PALETTE_CALLBACK palette_upload_callback
#define PALETTE_CYCLE_OFFSET palette_cycle_offset
#define SAVED_FREEZE_LED_STATE freeze_led_state
#define MENU_KEY_BYTE (*(uint8_t *)&current_graphics_key)
#define DOS_FIND_BLOCK (&directory_find_data)
#define DOS_FIND_ATTRIBUTE (directory_find_data.attribute)
#define DOS_FIND_FILENAME (directory_find_data.filename)

static HANDLE directory_search = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATAA directory_search_data;
static uint16_t directory_search_attributes;

static void menu_text_write(uint16_t byte_offset, uint16_t cell)
{
    unsigned row = (unsigned)byte_offset / TEXT_BYTES_PER_ROW;
    unsigned column = ((unsigned)byte_offset % TEXT_BYTES_PER_ROW) / 2u;
    if (row < 25u && column < 80u) {
        dz_video.text[row][column][0] = (uint8_t)cell;
        dz_video.text[row][column][1] = (uint8_t)(cell >> 8);
    }
}

static uint16_t menu_text_read(uint16_t byte_offset)
{
    unsigned row = (unsigned)byte_offset / TEXT_BYTES_PER_ROW;
    unsigned column = ((unsigned)byte_offset % TEXT_BYTES_PER_ROW) / 2u;
    if (row >= 25u || column >= 80u)
        return 0;
    return (uint16_t)(dz_video.text[row][column][0] |
        ((uint16_t)dz_video.text[row][column][1] << 8));
}

static int16_t menu_signed_word(uint16_t bits)
{
    return bits <= INT16_MAX ? (int16_t)bits : (int16_t)((int32_t)bits - 65536);
}

static int16_t menu_add_word(int16_t value, uint16_t amount)
{
    return menu_signed_word((uint16_t)((uint32_t)(uint16_t)value + amount));
}

static int16_t menu_sub_word(int16_t value, uint16_t amount)
{
    return menu_signed_word((uint16_t)((uint32_t)(uint16_t)value - amount));
}

static int16_t menu_auto_pause(void)
{
    return menu_signed_word(auto_fade_pause_ticks);
}

static unsigned menu_auto_pause_seconds(void)
{
    return (unsigned)(uint16_t)(menu_auto_pause() / 18);
}

static int16_t menu_getcwd(char *buffer, uint16_t size)
{
    if (size == 0 || !_getcwd(buffer, (int)size))
        return 0;
    return 1;
}

static int16_t menu_chdir(const char *path)
{
    return (int16_t)_chdir(path);
}

static int menu_copy_string(char *destination, size_t capacity, const char *source)
{
    size_t length = strlen(source);
    if (capacity == 0 || length >= capacity) {
        if (capacity != 0)
            destination[0] = 0;
        return 0;
    }
    memcpy(destination, source, length + 1u);
    return 1;
}

static int menu_append_string(char *destination, size_t capacity, const char *source)
{
    size_t current_length = strlen(destination);
    size_t addition_length = strlen(source);
    if (current_length >= capacity || addition_length >= capacity - current_length)
        return 0;
    memcpy(destination + current_length, source, addition_length + 1u);
    return 1;
}

static void menu_setdisk(uint16_t zero_based_drive)
{
    (void)_chdrive((int)zero_based_drive + 1);
}

static int menu_copy_dos_name(char destination[13])
{
    const char *name = directory_search_data.cAlternateFileName[0]
        ? directory_search_data.cAlternateFileName : directory_search_data.cFileName;
    size_t length = strlen(name);
    if (length > 12u)
        return 0;
    memcpy(destination, name, length + 1u);
    return 1;
}

static uint16_t menu_find_advance(struct dos_find_block *block, uint16_t attributes)
{
    for (;;) {
        DWORD flags = directory_search_data.dwFileAttributes;
        int is_directory = (flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
        int include = attributes == 0 ||
            ((attributes & DOS_DIRECTORY_ATTRIBUTE) ? is_directory : !is_directory);
        int hidden_or_system = (flags & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
        if (include && !hidden_or_system && menu_copy_dos_name(block->filename)) {
            block->attribute = (uint8_t)((is_directory ? DOS_DIRECTORY_ATTRIBUTE : 0) |
                ((flags & FILE_ATTRIBUTE_READONLY) ? DOS_READ_ONLY_ATTRIBUTE : 0));
            return 0;
        }
        if (!FindNextFileA(directory_search, &directory_search_data)) {
            FindClose(directory_search);
            directory_search = INVALID_HANDLE_VALUE;
            return UINT16_MAX;
        }
    }
}

static uint16_t menu_findfirst(const char *pattern, struct dos_find_block *block,
                              uint16_t attributes)
{
    if (directory_search != INVALID_HANDLE_VALUE) {
        FindClose(directory_search);
        directory_search = INVALID_HANDLE_VALUE;
    }
    directory_search = FindFirstFileA(pattern, &directory_search_data);
    if (directory_search == INVALID_HANDLE_VALUE)
        return UINT16_MAX;
    directory_search_attributes = attributes;
    return menu_find_advance(block, attributes);
}

static uint16_t menu_findnext(struct dos_find_block *block)
{
    if (directory_search == INVALID_HANDLE_VALUE ||
        !FindNextFileA(directory_search, &directory_search_data)) {
        if (directory_search != INVALID_HANDLE_VALUE)
            FindClose(directory_search);
        directory_search = INVALID_HANDLE_VALUE;
        return UINT16_MAX;
    }
    return menu_find_advance(block, directory_search_attributes);
}

static int16_t menu_parse_unsigned(const char *text, uint16_t *value)
{
    char *end;
    unsigned long parsed;
    while (*text == ' ' || *text == '\t')
        ++text;
    if (*text < '0' || *text > '9')
        return 0;
    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno || end == text || parsed > UINT16_MAX)
        return 0;
    *value = (uint16_t)parsed;
    return 1;
}

static void menu_beep(uint16_t hz, uint16_t ms)
{
    if (!muted) {
        dz_sound(hz);
        dz_delay_ms(ms);
        dz_nosound();
    }
}

static uint16_t menu_split_path(const char *path, char *drive, char *directory,
                                char *name, char *extension)
{
    const char *base = path, *slash = NULL, *dot = NULL;
    uint16_t flags = 0;
    if (drive) drive[0] = 0;
    if (directory) directory[0] = 0;
    if (name) name[0] = 0;
    if (extension) extension[0] = 0;
    if (path[0] && path[1] == ':') {
        if (drive) { drive[0] = path[0]; drive[1] = ':'; drive[2] = 0; }
        flags |= 1;
        base = path + 2;
    }
    for (const char *p = base; *p; ++p)
        if (*p == '\\' || *p == '/') slash = p;
    if (slash) {
        if (directory) {
            size_t length = (size_t)(slash - base) + 1u;
            if (length > 79u) length = 79u;
            memcpy(directory, base, length);
            directory[length] = 0;
        }
        flags |= 2;
        base = slash + 1;
    }
    for (const char *p = base; *p; ++p)
        if (*p == '.') dot = p;
    if (dot) flags |= 8;
    if (base[0]) flags |= 4;
    if (name) {
        size_t length = (size_t)((dot ? dot : base + strlen(base)) - base);
        if (length > 8u) length = 8u;
        memcpy(name, base, length);
        name[length] = 0;
    }
    if (extension && dot) {
        size_t length = strlen(dot);
        if (length > 5u) length = 5u;
        memcpy(extension, dot, length);
        extension[length] = 0;
    }
    return flags;
}

static int menu_merge_name(char *output, size_t capacity, const char *drive,
                           const char *directory, const char *name, const char *extension)
{
    if (capacity == 0)
        return 0;
    output[0] = 0;
    if ((drive && !menu_append_string(output, capacity, drive)) ||
        (directory && !menu_append_string(output, capacity, directory)) ||
        (name && !menu_append_string(output, capacity, name)) ||
        (extension && !menu_append_string(output, capacity, extension))) {
        output[0] = 0;
        return 0;
    }
    return 1;
}

static int16_t menu_poll_graphics_keys(void)
{
    if (mouse_abort_pending) {
        mouse_abort_pending = 0;
        exit_requested = 1;
    }
    if (exit_requested)
        return 1;
    if (dz_bioskey(1)) {
        current_graphics_key = dz_bioskey(0);
        if ((current_graphics_key & 0xffu) == 27u)
            exit_requested = 1;
    }
    return (int16_t)exit_requested;
}

static int16_t menu_abort_requested(void)
{
    if (mouse_abort_pending) {
        mouse_abort_pending = 0;
        exit_requested = 1;
    }
    return (int16_t)exit_requested;
}

/* Menus owns DS:249E..39A6. Initialized records precede its literal pool. */

uint16_t help_menu_active = 0;
uint16_t configuration_active = 0;
uint16_t menu_active = 0;
char readonly_search_spec[14] = "*.DZL";
char file_dialog_mode = 's';
char file_mode_terminator = 0;
struct node *head = 0;
char directory_path[80];
struct dos_find_block directory_find_data;
struct node *directory_tail;
uint16_t editable_image_count;
ImageConfigRow *editable_images;
uint16_t error_screen_backup[3][78];

struct menu_descriptor help_page_data[5] = {
    {21, 63, MENU_ATTR_NORMAL,
        "                DAZZLE Help Screen Subsystem\n"
        "\n"
        "To reduce executable file size these menus are limited in\n"
        "scope.  Please take the time to read the DAZZLE.DOC file, it\n"
        "will show you how to get the most out of this program. Use the\n"
        "'DAZZLE -?' command at the DOS prompt for an outline of\n"
        "command line options. DAZZLE can create a new copy of\n"
        "DAZZLE.DOC if you invoke it as 'DAZZLE -Z' at the DOS prompt!\n"
        "\n"
        "The keyboard functions under these menu screens vary from the\n"
        "actions when DAZZLE is actively displaying graphic screens.\n"
        "Cursor movement is accepted from the arrow keys. The HOME key\n"
        "jumps to the first menu or field, the END key jumps to the\n"
        "last menu or field.  PAGE UP (PgUp), and PAGE DOWN (PgDn) keys\n"
        "step from one menu to another. Note the display at the bottom\n"
        "right of this text window, it has [PgDn], which means that\n"
        "more pages are available. ENTER accepts any input you provide.\n"
        "\n"
        "To exit a menu subsystem, press the same key that activated\n"
        "that system.  Therefore, to exit HELP press F1 again. To see\n"
        "more help press the PgDn key."},
    {20, 68, MENU_ATTR_NORMAL,
        "The following keys can be used while in the 'Semi-Automatic' mode:\n"
        "*F1      = Toggle HELP display.\n"
        "*F2      = Toggle Configuration menu.\n"
        "*F5      = Dump primary screen image to disk file.\n"
        " F8      = Toggle continuous drawing mode.\n"
        " F9      = Select next random image algorithm.\n"
        "*Alt-F9  = Toggle between 'Automatic' and 'Semi-Automatic' mode.\n"
        "            A multi-tone chirp indicates 'Semi-Automatic' is\n"
        "            ready to begin image generation.\n"
        " F10     = Draw current image algorithm while key is pressed.\n"
        " Alt-F10 = Reinitialize the current image algorithm.\n"
        "*Shf-F10 = Select new group of image colors.\n"
        "*Ctl-F10 = Select new background colors.\n"
        "\n"
        "Various Alternate and Control keys can be used in 'Semi-Automatic'\n"
        "to manually select a specific image pattern for the F8 and F10 keys.\n"
        "Refer to IMAGE SIZE configuration menus to match keys to images.\n"
        "\n"
        "NOTE: keys marked with '*' are also active outside of the \n"
        "      'Semi-Automatic' mode of operation."},
    {15, 63, MENU_ATTR_NORMAL,
        "Each 'Fade' effect can be manually invoked by pressing the\n"
        "specific keys listed in the FADE ENABLE configuration menu.\n"
        "\n"
        "The P (Pan/Scroll fade) key will induce a random fade effect\n"
        "when pressed in the 'semi-auto' mode.\n"
        "\n"
        "Some fades are only available in VGA mode.\n"
        "\n"
        "The following keys have special effects and are not listed\n"
        "in the FADE ENABLE configuration menu:\n"
        "H = Hold current display from fading (ignored in 'semi-auto').\n"
        "N = Next display now, immediate clear of image display.\n"
        "\n"
        "Press these letters without the ALT or CTL shift keys.\n"
        "These are effective in 'Automatic' and 'Semi-Automatic' modes."},
    {16, 65, MENU_ATTR_NORMAL,
        "The F2 key activates the Configuration Subsystem. From it's menu\n"
        "the image size of each drawing algorithm can be adjusted, and\n"
        "specific 'fades' can be disabled. The number of images that are\n"
        "displayed in the Auto Mode can be customized. It also allows the\n"
        "saving or loading of files containing DAZZLE command scripts.\n"
        "This file contains information from the configuration menus. It\n"
        "can be specified on the command line when DAZZLE is invoked to\n"
        "cause the program to operate with the saved options. The program\n"
        "will automatically load a file MTDAZZLE.DZL if it can be found.\n"
        "Using an \"*\" character on the command line will prevent loading\n"
        "of this file.\n"
        "\n"
        "This is a text file that can be edited. It contains comments\n"
        "about the meaning of it's entries. It is suggested that you print\n"
        "one of these files and review it while viewing the menu presented\n"
        "by the F2 key."},
    {16, 64, MENU_ATTR_NORMAL,
        "The TAB key initiates FREEZE mode, halting all activity within\n"
        "the program.  While in FREEZE mode the TAB key toggles between\n"
        "cycling and non-cycling of the colors of the displayed image.\n"
        "\n"
        "Press SPACE to exit the FREEZE mode, and restore normal program\n"
        "operation (console beeps).\n"
        "\n"
        "HELP can be called when in FREEZE mode, but it will terminate\n"
        "the FREEZE mode.\n"
        "\n"
        "\n"
        "During the display press \"+\" or \"-\" to change the color cycling\n"
        "speed. In VGA mode press \"Page Up\" or \"Page Down\" to change the\n"
        "color cycling step range.\n"
        "\n"
        "Please refer to DAZZLE.DOC for more details."},
};

struct config_panel configuration_panels[5] = {
    {3, 60, MENU_ATTR_NORMAL,
        "Enter the IMAGE SIZE values desired in the following fields:\n"
        "   (KEY = Alt-key input for image in 'Semi-Auto' mode)\n"
        " KEY   NAME          SIZE          KEY   NAME          SIZE\n"},
    {3, 60, MENU_ATTR_NORMAL,
        "Enter the IMAGE SIZE values desired in the following fields:\n"
        "   (KEY = Ctl-key input for image in 'Semi-Auto' mode)\n"
        " KEY   NAME          SIZE          KEY   NAME          SIZE\n"},
    {3, 56, MENU_ATTR_NORMAL,
        "        Fades may be enabled (y) or disabled (n)\n"
        "\n"
        "   Key Fade           Enable    Key Fade           Enable\n"},
    {10, 61, MENU_ATTR_NORMAL,
        "               Auto Mode Display Customization\n"
        "\n"
        " Minimum images per display       Maximum images per display \n"
        "\n"
        "\n"
        "\n"
        " Pause delay before start of FADE in seconds\n"
        "\n"
        " These numbers will be internally adjusted if in Alternate or \n"
        " Mad mode, or using an EGA.\n"},
    {9, 67, MENU_ATTR_NORMAL,
        "                         File Access Menu\n"
        "\n"
        " Select Mode : Save/Load [ ]            File Name :\n"
        " Path :\n"
        "\n"
        "\n"
        "      Press F3 for disk directory,  F4 to execute Save/Load.\n"
        "\n"
        " Note that the disk head is no longer parked in powerdown position.\n"},
};

uint8_t image_size_layout[7] = {4, 2, 7, 21, 36, 41, 55};

/* Allocates a directory-list node and appends the supplied path; reports allocation failure.
 * Original 158D:0004, 158 bytes. */
int16_t append_directory_entry(char *entry_path)
{
    struct node *directory_entry;

    directory_entry = (struct node *)malloc(sizeof *directory_entry);
    if (directory_entry != 0) {
        directory_entry->next = 0;
        if (!menu_copy_string(directory_entry->path, sizeof directory_entry->path, entry_path)) {
            free(directory_entry);
            menu_error_message("Directory entry is too long");
            return 1;
        }
        if (head == 0)
            head = directory_entry;
        else
            directory_tail->next = directory_entry;
        directory_tail = directory_entry;
    } else {
        menu_error_message("Memory limit, dirlist truncated");
        return 1;
    }
    return 0;
}

/* Frees every directory-list node and clears the list head.
 * Original 158D:00A2, 93 bytes. */
void free_nodes(void)
{
    struct node *current_node, *previous_node;
    current_node = head;
    if (head) {
        do {
            previous_node = current_node;
            current_node = current_node->next;
            free(previous_node);
        } while (current_node);
    }
    head = 0;
    directory_tail = 0;
}

/* Enumerates subdirectories and matching files into the shared list, returning the number added.
 * Original 158D:00FF, 282 bytes. */
uint16_t enumerate_directory_entries(void)
{
    char entry_path[14];
    uint16_t entry_count;
    uint16_t list_truncated;
    uint16_t search_status;

    entry_count = 0;
    list_truncated = 0;
    free_nodes();
    search_status = menu_findfirst("*.*", DOS_FIND_BLOCK, DOS_DIRECTORY_ATTRIBUTE);
    while (search_status == 0) {
        if (DOS_FIND_ATTRIBUTE & DOS_DIRECTORY_ATTRIBUTE) {
            if (!menu_copy_string(entry_path, sizeof entry_path, DOS_FIND_FILENAME)) {
                list_truncated = 1;
                break;
            }
            if (entry_path[0] && entry_path[strlen(entry_path) - 1] != '\\' &&
                !menu_append_string(entry_path, sizeof entry_path, "\\")) {
                list_truncated = 1;
                break;
            }
            if (append_directory_entry(entry_path)) {
                list_truncated = 1;
                break;
            }
            ++entry_count;
        }
        search_status = menu_findnext(DOS_FIND_BLOCK);
    }

    if (!list_truncated) {
        search_status = menu_findfirst(readonly_search_spec, DOS_FIND_BLOCK, DOS_READ_ONLY_ATTRIBUTE);
        while (search_status == 0) {
            if (!menu_copy_string(entry_path, sizeof entry_path, DOS_FIND_FILENAME)) {
                list_truncated = 1;
                break;
            }
            if (DOS_FIND_ATTRIBUTE & DOS_READ_ONLY_ATTRIBUTE)
                for (char *p = entry_path; *p; ++p)
                    *p = (char)tolower((unsigned char)*p);
            if (append_directory_entry(entry_path)) {
                list_truncated = 1;
                break;
            }
            ++entry_count;
            search_status = menu_findnext(DOS_FIND_BLOCK);
        }
    }

    if (directory_search != INVALID_HANDLE_VALUE) {
        FindClose(directory_search);
        directory_search = INVALID_HANDLE_VALUE;
    }

    return entry_count;
}

/* Writes one character and attribute to a bounded text-mode screen cell.
 * Original 158D:0219, 64 bytes. */
void render_menu_text(uint16_t x, uint16_t y, char character, char attribute)
{
    if (y < 25 && x < 80) {
        dz_video.text[y][x][0] = (uint8_t)character;
        dz_video.text[y][x][1] = (uint8_t)attribute;
    }
}

/* Enters or leaves text-menu mode, clears the text screen, and restores graphics callbacks and
 * LED state.
 * Original 158D:0259, 217 bytes. */
void toggle_menu_mode(uint16_t active)
{
    int16_t screen_offset;

    if (active) {
        if (menu_active == 0) {
            dz_video_set_mode(DZ_MODE_TEXT, 0);
            dz_conio_reset();
            menu_active = 1;
            SAVED_FREEZE_LED_STATE = keyboard_led_flags(0x10, 0);
            keyboard_led_flags(0x60, 1);
        }
        for (screen_offset = 0; screen_offset < TEXT_SCREEN_BYTES; screen_offset += 2)
            menu_text_write((uint16_t)(screen_offset), (uint16_t)(MENU_CELL_SHADOW_FILL));
    } else {
        menu_active = 0;
        MENU_REDRAW_NEEDED = 1;
        if (VIDEO_MODE_CALLBACK)
            VIDEO_MODE_CALLBACK(1);
        if (PALETTE_CALLBACK && palette_buffer)
            PALETTE_CALLBACK(palette_buffer + PALETTE_CYCLE_OFFSET);
        keyboard_led_flags(SAVED_FREEZE_LED_STATE, 2);
    }
}

/* Centers a framed text box, draws its border and shadow, selects a title, and returns its
 * origin.
 * Original 158D:0332, 682 bytes. */
uint32_t draw_framed_box(uint16_t height, uint16_t width,
                                  uint16_t title_id)
{
    uint16_t box_left, box_right, box_top, box_bottom;
    union {
        struct { uint16_t x, y; } point;
        uint32_t value;
    } origin;
    char *default_title, *fade_title, *auto_title, *file_title;
    char *title;
    uint16_t row;
    uint16_t column;

    default_title = "[end]";
    fade_title = "[PgUp]";
    auto_title = "[PgUp/PgDn]";
    file_title = "[PgDn]";
    if (width >= 80)
        width = 79;
    if (height >= 25)
        height = 24;
    column = width;
    box_left = (uint16_t)(((uint32_t)(80u - column) / 2u) - 1u);
    origin.point.x = box_left;
    ++origin.point.x;
    box_right = 80 - box_left + 1;
    row = height;
    box_top = (uint16_t)((25u - row) / 2u);
    origin.point.y = box_top;
    box_bottom = 25 - box_top - 1;
    if (height & 1)
        ++box_bottom;

    for (row = box_top; row < box_bottom; ++row)
        for (column = box_left; column < box_right; ++column)
            menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + column * 2), (uint16_t)(MENU_CELL_TEXT));
    --box_left;
    --box_top;
    column = box_left * 2;
    for (row = box_top; row < box_bottom; ++row)
        menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + column), (uint16_t)(MENU_CELL_FRAME_VERTICAL));
    column = box_right * 2;
    for (row = box_top; row < box_bottom; ++row)
        menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + column), (uint16_t)(MENU_CELL_FRAME_VERTICAL));
    row = box_top * TEXT_BYTES_PER_ROW;
    for (column = box_left; column < box_right; ++column)
        menu_text_write((uint16_t)(row + column * 2), (uint16_t)(MENU_CELL_FRAME_HORIZONTAL));
    row = box_bottom * TEXT_BYTES_PER_ROW;
    for (column = box_left; column < box_right; ++column)
        menu_text_write((uint16_t)(row + column * 2), (uint16_t)(MENU_CELL_FRAME_HORIZONTAL));
    menu_text_write((uint16_t)(box_top * TEXT_BYTES_PER_ROW + box_left * 2), (uint16_t)(MENU_CELL_FRAME_TOP_LEFT));
    menu_text_write((uint16_t)(box_top * TEXT_BYTES_PER_ROW + box_right * 2), (uint16_t)(MENU_CELL_FRAME_TOP_RIGHT));
    menu_text_write((uint16_t)(box_bottom * TEXT_BYTES_PER_ROW + box_left * 2), (uint16_t)(MENU_CELL_FRAME_BOTTOM_LEFT));
    menu_text_write((uint16_t)(box_bottom * TEXT_BYTES_PER_ROW + box_right * 2), (uint16_t)(MENU_CELL_FRAME_BOTTOM_RIGHT));
    column = box_right + 1;
    for (row = box_top + 1; row <= box_bottom + 1; ++row)
        menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + column * 2), (uint16_t)(MENU_CELL_SHADOW));
    row = box_bottom + 1;
    for (column = box_left + 1; column < box_right + 1; ++column)
        menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + column * 2), (uint16_t)(MENU_CELL_SHADOW));

    switch (title_id) {
    case 1: title = fade_title; break;
    case 2: title = auto_title; break;
    case 3: title = file_title; break;
    default: title = default_title; break;
    }
    if (title_id != 4) {
        column = (uint16_t)((uint32_t)box_right - 2u - (uint16_t)strlen(title));
        row = 0;
        while (title[row] != 0)
            render_menu_text(column++, box_bottom, title[row++], MENU_ATTR_FRAME);
    }
    return origin.value;
}


/* Displays the selected help page and handles page navigation or the shared menu exit path.
 * Original 158D:05DC, 459 bytes. */
/* Retain the original shared exits and short/jump forms. */
void help_screen(void)
{
    union {
        struct { uint16_t x, y; } point;
        uint32_t value;
    } origin;
    uint16_t text_column, text_row;
    struct menu_descriptor *descriptor;
    uint16_t text_offset;
    int16_t page = 0;

    if (help_menu_active) {
        previous_graphics_key = current_graphics_key = 0;
    } else {
        help_menu_active = 1;
        /* Page keys return here to render the selected help page again. */
        help_redraw:
        {
            switch (page) {
            case 4: descriptor = &help_page_data[4]; text_offset = 1; break;
            case 3: descriptor = &help_page_data[3]; text_offset = 2; break;
            case 2: descriptor = &help_page_data[2]; text_offset = 2; break;
            case 1: descriptor = &help_page_data[1]; text_offset = 2; break;
            default: descriptor = &help_page_data[0]; text_offset = 3;
            }
            toggle_menu_mode(1);
            origin.value = draw_framed_box(descriptor->height,
                                            descriptor->width, text_offset);
            text_offset = 0;
            text_column = origin.point.x;
            text_row = origin.point.y;
            while (descriptor->text[text_offset] != 0) {
                if (descriptor->text[text_offset] == '\n') {
                    text_column = origin.point.x;
                    ++text_row;
                } else {
                    render_menu_text(text_column++, text_row, descriptor->text[text_offset],
                                     descriptor->attribute);
                }
                ++text_offset;
            }
            menu_beep(0x190, 5);
            while (mouse_abort_pending == 0 && dz_bioskey(1) == 0 &&
                   exit_requested == 0)
                input_pending = 0;
            if (menu_poll_graphics_keys() != 0)
                goto help_finished;
            switch (current_graphics_key) {
            default:
                previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
        /* Poll failure and non-navigation keys share menu cleanup here. */
        help_finished:
                toggle_menu_mode(0);
                help_menu_active = 0;
                return;
            case BIOS_KEY_PAGE_UP: if (page <= 0) goto help_redraw; --page; goto help_redraw;
            case BIOS_KEY_PAGE_DOWN: if (page >= 4) goto help_redraw; ++page; goto help_redraw;
            case BIOS_KEY_HOME: page = 0; goto help_redraw;
            case BIOS_KEY_END: page = 4; goto help_redraw;
            }
        }
    }
}


/* Waits for menu input, maps navigation keys to actions, edits text, and redraws the active
 * field.
 * Original 158D:07A7, 1122 bytes. */

uint16_t menu_navigation(uint16_t x, uint16_t y, uint16_t max_characters,
                             char *text, uint16_t mode)
{
    uint16_t edit_state = 1;
    uint16_t cursor_position = 0;
    uint16_t text_index;

    if (mode == 1)
        for (text_index = (uint16_t)strlen(text); text_index < max_characters; ++text_index)
            text[text_index] = 0;
    for (;;) {
        text_index = mouse_abort_pending = input_pending = 0;
        while (exit_requested == 0 && mouse_abort_pending == 0 && dz_bioskey(1) == 0) {
            dz_service();
        }
        if (mouse_abort_pending || exit_requested) {
            gotoxy(1, 1);
            return 9;
        }
        current_graphics_key = dz_bioskey(0);
        /* ASCII keys edit the field; extended scan codes select menu actions. */
        if (MENU_KEY_BYTE != 0) {
            if (MENU_KEY_BYTE == 0x1b) {
                exit_requested = 1;
                gotoxy(1, 1);
                return 9;
            }
            if (MENU_KEY_BYTE == '\r' || MENU_KEY_BYTE == '\b' || MENU_KEY_BYTE == '\t')
                text_index = 1;
            else {
                switch (mode) {
                case 0:
                    if (MENU_KEY_BYTE >= '0' && MENU_KEY_BYTE <= '9') {
                        text_index = 1;
                        edit_state = 0;
                    }
                    break;
                case 1:
                    if (MENU_KEY_BYTE >= ' ' && MENU_KEY_BYTE <= '~') {
                        text_index = 1;
                        edit_state = 0;
                    }
                    break;
                case 2:
                    MENU_KEY_BYTE = (uint8_t)tolower(MENU_KEY_BYTE);
                    if (MENU_KEY_BYTE == 'n' || MENU_KEY_BYTE == 'y') {
                        text_index = 1;
                        edit_state = 0;
                    }
                    break;
                case 3:
                    MENU_KEY_BYTE = (uint8_t)tolower(MENU_KEY_BYTE);
                    if (MENU_KEY_BYTE == 's' || MENU_KEY_BYTE == 'l') {
                        text_index = 1;
                        edit_state = 0;
                    }
                    break;
                }
            }
        } else {
            switch (edit_state) {
            case 1:
                gotoxy(1, 1);
                switch (current_graphics_key) {
                case BIOS_KEY_LEFT: return 0;
                case BIOS_KEY_UP: return 2;
                case BIOS_KEY_RIGHT: return 1;
                case BIOS_KEY_DOWN: return 3;
                case BIOS_KEY_PAGE_UP: return 6;
                case BIOS_KEY_PAGE_DOWN: return 7;
                case BIOS_KEY_HOME: return 4;
                case BIOS_KEY_END: return 5;
                case BIOS_KEY_F1:
                case BIOS_KEY_F2:
                case BIOS_KEY_F3:
                case BIOS_KEY_F4: return 10;
                default:
                    mouse_abort_pending = input_pending = 0;
                    previous_graphics_key = current_graphics_key = 0;
                    return 9;
                }
            case 0:
                text_index = 1;
                switch (current_graphics_key) {
                case BIOS_KEY_UP: current_graphics_key = 8; break;
                case BIOS_KEY_LEFT: current_graphics_key = 8; break;
                case BIOS_KEY_RIGHT: current_graphics_key = 9; break;
                case BIOS_KEY_DOWN: current_graphics_key = 9; break;
                default:
                    current_graphics_key = 0;
                    text_index = 0;
                    menu_beep(0x04c9, 5);
                }
            }
        }
        if (text_index == 0)
            continue;
        if (cursor_position > max_characters && MENU_KEY_BYTE != 8)
            cursor_position = max_characters;
        switch ((uint16_t)MENU_KEY_BYTE) {
        case 9:
            if (cursor_position < max_characters && mode == 1) {
                ++cursor_position;
                edit_state = 0;
            }
            break;
        case 8:
            if ((int16_t)cursor_position > 0)
                --cursor_position;
            text[cursor_position] = 0;
            break;
        case 13:
            if (edit_state == 0) {
                *(text + cursor_position + 1) = 0;
                while ((int16_t)cursor_position > 0) {
                    if (text[cursor_position] == ' ') {
                        text[cursor_position] = 0;
                        --cursor_position;
                    } else
                        cursor_position = 0;
                }
            }
            gotoxy(1, 1);
            return 8;
        default:
            if (cursor_position < max_characters) {
                text[cursor_position] = (char)MENU_KEY_BYTE;
                ++cursor_position;
                text[cursor_position] = 0;
            }
        }
        for (text_index = 0; text_index < cursor_position; ++text_index)
            render_menu_text(x + text_index, y, text[text_index], MENU_ATTR_SELECTED);
        if (mode != 2)
            gotoxy(x + text_index + 1, y + 1);
        else
            gotoxy(x + text_index, y + 1);
        for (; text_index < max_characters; ++text_index)
            render_menu_text(x + text_index, y, ' ', MENU_ATTR_SELECTED);
    }
}


/* Checks whether any Alt- or Ctrl-image size is enabled and reports when none are configured.
 * Original 158D:0C09, 115 bytes. */
int16_t check_any_image_configured(void)
{
    uint16_t image_index;

    for (image_index = 0; image_index < IMAGE_COUNT + OTHER_IMAGE_COUNT; ++image_index) {
        if (image_index < IMAGE_COUNT) {
            if (ALT_IMAGE_SIZES[image_index].size > 0)
                return 1;
        } else if (CTRL_IMAGE_SIZES[image_index - IMAGE_COUNT].size > 0) {
            return 1;
        }
    }
    menu_error_message("At least one function must be set to more than zero!");
    return 0;
}


/* Renders and edits the two-column Alt/Ctrl image-size list through menu_navigation.
 * Original 158D:0C7C, 980 bytes. */
void edit_image_sizes(uint16_t x, uint16_t y)
{
    uint16_t selected = 0;
    uint16_t clear_column, display_column, display_row, input_column = 1, input_row = 1;
    uint16_t new_size = 0;
    uint16_t current_record = 0;
    char format[6], input[6];
    char attribute;
    uint8_t remaining;
    uint16_t action_index = 4;

    sprintf(format, "%%%du", (uint16_t)image_size_layout[0]);
    while (action_index != 9) {
        display_row = y;
        switch (action_index) {
        case 5: selected = editable_image_count - 1; break;
        case 2: if (selected > 1) selected -= 2; break;
        case 3: if ((uint32_t)selected + 2u < editable_image_count) selected += 2; break;
        case 0: selected &= 0xfffe; break;
        case 1: selected |= 1; break;
        case 6:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_UP;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_DOWN;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: selected = 0; break;
        case 8:
            if (menu_parse_unsigned(input, &new_size)) {
                if (new_size > maximum_image_size)
                    new_size = maximum_image_size;
                editable_images[current_record].size = new_size;
                if (check_any_image_configured() == 0)
                    editable_images[current_record].size = maximum_image_size;
                else
                    ++selected;
            }
            break;
        default:
            previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
            return;
        }
        if (selected >= editable_image_count)
            selected = 0;
        for (action_index = 0; action_index < editable_image_count; ++action_index) {
            clear_column = (uint16_t)((uint32_t)x +
                ((action_index & 1) ? image_size_layout[4] : image_size_layout[1]) - 1u);
            if (action_index != selected)
                attribute = MENU_ATTR_NORMAL;
            else {
                attribute = MENU_ATTR_SELECTED;
                input_row = display_row;
                current_record = action_index;
                input_column = (uint16_t)(x + ((action_index & 1) ? image_size_layout[6] : image_size_layout[3]));
            }
            for (remaining = (uint8_t)(image_size_layout[3] - image_size_layout[1] + image_size_layout[0]);
                 remaining > 0; remaining--) {
                render_menu_text(clear_column++, display_row, ' ', attribute);
            }
            clear_column = 0;
            display_column = (uint16_t)(x + ((action_index & 1) ? image_size_layout[4] : image_size_layout[1]));
            render_menu_text(display_column, display_row, (char)editable_images[action_index].key, attribute);
            if (action_index & 1)
                display_column = (uint16_t)(x + image_size_layout[5]);
            else
                display_column = (uint16_t)(x + image_size_layout[2]);
            while (editable_images[action_index].name[clear_column] != 0)
                render_menu_text(display_column++, display_row, editable_images[action_index].name[clear_column++], attribute);
            display_column = (uint16_t)(x + ((action_index & 1) ? image_size_layout[6] : image_size_layout[3]));
            if (sprintf(input, format, (unsigned)editable_images[action_index].size) != -1) {
                for (clear_column = 0; clear_column <= image_size_layout[0]; ++clear_column)
                    render_menu_text(display_column++, display_row, input[clear_column], attribute);
            }
            if (action_index & 1)
                ++display_row;
        }
        if (check_any_image_configured() == 0)
            editable_images[current_record].size = maximum_image_size;
        sprintf(input, format, (unsigned)editable_images[current_record].size);
        action_index = menu_navigation(input_column, input_row, image_size_layout[0], input, 0);
    }
}


/* Renders and edits the two-column fade enable list through menu_navigation.
 * Original 158D:1050, 817 bytes. */
void edit_fade_enables(uint16_t x, uint16_t y)
{
    uint16_t selected = 0;
    uint16_t clear_column, display_column, display_row, input_column = 1, input_row = 1;
    uint16_t current_record = 0;
    uint16_t menu_action = 4;
    char choice[3] = {0, 0, 0};
    char attribute;
    uint8_t remaining;
    uint8_t width = 1;
    uint16_t action_index;

    while (menu_action != 9) {
        display_row = y;
        switch (menu_action) {
        case 5: selected = fade_count - 1; break;
        case 2: if (selected > 1) selected -= 2; break;
        case 3: if (selected < fade_count - 2) selected += 2; break;
        case 0: selected &= 0xfffe; break;
        case 1: selected |= 1; break;
        case 6:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_UP;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_DOWN;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: selected = 0; break;
        case 8:
            switch (choice[0]) {
            case 'n': fade_records[current_record].enabled = 0; ++selected; break;
            case 'y': fade_records[current_record].enabled = 1; ++selected; break;
            default: menu_beep(0xfa, 0x0a); break;
            }
            break;
        default:
            previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
            return;
        }
        if (selected >= fade_count)
            selected = 0;
        for (action_index = 0; action_index < fade_count; ++action_index) {
            clear_column = (uint16_t)((uint32_t)x +
                ((action_index & 1) ? fade_layout[3] : fade_layout[0]) - 1u);
            if (action_index != selected)
                attribute = MENU_ATTR_NORMAL;
            else {
                attribute = MENU_ATTR_SELECTED;
                input_row = display_row;
                current_record = action_index;
                input_column = (uint16_t)(x + ((action_index & 1) ? fade_layout[5] : fade_layout[2]));
            }
            for (remaining = (uint8_t)(fade_layout[2] - fade_layout[0] + width);
                 remaining > 0; remaining--)
                render_menu_text(clear_column++, display_row, ' ', attribute);
            clear_column = 0;
            display_column = (uint16_t)(x + ((action_index & 1) ? fade_layout[3] : fade_layout[0]));
            render_menu_text(display_column, display_row, (char)fade_records[action_index].key, attribute);
            if (action_index & 1)
                display_column = (uint16_t)(x + fade_layout[4]);
            else
                display_column = (uint16_t)(x + fade_layout[1]);
            while (fade_records[action_index].name[clear_column] != 0)
                render_menu_text(display_column++, display_row, fade_records[action_index].name[clear_column++], attribute);
            display_column = (uint16_t)(x + ((action_index & 1) ? fade_layout[5] : fade_layout[2]));
            choice[1] = 0;
            if (fade_records[action_index].enabled)
                choice[0] = 'y';
            else
                choice[0] = 'n';
            render_menu_text(display_column++, display_row, choice[0], attribute);
            if (action_index & 1)
                ++display_row;
        }
        if (fade_records[current_record].enabled)
            choice[0] = 'y';
        else
            choice[0] = 'n';
        menu_action = menu_navigation(input_column, input_row, width, choice, 2);
    }
}

uint8_t fade_layout[6] = {4, 7, 24, 33, 36, 53};
uint8_t auto_input_width = 4;
uint8_t auto_field_positions[3] = {10, 45, 48};


/* Displays and edits Auto image-count and pause settings through menu_navigation.
 * Original 158D:1381, 896 bytes. */
void edit_auto_customization(uint16_t x, uint16_t y)
{
    uint16_t field = 0;
    uint16_t field_index, display_column, display_row, input_column = 1, input_row = 1;
    uint16_t current_field = 0;
    uint16_t setting_value;
    char format[6], input[6];
    char attribute;
    uint8_t remaining;
    uint16_t action_index = 4;

    sprintf(format, "%%%du", (uint16_t)auto_input_width);
    while (action_index != 9) {
        switch (action_index) {
        case 5: field = 2; break;
        case 2: field = 0; break;
        case 3: field = 2; break;
        case 0: if (field > 0) --field; break;
        case 1: ++field; break;
        case 6:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_UP;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_DOWN;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: field = 0; break;
        case 8:
            if (menu_parse_unsigned(input, &setting_value)) {
                if (setting_value > 0x100)
                    setting_value = 0x100;
                switch (current_field) {
                case 0:
                    minimum_images_per_display = setting_value;
                    if (minimum_images_per_display < 1) minimum_images_per_display = 1;
                    break;
                case 1:
                    maximum_images_per_display = setting_value;
                    if (maximum_images_per_display < minimum_images_per_display) maximum_images_per_display = minimum_images_per_display;
                    break;
                default:
                    auto_fade_pause_ticks = (uint16_t)((uint32_t)setting_value * 0x12u);
                    break;
                }
                ++field;
            }
            break;
        default:
            previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
            return;
        }
        if (field > 2)
            field = 0;
        for (field_index = 0; field_index < 3; ++field_index) {
            display_column = x + auto_field_positions[field_index];
            display_row = y - 6;
            if (field_index == 2) display_row = y - 4;
            if (field_index != field)
                attribute = MENU_ATTR_NORMAL;
            else {
                attribute = MENU_ATTR_SELECTED;
                input_row = display_row;
                current_field = field_index;
                input_column = display_column;
            }
            for (remaining = auto_input_width; remaining > 0; remaining--)
                render_menu_text(display_column++, display_row, ' ', attribute);
            display_column = x + auto_field_positions[field_index];
            display_row = y - 6;
            if (field_index == 2) display_row = y - 4;
            switch (field_index) {
            case 0:
                if (sprintf(input, format, (unsigned)minimum_images_per_display) != -1)
                    for (action_index = 0; action_index <= auto_input_width; ++action_index)
                        render_menu_text(display_column++, display_row, input[action_index], attribute);
                break;
            case 1:
                if (sprintf(input, format, (unsigned)maximum_images_per_display) != -1)
                    for (action_index = 0; action_index <= auto_input_width; ++action_index)
                        render_menu_text(display_column++, display_row, input[action_index], attribute);
                break;
            default:
                if (sprintf(input, format, menu_auto_pause_seconds()) != -1)
                    for (action_index = 0; action_index <= auto_input_width; ++action_index)
                        render_menu_text(display_column++, display_row, input[action_index], attribute);
                break;
            }
        }
        switch (current_field) {
        case 0: sprintf(input, format, (unsigned)minimum_images_per_display); break;
        case 1: sprintf(input, format, (unsigned)maximum_images_per_display); break;
        default: sprintf(input, format, menu_auto_pause_seconds()); break;
        }
        action_index = menu_navigation(input_column, input_row, auto_input_width, input, 0);
    }
}

/* Displays directory entries in a paged grid and returns a selected path or changes directories.
 * Original 158D:1701, 1095 bytes. */
void configuration_directory_menu(char *output_path)
{
    int16_t selected_entry = 0;
    int16_t first_visible_entry = 0;
    struct node *entry;
    int16_t node_index;
    uint16_t grid_column, entries_printed;
    char attribute;
    uint16_t entry_count, display_column, display_row;
    uint16_t height = 16, width = 70, column_width = 18, columns = 4;
    uint16_t page_entries = columns * height;
    union {
        struct { int16_t x, y; } point;
        uint32_t value;
    } origin;
    uint16_t action;
    char directory[80], entry_path[14];

    toggle_menu_mode(1);
    menu_beep(0x190, 5);
directory_restart:
    action = 4;
    if (menu_getcwd(directory_path, 80) == 0) {
        menu_error_message("Error trying to get Current Directory");
        previous_graphics_key = current_graphics_key = 'n';
        return;
    }
    entry_count = enumerate_directory_entries();
    menu_copy_string(directory, sizeof directory, " ");
    menu_append_string(directory, sizeof directory, directory_path);
    if (directory[strlen(directory) - 1] != '\\')
        menu_append_string(directory, sizeof directory, "\\");
    menu_append_string(directory, sizeof directory, readonly_search_spec);
    menu_append_string(directory, sizeof directory, " ");
    while (action != 9) {
        if (exit_requested)
            return;
        switch (action) {
        case 5: selected_entry = entry_count ? menu_signed_word((uint16_t)(entry_count - 1u)) : 0; break;
        case 0: selected_entry = menu_sub_word(selected_entry, 1); break;
        case 1: selected_entry = menu_add_word(selected_entry, 1); break;
        case 2: selected_entry = menu_sub_word(selected_entry, columns); break;
        case 3: selected_entry = menu_add_word(selected_entry, columns); break;
        case 6: selected_entry = menu_sub_word(selected_entry, page_entries); break;
        case 7: selected_entry = menu_add_word(selected_entry, page_entries); break;
        case 4: selected_entry = 0; break;
        case 8:
            entry = head;
            node_index = 0;
            while (entry && node_index++ < selected_entry)
                entry = entry->next;
            if (entry != 0) {
                menu_copy_string(entry_path, sizeof entry_path, entry->path);
                if (entry_path[strlen(entry_path) - 1] == '\\') {
                    entry_path[strlen(entry_path) - 1] = 0;
                    if (menu_chdir(entry_path) != 0)
                        menu_error_message("Error changing directory");
                    free_nodes();
                    goto directory_restart;
                } else {
                    menu_copy_string(output_path, 80, entry->path);
                    free_nodes();
                    return;
                }
            }
            free_nodes();
            return;
        default:
            previous_graphics_key = current_graphics_key = 'n';
            free_nodes();
            return;
        }
        if (selected_entry < 0)
            selected_entry = 0;
        if (entry_count == 0) {
            selected_entry = 0;
            first_visible_entry = 0;
        } else if ((uint16_t)selected_entry >= entry_count) {
            selected_entry = menu_signed_word((uint16_t)(entry_count - 1u));
        }
        origin.value = draw_framed_box(height, width, 4);
        uint16_t directory_length = (uint16_t)strlen(directory);
        uint16_t title_offset = (uint16_t)((uint16_t)(width - directory_length) / 2u);
        display_column = (uint16_t)(origin.point.x + title_offset);
        display_row = (uint16_t)(origin.point.y - 1);
        for (node_index = 0; directory[node_index] != 0; ++node_index)
            render_menu_text(display_column++, display_row, directory[node_index], MENU_ATTR_NORMAL);
        grid_column = 1;
        entries_printed = 0;
        display_column = (uint16_t)((uint16_t)origin.point.x + 2u);
        display_row = (uint16_t)origin.point.y;
        int32_t last_view_start = (int32_t)entry_count - (int32_t)columns - 1;
        if (last_view_start <= first_visible_entry)
            first_visible_entry = last_view_start < INT16_MIN ? INT16_MIN : (int16_t)last_view_start;
        if (first_visible_entry < 0)
            first_visible_entry = 0;
        if ((uint32_t)page_entries + (uint16_t)first_visible_entry <= (uint16_t)selected_entry) {
            first_visible_entry = (int16_t)((int32_t)selected_entry - page_entries);
            first_visible_entry = menu_add_word(first_visible_entry,
                (uint16_t)(columns - (uint16_t)first_visible_entry % columns));
        } else if (selected_entry <= first_visible_entry)
            first_visible_entry = selected_entry;
        entry = head;
        /* Keep each directory page aligned to a complete row of the grid. */
        first_visible_entry = menu_sub_word(first_visible_entry,
            (uint16_t)first_visible_entry % columns);
        if (first_visible_entry > 0) {
            node_index = 0;
            while (entry && node_index++ < first_visible_entry)
                entry = entry->next;
        }
        while (entry != 0) {
            node_index = 0;
            if (entries_printed + first_visible_entry == (uint16_t)selected_entry)
                attribute = MENU_ATTR_SELECTED;
            else
                attribute = MENU_ATTR_NORMAL;
            while (entry->path[node_index] != 0)
                render_menu_text(display_column++, display_row, entry->path[node_index++], attribute);
            entry = entry->next;
            display_column = (uint16_t)((uint16_t)origin.point.x + 2u + grid_column * column_width);
            ++grid_column;
            if (grid_column > columns) {
                grid_column = 1;
                display_column = (uint16_t)((uint16_t)origin.point.x + 2u);
                ++display_row;
            }
            ++entries_printed;
            if (entries_printed >= page_entries)
                break;
        }
        action = menu_navigation(1, 1, 0, directory, 4);
    }
    free_nodes();
}


/* Changes to a directory and selects its drive when the path includes a drive prefix.
 * Original 158D:1B48, 102 bytes. */
int16_t select_directory_drive(char *path)
{
    char *drive_separator;

    if (menu_chdir(path) != 0) {
        menu_error_message("Error changing directory");
    } else {
        drive_separator = strchr(path, DRIVE_SEPARATOR);
        if (drive_separator != 0)
            menu_setdisk((uint16_t)(tolower(drive_separator[-1]) + 0xff9f));
        return 1;
    }
    return 0;
}


/* Edits file mode, config name, and directory, then loads/saves settings or opens file browsing.
 * Original 158D:1BAE, 1704 bytes. */

void file_access_menu(uint16_t x, uint16_t y)
{
    uint16_t selected_field = 0;
    uint16_t display_column, display_row, text_index;
    uint16_t field_modes[3] = {3, 1, 1};
    uint16_t action = 4;
    char attribute, field_width;
    char drive[4], name[10], extension[6];
    char *extension_separator;
    int16_t search_status = 1;
    char path_buffer[MAX_PATH];
    char *field_text[3];
    uint16_t field_index;

    field_text[0] = &file_dialog_mode;
    field_text[1] = configuration_filename;
    field_text[2] = directory_path;
    y -= file_panel_height;
    field_index = menu_split_path(configuration_filename, drive, path_buffer, name, extension);
    if ((field_index & 4) == 0 || (field_index & 2) == 0)
        menu_copy_string(configuration_filename, sizeof configuration_filename,
                         default_config_name ? default_config_name : "MTDAZZLE.DZL");
    else if (!menu_merge_name(configuration_filename, sizeof configuration_filename,
                              NULL, NULL, name, extension))
        menu_copy_string(configuration_filename, sizeof configuration_filename,
                         default_config_name ? default_config_name : "MTDAZZLE.DZL");
    if ((field_index & 3) && path_buffer[0]) {
        char selected_directory[80];
        menu_merge_name(selected_directory, sizeof selected_directory,
                        drive[0] ? drive : NULL, path_buffer, NULL, NULL);
        size_t directory_length = strlen(selected_directory);
        if (directory_length > 1 && selected_directory[directory_length - 1] == '\\' &&
            !(directory_length == 3 && selected_directory[1] == ':'))
            selected_directory[directory_length - 1] = 0;
        select_directory_drive(selected_directory);
    }
    while (action != 9 && !exit_requested) {
        switch (action) {
        case 5: selected_field = 2; break;
        case 0:
        case 2: if (selected_field > 0) --selected_field; break;
        case 1:
        case 3: if (selected_field < 2) ++selected_field; break;
        case 6:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_UP;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_DOWN;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: selected_field = 0; break;
        case 8:
            switch (selected_field) {
            case 0:
                if (path_buffer[0] != 0)
                    file_dialog_mode = path_buffer[0];
                ++selected_field;
                break;
            case 1:
                if (path_buffer[0] != 0) {
                    if ((extension_separator = strchr(path_buffer, '.')) != 0) extension_separator[4] = 0;
                    else if (!menu_append_string(path_buffer, sizeof path_buffer, ".DZL")) {
                        menu_error_message("File name is too long");
                        path_buffer[0] = 0;
                    }
                    if (!menu_copy_string(configuration_filename, sizeof configuration_filename,
                                          path_buffer)) {
                        menu_error_message("File name is too long");
                        configuration_filename[0] = 0;
                    }
                }
                if (strchr(configuration_filename, '*') ||
                    strchr(configuration_filename, '?')) {
                menu_copy_string(readonly_search_spec, sizeof readonly_search_spec,
                                 configuration_filename);
        menu_copy_string(configuration_filename, sizeof configuration_filename,
                         default_config_name ? default_config_name : "MTDAZZLE.DZL");
/* F3 and wildcard names converge on the same directory-selection screen. */
file_browse:
                    configuration_directory_menu(configuration_filename);
                    previous_graphics_key = current_graphics_key = 0x8000;
                    return;
                } else
                    reject_exec_filename(configuration_filename);
                ++selected_field;
                break;
            default:
                if (select_directory_drive(path_buffer))
                    ++selected_field;
                break;
            }
            break;
        case 10:
            switch (current_graphics_key) {
            case BIOS_KEY_F3:
                menu_copy_string(readonly_search_spec, sizeof readonly_search_spec, "*.*");
                goto file_browse;
            case BIOS_KEY_F4:
                menu_beep(1000, 10);
                if (file_dialog_mode == 'l') {
                    if (search_status == 0) {
                        if (load_configuration_file(configuration_filename) == 0) {
                            menu_beep(1500, 8);
                            current_graphics_key = BIOS_KEY_PAGE_DOWN;
                            return;
                        } else
                            menu_error_message("Unable to open file for read");
                    }
                } else {
                    if (save_configuration_file(configuration_filename) == 0) {
                        menu_beep(500, 12);
                        current_graphics_key = BIOS_KEY_PAGE_DOWN;
                        return;
                    }
                }
                break;
            case BIOS_KEY_F1:
            case BIOS_KEY_F2:
                previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
                return;
            }
            break;
        default:
            previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
            return;
        }
        if (exit_requested)
            return;
        gotoxy(1, 1);
        if (selected_field > 2)
            selected_field = 0;
        if (menu_getcwd(directory_path, 80) == 0) {
            menu_error_message("Error trying to get Current Directory");
            previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
            return;
        }
        for (field_index = 0; field_index <= 2; ++field_index) {
            if (field_index != selected_field) attribute = MENU_ATTR_NORMAL;
            else attribute = MENU_ATTR_SELECTED;
            display_column = (uint16_t)(x + file_fields[field_index].x);
            display_row = (uint16_t)(y + file_fields[field_index].y);
            for (field_width = 0; field_width < file_fields[field_index].width; field_width++)
                render_menu_text(display_column++, display_row, ' ', attribute);
            display_column = (uint16_t)(x + file_fields[field_index].x);
            text_index = 0;
            while (field_text[field_index][text_index] != 0)
                render_menu_text(display_column++, display_row, field_text[field_index][text_index++], attribute);
            if (field_index == 1) {
                display_column = (uint16_t)(x + file_fields[field_index].x);
                ++display_row;
                for (field_width = 0; field_width < 9; field_width++)
                    render_menu_text(display_column++, display_row, ' ', MENU_ATTR_NORMAL);
                display_column = (uint16_t)(x + file_fields[field_index].x);
                menu_copy_string(path_buffer, sizeof path_buffer, directory_path);
                if (path_buffer[0] && path_buffer[strlen(path_buffer) - 1] != '\\')
                    menu_append_string(path_buffer, sizeof path_buffer, "\\");
                menu_append_string(path_buffer, sizeof path_buffer, configuration_filename);
                search_status = (int16_t)menu_findfirst(path_buffer, DOS_FIND_BLOCK, 1);
                path_buffer[0] = 0;
                if (file_dialog_mode == 's') {
                    if (search_status == 0) {
                        menu_copy_string(path_buffer, sizeof path_buffer, "EXIST");
                        if (DOS_FIND_ATTRIBUTE & DOS_READ_ONLY_ATTRIBUTE)
                            menu_copy_string(path_buffer, sizeof path_buffer, "READ ONLY");
                    }
                } else if (search_status != 0)
                    menu_copy_string(path_buffer, sizeof path_buffer, "NOT FOUND");
                text_index = 0;
                while (path_buffer[text_index] != 0)
                    render_menu_text(display_column++, display_row, path_buffer[text_index++], MENU_ATTR_FRAME);
                if (reject_exec_filename(configuration_filename)) {
                    selected_field = 1;
                    field_index = 0;
                }
            }
        }
        display_column = (uint16_t)(x + file_fields[selected_field].x);
        display_row = (uint16_t)(y + file_fields[selected_field].y);
        field_width = file_fields[selected_field].width;
        menu_copy_string(path_buffer, sizeof path_buffer, field_text[selected_field]);
        action = menu_navigation(display_column, display_row, (uint16_t)(uint8_t)field_width,
            path_buffer, field_modes[selected_field]);
    }
}


/* The signed field coordinates follow the file editor's field_modes initializer. */
struct file_field file_fields[3] = {{26, 2, 1}, {53, 2, 13}, {1, 4, 64}};


/* Displays the configuration panel menu and dispatches the selected image, fade, Auto, or file
 * editor.
 * Original 158D:2256, 572 bytes. */
void configuration_screen(void)
{
    union {
        struct { uint16_t x, y; } point;
        uint32_t value;
    } origin;
    uint16_t text_column, text_row, panel_title;
    struct config_panel *panel;
    configuration_editor editor;
    int16_t selection = 1;
    uint16_t panel_rows;

    if (configuration_active) {
        previous_graphics_key = current_graphics_key = 0;
    } else {
        configuration_active = 1;
        for (;;) {
        switch (selection) {
        case 1:
            panel = &configuration_panels[0];
            panel_title = 3;
            editor = edit_image_sizes;
            editable_images = alt_image_records;
            editable_image_count = alt_image_count;
            panel_rows = alt_image_count;
            ++panel_rows;
            panel_rows >>= 1;
            break;
        case 2:
            panel = &configuration_panels[1];
            panel_title = 3;
            editor = edit_image_sizes;
            editable_images = ctrl_image_records;
            editable_image_count = ctrl_image_count;
            panel_rows = ctrl_image_count;
            ++panel_rows;
            panel_rows >>= 1;
            break;
        case 3:
            panel = &configuration_panels[2];
            panel_title = 2;
            editor = edit_fade_enables;
            panel_rows = fade_count;
            ++panel_rows;
            panel_rows >>= 1;
            break;
        case 4:
            panel = &configuration_panels[3];
            panel_title = 2;
            editor = edit_auto_customization;
            panel_rows = 0;
            break;
        default:
            panel = &configuration_panels[4];
            panel_title = 1;
            editor = file_access_menu;
            panel_rows = 0;
        }
        toggle_menu_mode(1);
        origin.value = draw_framed_box(panel->x + panel_rows, panel->y, panel_title);
        panel_rows = 0;
        text_column = origin.point.x;
        text_row = origin.point.y;
        while (panel->title[panel_rows] != 0) {
            if (panel->title[panel_rows] == '\n') {
                text_column = origin.point.x;
                ++text_row;
            } else {
                render_menu_text(text_column++, text_row, panel->title[panel_rows], (char)panel->attributes);
            }
            ++panel_rows;
        }
        menu_beep(0x190, 5);
        editor(origin.point.x, origin.point.y + panel->x);
        if (menu_abort_requested()) {
            toggle_menu_mode(0);
            configuration_active = 0;
            return;
        } else {
            switch (current_graphics_key) {
            case 0x8000: continue;
            default:
                previous_graphics_key = current_graphics_key = MENU_COMMAND_N;
                toggle_menu_mode(0);
                configuration_active = 0;
                return;
            case BIOS_KEY_PAGE_UP:
                if (selection > 1) --selection;
                continue;
            case BIOS_KEY_PAGE_DOWN:
                if (selection < 5) ++selection;
                continue;
            }
        }
        }
    }
}


/* Shows a framed error message over the active menu, waits for input, and restores saved screen
 * cells.
 * Original 158D:2492, 502 bytes. */
/* These local controls retain the original repeated segment loads and exit JMP. */
void menu_error_message(char *message)
{
    uint16_t message_index;
    uint16_t message_width;
    uint16_t column;
    uint16_t row;

    if (menu_active == 0) {
        fputc('\a', stderr);
    } else {
        do {
        message_width = (uint16_t)strlen(message);
        if (message_width > 0x4b)
            message_width = 0x4b;
        --message_width;
        for (row = 22; row < 25; ++row)
            for (column = 0; column <= message_width + 2; ++column)
                error_screen_backup[row - 22][column] = menu_text_read(
                    (uint16_t)(row * TEXT_BYTES_PER_ROW + column * 2));
        for (row = 22; row < 25; ++row) {
            menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW), (uint16_t)(MENU_CELL_ERROR_VERTICAL));
            menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + (message_width + 2) * 2), (uint16_t)(MENU_CELL_ERROR_VERTICAL));
        }
        for (column = 0; column < message_width + 2; ++column) {
            menu_text_write((uint16_t)(24 * TEXT_BYTES_PER_ROW + column * 2), (uint16_t)(MENU_CELL_ERROR_HORIZONTAL));
            menu_text_write((uint16_t)(22 * TEXT_BYTES_PER_ROW + column * 2), (uint16_t)(MENU_CELL_ERROR_HORIZONTAL));
        }
        menu_text_write((uint16_t)(22 * TEXT_BYTES_PER_ROW), (uint16_t)(MENU_CELL_ERROR_TOP_LEFT));
        menu_text_write((uint16_t)(22 * TEXT_BYTES_PER_ROW + (message_width + 2) * 2), (uint16_t)(MENU_CELL_ERROR_TOP_RIGHT));
        menu_text_write((uint16_t)(24 * TEXT_BYTES_PER_ROW), (uint16_t)(MENU_CELL_ERROR_BOTTOM_LEFT));
        menu_text_write((uint16_t)(24 * TEXT_BYTES_PER_ROW + (message_width + 2) * 2), (uint16_t)(MENU_CELL_ERROR_BOTTOM_RIGHT));
        message_index = 0;
        column = 1;
        row = 23;
        while (message[message_index] != 0)
            render_menu_text(column++, row, message[message_index++], MENU_ATTR_ERROR);
        fputc('\a', stderr);
        if ((dz_bioskey(0) & 0x7f) == 0x1b)
            exit_requested = 1;
        for (row = 22; row < 25; ++row)
            for (column = 0; column <= message_width + 2; ++column)
                menu_text_write((uint16_t)(row * TEXT_BYTES_PER_ROW + column * 2), error_screen_backup[row - 22][column]);
        break;
        } while (0);
    }
}

