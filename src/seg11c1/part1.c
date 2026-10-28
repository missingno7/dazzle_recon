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

int far probe_mouse(unsigned int mask);
void restore_runtime_callbacks(void);
int default_signal_result(void);
void far mouse_position(int buttons, int x, int y);
void interrupt far timer_handler(void);
void interrupt far keyboard_handler(void);

/* objcmp: {"function":"keyboard_led_flags","address":"11c1:000a","size":101,"symbols":{"_bios_keyboard_flags":"DS:0c02","_window_mode":"DS:5dbc","_bashful_mode":"DS:5dca","_bioskey":"0000:2553"}} */
unsigned int bioskey(unsigned int command);
unsigned int keyboard_led_flags(unsigned int mask, unsigned int mode) {
    unsigned int old_flags;
    old_flags = *bios_keyboard_flags;
    if (!window_mode && !bashful_mode) {
        switch (mode) {
        case 0:
            *bios_keyboard_flags |= mask;
            bioskey(1);
            break;
        case 1:
            *bios_keyboard_flags &= ~mask;
            bioskey(1);
            break;
        default:
            *bios_keyboard_flags |= mask;
            *bios_keyboard_flags &= (unsigned int)(mask | 0xff8f);
            break;
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

/* objcmp: {"function":"install_runtime_callbacks","address":"11c1:00c9","size":236,"level":"SEMANTIC","blocker":"codegen/local-spill: target starts with sub sp,2 at 11c1:00cc and saves probe result at [bp-2]; candidate keeps it in DX; object 231 bytes, oracle 236","symbols":{"_skip_ctrlbreak_control":"DS:5d70","_timer_hook_installed":"DS:5d74","_keyboard_hook_installed":"DS:5d72","_original_ctrlbreak_setting":"DS:5dea","_saved_timer_vector":"DS:5ff6","_saved_keyboard_vector":"DS:5ff2","_current_image_header":"DS:5c5b","_mouse_available":"DS:5cb5","_window_mode":"DS:5dbc","_probe_mouse":"11c1:0442","_mouse_position":"11c1:0501","_timer_handler":"1899:14c7","_keyboard_handler":"1899:154c","_getvect":"0000:12c4","_setvect":"0000:12d3","_getcbrk":"0000:1216","_setcbrk":"0000:1220","_register_interrupt_handler":"0000:30ba","_ctrlbrk":"0000:1065"}} */
void install_runtime_callbacks(void) {
    int mouse_type;
    ctrlbrk((int (far *)(void))default_signal_result);
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
    if (mouse_available && (current_image_header[6] & 0x0800)) {
        mouse_type = probe_mouse(1);
        if (mouse_type) {
            mouse_position(0x2a | ((bashful_mode && mouse_type != 2) ? 1 : 0),
                           0x155f, 0x1899);
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

/* objcmp: {"function":"restore_runtime_callbacks","address":"11c1:01c7","size":173,"symbols":{"_timer_hook_installed":"DS:5d74","_keyboard_hook_installed":"DS:5d72","_saved_timer_vector":"DS:5ff6","_saved_keyboard_vector":"DS:5ff2","_original_ctrlbreak_setting":"DS:5dea","_mouse_available":"DS:5cb5","_mouse_header":"DS:5c5b","_mouse_callback_installed":"DS:5cb9","_keep_typematic":"DS:5dd2","_key_repeat_params":"DS:6006","_probe_mouse":"11c1:0442","_setvect":"0000:12d3","_setcbrk":"0000:1220","_restore_video_helpers":"0000:3f37","_keyboard_control":"0000:320e"}} */
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
    if (!mouse_available || (mouse_header[6] & 0x0800))
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

/* objcmp: {"function":"check_registration_frame","address":"11c1:0274","size":100,"level":"SEMANTIC","blocker":"blocked: linker-farcall-rewrite at 11c1:029c: oracle 90 0E E8 15 08 calls print_order_info at 11c1:0AB6; object retains far-call relocation","symbols":{"_restore_runtime_callbacks":"11c1:01c7","_restore_graphics_text_mode":"1899:0bcd","_print_order_info":"11c1:0ab6","_math_error_text":"DS:0c5c","_program_revision_format":"DS:0c96","_reason_label":"DS:0cb4","_program_revision":"DS:009e","_cprintf":"0000:277a"}} */
int check_registration_frame(void far *frame) {
    if (((unsigned int far *)frame)[0] == 5 ||
        ((unsigned int far *)frame)[0] == 4)
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
extern char segment_map_end_format[];
extern char far *order_string_one;
extern char far *order_string_two;
extern unsigned int far get_dzle_segment(void);
extern unsigned int far get_dzl2_segment(void);
extern unsigned int far get_dzfd_segment(void);
extern unsigned int far get_dvdt_segment(void);
extern unsigned int far get_dhuf_segment(void);
extern unsigned int far get_dlogo_segment(void);
extern unsigned int far get_dazpcx_segment(void);
extern void exit(int);

/* objcmp: {"function":"signal_diagnostic","address":"11c1:02d8","size":362,"level":"SEMANTIC","blocker":"blocked: linker-farcall-rewrite at 11c1:042e: oracle 90 0E E8 83 06 calls print_order_info at 11c1:0AB6; object retains far-call relocation","symbols":{"_restore_runtime_callbacks":"11c1:01c7","_restore_graphics_text_mode":"1899:0bcd","_signal_error_text":"DS:0cbc","_signal_announcement":"DS:0cf2","_signal_register_format":"DS:0d2b","_segment_map_title":"DS:0d4e","_segment_dzle_format":"DS:0d5e","_segment_dzl2_format":"DS:0d70","_segment_dzfd_format":"DS:0d82","_segment_dvdt_format":"DS:0d94","_segment_dosi_format":"DS:0da6","_segment_dhuf_format":"DS:0db8","_segment_dlogo_format":"DS:0dca","_segment_dazpcx_format":"DS:0ddc","_segment_map_end_format":"DS:0def","_standard_error_stream":"DS:441c","_order_string_one":"DS:009a","_order_string_two":"DS:009e","_fprintf":"0000:45d4","_get_dzle_segment":"066a:000b","_get_dzl2_segment":"0b84:0000","_get_dzfd_segment":"0ccd:000a","_get_dvdt_segment":"1899:1571","_get_dhuf_segment":"1a72:0007","_get_dlogo_segment":"17f5:0008","_get_dazpcx_segment":"19f0:0004","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"}} */
void signal_diagnostic(int signal_number, int signal_type, unsigned int far *registers) {
    unsigned int current_segment;
    current_segment = _CS;
    restore_runtime_callbacks();
    restore_graphics_text_mode(1);
    fprintf(&standard_error_stream, signal_error_text);
    fprintf(&standard_error_stream, signal_announcement, signal_number, signal_type);
    fprintf(&standard_error_stream, signal_register_format,
            registers[0x14 / 2], registers[0x12 / 2], registers[0x16 / 2]);
    fprintf(&standard_error_stream, segment_map_title);
    fprintf(&standard_error_stream, segment_dzle_format, get_dzle_segment());
    fprintf(&standard_error_stream, segment_dzl2_format, get_dzl2_segment());
    fprintf(&standard_error_stream, segment_dzfd_format, get_dzfd_segment());
    fprintf(&standard_error_stream, segment_dvdt_format, get_dvdt_segment());
    fprintf(&standard_error_stream, segment_dosi_format, current_segment);
    fprintf(&standard_error_stream, segment_dhuf_format, get_dhuf_segment());
    fprintf(&standard_error_stream, segment_dlogo_format, get_dlogo_segment());
    fprintf(&standard_error_stream, segment_dazpcx_format, get_dazpcx_segment());
    fprintf(&standard_error_stream, segment_map_end_format,
            order_string_one, order_string_two);
    print_order_info(0);
    exit(10);
}

/* objcmp: {"function":"probe_mouse","address":"11c1:0442","size":191,"level":"SEMANTIC","blocker":"MZ-relocation-coverage at relative offset 0x43 (11c1:0485): candidate delay-call operand occupies this offset while oracle has pop cx; retry-loop source shape shifts later relocation sites","symbols":{"_mouse_regs":"DS:743e","_intr":"0000:32d8","_delay":"0000:29a6"}} */
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
/* objcmp: {"function":"fatal_selfcheck_error","address":"11c1:052f","size":148,"level":"SEMANTIC","blocker":"blocked: linker-farcall-rewrite at 11c1:05b1: oracle 90 0E E8 00 05 calls print_order_info at 11c1:0AB6; object retains far-call relocation","symbols":{"_exit_requested":"DS:5d80","_computed_crc":"DS:6021","_expected_crc":"DS:6027","_serial_r1":"DS:602b","_serial_r2":"DS:602d","_selfcheck_format_with_crc":"DS:0e11","_selfcheck_format_without_crc":"DS:0e96","_selfcheck_reason_format":"DS:0ec6","_standard_error_stream":"DS:441c","_restore_graphics_text_mode":"1899:0bcd","_fprintf":"0000:45d4","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"}} */
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

/* objcmp: {"function":"genCRC","address":"11c1:066c","size":501,"level":"SEMANTIC","blocker":"table-generation/codegen: target 11c1:066f sub sp,0x41a; candidate sub sp,0x414 and is 51 bytes short because target expands table entries pairwise; pairwise C trial was 526 bytes","symbols":{"_crc_polynomial":"DS:0c06","_allocate":"11c1:062b","_fseek":"0000:47f4","_fread":"0000:46e4","_free":"0000:2b47","_fatal_selfcheck_error":"11c1:052f","_crc_buffer_error":"DS:0f05","_crc_read_error":"DS:0eec"}} */
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
extern char far *searchpath(char near *, char near *);
extern int fnsplit(const char far *, char far *, char far *, char far *, char far *);
extern int stricmp(const char far *, const char far *);

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

extern char integrity_check_intro[];
extern char selfcheck_missing_name[];
extern char selfcheck_bad_program_name[];
extern char selfcheck_bad_crc[];
extern char selfcheck_missing_data[];
extern char selfcheck_done[];
extern char far *help_program_name;
extern char near *g_0096;
extern char near *g_0098;
extern char far *strncpy(char far *, const char far *, unsigned int);
extern unsigned int strlen(const char far *);
extern void beep(unsigned int, unsigned int);

/* objcmp: {"function":"check_executable_integrity","address":"11c1:0949","size":365,"level":"SEMANTIC","blocker":"control-flow/codegen: candidate 363 bytes, oracle 365; oracle at 11c1:0A8A selects the bad-CRC reason and joins a shared fatal-call tail at 11c1:0A94, while candidate emits separate fatal calls","symbols":{"_insecure_argument_index":"DS:5dc8","_executable_path":"DS:5d17","_exit_requested":"DS:5d80","_expected_crc":"DS:6027","_computed_crc":"DS:6021","_deRead":"11c1:0861","_fatal_selfcheck_error":"11c1:052f","_setverify":"0000:25fc","_cprintf":"0000:277a","_stricmp":"0000:59f5","_fnsplit":"0000:439f","_searchpath":"0000:1ea4","_strlen":"0000:5a36","_strncpy":"0000:5a79","_standard_error_stream":"DS:441c","_fprintf":"0000:45d4","_beep":"11c1:0e07","_integrity_check_intro":"DS:0f6c","_selfcheck_missing_name":"DS:0f82","_selfcheck_bad_program_name":"DS:0fa3","_selfcheck_bad_crc":"DS:0fca","_selfcheck_missing_data":"DS:1000","_selfcheck_done":"DS:1017","_help_program_name":"DS:009a","_g_0096":"DS:0096","_g_0098":"DS:0098"}} */
void check_executable_integrity(void) {
    char far *resolved_path;
    int path_flags;
    char drive[6];
    char directory[68];
    char basename[6];
    char extension[6];

    if (!insecure_argument_index) {
        textcolor(14);
        cprintf(integrity_check_intro);
        beep(2000, 4);
        textcolor(3);
    }

    if (strlen(executable_path) == 0) {
        resolved_path = searchpath(g_0096, g_0098);
        if (!resolved_path) {
            fatal_selfcheck_error(0, selfcheck_missing_name);
        } else {
            strncpy(executable_path, resolved_path, 80);
        }
    }

    path_flags = fnsplit(executable_path, drive, directory, basename, extension);
    if ((path_flags & 4) && stricmp(basename, help_program_name) != 0 &&
        !insecure_argument_index) {
        fprintf(&standard_error_stream, selfcheck_bad_program_name,
                help_program_name, basename, extension);
        delay(500);
    }

    if (deRead(&expected_crc, 10))
        fatal_selfcheck_error(1, selfcheck_missing_data);
    if (!insecure_argument_index && expected_crc != computed_crc)
        fatal_selfcheck_error(1, selfcheck_bad_crc);

    cprintf(selfcheck_done);
    exit_requested = 0;
}

extern char order_form_text[];
extern char registration_summary[];
extern char order_form_notice[];
extern char order_name_format[];
extern char order_address_format[];
extern char far *order_string_one;
extern char far *order_string_two;
extern char far *order_string_three;
extern char address_text[];
extern char compuserve_text[];
extern char company_text[];
extern char phone_text[];
extern char mailing_text[];
extern int textcolor(int);

/* objcmp: {"function":"print_order_info","address":"11c1:0ab6","size":311,"symbols":{"_serial_r1":"DS:602b","_serial_r2":"DS:602d","_standard_output_stream":"DS:4408","_order_form_text":"DS:10f0","_registration_summary":"DS:1285","_order_form_notice":"DS:165a","_order_name_format":"DS:16a1","_order_address_format":"DS:16a4","_address_text":"DS:102d","_compuserve_text":"DS:1046","_company_text":"DS:1077","_phone_text":"DS:1086","_mailing_text":"DS:10c1","_order_string_one":"DS:009a","_order_string_two":"DS:009e","_order_string_three":"DS:00a2","_textcolor":"0000:25fc","_cprintf":"0000:277a","_fprintf":"0000:45d4"}} */
void print_order_info(int show_order_form) {
    char far *address_line;
    char far *compuserve_line;
    char far *company_line;
    char far *phone_line;
    char far *mailing_line;

    address_line = address_text;
    compuserve_line = compuserve_text;
    company_line = company_text;
    phone_line = phone_text;
    mailing_line = mailing_text;
    if (show_order_form) {
        fprintf(&standard_output_stream, order_form_text,
                order_string_one, order_string_two, serial_r1, serial_r2,
                company_line, mailing_line, order_string_three);
        fprintf(&standard_output_stream, registration_summary, order_string_one);
        textcolor(3);
        cprintf(order_form_notice);
    }
    textcolor(3);
    cprintf(order_name_format, address_line);
    textcolor(7);
    cprintf(order_address_format, compuserve_line);
    textcolor(15);
    cprintf(order_name_format, company_line);
    textcolor(7);
    cprintf(order_address_format, phone_line);
    textcolor(15);
    cprintf(order_address_format, mailing_line);
}

extern char help_intro[];
extern char help_heading[];
extern char help_option_2[];
extern char help_option_a[];
extern char help_option_more[];
extern char help_option_toggle[];
extern char help_order_form[];
extern char help_option_suffix[];
extern char help_default_config_format[];
extern char help_footer[];
extern char help_palette_option[];
extern char help_cycle_option[];
extern char help_misc_option[];
extern char help_extra_option[];
extern char far *default_config_name;
extern char far *help_program_name;
extern int getch(void);

/* objcmp: {"function":"command_line_help","address":"11c1:0bed","size":345,"symbols":{"_restore_video_helpers":"0000:3f37","_textcolor":"0000:25fc","_cprintf":"0000:277a","_exit":"0000:11a8","_help_intro":"DS:16aa","_help_heading":"DS:16cb","_help_option_2":"DS:16f3","_help_option_a":"DS:173a","_help_option_more":"DS:176f","_help_option_toggle":"DS:19de","_help_option_suffix":"DS:1a18","_help_default_config_format":"DS:1b2a","_help_footer":"DS:1bcc","_help_palette_option":"DS:1abb","_help_cycle_option":"DS:1ac2","_help_misc_option":"DS:1add","_help_extra_option":"DS:1b0f","_help_program_name":"DS:009a","_help_order_form":"DS:1c7b","_default_config_name":"DS:5ffa","_print_order_info":"11c1:0ab6","_getch":"0000:2f7c"}} */
void command_line_help(void) {
    restore_video_helpers();
    textcolor(7);
    cprintf(help_intro, help_program_name);
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
    textcolor(12);
    cprintf(help_palette_option);
    textcolor(7);
    cprintf(help_cycle_option);
    textcolor(14);
    cprintf(help_misc_option);
    textcolor(7);
    cprintf(help_extra_option);
    textcolor(3);
    getch();
    textcolor(7);
    cprintf(help_default_config_format, default_config_name);
    textcolor(3);
    cprintf(help_footer);
    textcolor(2);
    cprintf(help_order_form);
    print_order_info(0);
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

extern unsigned char far *palette_buffer;
extern unsigned int palette_length;
extern unsigned int palette_entries;
extern unsigned int palette_update_index;
extern unsigned int palette_random_mode;
extern unsigned int palette_fixed_mode;
extern unsigned int palette_alternate_mode;
extern unsigned int palette_four_ega_colors;
extern unsigned int palette_base_red;
extern unsigned int palette_base_green;
extern unsigned int palette_base_blue;
extern unsigned int ega_red_limit;
extern double cos(double);
extern unsigned int ega_green_limit;
extern unsigned int ega_blue_limit;
extern unsigned int ega_dither_value;
extern unsigned int ega_palette_count;
extern unsigned int g_5dec;
extern unsigned int g_5d96;
extern unsigned int g_7438;
extern unsigned int g_743a;
extern unsigned int g_7484;
extern unsigned int g_7488;
extern unsigned int mouse_was_pressed;
extern unsigned int exit_requested;
extern unsigned int palette_timer_ticks;
extern unsigned int palette_cycle_interval;
extern unsigned int keyboard_led_flags(unsigned int, unsigned int);
extern void advance_palette_cycle(void);
extern void far set_logo_palette(unsigned int);
extern void (far *palette_backend)(unsigned char far *);
extern void randomize_palette_state(unsigned int);
extern int dispatch_keyboard_and_mouse(void);
extern void movmem(void far *, void far *, unsigned int);

/* objcmp: {"function":"generate_palette","address":"11c1:0e45","size":884,"level":"SEMANTIC","blocker":"register-allocation/codegen: first divergent instruction at 11c1:0E4C (oracle stores palette_base_green; candidate pushes DI); candidate 911 bytes, oracle 884","symbols":{"_palette_buffer":"DS:5d94","_palette_length":"DS:5db6","_palette_entries":"DS:5dba","_palette_update_index":"DS:7488","_palette_random_mode":"DS:5dda","_palette_fixed_mode":"DS:5de2","_palette_alternate_mode":"DS:5de0","_palette_four_ega_colors":"DS:5dd0","_palette_base_red":"DS:5d8a","_palette_base_green":"DS:5d6c","_palette_base_blue":"DS:5d6e","_ega_red_limit":"DS:743c","_ega_green_limit":"DS:7438","_ega_blue_limit":"DS:743a","_g_5dec":"DS:5dec","_keyboard_led_flags":"11c1:000a","_randomize_palette_state":"11c1:11b9","_dispatch_keyboard_and_mouse":"11c1:18e7","_advance_palette_cycle":"11c1:2515","_movmem":"0000:4d55","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void generate_palette(unsigned int allow_cycle) {
    unsigned int i;
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    int red_direction;
    int green_direction;
    int blue_direction;
    unsigned int red_phase;
    unsigned int green_phase;
    unsigned int blue_phase;
    unsigned int red_period;
    unsigned int green_period;
    unsigned int blue_period;
    unsigned int color;

    palette_base_green = 10;
    palette_base_blue = 12;
    palette_buffer[0] = (unsigned char)palette_base_red;
    palette_buffer[1] = (unsigned char)palette_base_green;
    palette_buffer[2] = (unsigned char)palette_base_blue;

    if (palette_random_mode) {
        for (i = 3; i < palette_entries; ++i)
            palette_buffer[i] = (unsigned char)random(palette_entries + 1);
    } else {
        red_period = random(palette_length - 4) + 4;
        green_period = random(palette_length - 4) + 4;
        blue_period = random(palette_length - 4) + 4;
        if (palette_alternate_mode) {
            if (red_period >= 8) red_period >>= 1;
            if (green_period >= 8) green_period >>= 1;
            if (blue_period >= 8) blue_period >>= 1;
        }

        red_phase = 0;
        green_phase = 0;
        blue_phase = 0;
        if (palette_fixed_mode) {
            red_period = palette_length;
            green_period = palette_length / 2;
            blue_period = 0;
            red = green = blue = 0;
        } else {
            red = random(2);
            green = random(2);
            blue = random(2);
        }

        red_direction = green_direction = blue_direction = 1;
        for (i = 3; i < palette_entries; ++i) {
            if (++red_phase > red_period) {
                red_phase = 0;
                red = (unsigned int)((int)red + red_direction);
                if (red >= 3 || red == 0)
                    red_direction = -red_direction;
            }
            if (++green_phase > green_period) {
                green_phase = 0;
                green = (unsigned int)((int)green + green_direction);
                if (green >= 3 || green == 0)
                    green_direction = -green_direction;
            }
            if (++blue_phase > blue_period) {
                blue_phase = 0;
                blue = (unsigned int)((int)blue + blue_direction);
                if (blue >= 3 || blue == 0)
                    blue_direction = -blue_direction;
            }
            color = ((red & 1) << 5) | ((red & 2) << 1) |
                    ((green & 1) << 4) | (green & 2) |
                    ((blue & 1) << 3) | ((blue & 2) >> 1);
            palette_buffer[i] = (unsigned char)color;
        }
    }

    if (palette_four_ega_colors) {
        palette_buffer[1] = (unsigned char)random(palette_entries + 1);
        palette_buffer[2] = (unsigned char)random(palette_entries + 1);
        palette_buffer[3] = (unsigned char)random(palette_entries + 1);
        for (i = 4; i < palette_entries; ++i)
            palette_buffer[i] = palette_buffer[i & 3];
    }

    movmem(palette_buffer + 3, palette_buffer + i, palette_length);
    ++palette_update_index;
    if (palette_update_index >= palette_length)
        palette_update_index = 3;
    if (allow_cycle && palette_timer_ticks >= palette_cycle_interval)
        advance_palette_cycle();
    keyboard_led_flags(1, 0x40);
}

/* objcmp: {"function":"randomize_palette_state","address":"11c1:11b9","size":401,"symbols":{"_palette_buffer":"DS:5d94","_palette_length":"DS:5db6","_palette_entries":"DS:5dba","_palette_random_mode":"DS:5dda","_palette_base_red":"DS:5d8a","_palette_base_green":"DS:5d6c","_palette_base_blue":"DS:5d6e","_ega_green_limit":"DS:7438","_ega_blue_limit":"DS:743a","_memset":"0000:4d36","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"}} */
void randomize_palette_state(unsigned int replace_base_colors) {
    unsigned char seed;

    if (random(20) && !replace_base_colors)
        seed = 0;
    else
        seed = (unsigned int)(random(16) + 0xef);
    memset(palette_buffer, seed, 9);
    if (replace_base_colors) {
        palette_buffer[0] = (unsigned char)random(seed);
        palette_buffer[1] = (unsigned char)random(seed);
        palette_buffer[2] = (unsigned char)random(seed);
    }
    palette_buffer[4] = (unsigned char)random(g_7438);
    palette_buffer[5] = (unsigned char)(g_7438 + random(g_7438));
    palette_buffer[6] = (unsigned char)random(ega_blue_limit);
    palette_buffer[8] = (unsigned char)g_7438;
}

/* objcmp: {"function":"change_background_palette","address":"11c1:134a","size":165,"symbols":{"_g_7484":"DS:7484","_randomize_palette_state":"11c1:11b9","_set_logo_palette":"17f5:030f","_palette_buffer":"DS:5d94","_palette_base_green":"DS:5d6c","_palette_base_blue":"DS:5d6e","_g_5dec":"DS:5dec","_palette_backend":"DS:5c37","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"}} */
void change_background_palette(void) {
    if (g_7484 == 8) {
        randomize_palette_state(1);
        set_logo_palette(3);
    } else {
        palette_buffer[0] = (unsigned char)(random(8) + 8);
        palette_buffer[1] = (unsigned char)(palette_base_green = random(8) + 8);
        palette_buffer[2] = (unsigned char)(palette_base_blue = random(8) + 8);
    }
    palette_backend(palette_buffer + g_5dec);
}

/* objcmp: {"function":"build_ega_palette","address":"11c1:13ef","size":1089,"level":"SEMANTIC","blocker":"local-frame/codegen: at 11c1:13F2 oracle reserves 0x2c bytes and candidate 0x1c; complete cosine, grayscale, polling, copy, and cycle paths are present; candidate 1071 bytes, oracle 1089","symbols":{"_palette_fixed_mode":"DS:5de2","_palette_alternate_mode":"DS:5de0","_palette_random_mode":"DS:5dda","_palette_length":"DS:5db6","_palette_entries":"DS:5dba","_palette_buffer":"DS:5d94","_g_7438":"DS:7438","_ega_red_limit":"DS:743c","_g_7484":"DS:7484","_randomize_palette_state":"11c1:11b9","_keyboard_led_flags":"11c1:000a","_dispatch_keyboard_and_mouse":"11c1:18e7","_advance_palette_cycle":"11c1:2515","_movmem":"0000:4d55","_rand":"0000:1694","_cos":"0000:0f6e","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXURSH@":"0000:13d6"}} */
void build_ega_palette(int update_cycle) {
    unsigned int phase[3];
    unsigned int amplitude[3];
    unsigned int i;
    unsigned int sample;
    float frequency[3];

    keyboard_led_flags(0x40, 0);
    if (palette_fixed_mode) {
        for (i = 0; i < 3; ++i) {
            phase[i] = 0;
            amplitude[i] = 47;
        }
    } else {
        for (i = 0; i < 3; ++i) {
            phase[i] = random(palette_length);
            if (random(3))
                amplitude[i] = 47;
            else
                amplitude[i] = random(23) + 23;
        }
    }

    if (palette_alternate_mode) {
        frequency[0] = 0.075f; frequency[1] = 0.087f; frequency[2] = 0.100f;
    } else {
        frequency[0] = 0.025f; frequency[1] = 0.033f; frequency[2] = 0.040f;
    }
    if (palette_random_mode) {
        frequency[0] = 0.150f; frequency[1] = 0.175f; frequency[2] = 0.200f;
    }
    randomize_palette_state(0);
    if (random(50)) {
        for (i = 0; i < palette_entries; ++i) {
            if (mouse_was_pressed || exit_requested) {
                if (dispatch_keyboard_and_mouse())
                    return;
            }
            sample = (unsigned int)(amplitude[0] *
                    cos((double)(phase[0] + i) * frequency[0]) +
                    amplitude[0] + 1.0f);
            palette_buffer[9 + i * 3] =
                (unsigned char)(sample > 31 ? sample - 31 : 0);
            sample = (unsigned int)(amplitude[1] *
                    cos((double)(phase[1] + i) * frequency[1]) +
                    amplitude[1] + 1.0f);
            palette_buffer[10 + i * 3] =
                (unsigned char)(sample > 31 ? sample - 31 : 0);
            sample = (unsigned int)(amplitude[2] *
                    cos((double)(phase[2] + i) * frequency[2]) +
                    amplitude[2] + 1.0f);
            palette_buffer[11 + i * 3] =
                (unsigned char)(sample > 31 ? sample - 31 : 0);
            if (update_cycle && palette_timer_ticks >= palette_cycle_interval)
                advance_palette_cycle();
        }
    } else {
        for (i = 0; i < palette_entries; ++i) {
            if (mouse_was_pressed || exit_requested) {
                if (dispatch_keyboard_and_mouse())
                    return;
            }
            sample = (unsigned int)(31.0 * cos((double)i * frequency[0]) + 31.0);
            palette_buffer[9 + i * 3] = (unsigned char)sample;
            palette_buffer[10 + i * 3] = (unsigned char)sample;
            palette_buffer[11 + i * 3] = (unsigned char)sample;
            if (update_cycle && palette_timer_ticks >= palette_cycle_interval)
                advance_palette_cycle();
        }
    }
    movmem(palette_buffer + 9, palette_buffer + palette_entries * 3,
           palette_length * 3);
    ++palette_update_index;
    if (palette_update_index >= palette_length)
        palette_update_index = 3;
    keyboard_led_flags(0x40, 1);
}

/* The heuristic starts at 15CB, 1626, 1681, and 1780 are not independently
   confirmed function entries; retain them as unresolved labels pending callsite review. */

extern unsigned int g_0bfe;
extern unsigned int g_0c00;
extern unsigned int freeze_led_state;
extern unsigned int mouse_was_pressed;
extern unsigned int exit_requested;
extern unsigned int palette_timer_ticks;
extern unsigned int palette_cycle_interval;
extern unsigned int menu_active;
extern unsigned int keyboard_led_flags(unsigned int, unsigned int);
extern void beep(unsigned int, unsigned int);
extern void advance_palette_cycle(void);
extern int dispatch_keyboard_and_mouse(void);

/* objcmp: {"function":"freeze_display","address":"11c1:1830","size":183,"level":"PROVISIONAL","blocker":"return-value/control-flow: candidate is 185 bytes, oracle 183; branch displacement at 11c1:184A targets 18AE instead of 18AC; bare return preserves dispatcher AX in Borland codegen","symbols":{"_g_0bfe":"DS:0bfe","_g_0c00":"DS:0c00","_freeze_led_state":"DS:5d82","_mouse_was_pressed":"DS:5d7e","_exit_requested":"DS:5d80","_palette_timer_ticks":"DS:5d7a","_palette_cycle_interval":"DS:5da6","_menu_active":"DS:24a2","_keyboard_led_flags":"11c1:000a","_beep":"11c1:0e07","_dispatch_keyboard_and_mouse":"11c1:18e7","_advance_palette_cycle":"11c1:2515"}} */
int freeze_display(unsigned int operation) {
    if (menu_active)
        operation = 2;

    switch (operation) {
    case 0:
        if (g_0bfe)
            return !g_0c00;
        g_0bfe = 1;
        freeze_led_state = keyboard_led_flags(16, 0);
        while (g_0bfe) {
            if (mouse_was_pressed || exit_requested) {
                if (dispatch_keyboard_and_mouse())
                    return;
            }
            if (g_0c00 && palette_timer_ticks >= palette_cycle_interval)
                advance_palette_cycle();
        }
        return 0;
    case 1:
        if (g_0bfe && !exit_requested)
            beep(440, 10);
        /* fall through */
    default:
        if (g_0bfe)
            keyboard_led_flags(freeze_led_state, 2);
        return g_0c00 = g_0bfe = 0;
    }
}

extern unsigned int mouse_abort_mask;
extern unsigned int keyboard_event_blocked;
extern unsigned int g_5d90;
extern unsigned int g_5d92;
extern unsigned char g_5d93;
extern unsigned int g_5d9e;
extern unsigned int auto_mode;
extern unsigned int video_mode;
extern unsigned int g_5dce;
extern unsigned int g_5d9c;
extern unsigned int g_0c0a;
extern unsigned int g_5dec;
extern int tolower(int);
extern int far _bios_keybrd(unsigned int);
extern void far help_menu(void);
extern void far configuration_shell(void);
extern void far save_pcx_image(void);
extern void (far *clear_palette_window)(unsigned int);
extern void (far *palette_backend)(unsigned char far *);
extern void change_background_palette(void);
extern unsigned int palette_step;

/* objcmp: {"function":"dispatch_keyboard_and_mouse","address":"11c1:18e7","size":1149,"level":"SEMANTIC","blocker":"shared-exit-tail/codegen: at 11c1:190C candidate jumps to 1CFB, oracle jumps to 1CF5; candidate is 6 bytes longer and duplicates the exit_requested store","symbols":{"_mouse_was_pressed":"DS:5d7e","_mouse_abort_mask":"DS:5d7c","_exit_requested":"DS:5d80","_bashful_mode":"DS:5dca","_keyboard_event_blocked":"DS:5d8e","_g_5d90":"DS:5d90","_g_5d92":"DS:5d92","_g_5d93":"DS:5d93","_g_5d9e":"DS:5d9e","_auto_mode":"DS:5da0","_palette_cycle_interval":"DS:5da6","_palette_step":"DS:5da4","_menu_active":"DS:24a2","_video_mode":"DS:7484","_g_5dce":"DS:5dce","_g_5d9c":"DS:5d9c","_g_0c0a":"DS:0c0a","_palette_buffer":"DS:5d94","_g_5dec":"DS:5dec","_bioskey":"0000:2553","__bios_keybrd":"0000:2578","_tolower":"0000:1f66","_beep":"11c1:0e07","_freeze_display":"11c1:1830","_help_menu":"158d:05dc","_configuration_shell":"158d:2256","_save_pcx_image":"19f0:060b","_clear_palette_window":"DS:5c47","_palette_backend":"DS:5c37","_change_background_palette":"11c1:134a"}} */
int dispatch_keyboard_and_mouse(void) {
    unsigned int key;

    if (mouse_was_pressed)
        --mouse_was_pressed;
    if (mouse_abort_mask) {
        mouse_abort_mask = 0;
        if (keyboard_event_blocked)
            return exit_requested;
        beep(1200, 5);
        exit_requested = 1;
    }
    if (exit_requested || bashful_mode) {
        exit_requested = 1;
        return exit_requested;
    }
    if (!bioskey(1))
        return exit_requested;

    g_5d92 = (unsigned int)_bios_keybrd(0);
    if ((g_5d92 & 0xff) != 0) {
        g_5d92 = (unsigned int)tolower((int)(g_5d92 & 0x7f));
        g_5d93 = 0;
        key = g_5d92;
        switch (key) {
        case 0x0009:
            freeze_display(0);
            g_5d92 = g_5d90;
            return exit_requested;
        case 0x000a:
        case 0x000d:
            g_5d90 = g_5d92;
            if (menu_active || keyboard_event_blocked)
                return exit_requested;
            g_5d90 = 0;
            exit_requested = 1;
            return exit_requested;
        case 0x001b:
            g_5d90 = 0;
            exit_requested = 1;
            return exit_requested;
        case ' ':
            freeze_display(1);
            g_5d92 = g_5d90;
            return exit_requested;
        case '+':
            if (palette_cycle_interval > 2)
                palette_cycle_interval -= palette_cycle_interval >> 1;
            else
                --palette_cycle_interval;
            if (palette_cycle_interval < 1) {
                palette_cycle_interval = 1;
                beep(210, 20);
            }
            return exit_requested;
        case '-':
            if (palette_cycle_interval > 2)
                palette_cycle_interval += palette_cycle_interval >> 1;
            else
                ++palette_cycle_interval;
            if (palette_cycle_interval > 0x40) {
                palette_cycle_interval = 0x40;
                beep(210, 20);
            }
            return exit_requested;
        case 'n':
            freeze_display(1);
            g_5d90 = g_5d92;
            return exit_requested;
        default:
            if (auto_mode)
                g_5d90 = g_5d92;
            else
                g_5d92 = g_5d90;
            return exit_requested;
        }
    }

    key = g_5d92;
    g_5d93 = (unsigned char)(key >> 8);
    if (g_5dce || g_0c0a) {
        g_5d92 = g_5d90;
        return exit_requested;
    }
    if (g_5d93 >= 0x78 && g_5d93 <= 0x81) {
        g_5d92 = (unsigned char)(0x80 | (unsigned char)(g_5d93 + 0x88));
    } else if (g_5d93 >= 0x10 && g_5d93 <= 0x19) {
        g_5d92 = (unsigned char)(0x80 | (unsigned char)(g_5d93 + 0xfa));
    } else if (g_5d93 >= 0x1e && g_5d93 <= 0x26) {
        g_5d92 = (unsigned char)(0x80 | (unsigned char)(g_5d93 + 0xf6));
    } else if (g_5d93 >= 0x2c && g_5d93 <= 0x32) {
        g_5d92 = (unsigned char)(0x80 | (unsigned char)(g_5d93 + 0xf1));
    } else {
        switch (key) {
        case 0x3b00:
            g_5d92 = 0;
            g_5d90 = 0;
            help_menu();
            freeze_display(2);
            return exit_requested;
        case 0x3c00:
            g_5d92 = 0;
            g_5d90 = 0;
            configuration_shell();
            freeze_display(2);
            return exit_requested;
        case 0x3f00:
            if (keyboard_event_blocked || menu_active) {
                g_5d92 = g_5d90;
                return exit_requested;
            }
            save_pcx_image();
            g_5d92 = g_5d90;
            return exit_requested;
        case 0x4200:
            if (!auto_mode) {
                g_5d92 = g_5d90;
                return exit_requested;
            }
            g_5d90 = g_5d92;
            g_5d9e = !g_5d9e;
            if (g_5d9e)
                beep(3000, 2);
            else
                beep(2200, 2);
            return exit_requested;
        case 0x4300:
        case 0x4400:
            if (auto_mode)
                g_5d90 = g_5d92;
            else
                g_5d92 = g_5d90;
            return exit_requested;
        case 0x4700:
            g_5d92 = 0x4700;
            g_5d90 = g_5d92;
            return exit_requested;
        case 0x4800:
            g_5d92 = 0x4900;
            g_5d90 = g_5d92;
            return exit_requested;
        case 0x4900:
            if (menu_active || video_mode != 8) {
                g_5d92 = 0x4900;
                g_5d90 = g_5d92;
                return exit_requested;
            }
            g_5d92 = g_5d90;
            if (palette_step < 0x39)
                palette_step += 3;
            if (palette_step >= 0x39) {
                palette_step = 0x39;
                beep(190, 20);
            }
            return exit_requested;
        case 0x4b00:
            g_5d92 = 0x4700;
            g_5d90 = g_5d92;
            return exit_requested;
        case 0x4d00:
        case 0x4f00:
            g_5d92 = 0x4f00;
            g_5d90 = g_5d92;
            return exit_requested;
        case 0x5000:
            g_5d92 = 0x5100;
            g_5d90 = g_5d92;
            return exit_requested;
        case 0x5100:
            if (menu_active || video_mode != 8) {
                g_5d92 = 0x5100;
                g_5d90 = g_5d92;
                return exit_requested;
            }
            g_5d92 = g_5d90;
            if (palette_step > 3)
                palette_step -= 3;
            if (palette_step <= 3) {
                palette_step = 3;
                beep(190, 20);
            }
            return exit_requested;
        case 0x5d00:
            if (menu_active) {
                g_5d92 = g_5d90;
                return exit_requested;
            }
            g_0c0a = 1;
            beep(1000, 10);
            clear_palette_window(1);
            palette_backend(palette_buffer + g_5dec);
            beep(1000, 5);
            g_0c0a = 0;
            g_5d92 = g_5d90;
            return exit_requested;
        case 0x6700:
            if (menu_active) {
                g_5d92 = g_5d90;
                return exit_requested;
            }
            beep(1500, 10);
            change_background_palette();
            beep(1000, 5);
            g_5d92 = g_5d90;
            return exit_requested;
        case 0x7000:
            auto_mode = !auto_mode;
            g_5d92 = 'n';
            g_5d90 = g_5d92;
            if (auto_mode) {
                freeze_display(2);
                beep(880, 20);
            } else {
                beep(440, 20);
            }
            return exit_requested;
        case 0x7100:
            if (auto_mode)
                g_5d90 = g_5d92;
            else
                g_5d92 = g_5d90;
            return exit_requested;
        default:
            g_5d92 = g_5d90;
            return exit_requested;
        }
    }

    g_5d90 = g_5d92;
    if ((g_5d92 & 0x7f) >= g_5d9c) {
        g_5d92 = 0;
        g_5d90 = 0;
    }
    return exit_requested;
}
