/* Reconstructed 11C1 startup, integrity, and input routines. */
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>

extern FILE standard_error_stream;

extern struct REGPACK mouse_regs;
extern int muted;

extern unsigned int far *bios_keyboard_flags;
extern unsigned int window_mode;
extern unsigned int bashful_mode;
extern unsigned int skip_ctrlbreak_control;
extern unsigned int timer_hook_installed;
extern unsigned int keyboard_hook_installed;
extern unsigned int original_ctrlbreak_setting;
extern unsigned int mouse_enabled;
extern unsigned int mouse_abort_mask;
extern unsigned int mouse_was_pressed;
extern unsigned int mouse_available;
extern unsigned int keep_typematic;
extern void (interrupt far *saved_timer_vector)(void);
extern void (interrupt far *saved_keyboard_vector)(void);
extern unsigned int far *current_image_header;

int probe_mouse(unsigned int mask);
void restore_runtime_callbacks(void);
int default_signal_result(void);
void mouse_position(int buttons, int x, int y);
void interrupt far timer_handler(void);
void interrupt far keyboard_handler(void);

/* objcmp: {"function":"keyboard_led_flags","address":"11c1:000a","size":101,"level":"PROVISIONAL","blocker":"compiler-register-allocation: mode argument is cached in CX; target loads it into AX after the mode guards","symbols":{"_bios_keyboard_flags":"DS:0c02","_window_mode":"DS:5dbc","_bashful_mode":"DS:5dca","_bioskey":"0000:2553"}} */
unsigned int bioskey(unsigned int command);
unsigned int keyboard_led_flags(unsigned int mask, unsigned int mode) {
    unsigned int old_flags;
    old_flags = *bios_keyboard_flags;
    if (!window_mode && !bashful_mode) {
        if (mode == 0) {
            *bios_keyboard_flags |= mask;
            bioskey(1);
        } else if (mode == 1) {
            *bios_keyboard_flags &= ~mask;
            bioskey(1);
        } else {
            *bios_keyboard_flags |= mask;
            *bios_keyboard_flags &= (unsigned int)(mask | 0xff8f);
        }
    }
    return old_flags;
}

void sound(unsigned);
void delay(unsigned);
void nosound(void);

/* objcmp: {"function":"mode_ready_chirp","address":"11c1:006f","size":73,"symbols":{"_muted":"DS:5dbe","_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4"}} */
void mode_ready_chirp(void) {
    if (!muted) {
        sound(220);
        delay(30);
        sound(440);
        delay(40);
        sound(880);
        delay(50);
        nosound();
    }
}

/* objcmp: {"function":"default_signal_result","address":"11c1:00b8","size":4} */
int default_signal_result(void) {
    return 1;
}

void signal_cleanup(int);

/* objcmp: {"function":"interrupt_signal_result","address":"11c1:00bc","size":13,"symbols":{"_signal_cleanup":"0000:30cc"}} */
int interrupt_signal_result(void) {
    signal_cleanup(0);
    return 2;
}

void register_interrupt_handler(int (far *handler)(void));

/* objcmp: {"function":"install_runtime_callbacks","address":"11c1:00c9","size":236,"level":"PROVISIONAL","blocker":"object is two bytes short after registering default_signal_result; callback setup code shape differs","symbols":{"_skip_ctrlbreak_control":"DS:5d70","_timer_hook_installed":"DS:5d74","_keyboard_hook_installed":"DS:5d72","_original_ctrlbreak_setting":"DS:5dea","_saved_timer_vector":"DS:5ff6","_saved_keyboard_vector":"DS:5ff2","_current_image_header":"DS:5c5b","_mouse_available":"DS:5cb5","_window_mode":"DS:5dbc","_probe_mouse":"11c1:0442","_mouse_position":"11c1:0501","_timer_handler":"1899:14c7","_keyboard_handler":"1899:154c","_getvect":"0000:12c4","_setvect":"0000:12d3","_getcbrk":"0000:1216","_setcbrk":"0000:1220","_register_interrupt_handler":"0000:30ba"}} */
void install_runtime_callbacks(void) {
    unsigned int mouse_type;
    unsigned int mask;
    atexit((void (far *)(void))default_signal_result);
    if (!skip_ctrlbreak_control) {
        original_ctrlbreak_setting = getcbrk();
        setcbrk(0);
    }
    if (!timer_hook_installed) {
        saved_timer_vector = getvect(0x1c);
        setvect(0x1c, timer_handler);
        timer_hook_installed = 1;
    }
    if (!keyboard_hook_installed) {
        saved_keyboard_vector = getvect(9);
        setvect(9, keyboard_handler);
        keyboard_hook_installed = 1;
    }
    if (mouse_available && current_image_header &&
        (current_image_header[6] & 0x0800)) {
        mouse_type = probe_mouse(1);
        if (mouse_type) {
            mask = 0x2a;
            if (bashful_mode && mouse_type != 2)
                mask |= 1;
            mouse_position(mask, 0x155f, 0x1899);
        }
    }
    register_interrupt_handler(interrupt_signal_result);
}

extern unsigned char original_text_attribute;
extern void normvideo(void);
extern void textattr(unsigned int);

/* objcmp: {"function":"restore_text_attributes","address":"11c1:01b5","size":18,"symbols":{"_original_text_attribute":"DS:601a","_normvideo":"0000:2641","_textattr":"0000:262a"}} */
void restore_text_attributes(void) {
    normvideo();
    textattr(original_text_attribute);
}

extern int restore_video_helpers(void);
extern void restore_graphics_text_mode(int);
extern void print_order_info(int);
extern int cprintf(const char far *, ...);
extern char math_error_text[];
extern char program_revision_format[];
extern char reason_label[];
extern char far *program_revision;
extern FILE standard_output_stream;

extern unsigned char far mouse_header_flags[];
extern unsigned int far *mouse_header;
extern unsigned char key_repeat_params[];
extern unsigned int mouse_callback_installed;
extern void keyboard_control(unsigned int, unsigned char far *,
                             unsigned char far *);

/* objcmp: {"function":"restore_runtime_callbacks","address":"11c1:01c7","size":173,"level":"PROVISIONAL","blocker":"blocked: linker-farcall-rewrite; later same-segment probe_mouse call","symbols":{"_timer_hook_installed":"DS:5d74","_keyboard_hook_installed":"DS:5d72","_saved_timer_vector":"DS:5ff6","_saved_keyboard_vector":"DS:5ff2","_original_ctrlbreak_setting":"DS:5dea","_mouse_available":"DS:5cb5","_mouse_header":"DS:5c5b","_mouse_callback_installed":"DS:5cb9","_keep_typematic":"DS:5dd2","_key_repeat_params":"DS:6006","_probe_mouse":"11c1:0442","_setvect":"0000:12d3","_setcbrk":"0000:1220","_restore_video_helpers":"0000:3f37","_keyboard_control":"0000:320e"}} */
void restore_runtime_callbacks(void) {
    if (timer_hook_installed) {
        setvect(0x1c, saved_timer_vector);
        timer_hook_installed = 0;
    }
    if (keyboard_hook_installed) {
        setvect(9, saved_keyboard_vector);
        keyboard_hook_installed = 0;
    }
    setcbrk(original_ctrlbreak_setting);
    if (mouse_available && (mouse_header[6] & 0x0800))
        probe_mouse(0);
    if (!keep_typematic) {
        key_repeat_params[1] = 3;
        key_repeat_params[0] = 5;
        key_repeat_params[3] = 1;
        key_repeat_params[2] = 7;
        keyboard_control(0x16, key_repeat_params, key_repeat_params);
    }
    restore_video_helpers();
    if (mouse_callback_installed)
        mouse_header[5] &= 0xf7ff;
}

void delay(unsigned);

/* objcmp: {"function":"check_registration_frame","address":"11c1:0274","size":100,"level":"PROVISIONAL","blocker":"semantic candidate is 17 bytes long; compiler argument formatting and segment calls differ","symbols":{"_restore_runtime_callbacks":"11c1:01c7","_restore_graphics_text_mode":"1899:0bcd","_print_order_info":"11c1:0ab6","_math_error_text":"DS:0c5c","_program_revision_format":"DS:0c96","_reason_label":"DS:0cb4","_program_revision":"DS:009e","_cprintf":"0000:277a"}} */
int check_registration_frame(void far *frame) {
    unsigned int far *words;
    words = (unsigned int far *)frame;
    if (words[0] == 5 || words[0] == 4)
        return 1;
    restore_runtime_callbacks();
    restore_graphics_text_mode(1);
    print_order_info(0);
    cprintf(math_error_text);
    fprintf(&standard_error_stream, program_revision_format, program_revision);
    cprintf(reason_label);
    return 0;
}


extern char signal_error_text[];
extern char signal_announcement[];
extern char signal_register_format[];
extern char segment_map_title[];
extern char segment_dzle_format[];
extern char segment_dzl2_format[];
extern char segment_dzfd_format[];
extern char segment_dvdt_format[];
extern char segment_dosi_format[];
extern char segment_dhuf_format[];
extern char segment_dlogo_format[];
extern char segment_dazpcx_format[];
extern unsigned int far get_dzle_segment(void);
extern unsigned int far get_dzl2_segment(void);
extern unsigned int far get_dzfd_segment(void);
extern unsigned int far get_dvdt_segment(void);
extern unsigned int far get_dhuf_segment(void);
extern unsigned int far get_dlogo_segment(void);
extern unsigned int far get_dazpcx_segment(void);
extern void exit(int);

/* objcmp: {"function":"signal_diagnostic","address":"11c1:02d8","size":362,"level":"PROVISIONAL","blocker":"semantic register/segment report candidate is 41 bytes short; variadic call sequence differs","symbols":{"_restore_runtime_callbacks":"11c1:01c7","_restore_graphics_text_mode":"1899:0bcd","_signal_error_text":"DS:0cbc","_signal_announcement":"DS:0cf2","_signal_register_format":"DS:0d2b","_segment_map_title":"DS:0d4e","_segment_dzle_format":"DS:0d5e","_segment_dzl2_format":"DS:0d70","_segment_dzfd_format":"DS:0d82","_segment_dvdt_format":"DS:0d94","_segment_dosi_format":"DS:0da6","_segment_dhuf_format":"DS:0db8","_segment_dlogo_format":"DS:0dca","_segment_dazpcx_format":"DS:0ddc","_standard_error_stream":"DS:441c","_fprintf":"0000:45d4","_get_dzle_segment":"066a:000b","_get_dzl2_segment":"0b84:0000","_get_dzfd_segment":"0ccd:000a","_get_dvdt_segment":"1899:1571","_get_dhuf_segment":"1a72:0007","_get_dlogo_segment":"17f5:0008","_get_dazpcx_segment":"19f0:0004","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"}} */
void signal_diagnostic(int signal_number, int signal_type, unsigned int far *registers) {
    restore_runtime_callbacks();
    restore_graphics_text_mode(1);
    fprintf(&standard_error_stream, signal_error_text);
    fprintf(&standard_error_stream, signal_announcement, signal_number, signal_type);
    fprintf(&standard_error_stream, signal_register_format,
            registers[0x16 / 2], registers[0x12 / 2], registers[0x14 / 2]);
    fprintf(&standard_error_stream, segment_map_title);
    fprintf(&standard_error_stream, segment_dzle_format, get_dzle_segment());
    fprintf(&standard_error_stream, segment_dzl2_format, get_dzl2_segment());
    fprintf(&standard_error_stream, segment_dzfd_format, get_dzfd_segment());
    fprintf(&standard_error_stream, segment_dvdt_format, get_dvdt_segment());
    fprintf(&standard_error_stream, segment_dosi_format, 0x11c1);
    fprintf(&standard_error_stream, segment_dhuf_format, get_dhuf_segment());
    fprintf(&standard_error_stream, segment_dlogo_format, get_dlogo_segment());
    fprintf(&standard_error_stream, segment_dazpcx_format, get_dazpcx_segment());
    print_order_info(0);
    exit(10);
}

/* objcmp: {"function":"probe_mouse","address":"11c1:0442","size":191,"level":"PROVISIONAL","blocker":"register allocation now follows SI=status/DI=attempts; candidate remains one byte short and MZ relocation sites differ","symbols":{"_mouse_regs":"DS:743e","_intr":"0000:32d8","_delay":"0000:29a6"}} */
int probe_mouse(unsigned int mask) {
    unsigned int attempts;
    unsigned int buttons;
    unsigned int status;
    mouse_regs.r_ax = 0;
    intr(0x33, &mouse_regs);
    status = mouse_regs.r_ax != 0;
    buttons = 0;
    attempts = 0;
    if (status && mask) {
        do {
            delay(10);
            mouse_regs.r_ax = 5;
            intr(0x33, &mouse_regs);
            buttons = mouse_regs.r_bx;
            mouse_regs.r_ax = 6;
            intr(0x33, &mouse_regs);
            buttons |= mouse_regs.r_bx;
            mouse_regs.r_ax = 11;
            intr(0x33, &mouse_regs);
            ++attempts;
            if (attempts > 50)
                status = 2;
        } while (attempts <= 50 &&
                 (buttons | mouse_regs.r_cx | mouse_regs.r_dx) == 0);
    }
    return status & mask;
}

/* objcmp: {"function":"mouse_position","address":"11c1:0501","size":46,"symbols":{"_mouse_regs":"DS:743e","_intr":"0000:32d8"}} */
void mouse_position(int buttons, int x, int y) {
    mouse_regs.r_cx = buttons;
    mouse_regs.r_es = y;
    mouse_regs.r_dx = x;
    mouse_regs.r_ax = 12;
    intr(0x33, &mouse_regs);
}

extern unsigned int exit_requested;
extern unsigned long computed_crc;
extern unsigned long expected_crc;
extern unsigned int serial_r1;
extern unsigned int serial_r2;
extern char selfcheck_format_with_crc[];
extern char selfcheck_format_without_crc[];
extern char selfcheck_reason_format[];
extern char selfcheck_read_error[];
extern void restore_graphics_text_mode(int);
extern void print_order_info(int);
extern void exit(int);
/* objcmp: {"function":"fatal_selfcheck_error","address":"11c1:052f","size":148,"level":"PROVISIONAL","blocker":"blocked: linker-farcall-rewrite; later same-segment print_order_info call","symbols":{"_exit_requested":"DS:5d80","_computed_crc":"DS:6021","_expected_crc":"DS:6027","_serial_r1":"DS:602b","_serial_r2":"DS:602d","_selfcheck_format_with_crc":"DS:0e11","_selfcheck_format_without_crc":"DS:0e96","_selfcheck_reason_format":"DS:0ec6","_standard_error_stream":"DS:441c","_restore_graphics_text_mode":"1899:0bcd","_fprintf":"0000:45d4","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"}} */
void fatal_selfcheck_error(int show_crc, char far *reason) {
    exit_requested = 1;
    restore_graphics_text_mode(1);
    if (show_crc)
        fprintf(&standard_error_stream, selfcheck_format_with_crc, computed_crc, serial_r1,
                serial_r2, expected_crc);
    else
        fprintf(&standard_error_stream, selfcheck_format_without_crc, expected_crc, serial_r1,
                serial_r2);
    fprintf(&standard_error_stream, selfcheck_reason_format, reason);
    print_order_info(0);
    exit(100);
}

extern long resource_base;

/* objcmp: {"function":"read_mz_resource_base","address":"11c1:05c3","size":104,"symbols":{"_resource_base":"DS:5d68","_fread":"0000:46e4","F_LXMUL@":"0000:11e3","_fatal_selfcheck_error":"11c1:052f","_selfcheck_read_error":"DS:0ed6"}} */
void read_mz_resource_base(FILE far *input) {
    int header[3];
    if (fread(header, 2, 3, input) != 3)
        fatal_selfcheck_error(1, selfcheck_read_error);
    resource_base = header[2] - 1;
    resource_base *= 512;
    resource_base += header[1] + 1;
}

/* objcmp: {"function":"allocate","address":"11c1:062b","size":65,"symbols":{"_malloc":"0000:2c51"}} */
void *allocate(unsigned *amount, unsigned minimum) {
    unsigned n;
    void *p;
    n = *amount;
    while (n >= minimum && (p = malloc(n)) == 0)
        n /= 2;
    *amount = n;
    return p;
}

extern unsigned long crc_polynomial;
extern unsigned int fread(void far *, unsigned int, unsigned int, FILE far *);
extern void free(void far *);
extern char crc_buffer_error[];
extern char crc_read_error[];

/* objcmp: {"function":"genCRC","address":"11c1:066c","size":501,"level":"PROVISIONAL","blocker":"CRC-table and buffered-loop code generation differs; candidate is 51 bytes short","symbols":{"_crc_polynomial":"DS:0c06","_allocate":"11c1:062b","_fseek":"0000:47f4","_fread":"0000:46e4","_free":"0000:2b47","_fatal_selfcheck_error":"11c1:052f","_crc_buffer_error":"DS:0f05","_crc_read_error":"DS:0eec"}} */
unsigned long genCRC(FILE far *input, unsigned long length) {
    unsigned long table[256];
    unsigned long crc;
    unsigned long processed;
    unsigned long remaining;
    unsigned int amount;
    unsigned int got;
    unsigned int i;
    unsigned int bit;
    unsigned int j;
    unsigned int index;
    unsigned char far *buffer;

    amount = 0x2000;
    buffer = (unsigned char far *)allocate(&amount, 0x200);
    if (!buffer)
        fatal_selfcheck_error(1, crc_buffer_error);

    for (i = 0; i < 256; ++i) {
        crc = (unsigned long)i << 24;
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 0x80000000UL)
                crc = (crc << 1) ^ crc_polynomial;
            else
                crc <<= 1;
        }
        table[i] = crc;
    }

    crc = 0;
    processed = 0;
    if (fseek(input, 0L, 0) != 0) {
        free(buffer);
        fatal_selfcheck_error(1, crc_read_error);
    }
    for (;;) {
        remaining = length - processed - 1;
        got = remaining < amount ? (unsigned int)remaining : amount;
        got = fread(buffer, 1, got, input);
        if (got == 0)
            break;
        for (j = 0; j < got; ++j) {
            index = (unsigned int)((crc >> 24) ^ buffer[j]);
            crc = (crc << 8) ^ table[index];
        }
        processed += got;
        if (processed >= length)
            break;
    }
    free(buffer);
    return crc;
}

extern FILE far *selfcheck_stream;
extern unsigned long expected_crc;
extern unsigned int insecure_argument_index;
extern char executable_path[];
extern char selfcheck_open_error[];
extern char selfcheck_seek_error[];
extern char selfcheck_metadata_error[];
extern void far *metadata_buffer;
extern int fclose(FILE far *);
extern FILE far *fopen(const char far *, const char far *);

/* objcmp: {"function":"deRead","address":"11c1:0861","size":232,"segments":{"_DATA":"29bf:0f23"},"symbols":{"_selfcheck_stream":"DS:6002","_resource_base":"DS:5d68","_insecure_argument_index":"DS:5dc8","_executable_path":"DS:5d17","_selfcheck_open_error":"DS:0f26","_selfcheck_seek_error":"DS:0f3d","_selfcheck_metadata_error":"DS:0f54","_computed_crc":"DS:6021","_genCRC":"11c1:066c","_read_mz_resource_base":"11c1:05c3","_fatal_selfcheck_error":"11c1:052f","_fopen":"0000:459f","_fseek":"0000:47f4","_fread":"0000:46e4","_fclose":"0000:407f"}} */
int deRead(void far *metadata, unsigned int size) {
    int result;
    result = 0;
    if (!(selfcheck_stream = fopen(executable_path, "rb")))
        fatal_selfcheck_error(1, selfcheck_open_error);
    read_mz_resource_base(selfcheck_stream);
    if (!insecure_argument_index)
        computed_crc = genCRC(selfcheck_stream, resource_base);
    if (fseek(selfcheck_stream, resource_base, 0) != 0)
        fatal_selfcheck_error(1, selfcheck_seek_error);
    if (fread(metadata, size, 1, selfcheck_stream) != 1)
        result = !insecure_argument_index;
    if (fclose(selfcheck_stream) != 0)
        fatal_selfcheck_error(1, selfcheck_metadata_error);
    return result;
}

/* objcmp: {"function":"check_executable_integrity","address":"11c1:0949","size":365,"level":"PROVISIONAL","blocker":"path splitting, name warning, and CRC comparison body remains to be reconstructed","symbols":{"_insecure_argument_index":"DS:5dc8","_executable_path":"DS:5d17","_exit_requested":"DS:5d80","_expected_crc":"DS:6027","_computed_crc":"DS:6021","_deRead":"11c1:0861","_fatal_selfcheck_error":"11c1:052f","_setverify":"0000:25fc","_cprintf":"0000:277a","_strupr":"0000:5b18","_stricmp":"0000:59f5","_fnsplit":"0000:439f","_standard_error_stream":"DS:441c","_fprintf":"0000:45d4"}} */
/* C description: resolve the running EXE name, warn on a nonmatching basename,
   read the ten-byte metadata record, compare CRC unless /I was supplied, and
   report a bad checksum through fatal_selfcheck_error. */

extern char order_form_text[];
extern char registration_summary[];
extern char order_form_notice[];
extern char order_name_format[];
extern char order_address_format[];
extern char order_phone_format[];
extern char order_address_one[];
extern char order_address_two[];
extern char far *order_product_name;
extern char far *order_revision_text;
extern char far *order_registration_name;
extern char far *order_registration_revision;
extern int textcolor(int);

/* objcmp: {"function":"print_order_info","address":"11c1:0ab6","size":311,"level":"PROVISIONAL","blocker":"semantic order-form output candidate is 105 bytes short; formatting and color sequence differ","symbols":{"_serial_r1":"DS:602b","_serial_r2":"DS:602d","_expected_crc":"DS:6027","_computed_crc":"DS:6021","_standard_output_stream":"DS:4408","_order_form_text":"DS:10f0","_registration_summary":"DS:1285","_order_form_notice":"DS:165a","_order_name_format":"DS:16a1","_order_address_format":"DS:16a4","_order_phone_format":"DS:16aa","_order_address_one":"DS:102d","_order_address_two":"DS:1046","_order_product_name":"DS:009e","_order_revision_text":"DS:00a2","_order_registration_name":"DS:009e","_order_registration_revision":"DS:00a2","_textcolor":"0000:25fc","_cprintf":"0000:277a","_fprintf":"0000:45d4"}} */
void print_order_info(int show_order_form) {
    if (show_order_form) {
        fprintf(&standard_output_stream, order_form_text,
                order_product_name, order_revision_text, serial_r1, serial_r2,
                order_address_one, order_address_two);
        fprintf(&standard_output_stream, order_name_format,
                order_registration_name, order_registration_revision);
        fprintf(&standard_output_stream, order_address_format,
                order_address_one, order_address_two);
        fprintf(&standard_output_stream, order_phone_format,
                order_registration_name, order_registration_revision);
    }
    textcolor(3);
    cprintf(registration_summary);
    textcolor(7);
    cprintf(order_form_notice);
}

extern char help_intro[];
extern char help_heading[];
extern char help_option_2[];
extern char help_option_a[];
extern char help_option_more[];
extern char help_option_toggle[];
extern char help_option_suffix[];
extern char help_default_config_format[];
extern char help_footer[];
extern char far *default_config_name;

/* objcmp: {"function":"command_line_help","address":"11c1:0bed","size":345,"level":"PROVISIONAL","blocker":"help text is present, but the original pause/key-consumption and output sequence remain incomplete","symbols":{"_restore_video_helpers":"0000:3f37","_textcolor":"0000:25fc","_cprintf":"0000:277a","_exit":"0000:11a8","_help_intro":"DS:16aa","_help_heading":"DS:16cb","_help_option_2":"DS:16f3","_help_option_a":"DS:173a","_help_option_more":"DS:176f","_help_option_toggle":"DS:19de","_help_option_suffix":"DS:1a18","_help_default_config_format":"DS:1b2a","_help_footer":"DS:1bcc","_default_config_name":"DS:5ffa"}} */
void command_line_help(void) {
    restore_video_helpers();
    textcolor(7);
    cprintf(help_intro);
    textcolor(11);
    cprintf(help_heading);
    textcolor(3);
    cprintf(help_option_2);
    cprintf(help_option_a);
    cprintf(help_option_more);
    textcolor(11);
    cprintf(help_option_toggle);
    textcolor(7);
    cprintf(help_option_suffix);
    fprintf(&standard_output_stream, help_default_config_format,
            default_config_name);
    textcolor(3);
    cprintf(help_footer);
    exit(10);
}

extern char closing_text[];
extern char far *closing_registration_text;
extern char far *closing_version_text;

/* objcmp: {"function":"print_closing_text_and_exit","address":"11c1:0d46","size":63,"symbols":{"_bashful_mode":"DS:5dca","_serial_r1":"DS:602b","_serial_r2":"DS:602d","_closing_text":"DS:1d9e","_closing_registration_text":"DS:009e","_closing_version_text":"DS:00a2","_standard_output_stream":"DS:4408","_fprintf":"0000:45d4","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"}} */
void print_closing_text_and_exit(int status) {
    if (!bashful_mode) {
        fprintf(&standard_output_stream, closing_text, closing_version_text, closing_registration_text);
        print_order_info(0);
    }
    exit(status);
}

extern char far *default_config_name;
extern char far *strupr(char far *);
extern char far *strstr(const char far *, const char far *);
extern char far *strcpy(char far *, const char far *);
extern int show_rejected_config_name(char far *);
extern char rejected_name_warning[];
extern char exe_suffix[];
extern char bat_suffix[];
extern char com_suffix[];

/* objcmp: {"function":"reject_exec_filename","address":"11c1:0d85","size":130,"symbols":{"_strupr":"0000:5b18","_strstr":"0000:5aaa","_strcpy":"0000:59cc","_default_config_name":"DS:5ffa","_show_rejected_config_name":"158d:2492","_exe_suffix":"DS:1fc0","_bat_suffix":"DS:1fc5","_com_suffix":"DS:1fca","_rejected_name_warning":"DS:1fcf"}} */
int reject_exec_filename(char far *filename) {
    int rejected;
    rejected = 0;
    strupr(filename);
    if (strstr(filename, exe_suffix) || strstr(filename, bat_suffix) ||
        strstr(filename, com_suffix)) {
        show_rejected_config_name(rejected_name_warning);
        strcpy(filename, default_config_name);
        rejected = 1;
    }
    return rejected;
}

/* objcmp: {"function":"beep","address":"11c1:0e07","size":35,"symbols":{"_muted":"DS:5dbe","_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4"}} */
void beep(unsigned hz, unsigned ms) {
    if (!muted) {
        sound(hz);
        delay(ms);
        nosound();
    }
}

/* objcmp: {"function":"set_ega_raster_xor","address":"11c1:0e2a","size":27,"level":"UNKNOWN","blocker":"asm-origin"} */
/* Assembly description: write graphics-controller index 3 through ports 3CEh/3CFh;
   select XOR mode for a nonzero argument and replace mode for zero. */

/* objcmp: {"function":"generate_palette","address":"11c1:0e45","size":884,"level":"PROVISIONAL","blocker":"large palette generator not reconstructed; random color and EGA/VGA branches need full decompilation"} */
/* C description: fill the palette buffer from rand(), with Alternate/Mad and
   fixed-palette modes controlling the color walk. */

/* objcmp: {"function":"randomize_palette_state","address":"11c1:11b9","size":401,"level":"PROVISIONAL","blocker":"palette-state generator and far backend call sequence not reconstructed"} */
/* C description: randomize palette indices and refresh the selected palette backend. */

/* objcmp: {"function":"change_background_palette","address":"11c1:134a","size":165,"level":"PROVISIONAL","blocker":"VGA palette and DLOGO interrupt branches not reconstructed"} */
/* C description: alter reserved/background palette entries for the Ctrl-F10 path. */

/* objcmp: {"function":"build_ega_palette","address":"11c1:13ef","size":476,"level":"PROVISIONAL","blocker":"palette-table initialization loops not reconstructed"} */
/* C description: generate the packed EGA palette entries from the mode and palette state. */

/* The heuristic starts at 15CB, 1626, 1681, and 1780 are not independently
   confirmed function entries; retain them as unresolved labels pending callsite review. */

/* objcmp: {"function":"freeze_display","address":"11c1:1830","size":183,"level":"PROVISIONAL","blocker":"frozen-state polling loop and key handling not reconstructed"} */
/* C description: pause drawing while polling pending input; TAB also advances palette state. */

/* objcmp: {"function":"dispatch_keyboard_and_mouse","address":"11c1:18e7","size":1149,"level":"PROVISIONAL","blocker":"complete dispatcher crosses the 1B47 worker boundary and includes two trailing switch tables"} */
/* C description: poll and consume BIOS keys, normalize ASCII/Alt/Ctrl codes, handle
   mouse abort and exit flags, and dispatch function keys through the ASCII and
   extended-key tables ending at 1D63. */
