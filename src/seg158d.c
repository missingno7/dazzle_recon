#include <dos.h>

struct node {
    struct node far *next;
    char path[14];
};

extern struct node far *head;
extern void far free(void far *);
extern void far *malloc(unsigned size);
extern char far *strcpy(char far *destination, const char far *source);
extern struct node far * far directory_tail;
extern char near directory_entry_error[];
extern char near directory_blank[];
extern char far directory_path[80];
extern char near invalid_directory_error[];
extern void far menu_error_message(char far *message);
extern char near directory_allocation_error[];
extern char near no_image_configured_error[];
extern unsigned far set_image_mode(unsigned mask, unsigned value);
extern unsigned menu_active;
typedef void (far *menu_mode_callback)(unsigned value);
typedef void (far *menu_palette_callback)(unsigned char far *palette);
#define MENU_REDRAW_NEEDED (*(unsigned far *)MK_FP(0x29bf, 0x5cb1))
#define VIDEO_MODE_CALLBACK (*(menu_mode_callback far *)MK_FP(0x29bf, 0x5c43))
#define PALETTE_CALLBACK (*(menu_palette_callback far *)MK_FP(0x29bf, 0x5c37))
#define PALETTE_BUFFER_OFFSET (*(unsigned far *)MK_FP(0x29bf, 0x5d94))
#define PALETTE_BUFFER_SEGMENT (*(unsigned far *)MK_FP(0x29bf, 0x5d96))
#define PALETTE_CYCLE_OFFSET (*(unsigned far *)MK_FP(0x29bf, 0x5dec))
#define SAVED_FREEZE_LED_STATE (*(unsigned far *)MK_FP(0x29bf, 0x5d82))
extern int far findfirst(char far *filespec, void far *search_block,
                         unsigned attribute_mask);
extern int far findnext(void far *search_block);
extern unsigned far strlen(const char far *text);
extern char far *strcat(char far *destination, const char far *source);
extern char far *strlwr(char far *text);
extern int far path_access(char far *path);
extern void far *far file_exists(char far *path, unsigned attributes);

struct dos_find_block {
    unsigned char reserved[0x15];
    unsigned char attribute;
    unsigned char reserved2[8];
    char filename[13];
};

extern char near directory_search_spec[];
extern char near directory_separator[];
extern char near readonly_search_spec[];
extern char near box_title_default[];
extern char near box_title_fade[];
extern char near box_title_auto[];
extern char near box_title_file[];
extern struct dos_find_block far directory_find_data;
#define DOS_FIND_BLOCK ((struct dos_find_block far *)MK_FP( \
    FP_SEG(&directory_find_data), 0x01d8))
#define DOS_FIND_ATTRIBUTE (*(unsigned char far *)MK_FP( \
    FP_SEG(&directory_find_data), 0x01ed))
#define DOS_FIND_FILENAME ((char far *)MK_FP( \
    FP_SEG(&directory_find_data), 0x01f6))

/* objcmp: {"function":"append_directory_entry","address":"158d:0004","size":158,"symbols":{"_head":"DS:24b4","_free":"0000:2b47","_malloc":"0000:2c51","_strcpy":"0000:59cc","_directory_tail":"1e81:01d4","_menu_error_message":"158d:2492","_directory_allocation_error":"DS:38a3"}} */
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
        menu_error_message(directory_allocation_error);
        return 1;
    }
    return 0;
}

/* objcmp: {"function":"free_nodes","address":"158d:00a2","size":93,"symbols":{"_head":"DS:24b4","_free":"0000:2b47"}} */
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

/* objcmp: {"function":"enumerate_directory_entries","address":"158d:00ff","size":282,"symbols":{"_free_nodes":"158d:00a2","_findfirst":"0000:42d2","_findnext":"0000:4305","_strcpy":"0000:59cc","_strlen":"0000:5a36","_strcat":"0000:5950","_strlwr":"0000:5a55","_append_directory_entry":"158d:0004","_directory_search_spec":"DS:38c3","_directory_separator":"DS:38c7","_readonly_search_spec":"DS:24a4","_directory_find_data":"1e81:01d8"}} */
unsigned far enumerate_directory_entries(void)
{
    char path[14];
    unsigned found;
    unsigned allocation_failed;
    register unsigned search_status;

    found = 0;
    allocation_failed = 0;
    free_nodes();
    search_status = findfirst(directory_search_spec, DOS_FIND_BLOCK, 0x10);
    while (search_status == 0) {
        if (DOS_FIND_ATTRIBUTE & 0x10) {
            strcpy(path, DOS_FIND_FILENAME);
            if (path[strlen(path) - 1] != '\\')
                strcat(path, directory_separator);
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

/* objcmp: {"function":"render_menu_text","address":"158d:0219","size":64} */
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

/* objcmp: {"function":"toggle_menu_mode","address":"158d:0259","size":217,"symbols":{"_menu_active":"DS:24a2","_set_image_mode":"11c1:000a","_int86":"0000:320e","_menu_registers":"DS:6006","_saved_freeze_led_state":"DS:5d82","_menu_redraw_needed":"DS:5cb1","_set_video_mode_callback":"DS:5c43","_upload_palette_callback":"DS:5c37","_palette_buffer":"DS:5d94","_palette_buffer_segment":"DS:5d96","_palette_cycle_offset":"DS:5dec"},"level":"PROVISIONAL","blocker":"first-candidate"} */
void far toggle_menu_mode(unsigned active)
{
    int offset;

    if (active) {
        if (menu_active == 0) {
            ((union REGS far *)MK_FP(0x29bf, 0x6006))->x.ax = 3;
            int86(0x10, (union REGS far *)MK_FP(0x29bf, 0x6006),
                  (union REGS far *)MK_FP(0x29bf, 0x6006));
            menu_active = 1;
            SAVED_FREEZE_LED_STATE = set_image_mode(0x10, 0);
            set_image_mode(0x60, 1);
        }
        for (offset = 0; offset < 0x0fa0; offset += 2)
            *(unsigned far *)MK_FP(0xb800, offset) = 0x09b1;
    } else {
        menu_active = 0;
        MENU_REDRAW_NEEDED = 1;
        VIDEO_MODE_CALLBACK(1);
        PALETTE_CALLBACK((unsigned char far *)MK_FP(
            PALETTE_BUFFER_SEGMENT,
            PALETTE_BUFFER_OFFSET + PALETTE_CYCLE_OFFSET));
        set_image_mode(SAVED_FREEZE_LED_STATE, 2);
    }
}

/* objcmp: {"function":"draw_framed_box","address":"158d:0332","size":682,"symbols":{"_render_menu_text":"158d:0219","_box_title_default":"DS:38c9","_box_title_fade":"DS:38cf","_box_title_auto":"DS:38d6","_box_title_file":"DS:38e2"},"level":"PROVISIONAL","blocker":"first-candidate"} */
unsigned long far draw_framed_box(unsigned height, unsigned width,
                                  unsigned title_id)
{
    int left;
    int right;
    int top;
    int bottom;
    register int row;
    register int column;
    unsigned length;
    char far *title;
    char far *titles[4];

    titles[0] = box_title_default;
    titles[1] = box_title_fade;
    titles[2] = box_title_auto;
    titles[3] = box_title_file;

    if (width >= 80)
        width = 79;
    if (height >= 25)
        height = 24;

    left = ((80 - width) / 2) - 1;
    right = 81 - left;
    top = (25 - height) / 2;
    bottom = 24 - top;
    if (height & 1)
        ++bottom;

    for (row = top; row < bottom; ++row) {
        register unsigned far *cell = (unsigned far *)MK_FP(0xb800,
            row * 160 + left * 2);
        for (column = left; column < right; ++column)
            *cell++ = 0x7020;
    }

    for (row = top; row < bottom; ++row) {
        *(unsigned far *)MK_FP(0xb800,
            row * 160 + (left - 1) * 2) = 0x74ba;
    }
    for (row = top; row < bottom; ++row) {
        *(unsigned far *)MK_FP(0xb800, row * 160 + right * 2) = 0x74ba;
    }
    for (column = left; column < right; ++column) {
        *(unsigned far *)MK_FP(0xb800,
            (top - 1) * 160 + column * 2) = 0x74cd;
    }
    for (column = left; column < right; ++column) {
        *(unsigned far *)MK_FP(0xb800, bottom * 160 + column * 2) = 0x74cd;
    }

    *(unsigned far *)MK_FP(0xb800,
        (top - 1) * 160 + (left - 1) * 2) = 0x74c9;
    *(unsigned far *)MK_FP(0xb800,
        (top - 1) * 160 + right * 2) = 0x74bb;
    *(unsigned far *)MK_FP(0xb800,
        bottom * 160 + (left - 1) * 2) = 0x74c8;
    *(unsigned far *)MK_FP(0xb800, bottom * 160 + right * 2) = 0x74bc;

    for (row = top + 1; row <= bottom + 1; ++row) {
        *(unsigned far *)MK_FP(0xb800,
            row * 160 + (right + 1) * 2) = 0x01b1;
    }
    for (column = left + 1; column <= right; ++column) {
        *(unsigned far *)MK_FP(0xb800,
            (bottom + 1) * 160 + column * 2) = 0x01b1;
    }

    if (title_id != 4) {
        switch (title_id) {
        case 1: title = titles[1]; break;
        case 2: title = titles[2]; break;
        case 3: title = titles[3]; break;
        default: title = titles[0]; break;
        }
        length = strlen(title);
        column = right - 2 - length;
        for (row = 0; row < length; ++row)
            render_menu_text(column + row, bottom, title[row], 0x74);
    }
    return ((unsigned long)(unsigned)top << 16) | (unsigned)(left + 1);
}

extern char far help_page_data[];
extern unsigned help_menu_active;
extern unsigned help_current_page;
extern unsigned help_previous_page;
extern unsigned help_wait_flag;
extern unsigned help_key_state;
extern unsigned help_countdown;
extern void far help_delay(unsigned ticks, unsigned count);
extern int far keyboard_status(unsigned wait);
extern unsigned far update_key_state(void);

/* objcmp: {"function":"help_screen","address":"158d:05dc","size":459,"symbols":{"_help_page_data":"DS:24b8","_help_menu_active":"DS:249e","_help_current_page":"DS:5d92","_help_previous_page":"DS:5d90","_help_wait_flag":"DS:5d7c","_help_key_state":"DS:5d80","_help_countdown":"DS:5d7e","_toggle_menu_mode":"158d:0259","_draw_framed_box":"158d:0332","_render_menu_text":"158d:0219","_help_delay":"11c1:0e07","_keyboard_status":"0000:2553","_update_key_state":"11c1:18e7","_strlen":"0000:5a36"},"level":"PROVISIONAL","blocker":"first-candidate"} */
void far help_screen(void)
{
    unsigned page;
    unsigned title_id;
    unsigned row;
    unsigned column;
    unsigned text_column;
    unsigned code;
    unsigned long origin;
    unsigned char far *descriptor;
    char far *text;
    unsigned char character;

    page = 0;
    if (help_menu_active) {
        help_current_page = 0;
        help_previous_page = 0;
        return;
    }

    help_menu_active = 1;
    for (;;) {
        switch (page) {
        case 0:
            descriptor = (unsigned char far *)help_page_data + 4 * 9;
            title_id = 1;
            break;
        case 1:
            descriptor = (unsigned char far *)help_page_data + 3 * 9;
            title_id = 2;
            break;
        case 2:
            descriptor = (unsigned char far *)help_page_data + 2 * 9;
            title_id = 2;
            break;
        case 3:
            descriptor = (unsigned char far *)help_page_data + 1 * 9;
            title_id = 2;
            break;
        default:
            descriptor = (unsigned char far *)help_page_data;
            title_id = 3;
            break;
        }

        toggle_menu_mode(1);
        origin = draw_framed_box(*(unsigned far *)descriptor,
                                 *(unsigned far *)(descriptor + 2), title_id);
        row = (unsigned)(origin >> 16);
        column = (unsigned)origin;
        text_column = column;
        text = *(char far * far *)(descriptor + 5);
        code = 0;
        while ((character = (unsigned char)text[code]) != 0) {
            if (character == '\n') {
                ++row;
                column = text_column;
            } else {
                render_menu_text(column, row, character, descriptor[4]);
                ++column;
            }
            ++code;
        }

        help_delay(0x190, 5);
        if (help_wait_flag == 0) {
            if (keyboard_status(1) == 0 && help_key_state == 0) {
                help_countdown = 0;
                continue;
            }
        }

        if (update_key_state() != 0)
            break;

        switch (help_current_page) {
        case 0x4700:
            page = 0;
            continue;
        case 0x4900:
            if (page < 4)
                ++page;
            continue;
        case 0x4f00:
            if (page > 0)
                --page;
            continue;
        case 0x5100:
            page = 4;
            continue;
        default:
            help_current_page = 0x6e;
            help_previous_page = 0x6e;
            break;
        }
        break;
    }

    toggle_menu_mode(0);
    help_menu_active = 0;
}

extern unsigned far menu_countdown;
extern unsigned far menu_wait_flag;
extern unsigned far menu_cancelled;
extern unsigned far menu_key_state;
extern unsigned far menu_previous_key;
extern void far help_delay(unsigned ticks, unsigned count);
extern int far keyboard_status(unsigned wait);
extern unsigned far read_keyboard_key(unsigned wait);
extern void far menu_cursor(unsigned x, unsigned y);
extern void far menu_idle(unsigned far *delay_cell, unsigned count);
extern int far tolower(int value);
extern char far *strchr(char far *text, int value);

/* objcmp: {"function":"menu_navigation","address":"158d:07a7","size":1122,"symbols":{"_menu_countdown":"DS:5d7e","_menu_wait_flag":"DS:5d7c","_menu_cancelled":"DS:5d80","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_help_delay":"11c1:0e07","_keyboard_status":"0000:2553","_read_keyboard_key":"0000:2578","_menu_cursor":"0000:2fd8","_menu_idle":"0000:32d8","_tolower":"0000:1f66","_strlen":"0000:5a36","_render_menu_text":"158d:0219"},"level":"PROVISIONAL","blocker":"complete-control-flow candidate; strict extent mismatch at 158d:07a7"} */
unsigned far menu_navigation(unsigned x, unsigned y, unsigned maximum,
                             char far *text, unsigned mode)
{
    unsigned position = 0;
    unsigned cursor;
    unsigned key;
    unsigned i;
    unsigned edit_state = 1;
    unsigned special;
    unsigned action;
    char ch;

    if (mode == 1) {
        position = strlen(text);
        if (position < maximum) {
            for (i = position; i < maximum; ++i)
                text[i] = 0;
        }
    }
    menu_countdown = 0;
    menu_wait_flag = 0;

    for (;;) {
        while (!menu_cancelled && !menu_wait_flag) {
            if (keyboard_status(1) != 0)
                break;
            geninterrupt(0x28);
            if (*(unsigned char far *)MK_FP(0x29bf, 0x007d) >= 3) {
                unsigned wait_cell = 0x1680;
                menu_idle((unsigned far *)&wait_cell, 0x2f);
            }
        }
        if (menu_cancelled || menu_wait_flag) {
            menu_cursor(1, 1);
            return 9;
        }
        key = read_keyboard_key(0);
        menu_key_state = key;
        if (key == 0)
            continue;
        ch = (char)key;
        if (ch == 0x1b) {
            menu_cancelled = 1;
            menu_cursor(1, 1);
            return 9;
        }

        if (ch == 0) {
            special = 1;
            action = 0;
            if (edit_state == 0) {
                switch (key) {
                case 0x4800:
                case 0x4b00: action = 8; break;
                case 0x4d00:
                case 0x5000: action = 9; break;
                default: special = 0; break;
                }
            } else {
                switch (key) {
                case 0x3b00:
                case 0x3c00:
                case 0x3d00:
                case 0x3e00: action = 10; break;
                case 0x4700: action = 4; break;
                case 0x4800: action = 2; break;
                case 0x4900: action = 6; break;
                case 0x4b00: action = 0; break;
                case 0x4d00: action = 1; break;
                case 0x4f00: action = 5; break;
                case 0x5000: action = 3; break;
                case 0x5100: action = 7; break;
                default: special = 0; break;
                }
            }
            if (special) {
                menu_countdown = 0;
                menu_wait_flag = 0;
                menu_previous_key = key;
                return action;
            }
            menu_key_state = menu_previous_key = 0;
            menu_countdown = 0;
            menu_wait_flag = 0;
            help_delay(0x04c9, 5);
            continue;
        }

        if (ch == '\r') {
            if (edit_state == 0) {
                text[position + 1] = 0;
                while (position != 0 && text[position - 1] == ' ')
                    text[--position] = 0;
            }
            menu_cursor(1, 1);
            return 8;
        } else if (ch == '\t') {
            if (mode == 1 && position < maximum) {
                ++position;
                edit_state = 0;
            }
        } else if (ch == '\b') {
            if (edit_state == 0) {
                text[position + 1] = 0;
            } else {
                while (position != 0) {
                    if (text[position - 1] == ' ') {
                        text[--position] = 0;
                    } else {
                        position = 0;
                    }
                }
            }
        } else {
            switch (mode) {
            case 0:
                if (ch < '0' || ch > '9')
                    ch = 0;
                break;
            case 1:
                if (ch < ' ' || ch > '~')
                    ch = 0;
                break;
            case 2:
                ch = (char)tolower(ch);
                if (ch != 'y' && ch != 'n')
                    ch = 0;
                break;
            case 3:
                ch = (char)tolower(ch);
                if (ch != 's' && ch != 'l')
                    ch = 0;
                break;
            default:
                ch = 0;
                break;
            }
            if (ch != 0 && position < maximum) {
                text[position++] = ch;
                text[position] = 0;
                edit_state = 0;
            }
        }

        cursor = 0;
        while (cursor < position && text[cursor] != 0) {
            render_menu_text(x + cursor, y, text[cursor], 0x0f);
            ++cursor;
        }
        while (cursor < maximum) {
            render_menu_text(x + cursor, y, ' ', 0x0f);
            ++cursor;
        }
        if (mode == 2)
            menu_cursor(x + position, y + 1);
        else
            menu_cursor(x + position + 1, y + 1);
        menu_previous_key = menu_key_state;
        help_delay(0x04c9, 5);
    }
}

struct image_size_record {
    unsigned size;
    unsigned char reserved[13];
};

extern void far menu_error_message(char far *message);

extern unsigned far configured_image_count;
extern unsigned far configured_control_count;
extern struct image_size_record far alternate_image_sizes[];
extern struct image_size_record far control_image_sizes[];

#define IMAGE_COUNT configured_image_count
#define OTHER_IMAGE_COUNT configured_control_count
#define ALT_IMAGE_SIZES alternate_image_sizes
#define CTRL_IMAGE_SIZES control_image_sizes

/* objcmp: {"function":"check_any_image_configured","address":"158d:0c09","size":115,"symbols":{"_menu_error_message":"158d:2492","_no_image_configured_error":"DS:38e9","_configured_image_count":"DS:5d9c","_configured_control_count":"DS:5d9a","_alternate_image_sizes":"DS:0202","_control_image_sizes":"DS:041e"}} */
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
    menu_error_message(no_image_configured_error);
    return 0;
}

struct editable_image_record {
    void (far *draw)(void);
    unsigned size;
    char far *description;
    unsigned char reserved[4];
    unsigned char key;
};

extern struct editable_image_record far * far editable_images;
extern unsigned far editable_image_count;
extern unsigned far maximum_image_size;
extern char near size_format[];
extern char near size_parse_format[];
extern int far sprintf(char far *destination, const char far *format, ...);
extern int far sscanf(const char far *source, const char far *format, ...);
extern unsigned far configured_fade_count;
extern unsigned far configured_auto_count;
extern char far * far image_fade_names[];
extern unsigned far image_fade_flags[];
extern char far *strcpy(char far *destination, const char far *source);
extern void far menu_delay(unsigned ticks, unsigned count);

/* objcmp: {"function":"edit_image_sizes","address":"158d:0c7c","size":980,"symbols":{"_editable_images":"1e81:0203","_editable_image_count":"1e81:0257","_maximum_image_size":"DS:5d88","_size_format":"DS:391e","_size_parse_format":"DS:3924","_sprintf":"0000:5893","_sscanf":"0000:5909","_check_any_image_configured":"158d:0c09","_menu_navigation":"158d:07a7","_render_menu_text":"158d:0219","_configured_image_count":"DS:5d9c","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_menu_countdown":"DS:5d7e","_menu_wait_flag":"DS:5d7c"},"level":"PROVISIONAL","blocker":"dispatch-layout +116 bytes at 158d:0c7c; all action cases and two-column record rendering represented"} */
void far edit_image_sizes(unsigned x, unsigned y)
{
    unsigned selected = 0;
    unsigned action;
    unsigned i;
    unsigned column;
    unsigned size;
    unsigned char far *layout = (unsigned char far *)MK_FP(0x29bf, 0x2512);
    char format[8];
    char input[8];

    sprintf(format, size_format, (unsigned)layout[0]);
    input[0] = 0;
    for (;;) {
        switch (action) {
        case 0:
            selected &= 0xfffe;
            break;
        case 1:
            selected |= 1;
            break;
        case 2:
            if (selected > 1)
                selected -= 2;
            break;
        case 3:
            if (selected + 2 < editable_image_count)
                selected += 2;
            break;
        case 4:
            selected = 0;
            break;
        case 5:
            selected = editable_image_count - 1;
            break;
        case 6:
            menu_key_state = menu_previous_key = 0x4900;
            menu_countdown = 0;
            menu_wait_flag = 0;
            return;
        case 7:
            menu_key_state = menu_previous_key = 0x5100;
            menu_countdown = 0;
            menu_wait_flag = 0;
            return;
        case 8:
            size = 0;
            if (sscanf(input, size_parse_format, &size) == 1) {
                if (size > maximum_image_size)
                    size = maximum_image_size;
                editable_images[selected].size = size;
                if (check_any_image_configured() == 0)
                    editable_images[selected].size = maximum_image_size;
                else
                    ++selected;
            }
            break;
        default:
            return;
        }

        if (selected >= editable_image_count)
            selected = 0;
        for (i = 0; i < editable_image_count; ++i) {
            unsigned side = i & 1;
            unsigned row = y + (i >> 1);
            unsigned base = side ? layout[4] : layout[1];
            unsigned char attribute = (i == selected) ? 0x0f : 0x70;

            for (column = 0;
                 column < (unsigned)(layout[3] - layout[1] + layout[0]);
                 ++column)
                render_menu_text(x + base - 1 + column, row, ' ', attribute);

            render_menu_text(x + base, row, editable_images[i].key, attribute);
            for (column = 0; editable_images[i].description[column] != 0;
                 ++column)
                render_menu_text(x + (side ? layout[5] : layout[2]) + column,
                                 row, editable_images[i].description[column],
                                 attribute);

            sprintf(input, format, editable_images[i].size);
            for (column = 0; input[column] != 0; ++column)
                render_menu_text(x + (side ? layout[6] : layout[3]) + column,
                                 row, input[column], attribute);
        }

        if (check_any_image_configured() == 0)
            editable_images[selected].size = maximum_image_size;
        size = editable_images[selected].size;
        sprintf(input, format, size);
        action = menu_navigation(
            x + ((selected & 1) ? layout[6] : layout[3]),
            y + (selected >> 1), layout[0], input, 0);
    }
}

struct fade_record {
    unsigned char identifier;
    char far *name;
    unsigned enabled;
};

extern struct fade_record far fade_records[];
extern unsigned far fade_record_count;
extern char near fade_enabled_format[];
extern char near fade_disabled_format[];
extern unsigned far menu_countdown;
extern unsigned far menu_wait_flag;
extern unsigned char near fade_layout[];

/* objcmp: {"function":"edit_fade_enables","address":"158d:1050","size":817,"symbols":{"_fade_records":"DS:0474","_fade_record_count":"DS:5d98","_fade_enabled_format":"DS:3932","_fade_disabled_format":"DS:3936","_fade_layout":"DS:2512","_menu_navigation":"158d:07a7","_render_menu_text":"158d:0219","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_menu_countdown":"DS:5d7e","_menu_wait_flag":"DS:5d7c","_menu_delay":"11c1:0e07"},"level":"PROVISIONAL","blocker":"complete-control-flow candidate; strict extent mismatch at 158d:1050"} */
void far edit_fade_enables(unsigned x, unsigned y)
{
    unsigned selected = 0;
    unsigned i;
    unsigned col;
    char choice[4];
    unsigned action;

    choice[0] = 'y';
    choice[1] = 0;
    action = 4;
    do {
        unsigned char far *record_bytes;
        unsigned char attribute;
        unsigned start;
        unsigned width;
        unsigned name_x;
        unsigned key_x;
        unsigned state_x;

        switch (action) {
        case 0:
            selected &= 0xfffe;
            break;
        case 1:
            selected |= 1;
            break;
        case 2:
            if (selected > 1)
                selected -= 2;
            break;
        case 3:
            if (selected + 2 < fade_record_count)
                selected += 2;
            break;
        case 4:
            selected = 0;
            break;
        case 5:
            selected = fade_record_count - 1;
            break;
        case 6:
            menu_key_state = menu_previous_key = 0x4900;
            menu_countdown = 0;
            menu_wait_flag = 0;
            return;
        case 7:
            menu_key_state = menu_previous_key = 0x5100;
            menu_countdown = 0;
            menu_wait_flag = 0;
            return;
        case 8:
            if (choice[0] == 'n') {
                fade_records[selected].enabled = 0;
                ++selected;
            } else if (choice[0] == 'y') {
                fade_records[selected].enabled = 1;
                ++selected;
            } else {
                menu_delay(0xfa, 0x0a);
            }
            break;
        }

        if (selected >= fade_record_count)
            selected = 0;
        for (i = 0; i < fade_record_count; ++i) {
            unsigned side = i & 1;
            unsigned row = y + (i >> 1);
            start = x + (side ? fade_layout[13] : fade_layout[10]) - 1;
            width = fade_layout[12] - fade_layout[10] + 1;
            attribute = (i == selected) ? 0x0f : 0x70;
            for (col = 0; col < width; ++col)
                render_menu_text(start + col, row, ' ', attribute);

            record_bytes = (unsigned char far *)&fade_records[i];
            key_x = x + (side ? fade_layout[14] : fade_layout[11]);
            render_menu_text(key_x, row, record_bytes[0], attribute);

            name_x = x + (side ? fade_layout[13] : fade_layout[12]);
            for (col = 0; fade_records[i].name[col] != 0; ++col)
                render_menu_text(name_x + col, row,
                                 fade_records[i].name[col], attribute);

            state_x = x + (side ? fade_layout[15] : fade_layout[12]);
            render_menu_text(state_x, row,
                             fade_records[i].enabled ? 'y' : 'n', attribute);
        }

        choice[0] = fade_records[selected].enabled ? 'y' : 'n';
        choice[1] = 0;
        start = x + (selected & 1 ? fade_layout[15] : fade_layout[12]);
        action = menu_navigation(start, y + (selected >> 1), 1, choice, 2);
    } while (action != 9);
}

extern unsigned far auto_minimum;
extern unsigned far auto_maximum;
extern unsigned far auto_pause;
extern char near auto_value_format[];
extern char near auto_value_parse_format[];
extern unsigned char near auto_input_width;
extern unsigned char near auto_field_positions[];

/* objcmp: {"function":"edit_auto_customization","address":"158d:1381","size":896,"symbols":{"_auto_minimum":"DS:5cc5","_auto_maximum":"DS:5cc3","_auto_pause":"DS:5da8","_auto_value_format":"DS:391e","_auto_value_parse_format":"DS:3924","_auto_input_width":"DS:2522","_auto_field_positions":"DS:2523","_menu_navigation":"158d:07a7","_render_menu_text":"158d:0219","_sprintf":"0000:5893","_sscanf":"0000:5909","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_menu_countdown":"DS:5d7e","_menu_wait_flag":"DS:5d7c"},"level":"PROVISIONAL","blocker":"complete-control-flow candidate; strict extent mismatch at 158d:1381"} */
void far edit_auto_customization(unsigned x, unsigned y)
{
    unsigned field = 0;
    unsigned value;
    unsigned i;
    char input[16];
    char format[8];
    char rendered[16];
    unsigned action;

    sprintf(format, auto_value_format, auto_input_width);
    input[0] = 0;
    do {
        switch (action) {
        case 0:
            field = 2;
            break;
        case 1:
            field = 0;
            break;
        case 2:
            field = 2;
            break;
        case 3:
            if (field != 0)
                --field;
            break;
        case 4:
            break;
        case 5:
            menu_key_state = menu_previous_key = 0x4900;
            menu_countdown = 0;
            menu_wait_flag = 0;
            return;
        case 6:
            menu_key_state = menu_previous_key = 0x5100;
            menu_countdown = 0;
            menu_wait_flag = 0;
            return;
        case 7:
            field = 0;
            break;
        case 8:
            value = 0;
            if (sscanf(input, auto_value_parse_format, &value) == 1) {
                if (value > 0x100)
                    value = 0x100;
                if (field == 0) {
                    auto_minimum = value;
                    if (auto_minimum < 1)
                        auto_minimum = 1;
                } else if (field == 1) {
                    auto_maximum = value;
                    if (auto_maximum < auto_minimum)
                        auto_maximum = auto_minimum;
                } else {
                    auto_pause = value * 0x12;
                }
                ++field;
            }
            break;
        default:
            return;
        }

        if (field > 2)
            field = 0;

        for (i = 0; i < 3; ++i) {
            unsigned row = y - 6;
            unsigned column = x + auto_field_positions[i];
            unsigned shown;
            unsigned count;
            unsigned char attribute = (i == field) ? 0x0f : 0x70;

            if (i == 2)
                row = y - 4;
            for (count = 0; count < auto_input_width; ++count)
                render_menu_text(column + count, row, ' ', attribute);
            if (i == 0)
                shown = auto_minimum;
            else if (i == 1)
                shown = auto_maximum;
            else
                shown = auto_pause / 0x12;
            sprintf(rendered, format, shown);
            for (count = 0; rendered[count] != 0; ++count)
                render_menu_text(column + count, row, rendered[count],
                                 attribute);
        }

        if (field == 0)
            value = auto_minimum;
        else if (field == 1)
            value = auto_maximum;
        else
            value = auto_pause / 0x12;
        sprintf(input, format, value);
        action = menu_navigation(x + auto_field_positions[field], y - 6,
                                auto_input_width, input, 0);
    } while (action != 9);
}

/* objcmp: {"function":"configuration_directory_menu","address":"158d:1701","size":1095,"symbols":{"_toggle_menu_mode":"158d:0259","_help_delay":"11c1:0e07","_file_exists":"0000:4b1f","_path_access":"0000:1017","_menu_error_message":"158d:2492","_enumerate_directory_entries":"158d:00ff","_strcpy":"0000:59cc","_strcat":"0000:5950","_strlen":"0000:5a36","_free_nodes":"158d:00a2","_directory_search_spec":"DS:38c3","_directory_separator":"DS:38c7","_readonly_search_spec":"DS:24a4","_head":"DS:24b4","_directory_tail":"1e81:01d4","_directory_path":"1e81:0207","_directory_entry_error":"DS:3927","_invalid_directory_error":"DS:394f","_directory_blank":"DS:394d","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_menu_cancelled":"DS:5d80","_menu_navigation":"158d:07a7","_draw_framed_box":"158d:0332","_render_menu_text":"158d:0219"},"level":"PROVISIONAL","blocker":"complete browser candidate; four-column 64-entry page and output-path branch implemented; strict layout remains"} */
void far configuration_directory_menu(char far *output_path)
{
    char directory[0x50];
    char entry_path[14];
    struct node far *entry;
    register int selected = 0;
    register int first = 0;
    unsigned count;
    unsigned i;
    unsigned row;
    unsigned column;
    unsigned box_x;
    unsigned box_y;
    unsigned length;
    unsigned action;
    unsigned box_height;
    unsigned box_width;
    unsigned column_width;
    unsigned columns;
    unsigned page_entries;

    box_height = 0x10;
    box_width = 0x46;
    column_width = 0x12;
    columns = 4;
    page_entries = columns * box_height;
    toggle_menu_mode(1);
    help_delay(0x190, 5);
    action = 4;
    if (file_exists(directory_path, 0x50) == 0) {
        menu_error_message(directory_entry_error);
        menu_key_state = menu_previous_key = 'n';
        goto done;
    }

    directory[0] = 0;
    strcpy((char far *)directory, directory_blank);
    strcat((char far *)directory, directory_path);
    length = strlen((char far *)directory);
    if (length != 0 && directory[length - 1] != '\\')
        strcat((char far *)directory, directory_separator);
    strcat((char far *)directory, readonly_search_spec);
    strcat((char far *)directory, directory_blank);

    for (;;) {
        count = enumerate_directory_entries();
        if (menu_cancelled)
            goto done;
        if (action > 8) {
            menu_key_state = menu_previous_key = 'n';
            goto done;
        }
        switch (action) {
        case 0: selected = count - 1; break;
        case 1: --selected; break;
        case 2: ++selected; break;
        case 3: selected -= columns; break;
        case 4: selected += columns; break;
        case 5: selected -= page_entries; break;
        case 6: selected += page_entries; break;
        case 7: selected = 0; break;
        case 8:
            entry = head;
            for (i = 0; (int)i < selected && entry != 0; ++i)
                entry = entry->next;
            if (entry == 0)
                goto done;
            strcpy((char far *)entry_path, entry->path);
            length = strlen((char far *)entry_path);
            if (length != 0 && entry_path[length - 1] == '\\') {
                entry_path[length - 1] = 0;
                if (path_access((char far *)entry_path) != 0) {
                    menu_error_message(invalid_directory_error);
                    free_nodes();
                    continue;
                }
                free_nodes();
                continue;
            } else {
                strcpy(output_path, (char far *)entry_path);
                goto done;
            }
        }

        if (selected < 0)
            selected = 0;
        if (selected >= (int)count)
            selected = count - 1;

        {
            unsigned long box = draw_framed_box(box_height, box_width, 4);
            box_x = (unsigned)box;
            box_y = (unsigned)(box >> 16);
        }
        length = strlen((char far *)directory);
        for (i = 0; i < length; ++i)
            render_menu_text(box_x + (box_width - length) / 2 + i,
                             box_y - 1, directory[i], 0x70);

        if ((int)count - (int)columns - 1 <= first)
            first = (int)count - (int)columns - 1;
        if (first < 0)
            first = 0;
        if (first + (int)page_entries <= selected) {
            first = selected - page_entries;
            first += columns - (unsigned)first % columns;
        } else if (selected <= first) {
            first = selected;
        }
        first -= (unsigned)first % columns;
        entry = head;
        for (i = 0; i < (unsigned)first && entry != 0; ++i)
            entry = entry->next;

        row = 0;
        column = 1;
        i = (unsigned)first;
        while (entry != 0 && row < box_height) {
            unsigned j;
            unsigned char attribute = (i == (unsigned)selected) ? 0x0f : 0x70;
            for (j = 0; entry->path[j] != 0; ++j)
                render_menu_text(box_x + 2 + column * column_width + j,
                                 box_y - 1 + row, entry->path[j], attribute);
            entry = entry->next;
            ++i;
            ++column;
            if (column > columns) {
                column = 1;
                ++row;
            }
        }

        action = menu_navigation(1, 1, 0, (char far *)directory, 4);
        if (action == 9)
            goto done;
    }

done:
    free_nodes();
}

extern char far *far copy_path_prefix(char far *path, unsigned maximum);
extern unsigned far select_current_drive(unsigned drive);
extern char near invalid_directory_error[];

/* objcmp: {"function":"select_directory_drive","address":"158d:1b48","size":102,"symbols":{"_path_access":"0000:1017","_menu_error_message":"158d:2492","_invalid_directory_error":"DS:394f","_copy_path_prefix":"0000:598f","_tolower":"0000:1f66","_select_current_drive":"0000:128e"},"level":"PROVISIONAL","blocker":"drive-path-setup at 158d:1b48"} */
int far select_directory_drive(char far *path)
{
    char far *prefix;

    if (path_access(path) != 0) {
        menu_error_message(invalid_directory_error);
        return 0;
    }
    prefix = copy_path_prefix(path, 0x3a);
    if (prefix != 0)
        select_current_drive((unsigned)(tolower(prefix[-1]) + 0xff9f));
    return 1;
}

extern unsigned far file_dialog_active;
extern unsigned far file_dialog_operation;
extern char near configuration_file_path[];
extern char far * far configuration_default_path;
extern char near file_dialog_mode;
extern char near configuration_extension[];
extern char near configuration_read_error[];
extern char near configuration_exists_text[];
extern char near configuration_readonly_text[];
extern char near configuration_not_found_text[];
extern unsigned far split_configuration_path(char far *path, char far *drive,
                                             char far *directory, char far *name,
                                             char far *extension);
extern void far make_configuration_directory(char far *path, char far *drive,
                                            char far *directory,
                                            char far *name,
                                            char far *extension);
extern int far open_configuration_file(char far *path, unsigned mode);
extern int far load_configuration_file(char far *path);
extern int far save_configuration_file(char far *path);
extern int far configuration_file_search(char far *pattern,
                                         void far *search_block,
                                         unsigned attributes);
extern int far configuration_file_next(void far *search_block);
extern void far *far configuration_file_exists(unsigned far *search_block,
                                               unsigned attribute_mask);
extern int far path_access(char far *path);
extern char far *far copy_path_prefix(char far *path, unsigned maximum);
extern unsigned char near configuration_file_layout[];

/* objcmp: {"function":"file_access_menu","address":"158d:1bae","size":1704,"symbols":{"_toggle_menu_mode":"158d:0259","_menu_navigation":"158d:07a7","_configuration_directory_menu":"158d:1701","_select_directory_drive":"158d:1b48","_menu_error_message":"158d:2492","_render_menu_text":"158d:0219","_draw_framed_box":"158d:0332","_split_configuration_path":"0000:439f","_make_configuration_directory":"0000:4374","_strcpy":"0000:59cc","_strcat":"0000:5950","_strlen":"0000:5a36","_configuration_file_search":"0000:42d2","_configuration_file_next":"0000:4305","_configuration_file_exists":"0000:4b1f","_path_access":"0000:1017","_copy_path_prefix":"0000:598f","_open_configuration_file":"11c1:0d85","_load_configuration_file":"11c1:29ae","_save_configuration_file":"11c1:2b44","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_menu_cancelled":"DS:5d80","_file_dialog_mode":"DS:24b2","_configuration_file_path":"DS:5c5f","_configuration_default_path":"DS:5ffa","_configuration_extension":"DS:3968","_configuration_read_error":"DS:396d","_configuration_exists_text":"DS:398a","_configuration_readonly_text":"DS:3990","_configuration_not_found_text":"DS:399a","_configuration_file_layout":"DS:252c","_directory_find_data":"1e81:01d8"},"level":"PROVISIONAL","blocker":"file-dialog-control-flow +719 bytes at 158d:1bae; path parsing, filtering, editing, drive selection, load and save branches represented"} */
void far file_access_menu(unsigned x, unsigned y)
{
    struct file_menu_field {
        unsigned char x;
        unsigned char y;
        unsigned char width;
    } far *layout = (struct file_menu_field far *)
        MK_FP(0x29bf, 0x252c);
    char path[0x50];
    char drive[8];
    char name[8];
    char extension[8];
    char far *lines[4];
    unsigned selection = 0;
    unsigned action;
    unsigned flags;
    unsigned row;
    unsigned column;
    unsigned i;
    unsigned key;
    unsigned long box;
    int search_status;
    char far *prefix;

    file_dialog_active = 1;
    flags = split_configuration_path(configuration_file_path, drive, path,
                                     name, extension);
    if ((flags & 4) == 0 || (flags & 2) == 0)
        strcpy(configuration_file_path, configuration_default_path);
    else
        make_configuration_directory(configuration_file_path, drive, path,
                                     name, extension);

    if ((flags & 8) != 0 || (flags & 0x10) != 0) {
        i = strlen(path);
        if (i > 1 && path[i - 1] == '\\')
            path[i - 1] = 0;
        select_directory_drive(path);
        file_dialog_active = 0;
        return;
    }

    lines[0] = (char far *)MK_FP(0x29bf, 0x24b2);
    lines[1] = configuration_file_path;
    lines[2] = DOS_FIND_FILENAME;
    lines[3] = (char far *)path;
    toggle_menu_mode(1);
    for (;;) {
        box = draw_framed_box(0x12, 0x46, 4);
        row = (unsigned)(box >> 16);
        column = (unsigned)box;

        for (i = 0; i < 3; ++i) {
            char far *text = lines[i];
            unsigned char attribute = (i == selection) ? 0x0f : 0x70;
            unsigned width = layout[i].width;
            unsigned field_x = x + layout[i].x;
            unsigned field_y = y + layout[i].y;

            for (key = 0; key < width; ++key)
                render_menu_text(field_x + key, field_y, ' ', attribute);
            for (key = 0; text[key] != 0 && key < width; ++key)
                render_menu_text(field_x + key, field_y, text[key], attribute);
        }

        if (selection == 1) {
            for (key = 0; key < 9; ++key)
                render_menu_text(x + layout[1].x + key, y + layout[1].y + 1,
                                 ' ', 0x70);
        }

        strcpy(path, lines[1]);
        search_status = strlen(path);
        if (search_status > 0 && path[search_status - 1] != '\\')
            strcat(path, directory_separator);
        strcat(path, readonly_search_spec);
        search_status = configuration_file_search(path, DOS_FIND_BLOCK, 0x10);
        i = 0;
        while (search_status == 0) {
            char far *filename = DOS_FIND_FILENAME;
            row = y + 6 + i;
            for (column = 0; filename[column] != 0; ++column)
                render_menu_text(x + 2 + column, row, filename[column], 0x07);
            ++i;
            search_status = configuration_file_next(DOS_FIND_BLOCK);
        }

        action = menu_navigation(x + layout[selection].x,
                                 y + layout[selection].y,
                                 layout[selection].width, lines[selection],
                                 selection == 0 ? 3 : 1);
        if (action == 9 || menu_cancelled)
            break;
        if (action == 6 || action == 7) {
            menu_key_state = menu_previous_key =
                (action == 6) ? 0x4900 : 0x5100;
            menu_countdown = 0;
            menu_wait_flag = 0;
            break;
        }
        if (action == 0) {
            selection = 2;
            continue;
        }
        if (action == 1) {
            if (selection != 0)
                --selection;
            continue;
        }
        if (action == 2) {
            if (selection < 2)
                ++selection;
            continue;
        }
        if (action == 3) {
            if (selection < 2)
                selection += 2;
            continue;
        }
        if (action == 4) {
            selection = 0;
            continue;
        }
        if (action == 5) {
            selection = 2;
            continue;
        }
        key = menu_key_state;
        if (action == 8) {
            if (selection == 0) {
                if (file_dialog_mode == 'l' || file_dialog_mode == 's')
                    ++selection;
            } else if (selection == 1) {
                prefix = copy_path_prefix(path, '.');
                if (prefix != 0)
                    prefix[4] = 0;
                else
                    strcat(path, configuration_extension);
                strcpy(configuration_file_path, path);
            } else {
                if (path_access(path) != 0) {
                    menu_error_message(configuration_not_found_text);
                    continue;
                }
                prefix = copy_path_prefix(path, ':');
                if (prefix == 0) {
                    select_directory_drive(path);
                    continue;
                }
                if (file_dialog_mode == 'l') {
                    if (!open_configuration_file(configuration_file_path, 0)) {
                        menu_error_message(configuration_read_error);
                        continue;
                    }
                    if (!load_configuration_file(configuration_file_path))
                        menu_error_message(configuration_read_error);
                } else {
                    if (configuration_file_exists(DOS_FIND_BLOCK, 0x3f)) {
                        menu_error_message(configuration_exists_text);
                        continue;
                    }
                    if (!open_configuration_file(configuration_file_path, 1)) {
                        menu_error_message(configuration_readonly_text);
                        continue;
                    }
                    if (!save_configuration_file(configuration_file_path)) {
                        menu_delay(0x1f4, 0x0c);
                        menu_key_state = 0x5100;
                    }
                }
                break;
            }
            continue;
        }
        if (key == 'd' || key == 'D') {
            configuration_directory_menu(configuration_file_path);
            continue;
        }
        if (key == 'm' || key == 'M') {
            make_configuration_directory(configuration_file_path, drive, path,
                                         name, extension);
            continue;
        }
        if (key == '\\' && path[0] == 0) {
            menu_error_message(configuration_not_found_text);
            continue;
        }
        if (configuration_file_exists(DOS_FIND_BLOCK, 0x50) == 0)
            menu_error_message(directory_entry_error);
    }
    file_dialog_active = 0;
    toggle_menu_mode(0);
}

struct config_panel {
    unsigned x;
    unsigned y;
    unsigned char attributes;
    char far *title;
};

extern struct config_panel far configuration_panels[];
extern unsigned far configuration_active;
extern unsigned far configured_image_count;
extern unsigned far configured_control_count;
extern unsigned far configured_fade_count;
extern unsigned far configured_auto_count;
extern unsigned far menu_key_state;
extern unsigned far menu_previous_key;
extern unsigned far menu_cancelled;
extern unsigned far update_key_state(void);

typedef void (far *configuration_editor)(unsigned x, unsigned y);

/* objcmp: {"function":"configuration_screen","address":"158d:2256","size":572,"symbols":{"_configuration_active":"DS:24a0","_configuration_panels":"DS:24e5","_configured_image_count":"DS:5d9c","_configured_control_count":"DS:5d9a","_configured_fade_count":"DS:5d98","_editable_images":"1e81:0203","_editable_image_count":"1e81:0257","_alternate_image_sizes":"DS:0202","_control_image_sizes":"DS:041e","_fade_records":"DS:0474","_fade_record_count":"DS:5d98","_menu_key_state":"DS:5d92","_menu_previous_key":"DS:5d90","_menu_cancelled":"DS:5d80","_toggle_menu_mode":"158d:0259","_draw_framed_box":"158d:0332","_render_menu_text":"158d:0219","_help_delay":"11c1:0e07","_update_key_state":"11c1:18e7","_edit_image_sizes":"158d:0c7c","_edit_fade_enables":"158d:1050","_edit_auto_customization":"158d:1381","_file_access_menu":"158d:1bae"},"level":"PROVISIONAL","blocker":"configuration-dispatch-layout at 158d:2256"} */
void far configuration_screen(void)
{
    unsigned selection = 1;
    unsigned i;
    unsigned row;
    unsigned long box;
    configuration_editor editor;
    struct config_panel far *panel;
    char far *text;

    if (configuration_active) {
        menu_key_state = 0;
        menu_previous_key = 0;
        return;
    }
    configuration_active = 1;

    while (selection >= 1 && selection <= 5) {
        panel = configuration_panels + selection - 1;
        if (selection == 1) {
            editable_images = (struct editable_image_record far *)MK_FP(
                0x29bf, 0x01fe);
            editable_image_count = configured_image_count;
            editor = edit_image_sizes;
        } else if (selection == 2) {
            editable_images = (struct editable_image_record far *)MK_FP(
                0x29bf, 0x041a);
            editable_image_count = configured_control_count;
            editor = edit_image_sizes;
        } else if (selection == 3) {
            fade_record_count = configured_fade_count;
            editor = edit_fade_enables;
        } else if (selection == 4) {
            editor = edit_auto_customization;
        } else {
            editor = file_access_menu;
        }

        toggle_menu_mode(1);
        box = draw_framed_box(panel->x, panel->y, panel->attributes);
        row = (unsigned)(box >> 16);
        text = panel->title;
        i = 0;
        while (text[i] != 0) {
            if (text[i] == '\n') {
                ++row;
                i = 0;
                ++text;
            } else {
                render_menu_text((unsigned)box + i, row, text[i], 0x0f);
                ++i;
            }
        }
        help_delay(0x190, 5);
        editor((unsigned)box, (unsigned)(box >> 16) + panel->x);
        if (update_key_state() == 0) {
            menu_key_state = 0x6e;
            menu_previous_key = 0x6e;
            break;
        }
        if (menu_key_state == 0x4900) {
            if (selection > 1)
                --selection;
        } else if (menu_key_state == 0x5100) {
            if (selection < 5)
                ++selection;
        } else if (menu_key_state == 0x8000 || menu_cancelled) {
            break;
        }
    }
    toggle_menu_mode(0);
    configuration_active = 0;
}

extern void far transfer_error_screen(unsigned offset, unsigned segment,
                                      char far *buffer);

/* objcmp: {"function":"menu_error_message","address":"158d:2492","size":502,"symbols":{"_menu_active":"DS:24a2","_strlen":"0000:5a36","_render_menu_text":"158d:0219","_transfer_error_screen":"0000:45d4","_keyboard_status":"0000:2553","_menu_cancelled":"DS:5d80","_error_screen_shadow":"DS:441c","_error_screen_backup":"1e81:0d80"},"level":"PROVISIONAL","blocker":"screen-backup-layout at 158d:2492"} */
void far menu_error_message(char far *message)
{
    unsigned length;
    unsigned width;
    unsigned row;
    unsigned column;
    unsigned i;
    unsigned offset;
    unsigned cell;
    unsigned long video;
    unsigned char far *backup;

    if (menu_active == 0) {
        transfer_error_screen(0x441c, 0x29bf,
                              (char far *)MK_FP(0x29bf, 0x39a4));
        return;
    }

    length = strlen(message);
    if (length > 0x4b)
        length = 0x4b;
    width = length - 1;
    backup = (unsigned char far *)MK_FP(0x1e81, 0x0d80);
    video = 0xb800;

    /* Save and frame the error rows so the box can be removed afterwards. */
    for (row = 22; row < 25; ++row) {
        for (column = 0; column <= width + 2; ++column) {
            offset = row * 160 + column * 2;
            cell = *(unsigned far *)MK_FP((unsigned)video, offset);
            *(unsigned far *)(backup + (row * 0x9c) + column * 2) = cell;
        }
    }
    for (row = 22; row < 25; ++row) {
        *(unsigned far *)MK_FP(0xb800, row * 160) = 0x4eba;
        *(unsigned far *)MK_FP(0xb800, row * 160 + (width + 2) * 2) = 0x4eba;
    }
    for (column = 0; column <= width + 2; ++column) {
        *(unsigned far *)MK_FP(0xb800, 22 * 160 + column * 2) = 0x4ecd;
        *(unsigned far *)MK_FP(0xb800, 24 * 160 + column * 2) = 0x4ecd;
    }
    *(unsigned far *)MK_FP(0xb800, 22 * 160) = 0x4ec9;
    *(unsigned far *)MK_FP(0xb800, 22 * 160 + (width + 2) * 2) = 0x4ebb;
    *(unsigned far *)MK_FP(0xb800, 24 * 160) = 0x4ec8;
    *(unsigned far *)MK_FP(0xb800, 24 * 160 + (width + 2) * 2) = 0x4ebc;

    for (i = 0; message[i] != 0; ++i)
        render_menu_text(i + 1, 23, message[i], 0x4e);
    transfer_error_screen(0x441c, 0x29bf,
                          (char far *)MK_FP(0x29bf, 0x39a4));
    if ((keyboard_status(0) & 0x7f) == 0x1b)
        menu_cancelled = 1;

    for (row = 22; row < 25; ++row) {
        for (column = 0; column <= width + 2; ++column) {
            offset = row * 160 + column * 2;
            cell = *(unsigned far *)(backup + row * 0x9c + column * 2);
            *(unsigned far *)MK_FP(0xb800, offset) = cell;
        }
    }
}
