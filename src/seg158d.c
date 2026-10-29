/* DAZZLE 5.0 menus; BC++ 3.00 -ml -O1. See worker notes for local controls. */
#pragma option -d

#define DAZZLE_MENU
#include "dazzle.h"

#define menu_redraw_needed (*(unsigned far *)&g_5cb1)
#define auto_pause (*(int far *)&g_5da8)
#define error_screen_shadow ((char far *)&_streams[2])
#define file_panel_height (configuration_panels[4].x)
#define alternate_image_sizes ((struct image_size_record far *)((char far *)alt_image_records+4))
#define control_image_sizes ((struct image_size_record far *)((char far *)ctrl_image_records+4))
#define IMAGE_COUNT alt_image_count
#define OTHER_IMAGE_COUNT ctrl_image_count
#define ALT_IMAGE_SIZES alternate_image_sizes
#define CTRL_IMAGE_SIZES control_image_sizes
#define MENU_REDRAW_NEEDED menu_redraw_needed
#define VIDEO_MODE_CALLBACK mode_setup_callback
#define PALETTE_CALLBACK palette_upload_callback
#define PALETTE_CYCLE_OFFSET palette_cycle_offset
#define SAVED_FREEZE_LED_STATE freeze_led_state
#define MENU_KEY_BYTE (*(unsigned char far *)&g_5d92)
#define DOS_FIND_BLOCK ((struct dos_find_block far *)MK_FP(FP_SEG(&directory_find_data),0x01d8))
#define DOS_FIND_ATTRIBUTE (*(unsigned char far *)MK_FP(FP_SEG(&directory_find_data),0x01ed))
#define DOS_FIND_FILENAME ((char far *)MK_FP(FP_SEG(&directory_find_data),0x01f6))




/* Menus owns DS:249E..39A6. Initialized records precede its literal pool. */

unsigned help_menu_active = 0;
unsigned configuration_active = 0;
unsigned menu_active = 0;
char near readonly_search_spec[14] = "*.DZL";
char near file_dialog_mode = 's';
char file_mode_terminator = 0;
struct node far *head = 0;

struct menu_descriptor help_page_data[5] = {
    {21, 63, 0x70,
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
    {20, 68, 0x70,
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
    {15, 63, 0x70,
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
    {16, 65, 0x70,
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
    {16, 64, 0x70,
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
        "Please refer to DAZZLE.DOC for far more details."},
};

struct config_panel configuration_panels[5] = {
    {3, 60, 0x70,
        "Enter the IMAGE SIZE values desired in the following fields:\n"
        "   (KEY = Alt-key input for image in 'Semi-Auto' mode)\n"
        " KEY   NAME          SIZE          KEY   NAME          SIZE\n"},
    {3, 60, 0x70,
        "Enter the IMAGE SIZE values desired in the following fields:\n"
        "   (KEY = Ctl-key input for image in 'Semi-Auto' mode)\n"
        " KEY   NAME          SIZE          KEY   NAME          SIZE\n"},
    {3, 56, 0x70,
        "        Fades may be enabled (y) or disabled (n)\n"
        "\n"
        "   Key Fade           Enable    Key Fade           Enable\n"},
    {10, 61, 0x70,
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
    {9, 67, 0x70,
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

unsigned char near image_size_layout[7] = {4, 2, 7, 21, 36, 41, 55};

/* objcmp: {"function":"append_directory_entry","address":"158d:0004","size":158,"symbols":{"_head":"DS:24b4","_free":"0000:2b47","_malloc":"0000:2c51","_strcpy":"0000:59cc","_directory_tail":"1e81:01d4","_menu_error_message":"158d:2492","_directory_allocation_error":"DS:38a3","_alt_image_records":"DS:01fe","_ctrl_image_records":"DS:041a","_configuration_panels":"DS:24e5","__streams":"DS:43f4","_g_5cb1":"DS:5cb1","_g_5da8":"DS:5da8"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
int far append_directory_entry(char far *path)
{
    struct node far *entry;

    entry = (struct node far *)malloc(0x12);
    if (entry != 0) {
        entry->next = 0;
        strcpy(entry->path, path);
        if (head == 0)
            head = entry;
        else
            directory_tail->next = entry;
        directory_tail = entry;
    } else {
        menu_error_message("Memory limit, dirlist truncated");
        return 1;
    }
    return 0;
}

/* objcmp: {"function":"free_nodes","address":"158d:00a2","size":93,"symbols":{"_head":"DS:24b4","_free":"0000:2b47"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far free_nodes(void)
{
    struct node far *p, *old;
    p = head;
    if (head) {
        do {
            old = p;
            p = p->next;
            free(old);
        } while (p);
    }
    head = 0;
}

/* objcmp: {"function":"enumerate_directory_entries","address":"158d:00ff","size":282,"symbols":{"_free_nodes":"158d:00a2","_findfirst":"0000:42d2","_findnext":"0000:4305","_strcpy":"0000:59cc","_strlen":"0000:5a36","_strcat":"0000:5950","_strlwr":"0000:5a55","_append_directory_entry":"158d:0004","_directory_search_spec":"DS:38c3","_directory_separator":"DS:38c7","_readonly_search_spec":"DS:24a4","_directory_find_data":"1e81:01d8"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
unsigned far enumerate_directory_entries(void)
{
    char path[14];
    unsigned found;
    unsigned allocation_failed;
    register unsigned search_status;

    found = 0;
    allocation_failed = 0;
    free_nodes();
    search_status = findfirst("*.*", DOS_FIND_BLOCK, 0x10);
    while (search_status == 0) {
        if (DOS_FIND_ATTRIBUTE & 0x10) {
            strcpy(path, DOS_FIND_FILENAME);
            if (path[strlen(path) - 1] != '\\')
                strcat(path, "\\");
            if (append_directory_entry(path)) {
                allocation_failed = 1;
                break;
            }
            ++found;
        }
        search_status = findnext(DOS_FIND_BLOCK);
    }

    if (!allocation_failed) {
        search_status = findfirst(readonly_search_spec, DOS_FIND_BLOCK, 1);
        while (search_status == 0) {
            strcpy(path, DOS_FIND_FILENAME);
            if (DOS_FIND_ATTRIBUTE & 1)
                strlwr(path);
            if (append_directory_entry(path)) {
                allocation_failed = 1;
                break;
            }
            ++found;
            search_status = findnext(DOS_FIND_BLOCK);
        }
    }

    return found;
}

/* objcmp: {"function":"render_menu_text","address":"158d:0219","size":64,"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far render_menu_text(unsigned x, unsigned y, char c, char attr)
{
    union {
        unsigned word;
        char byte[2];
    } cell;

    if (y < 25 && x < 80) {
        cell.byte[1] = attr;
        cell.byte[0] = c;
        *(unsigned far *)MK_FP(0xb800, y * 160 + x * 2) = cell.word;
    }
}

/* objcmp: {"function":"toggle_menu_mode","address":"158d:0259","size":217,"symbols":{"_menu_active":"DS:24a2","_keyboard_led_flags":"11c1:000a","_int86":"0000:320e","_keyboard_regs":"DS:6006","_freeze_led_state":"DS:5d82","_menu_redraw_needed":"DS:5cb1","_mode_setup_callback":"DS:5c43","_palette_upload_callback":"DS:5c37","_palette_buffer":"DS:5d94","_palette_buffer_segment":"DS:5d96","_palette_cycle_offset":"DS:5dec"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far toggle_menu_mode(unsigned active)
{
    int offset;

    if (active) {
        if (menu_active == 0) {
            keyboard_regs.x.ax = 3;
            int86(0x10, &keyboard_regs, &keyboard_regs);
            menu_active = 1;
            SAVED_FREEZE_LED_STATE = keyboard_led_flags(0x10, 0);
            keyboard_led_flags(0x60, 1);
        }
        for (offset = 0; offset < 0x0fa0; offset += 2)
            *(unsigned far *)MK_FP(0xb800, offset) = 0x09b1;
    } else {
        menu_active = 0;
        MENU_REDRAW_NEEDED = 1;
        VIDEO_MODE_CALLBACK(1);
        PALETTE_CALLBACK(palette_buffer + PALETTE_CYCLE_OFFSET);
        keyboard_led_flags(SAVED_FREEZE_LED_STATE, 2);
    }
}

/* objcmp: {"function":"draw_framed_box","address":"158d:0332","size":682,"symbols":{"_render_menu_text":"158d:0219","_box_title_default":"DS:38c9","_box_title_fade":"DS:38cf","_box_title_auto":"DS:38d6","_box_title_file":"DS:38e2"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
unsigned long far draw_framed_box(unsigned height, unsigned width,
                                  unsigned title_id)
{
    unsigned left, right, top, bottom;
    union {
        struct { unsigned x, y; } point;
        unsigned long value;
    } origin;
    char far *default_title, *fade_title, *auto_title, *file_title;
    char far *title;
    register unsigned row;
    register unsigned column;

    default_title = "[end]";
    fade_title = "[PgUp]";
    auto_title = "[PgUp/PgDn]";
    file_title = "[PgDn]";
    if (width >= 80)
        width = 79;
    if (height >= 25)
        height = 24;
    column = width;
    left = ((80 - column) / 2) - 1;
    origin.point.x = left;
    ++origin.point.x;
    right = 80 - left + 1;
    row = height;
    top = (25 - row) / 2;
    origin.point.y = top;
    bottom = 25 - top - 1;
    if (height & 1)
        ++bottom;

    for (row = top; row < bottom; ++row)
        for (column = left; column < right; ++column)
            *(unsigned far *)MK_FP(0xb800, row * 160 + column * 2) = 0x7020;
    --left;
    --top;
    column = left * 2;
    for (row = top; row < bottom; ++row)
        *(unsigned far *)MK_FP(0xb800, row * 160 + column) = 0x74ba;
    column = right * 2;
    for (row = top; row < bottom; ++row)
        *(unsigned far *)MK_FP(0xb800, row * 160 + column) = 0x74ba;
    row = top * 160;
    for (column = left; column < right; ++column)
        *(unsigned far *)MK_FP(0xb800, row + column * 2) = 0x74cd;
    row = bottom * 160;
    for (column = left; column < right; ++column)
        *(unsigned far *)MK_FP(0xb800, row + column * 2) = 0x74cd;
    *(unsigned far *)MK_FP(0xb800, top * 160 + left * 2) = 0x74c9;
    *(unsigned far *)MK_FP(0xb800, top * 160 + right * 2) = 0x74bb;
    *(unsigned far *)MK_FP(0xb800, bottom * 160 + left * 2) = 0x74c8;
    *(unsigned far *)MK_FP(0xb800, bottom * 160 + right * 2) = 0x74bc;
    column = right + 1;
    for (row = top + 1; row <= bottom + 1; ++row)
        *(unsigned far *)MK_FP(0xb800, row * 160 + column * 2) = 0x01b1;
    row = bottom + 1;
    for (column = left + 1; column < right + 1; ++column)
        *(unsigned far *)MK_FP(0xb800, row * 160 + column * 2) = 0x01b1;

    switch (title_id) {
    case 1: title = fade_title; break;
    case 2: title = auto_title; break;
    case 3: title = file_title; break;
    default: title = default_title; break;
    }
    if (title_id != 4) {
        column = right - 2 - strlen(title);
        row = 0;
        while (title[row] != 0)
            render_menu_text(column++, bottom, title[row++], 0x74);
    }
    return origin.value;
}


/* objcmp: {"function":"help_screen","address":"158d:05dc","size":459,"symbols":{"_help_page_data":"DS:24b8","_help_menu_active":"DS:249e","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_mouse_abort_pending":"DS:5d7c","_exit_requested":"DS:5d80","_input_pending":"DS:5d7e","_toggle_menu_mode":"158d:0259","_draw_framed_box":"158d:0332","_render_menu_text":"158d:0219","_beep":"11c1:0e07","_bioskey":"0000:2553","_poll_graphics_keys":"11c1:18e7","_strlen":"0000:5a36"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
/* Retain the original shared exits and short/near jump forms. */
#pragma option -O-
void far help_screen(void)
{
    union {
        struct { unsigned x, y; } point;
        unsigned long value;
    } origin;
    unsigned column, row;
    struct menu_descriptor far *descriptor;
    unsigned index;
    register int page = 0;

    if (help_menu_active) {
        g_5d90 = g_5d92 = 0;
    } else {
        help_menu_active = 1;
help_redraw:
        {
            switch (page) {
            case 4: descriptor = &help_page_data[4]; index = 1; break;
            case 3: descriptor = &help_page_data[3]; index = 2; break;
            case 2: descriptor = &help_page_data[2]; index = 2; break;
            case 1: descriptor = &help_page_data[1]; index = 2; break;
            default: descriptor = &help_page_data[0]; index = 3;
            }
            toggle_menu_mode(1);
            origin.value = draw_framed_box(descriptor->height,
                                            descriptor->width, index);
            index = 0;
            column = origin.point.x;
            row = origin.point.y;
            while (descriptor->text[index] != 0) {
                if (descriptor->text[index] == '\n') {
                    column = origin.point.x;
                    ++row;
                } else {
                    render_menu_text(column++, row, descriptor->text[index],
                                     descriptor->attribute);
                }
                ++index;
            }
            beep(0x190, 5);
            while (mouse_abort_pending == 0 && bioskey(1) == 0 &&
                   exit_requested == 0)
                input_pending = 0;
            if (poll_graphics_keys() != 0)
                goto help_finished;
            switch (g_5d92) {
            default:
                g_5d90 = g_5d92 = 0x6e;
help_finished:
                toggle_menu_mode(0);
                help_menu_active = 0;
                return;
            case 0x4900: if (page <= 0) goto help_redraw; --page; goto help_redraw;
            case 0x5100: if (page >= 4) goto help_redraw; ++page; goto help_redraw;
            case 0x4700: page = 0; goto help_redraw;
            case 0x4f00: page = 4; goto help_redraw;
            }
        }
    }
}

#pragma option -O

/* objcmp: {"function":"menu_navigation","address":"158d:07a7","size":1122,"symbols":{"_input_pending":"DS:5d7e","_mouse_abort_pending":"DS:5d7c","_exit_requested":"DS:5d80","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_beep":"11c1:0e07","_bioskey":"0000:2553","__bios_keybrd":"0000:2578","_gotoxy":"0000:2fd8","_intr":"0000:32d8","_tolower":"0000:1f66","_strlen":"0000:5a36","_render_menu_text":"158d:0219","__osmajor":"DS:007d"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */

unsigned far menu_navigation(unsigned x, unsigned y, unsigned maximum,
                             char far *text, unsigned mode)
{
    unsigned edit_state = 1;
    register unsigned position = 0;
    unsigned index;
    struct REGPACK idle_regs;

    if (mode == 1)
        for (index = strlen(text); index < maximum; ++index)
            text[index] = 0;
    for (;;) {
        index = mouse_abort_pending = input_pending = 0;
        while (exit_requested == 0 && mouse_abort_pending == 0 && bioskey(1) == 0) {
            geninterrupt(0x28);
            if (_osmajor >= 3) {
                idle_regs.r_ax = 0x1680;
                intr(0x2f, &idle_regs);
            }
        }
        if (mouse_abort_pending || exit_requested) {
cancelled:
            gotoxy(1, 1);
            return 9;
        }
        g_5d92 = _bios_keybrd(0);
        if (MENU_KEY_BYTE != 0) {
            if (MENU_KEY_BYTE == 0x1b) {
                exit_requested = 1;
                goto cancelled;
            }
            if (MENU_KEY_BYTE == '\r' || MENU_KEY_BYTE == '\b' || MENU_KEY_BYTE == '\t')
                index = 1;
            else {
                switch (mode) {
                case 0:
                    if (MENU_KEY_BYTE >= '0' && MENU_KEY_BYTE <= '9') {
                        index = 1;
                        edit_state = 0;
                    }
                    break;
                case 1:
                    if (MENU_KEY_BYTE >= ' ' && MENU_KEY_BYTE <= '~') {
                        index = 1;
                        edit_state = 0;
                    }
                    break;
                case 2:
                    MENU_KEY_BYTE = tolower(MENU_KEY_BYTE);
                    if (MENU_KEY_BYTE == 'n' || MENU_KEY_BYTE == 'y') {
                        index = 1;
                        edit_state = 0;
                    }
                    break;
                case 3:
                    MENU_KEY_BYTE = tolower(MENU_KEY_BYTE);
                    if (MENU_KEY_BYTE == 's' || MENU_KEY_BYTE == 'l') {
                        index = 1;
                        edit_state = 0;
                    }
                    break;
                }
            }
        } else {
            switch (edit_state) {
            case 1:
                gotoxy(1, 1);
                switch (g_5d92) {
                case 0x4b00: return 0;
                case 0x4800: return 2;
                case 0x4d00: return 1;
                case 0x5000: return 3;
                case 0x4900: return 6;
                case 0x5100: return 7;
                case 0x4700: return 4;
                case 0x4f00: return 5;
                case 0x3b00:
                case 0x3c00:
                case 0x3d00:
                case 0x3e00: return 10;
                default:
                    mouse_abort_pending = input_pending = 0;
                    g_5d90 = g_5d92 = 0;
                    return 9;
                }
            case 0:
                index = 1;
                switch (g_5d92) {
                case 0x4800: g_5d92 = 8; break;
                case 0x4b00: g_5d92 = 8; break;
                case 0x4d00: g_5d92 = 9; break;
                case 0x5000: g_5d92 = 9; break;
                default:
                    g_5d92 = 0;
                    index = 0;
                    beep(0x04c9, 5);
                }
            }
        }
        if (index == 0)
            continue;
        if (position > maximum && MENU_KEY_BYTE != 8)
            position = maximum;
        switch ((unsigned)MENU_KEY_BYTE) {
        case 9:
            if (position < maximum && mode == 1) {
                ++position;
                edit_state = 0;
            }
            break;
        case 8:
            if ((int)position > 0)
                --position;
            text[position] = 0;
            break;
        case 13:
            if (edit_state == 0) {
                *(text + position + 1) = 0;
                while ((int)position > 0) {
                    if (text[position] == ' ') {
                        text[position] = 0;
                        --position;
                    } else
                        position = 0;
                }
            }
            gotoxy(1, 1);
            return 8;
        default:
            if (position < maximum) {
                text[position] = MENU_KEY_BYTE;
                ++position;
                text[position] = 0;
            }
        }
        for (index = 0; index < position; ++index)
            render_menu_text(x + index, y, text[index], 0x0f);
        if (mode != 2)
            gotoxy(x + index + 1, y + 1);
        else
            gotoxy(x + index, y + 1);
        for (; index < maximum; ++index)
            render_menu_text(x + index, y, ' ', 0x0f);
    }
}


/* objcmp: {"function":"check_any_image_configured","address":"158d:0c09","size":115,"symbols":{"_menu_error_message":"158d:2492","_no_image_configured_error":"DS:38e9","_alt_image_count":"DS:5d9c","_ctrl_image_count":"DS:5d9a","_alternate_image_sizes":"DS:0202","_control_image_sizes":"DS:041e"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
int far check_any_image_configured(void)
{
    unsigned index;

    for (index = 0; index < IMAGE_COUNT + OTHER_IMAGE_COUNT; ++index) {
        if (index < IMAGE_COUNT) {
            if (ALT_IMAGE_SIZES[index].size > 0)
                return 1;
        } else if (CTRL_IMAGE_SIZES[index - IMAGE_COUNT - 1].size > 0) {
            return 1;
        }
    }
    menu_error_message("At least one function must be set to more than zero!");
    return 0;
}


/* objcmp: {"function":"edit_image_sizes","address":"158d:0c7c","size":980,"symbols":{"_editable_images":"1e81:0203","_editable_image_count":"1e81:0257","_g_5d88":"DS:5d88","_size_format":"DS:391e","_image_size_layout":"DS:2512","_size_parse_format":"DS:3924","_sprintf":"0000:5893","_sscanf":"0000:5909","_check_any_image_configured":"158d:0c09","_menu_navigation":"158d:07a7","_render_menu_text":"158d:0219","_alt_image_count":"DS:5d9c","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_input_pending":"DS:5d7e","_mouse_abort_pending":"DS:5d7c"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far edit_image_sizes(unsigned x, unsigned y)
{
    register unsigned selected = 0;
    unsigned blank_x, text_x, row, selected_x, selected_y;
    unsigned value = 0;
    unsigned selected_record = 0;
    char format[6], input[6];
    char attribute;
    unsigned char remaining;
    unsigned index = 4;

    sprintf(format, "%%%du", (unsigned)image_size_layout[0]);
    while (index != 9) {
        row = y;
        switch (index) {
        case 5: selected = editable_image_count - 1; break;
        case 2: if (selected > 1) selected -= 2; break;
        case 3: if (selected < editable_image_count + 0xfffe) selected += 2; break;
        case 0: selected &= 0xfffe; break;
        case 1: selected |= 1; break;
        case 6:
            g_5d90 = g_5d92 = 0x4900;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            g_5d90 = g_5d92 = 0x5100;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: selected = 0; break;
        case 8:
            if (sscanf(input, "%u", &value) == 1) {
                if (value > g_5d88)
                    value = g_5d88;
                editable_images[selected_record].size = value;
                if (check_any_image_configured() == 0)
                    editable_images[selected_record].size = g_5d88;
                else
                    ++selected;
            }
            break;
        default:
            g_5d90 = g_5d92 = 0x6e;
            return;
        }
        if (selected >= editable_image_count)
            selected = 0;
        for (index = 0; index < editable_image_count; ++index) {
            blank_x = x + ((index & 1) ? image_size_layout[4] : image_size_layout[1]) - 1;
            if (index != selected)
                attribute = 0x70;
            else {
                attribute = 0x0f;
                selected_y = row;
                selected_record = index;
                selected_x = x + ((index & 1) ? image_size_layout[6] : image_size_layout[3]);
            }
            for (remaining = image_size_layout[3] - image_size_layout[1] + image_size_layout[0];
                 remaining > 0; remaining--) {
                render_menu_text(blank_x++, row, ' ', attribute);
            }
            blank_x = 0;
            text_x = x + ((index & 1) ? image_size_layout[4] : image_size_layout[1]);
            render_menu_text(text_x, row, editable_images[index].key, attribute);
            if (index & 1)
                text_x = x + image_size_layout[5];
            else
                text_x = x + image_size_layout[2];
            while (editable_images[index].name[blank_x] != 0)
                render_menu_text(text_x++, row, editable_images[index].name[blank_x++], attribute);
            text_x = x + ((index & 1) ? image_size_layout[6] : image_size_layout[3]);
            if (sprintf(input, format, editable_images[index].size) != -1) {
                for (blank_x = 0; blank_x <= image_size_layout[0]; ++blank_x)
                    render_menu_text(text_x++, row, input[blank_x], attribute);
            }
            if (index & 1)
                ++row;
        }
        if (check_any_image_configured() == 0)
            editable_images[selected_record].size = g_5d88;
        sprintf(input, format, editable_images[selected_record].size);
        index = menu_navigation(selected_x, selected_y, image_size_layout[0], input, 0);
    }
}


/* objcmp: {"function":"edit_fade_enables","segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"},"address":"158d:1050","size":817,"symbols":{"F_SCOPY@":"0000:11fa","_fade_records":"DS:0474","_fade_count":"DS:5d98","_fade_enabled_format":"DS:3932","_fade_disabled_format":"DS:3936","_fade_layout":"DS:251c","_menu_navigation":"158d:07a7","_render_menu_text":"158d:0219","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_input_pending":"DS:5d7e","_mouse_abort_pending":"DS:5d7c","_beep":"11c1:0e07"}} */
void far edit_fade_enables(unsigned x, unsigned y)
{
    register unsigned selected = 0;
    unsigned blank_x, text_x, row, selected_x, selected_y;
    unsigned selected_record = 0;
    unsigned action = 4;
    char choice[3] = {0, 0, 0};
    char attribute;
    unsigned char remaining;
    unsigned char width = 1;
    unsigned index;

    while (action != 9) {
        row = y;
        switch (action) {
        case 5: selected = fade_count - 1; break;
        case 2: if (selected > 1) selected -= 2; break;
        case 3: if (selected < fade_count - 2) selected += 2; break;
        case 0: selected &= 0xfffe; break;
        case 1: selected |= 1; break;
        case 6:
            g_5d90 = g_5d92 = 0x4900;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            g_5d90 = g_5d92 = 0x5100;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: selected = 0; break;
        case 8:
            switch (choice[0]) {
            case 'n': fade_records[selected_record].enabled = 0; ++selected; break;
            case 'y': fade_records[selected_record].enabled = 1; ++selected; break;
            default: beep(0xfa, 0x0a); break;
            }
            break;
        default:
            g_5d90 = g_5d92 = 0x6e;
            return;
        }
        if (selected >= fade_count)
            selected = 0;
        for (index = 0; index < fade_count; ++index) {
            blank_x = x + ((index & 1) ? fade_layout[3] : fade_layout[0]) - 1;
            if (index != selected)
                attribute = 0x70;
            else {
                attribute = 0x0f;
                selected_y = row;
                selected_record = index;
                selected_x = x + ((index & 1) ? fade_layout[5] : fade_layout[2]);
            }
            for (remaining = fade_layout[2] - fade_layout[0] + width;
                 remaining > 0; remaining--)
                render_menu_text(blank_x++, row, ' ', attribute);
            blank_x = 0;
            text_x = x + ((index & 1) ? fade_layout[3] : fade_layout[0]);
            render_menu_text(text_x, row, fade_records[index].key, attribute);
            if (index & 1)
                text_x = x + fade_layout[4];
            else
                text_x = x + fade_layout[1];
            while (fade_records[index].name[blank_x] != 0)
                render_menu_text(text_x++, row, fade_records[index].name[blank_x++], attribute);
            text_x = x + ((index & 1) ? fade_layout[5] : fade_layout[2]);
            choice[1] = 0;
            if (fade_records[index].enabled)
                choice[0] = 'y';
            else
                choice[0] = 'n';
            render_menu_text(text_x++, row, choice[0], attribute);
            if (index & 1)
                ++row;
        }
        if (fade_records[selected_record].enabled)
            choice[0] = 'y';
        else
            choice[0] = 'n';
        action = menu_navigation(selected_x, selected_y, width, choice, 2);
    }
}

unsigned char near fade_layout[6] = {4, 7, 24, 33, 36, 53};
unsigned char near auto_input_width = 4;
unsigned char near auto_field_positions[3] = {10, 45, 48};


/* objcmp: {"function":"edit_auto_customization","address":"158d:1381","size":896,"symbols":{"_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_auto_pause":"DS:5da8","_auto_value_format":"DS:391e","_auto_value_parse_format":"DS:3924","_auto_input_width":"DS:2522","_auto_field_positions":"DS:2523","_menu_navigation":"158d:07a7","_render_menu_text":"158d:0219","_sprintf":"0000:5893","_sscanf":"0000:5909","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_input_pending":"DS:5d7e","_mouse_abort_pending":"DS:5d7c"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far edit_auto_customization(unsigned x, unsigned y)
{
    unsigned field = 0;
    unsigned column, text_x, row, selected_x, selected_y;
    unsigned selected_record = 0;
    unsigned value;
    char format[6], input[6];
    char attribute;
    unsigned char remaining;
    register unsigned index = 4;

    sprintf(format, "%%%du", (unsigned)auto_input_width);
    while (index != 9) {
        switch (index) {
        case 5: field = 2; break;
        case 2: field = 0; break;
        case 3: field = 2; break;
        case 0: if (field > 0) --field; break;
        case 1: ++field; break;
        case 6:
            g_5d90 = g_5d92 = 0x4900;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            g_5d90 = g_5d92 = 0x5100;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: field = 0; break;
        case 8:
            if (sscanf(input, "%u", &value) == 1) {
                if (value > 0x100)
                    value = 0x100;
                switch (selected_record) {
                case 0:
                    g_5cc5 = value;
                    if (g_5cc5 < 1) g_5cc5 = 1;
                    break;
                case 1:
                    g_5cc3 = value;
                    if (g_5cc3 < g_5cc5) g_5cc3 = g_5cc5;
                    break;
                default:
                    auto_pause = value * 0x12;
                    break;
                }
                ++field;
            }
            break;
        default:
            g_5d90 = g_5d92 = 0x6e;
            return;
        }
        if (field > 2)
            field = 0;
        for (column = 0; column < 3; ++column) {
            text_x = x + auto_field_positions[column];
            row = y - 6;
            if (column == 2) row = y - 4;
            if (column != field)
                attribute = 0x70;
            else {
                attribute = 0x0f;
                selected_y = row;
                selected_record = column;
                selected_x = text_x;
            }
            for (remaining = auto_input_width; remaining > 0; remaining--)
                render_menu_text(text_x++, row, ' ', attribute);
            text_x = x + auto_field_positions[column];
            row = y - 6;
            if (column == 2) row = y - 4;
            switch (column) {
            case 0:
                if (sprintf(input, format, g_5cc5) != -1)
                    for (index = 0; index <= auto_input_width; ++index)
                        render_menu_text(text_x++, row, input[index], attribute);
                break;
            case 1:
                if (sprintf(input, format, g_5cc3) != -1)
                    for (index = 0; index <= auto_input_width; ++index)
                        render_menu_text(text_x++, row, input[index], attribute);
                break;
            default:
                if (sprintf(input, format, auto_pause / 0x12) != -1)
                    for (index = 0; index <= auto_input_width; ++index)
                        render_menu_text(text_x++, row, input[index], attribute);
                break;
            }
        }
        switch (selected_record) {
        case 0: sprintf(input, format, g_5cc5); break;
        case 1: sprintf(input, format, g_5cc3); break;
        default: sprintf(input, format, auto_pause / 0x12); break;
        }
        index = menu_navigation(selected_x, selected_y, auto_input_width, input, 0);
    }
}

/* objcmp: {"function":"configuration_directory_menu","address":"158d:1701","size":1095,"symbols":{"_toggle_menu_mode":"158d:0259","_beep":"11c1:0e07","_getcwd":"0000:4b1f","_chdir":"0000:1017","_menu_error_message":"158d:2492","_enumerate_directory_entries":"158d:00ff","_strcpy":"0000:59cc","_strcat":"0000:5950","_strlen":"0000:5a36","_free_nodes":"158d:00a2","_directory_search_spec":"DS:38c3","_directory_separator":"DS:38c7","_readonly_search_spec":"DS:24a4","_head":"DS:24b4","_directory_tail":"1e81:01d4","_directory_path":"1e81:0207","_directory_entry_error":"DS:3927","_invalid_directory_error":"DS:394f","_directory_blank":"DS:394d","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_exit_requested":"DS:5d80","_menu_navigation":"158d:07a7","_draw_framed_box":"158d:0332","_render_menu_text":"158d:0219"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far configuration_directory_menu(char far *output_path)
{
    register int selected = 0;
    register int first = 0;
    struct node far *entry;
    int index;
    unsigned column_number, printed;
    char attribute;
    unsigned count, text_x, text_y;
    unsigned height = 16, width = 70, column_width = 18, columns = 4;
    unsigned page_entries = columns * height;
    union {
        struct { int x, y; } point;
        unsigned long value;
    } origin;
    unsigned action;
    char directory[80], entry_path[14];

    toggle_menu_mode(1);
    beep(0x190, 5);
directory_restart:
    action = 4;
    if (getcwd(directory_path, 80) == 0) {
        menu_error_message("Error trying to get Current Directory");
        g_5d90 = g_5d92 = 'n';
        return;
    }
    count = enumerate_directory_entries();
    strcpy(directory, " ");
    strcat(directory, directory_path);
    if (directory[strlen(directory) - 1] != '\\')
        strcat(directory, "\\");
    strcat(directory, readonly_search_spec);
    strcat(directory, " ");
    while (action != 9) {
        if (exit_requested)
            return;
        switch (action) {
        case 5: selected = count - 1; break;
        case 0: --selected; break;
        case 1: ++selected; break;
        case 2: selected -= columns; break;
        case 3: selected += columns; break;
        case 6: selected -= page_entries; break;
        case 7: selected += page_entries; break;
        case 4: selected = 0; break;
        case 8:
            entry = head;
            index = 0;
            while (entry && index++ < selected)
                entry = entry->next;
            if (entry != 0 && entry->path != 0) {
                strcpy(entry_path, entry->path);
                if (entry_path[strlen(entry_path) - 1] == '\\') {
                    entry_path[strlen(entry_path) - 1] = 0;
                    if (chdir(entry_path) != 0)
                        menu_error_message("Error changing directory");
                    free_nodes();
                    goto directory_restart;
                } else {
                    strcpy(output_path, entry->path);
                    free_nodes();
                    return;
                }
            }
            free_nodes();
            return;
        default:
            g_5d90 = g_5d92 = 'n';
            free_nodes();
            return;
        }
        if (selected < 0)
            selected = 0;
        if ((unsigned)selected >= count)
            selected = count - 1;
        origin.value = draw_framed_box(height, width, 4);
        text_x = origin.point.x + (width - strlen(directory)) / 2;
        text_y = origin.point.y - 1;
        for (index = 0; directory[index] != 0; ++index)
            render_menu_text(text_x++, text_y, directory[index], 0x70);
        column_number = 1;
        printed = 0;
        text_x = origin.point.x + 2;
        text_y = origin.point.y;
        if (count - columns - 1 <= (unsigned)first)
            first = count - columns - 1;
        if (first < 0)
            first = 0;
        if (page_entries + first <= (unsigned)selected) {
            first = selected - page_entries;
            first += columns - (unsigned)first % columns;
        } else if (selected <= first)
            first = selected;
        entry = head;
        first -= (unsigned)first % columns;
        if (first > 0) {
            index = 0;
            while (entry && index++ < first)
                entry = entry->next;
        }
        while (entry != 0) {
            index = 0;
            if (printed + first == (unsigned)selected)
                attribute = 0x0f;
            else
                attribute = 0x70;
            while (entry->path[index] != 0)
                render_menu_text(text_x++, text_y, entry->path[index++], attribute);
            entry = entry->next;
            text_x = origin.point.x + 2 + column_number * column_width;
            ++column_number;
            if (column_number > columns) {
                column_number = 1;
                text_x = origin.point.x + 2;
                ++text_y;
            }
            ++printed;
            if (printed >= page_entries)
                break;
        }
        action = menu_navigation(1, 1, 0, directory, 4);
    }
    free_nodes();
}


/* objcmp: {"function":"select_directory_drive","address":"158d:1b48","size":102,"symbols":{"_chdir":"0000:1017","_menu_error_message":"158d:2492","_invalid_directory_error":"DS:394f","_strchr":"0000:598f","_tolower":"0000:1f66","_setdisk":"0000:128e"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
int far select_directory_drive(char far *path)
{
    char far *prefix;

    if (chdir(path) != 0) {
        menu_error_message("Error changing directory");
    } else {
        prefix = strchr(path, 0x3a);
        if (prefix != 0)
            setdisk((unsigned)(tolower(prefix[-1]) + 0xff9f));
        return 1;
    }
    return 0;
}


/* objcmp: {"function":"file_access_menu","address":"158d:1bae","size":1704,"symbols":{"_toggle_menu_mode":"158d:0259","_menu_navigation":"158d:07a7","_configuration_directory_menu":"158d:1701","_select_directory_drive":"158d:1b48","_menu_error_message":"158d:2492","_render_menu_text":"158d:0219","_draw_framed_box":"158d:0332","_fnsplit":"0000:439f","_fnmerge":"0000:4374","_strcpy":"0000:59cc","_strcat":"0000:5950","_strlen":"0000:5a36","_findfirst":"0000:42d2","_configuration_file_next":"0000:4305","_configuration_file_exists":"0000:4b1f","_chdir":"0000:1017","_strchr":"0000:598f","_reject_exec_filename":"11c1:0d85","_load_configuration_file":"11c1:29ae","_save_configuration_file":"11c1:2b44","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_exit_requested":"DS:5d80","_file_dialog_mode":"DS:24b2","_g_5c5f":"DS:5c5f","_default_config_name":"DS:5ffa","_configuration_extension":"DS:3968","_configuration_read_error":"DS:396d","_configuration_exists_text":"DS:398a","_configuration_readonly_text":"DS:3990","_configuration_not_found_text":"DS:399a","_configuration_file_layout":"DS:252c","_directory_find_data":"1e81:01d8","_file_fields":"DS:252c","_file_panel_height":"DS:2509","_initial_file_modes":"DS:2526"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */

void far file_access_menu(unsigned x, unsigned y)
{
    register unsigned selection = 0;
    unsigned text_x, text_y, index;
    unsigned modes[3] = {3, 1, 1};
    unsigned action = 4;
    char attribute, count;
    char drive[4], name[10], extension[6];
    char far *prefix;
    int search_status = 1;
    char path[80];
    char far *lines[3];
    unsigned field;

    lines[0] = &file_dialog_mode;
    lines[1] = g_5c5f;
    lines[2] = directory_path;
    y -= file_panel_height;
    field = fnsplit(g_5c5f, drive, path, name, extension);
    if ((field & 4) == 0 || (field & 2) == 0)
        strcpy(g_5c5f, default_config_name);
    else
        fnmerge(g_5c5f, 0, 0, name, extension);
    if ((field & 8) || (field & 0x10)) {
        field = strlen(path);
        if (field > 1 && path[field - 1] == '\\')
            path[field - 1] = 0;
        select_directory_drive(path);
    }
    while (action != 9 && !exit_requested) {
        switch (action) {
        case 5: selection = 2; break;
        case 0:
        case 2: if (selection > 0) --selection; break;
        case 1:
        case 3: if (selection < 2) ++selection; break;
        case 6:
            g_5d90 = g_5d92 = 0x4900;
            mouse_abort_pending = input_pending = 0;
            return;
        case 7:
            g_5d90 = g_5d92 = 0x5100;
            mouse_abort_pending = input_pending = 0;
            return;
        case 4: selection = 0; break;
        case 8:
            switch (selection) {
            case 0:
                if (path[0] != 0)
                    file_dialog_mode = path[0];
                ++selection;
                break;
            case 1:
                if (path[0] != 0) {
                    if ((prefix = strchr(path, '.')) != 0) prefix[4] = 0;
                    else strcat(path, ".DZL");
                    strcpy(g_5c5f, path);
                }
                if (strchr(g_5c5f, '*') ||
                    strchr(g_5c5f, '?')) {
                    strcpy(readonly_search_spec, g_5c5f);
                    strcpy(g_5c5f, default_config_name);
file_browse:
                    configuration_directory_menu(g_5c5f);
                    g_5d90 = g_5d92 = 0x8000;
                    return;
                } else
                    reject_exec_filename(g_5c5f);
                ++selection;
                break;
            default:
                if (select_directory_drive(path))
                    ++selection;
                break;
            }
            break;
        case 10:
            switch (g_5d92) {
            case 0x3d00:
                strcpy(readonly_search_spec, "*.*");
                goto file_browse;
            case 0x3e00:
                beep(1000, 10);
                if (file_dialog_mode == 'l') {
                    if (search_status == 0) {
                        if (load_configuration_file(g_5c5f) == 0) {
                            beep(1500, 8);
                            g_5d92 = 0x5100;
                            return;
                        } else
                            menu_error_message("Unable to open file for read");
                    }
                } else {
                    if (save_configuration_file(g_5c5f) == 0) {
                        beep(500, 12);
                        g_5d92 = 0x5100;
                        return;
                    }
                }
                break;
            case 0x3b00:
            case 0x3c00:
                g_5d90 = g_5d92 = 0x6e;
                return;
            }
            break;
        default:
            g_5d90 = g_5d92 = 0x6e;
            return;
        }
        if (exit_requested)
            return;
        gotoxy(1, 1);
        if (selection > 2)
            selection = 0;
        if (getcwd(directory_path, 80) == 0) {
            menu_error_message("Error trying to get Current Directory");
            g_5d90 = g_5d92 = 0x6e;
            return;
        }
        for (field = 0; field <= 2; ++field) {
            if (field != selection) attribute = 0x70;
            else attribute = 0x0f;
            text_x = x + file_fields[field].x;
            text_y = y + file_fields[field].y;
            for (count = 0; count < file_fields[field].width; count++)
                render_menu_text(text_x++, text_y, ' ', attribute);
            text_x = x + file_fields[field].x;
            index = 0;
            while (lines[field][index] != 0)
                render_menu_text(text_x++, text_y, lines[field][index++], attribute);
            if (field == 1) {
                text_x = x + file_fields[field].x;
                ++text_y;
                for (count = 0; count < 9; count++)
                    render_menu_text(text_x++, text_y, ' ', 0x70);
                text_x = x + file_fields[field].x;
                strcpy(path, directory_path);
                if (path[strlen(path) - 1] != '\\')
                    strcat(path, "\\");
                strcat(path, g_5c5f);
                search_status = findfirst(path, DOS_FIND_BLOCK, 1);
                path[0] = 0;
                if (file_dialog_mode == 's') {
                    if (search_status == 0) {
                        strcpy(path, "EXIST");
                        if (DOS_FIND_ATTRIBUTE & 1)
                            strcpy(path, "READ ONLY");
                    }
                } else if (search_status != 0)
                    strcpy(path, "NOT FOUND");
                index = 0;
                while (path[index] != 0)
                    render_menu_text(text_x++, text_y, path[index++], 0x74);
                if (reject_exec_filename(g_5c5f)) {
                    selection = 1;
                    field = 0;
                }
            }
        }
        text_x = x + file_fields[selection].x;
        text_y = y + file_fields[selection].y;
        count = file_fields[selection].width;
        strcpy(path, lines[selection]);
        action = menu_navigation(text_x, text_y, count, path, modes[selection]);
    }
}


/* The signed field coordinates follow the file editor's modes initializer. */
struct file_field file_fields[3] = {{26, 2, 1}, {53, 2, 13}, {1, 4, 64}};


/* objcmp: {"function":"configuration_screen","address":"158d:2256","size":572,"symbols":{"_configuration_active":"DS:24a0","_configuration_panels":"DS:24e5","_alt_image_count":"DS:5d9c","_ctrl_image_count":"DS:5d9a","_fade_count":"DS:5d98","_editable_images":"1e81:0203","_editable_image_count":"1e81:0257","_alternate_image_sizes":"DS:0202","_control_image_sizes":"DS:041e","_fade_records":"DS:0474","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_exit_requested":"DS:5d80","_toggle_menu_mode":"158d:0259","_draw_framed_box":"158d:0332","_render_menu_text":"158d:0219","_beep":"11c1:0e07","_poll_graphics_keys":"11c1:18e7","_edit_image_sizes":"158d:0c7c","_edit_fade_enables":"158d:1050","_edit_auto_customization":"158d:1381","_file_access_menu":"158d:1bae","_alt_image_records":"DS:01fe","_ctrl_image_records":"DS:041a"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
void far configuration_screen(void)
{
    union {
        struct { unsigned x, y; } point;
        unsigned long value;
    } origin;
    unsigned column, row, title;
    struct config_panel far *panel;
    configuration_editor editor;
    register int selection = 1;
    unsigned index;

    if (configuration_active) {
        g_5d90 = g_5d92 = 0;
    } else {
        configuration_active = 1;
configuration_redraw:
        switch (selection) {
        case 1:
            panel = &configuration_panels[0];
            title = 3;
            editor = edit_image_sizes;
            editable_images = alt_image_records;
            editable_image_count = alt_image_count;
            index = alt_image_count;
configuration_rows:
            ++index;
            index >>= 1;
            goto configuration_ready;
        case 2:
            panel = &configuration_panels[1];
            title = 3;
            editor = edit_image_sizes;
            editable_images = ctrl_image_records;
            editable_image_count = ctrl_image_count;
            index = ctrl_image_count;
            goto configuration_rows;
        case 3:
            panel = &configuration_panels[2];
            title = 2;
            editor = edit_fade_enables;
            index = fade_count;
            goto configuration_rows;
        case 4:
            panel = &configuration_panels[3];
            title = 2;
            editor = edit_auto_customization;
            index = 0;
            break;
        default:
            panel = &configuration_panels[4];
            title = 1;
            editor = file_access_menu;
            index = 0;
        }
configuration_ready:
        toggle_menu_mode(1);
        origin.value = draw_framed_box(panel->x + index, panel->y, title);
        index = 0;
        column = origin.point.x;
        row = origin.point.y;
        while (panel->title[index] != 0) {
            if (panel->title[index] == '\n') {
                column = origin.point.x;
                ++row;
            } else {
                render_menu_text(column++, row, panel->title[index], panel->attributes);
            }
            ++index;
        }
        beep(0x190, 5);
        editor(origin.point.x, origin.point.y + panel->x);
        if (poll_graphics_keys() != 0) {
            toggle_menu_mode(0);
            configuration_active = 0;
            return;
        } else {
            switch (g_5d92) {
            case 0x8000: goto configuration_redraw;
            default:
                g_5d90 = g_5d92 = 0x6e;
                toggle_menu_mode(0);
                configuration_active = 0;
                return;
            case 0x4900:
                if (selection <= 1) goto configuration_redraw;
                --selection;
                goto configuration_redraw;
            case 0x5100:
                if (selection >= 5) goto configuration_redraw;
                ++selection;
                goto configuration_redraw;
            }
        }
    }
}


/* objcmp: {"function":"menu_error_message","address":"158d:2492","size":502,"symbols":{"_menu_active":"DS:24a2","_strlen":"0000:5a36","_render_menu_text":"158d:0219","_fprintf":"0000:45d4","_bioskey":"0000:2553","_exit_requested":"DS:5d80","_error_screen_shadow":"DS:441c","_error_screen_backup":"1e81:0000","_error_screen_format":"DS:39a4"},"segments":{"_DATA":"29bf:249e","FAR_DATA":"1e81:0000"}} */
/* These local controls retain the original repeated segment loads and exit JMP. */
#pragma option -Z- -O-
void far menu_error_message(char far *message)
{
    unsigned index;
    unsigned width;
    register unsigned column;
    register unsigned row;

    if (menu_active == 0) {
        fprintf(error_screen_shadow, "\007");
    } else {
        width = strlen(message);
        if (width > 0x4b)
            width = 0x4b;
        --width;
        for (row = 22; row < 25; ++row)
            for (column = 0; column <= width + 2; ++column)
                error_screen_backup[row][column] = *(unsigned far *)MK_FP(
                    0xb800, row * 160 + column * 2);
        for (row = 22; row < 25; ++row) {
            *(unsigned far *)MK_FP(0xb800, row * 160) = 0x4eba;
            *(unsigned far *)MK_FP(0xb800, row * 160 + (width + 2) * 2) = 0x4eba;
        }
        for (column = 0; column < width + 2; ++column) {
            *(unsigned far *)MK_FP(0xb800, 24 * 160 + column * 2) = 0x4ecd;
            *(unsigned far *)MK_FP(0xb800, 22 * 160 + column * 2) = 0x4ecd;
        }
        *(unsigned far *)MK_FP(0xb800, 22 * 160) = 0x4ec9;
        *(unsigned far *)MK_FP(0xb800, 22 * 160 + (width + 2) * 2) = 0x4ebb;
        *(unsigned far *)MK_FP(0xb800, 24 * 160) = 0x4ec8;
        *(unsigned far *)MK_FP(0xb800, 24 * 160 + (width + 2) * 2) = 0x4ebc;
        index = 0;
        column = 1;
        row = 23;
        while (message[index] != 0)
            render_menu_text(column++, row, message[index++], 0x4e);
        fprintf(error_screen_shadow, "\007");
        if ((bioskey(0) & 0x7f) == 0x1b)
            exit_requested = 1;
        for (row = 22; row < 25; ++row)
            for (column = 0; column <= width + 2; ++column)
                *(unsigned far *)MK_FP(0xb800, row * 160 + column * 2) =
                    error_screen_backup[row][column];
        goto error_finished;
    }
error_finished:
    ;
}

#pragma option -Z -O
