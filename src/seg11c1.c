/* DAZZLE 5.0 DOSI module, BC++ 3.00 -ml -O1. */
/* The original DOSI pool merges duplicate strings and shared suffixes. */
#pragma option -d
#include <stdlib.h>
#include <dos.h>
#include <math.h>
#include <stdio.h>
#include <conio.h>
#include <bios.h>
#include <dir.h>
#include <string.h>
#include <ctype.h>

#include "dazzle.h"

/* Initialized DOSI state. Text and numeric literals follow in source order. */
unsigned g_0bfe = 0, g_0c00 = 0;
unsigned far *bios_keyboard_flags = (unsigned far *)0x00000417UL;
unsigned long crc_polynomial = 0x00102100UL;
unsigned g_0c0a = 0;
unsigned tone_chord[4] = {0, 4, 7, 10};
float tone_base_frequency = 16.35f;
char config_load_error_text[] = "\n Unable to load file : %s\n";
char revision_label[] = " rev. ";
char copyright_text[] = "Copyright(c)1989-92 MicroTronics";

/* Reconstructed 11C1 startup, integrity, and input routines. */

/* objcmp: {"function":"keyboard_led_flags","address":"11c1:000a","size":101,"symbols":{"_bios_keyboard_flags":"DS:0c02","_window_mode":"DS:5dbc","_bashful_mode":"DS:5dca","_bioskey":"0000:2553","_executable_name_pointer":"DS:0096","_fade_records":"DS:0474","_background_fill_word":"DS:5d8a","_g_5d92":"DS:5d92","_palette_buffer":"DS:5d94","_image_source_bytes":"DS:7456","_resource_base":"DS:5d68","_selfcheck_stream":"DS:6002","_mode_setup_callback":"DS:5c43","_saved_text_info":"DS:6016","__streams":"DS:43f4","_saved_keyboard_vector":"DS:5ff2","_saved_timer_vector":"DS:5ff6","_palette_base_green":"DS:5d6c"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
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

/* objcmp: {"function":"mode_ready_chirp","address":"11c1:006f","size":73,"symbols":{"_muted":"DS:5dbe","_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
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

/* objcmp: {"function":"default_signal_result","address":"11c1:00b8","size":4,"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"symbols":{}} */
int default_signal_result(void) {
    return 1;
}

/* objcmp: {"function":"interrupt_signal_result","address":"11c1:00bc","size":13,"symbols":{"_hardresume":"0000:30cc"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
int interrupt_signal_result(void) {
    hardresume(0);
    return 2;
}

/* objcmp: {"function":"install_runtime_callbacks","address":"11c1:00c9","size":236,"symbols":{"_skip_ctrlbreak_control":"DS:5d70","_timer_hook_installed":"DS:5d74","_keyboard_hook_installed":"DS:5d72","_original_ctrlbreak_setting":"DS:5dea","_saved_timer_vector":"DS:5ff6","_saved_keyboard_vector":"DS:5ff2","_current_image_header":"DS:5c5b","_mouse_available":"DS:5cb5","_window_mode":"DS:5dbc","_probe_mouse":"11c1:0442","_mouse_position":"11c1:0501","_graphics_timer_tick_isr":"1899:14c7","_keyboard_pending_isr":"1899:154c","_getvect":"0000:12c4","_setvect":"0000:12d3","_getcbrk":"0000:1216","_setcbrk":"0000:1220","_harderr":"0000:30ba","_ctrlbrk":"0000:1065","_mouse_pending_callback":"1899:155f"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void install_runtime_callbacks(void) {
    volatile int mouse_type;
    unsigned int callback_mask;
    ctrlbrk((int (far *)(void))default_signal_result);
    if (!skip_ctrlbreak_control) {
        original_ctrlbreak_setting = getcbrk();
        setcbrk(0);
    }
    if (!timer_hook_installed) {
        saved_timer_vector = getvect(0x1c);
        setvect(0x1c, graphics_timer_tick_isr);
        timer_hook_installed = 1;
    }
    if (!keyboard_hook_installed) {
        saved_keyboard_vector = getvect(9);
        setvect(9, keyboard_pending_isr);
        keyboard_hook_installed = 1;
    }
    if (!mouse_available || (current_image_header[6] & 0x0800)) {
        if ((mouse_type = probe_mouse(1)) != 0) {
            callback_mask = 0x2a;
            if (bashful_mode && mouse_type != 2)
                callback_mask |= 1;
            mouse_position(callback_mask, FP_OFF(mouse_pending_callback), FP_SEG(mouse_pending_callback));
        }
    }
    harderr(interrupt_signal_result);
}

/* objcmp: {"function":"restore_text_attributes","address":"11c1:01b5","size":18,"symbols":{"_original_text_attribute":"DS:601a","_normvideo":"0000:2641","_textattr":"0000:262a"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void restore_text_attributes(void) {
    normvideo();
    textattr(original_text_attribute);
}

/* objcmp: {"function":"restore_runtime_callbacks","address":"11c1:01c7","size":173,"symbols":{"_timer_hook_installed":"DS:5d74","_keyboard_hook_installed":"DS:5d72","_saved_timer_vector":"DS:5ff6","_saved_keyboard_vector":"DS:5ff2","_original_ctrlbreak_setting":"DS:5dea","_mouse_available":"DS:5cb5","_current_image_header":"DS:5c5b","_mouse_callback_installed":"DS:5cb9","_keep_typematic":"DS:5dd2","_keyboard_regs":"DS:6006","_probe_mouse":"11c1:0442","_setvect":"0000:12d3","_setcbrk":"0000:1220","_fcloseall":"0000:3f37","_int86":"0000:320e"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
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
    if (!mouse_available || (current_image_header[6] & 0x0800))
        probe_mouse(0);
    if (!keep_typematic) {
        ((unsigned char *)&keyboard_regs)[1] = 3;
        ((unsigned char *)&keyboard_regs)[0] = 5;
        ((unsigned char *)&keyboard_regs)[3] = 1;
        ((unsigned char *)&keyboard_regs)[2] = 7;
        int86(0x16, &keyboard_regs, &keyboard_regs);
    }
    fcloseall();
    if (mouse_callback_installed)
        current_image_header[5] &= 0xf7ff;
}

/* objcmp: {"function":"check_registration_frame","address":"11c1:0274","size":100,"symbols":{"_restore_runtime_callbacks":"11c1:01c7","_restore_video_state":"1899:0bcd","_print_order_info":"11c1:0ab6","_math_error_text":"DS:0c5c","_program_revision_format":"DS:0c96","_reason_label":"DS:0cb4","_program_revision":"DS:009e","_cprintf":"0000:277a","_printf":"0000:4fb3"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
int check_registration_frame(void far *frame) {
    if (((unsigned int far *)frame)[0] == 5 ||
        ((unsigned int far *)frame)[0] == 4)
        return 1;
    restore_runtime_callbacks();
    restore_video_state(1);
    print_order_info(0);
    printf("\nmatherr -- program aborted. Contact us for assistance.\a\n");
    fprintf(&standard_error_stream, "Program revision level : %s\n\n", program_revision);
    printf("REASON ");
    return 0;
}

/* objcmp: {"function":"signal_diagnostic","address":"11c1:02d8","size":362,"symbols":{"_restore_runtime_callbacks":"11c1:01c7","_restore_video_state":"1899:0bcd","_signal_error_text":"DS:0cbc","_signal_announcement":"DS:0cf2","_signal_register_format":"DS:0d2b","_segment_map_title":"DS:0d4e","_segment_dzle_format":"DS:0d5e","_segment_dzl2_format":"DS:0d70","_segment_dzfd_format":"DS:0d82","_segment_dvdt_format":"DS:0d94","_segment_dosi_format":"DS:0da6","_segment_dhuf_format":"DS:0db8","_segment_dlogo_format":"DS:0dca","_segment_dazpcx_format":"DS:0ddc","_segment_map_end_format":"DS:0def","_standard_error_stream":"DS:441c","_help_program_name":"DS:009a","_program_revision":"DS:009e","_fprintf":"0000:45d4","_near_cs":"066a:000b","_get_code_segment":"0b84:0000","_get_dzfd_segment":"0ccd:000a","_video_code_segment":"1899:1571","_lzh_code_segment":"1a72:0007","_current_code_segment":"17f5:0008","_pcx_code_segment":"19f0:0004","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void signal_diagnostic(int signal_number, int signal_type, unsigned int far *registers) {
    unsigned int current_segment;
    current_segment = _CS;
    restore_runtime_callbacks();
    restore_video_state(1);
    fprintf(&standard_error_stream, "sigerr -- program aborted. Contact us for assistance\n");
    fprintf(&standard_error_stream, "Signal %d, Type %d,  Please print this error message!\a\n\n", signal_number, signal_type);
    fprintf(&standard_error_stream, "Error at %04X:%04X\t FLAGS = %04X\n\n",
            registers[0x14 / 2], registers[0x12 / 2], registers[0x16 / 2]);
    fprintf(&standard_error_stream, "SEGMENT MAP : \n");
    fprintf(&standard_error_stream, "DZLE_Seg  = %04X\n", near_cs());
    fprintf(&standard_error_stream, "DZL2_Seg  = %04X\n", get_code_segment());
    fprintf(&standard_error_stream, "DZFD_Seg  = %04X\n", get_dzfd_segment());
    fprintf(&standard_error_stream, "DVDT_Seg  = %04X\n", video_code_segment());
    fprintf(&standard_error_stream, "DOSI_Seg  = %04X\n", current_segment);
    fprintf(&standard_error_stream, "DHUF_Seg  = %04X\n", lzh_code_segment());
    fprintf(&standard_error_stream, "DLOGO_Seg = %04X\n", current_code_segment());
    fprintf(&standard_error_stream, "DAZPCX_Seg = %04X\n", pcx_code_segment());
    fprintf(&standard_error_stream, "\nProgram revision level :%s, %s\n\n",
            help_program_name, program_revision);
    print_order_info(0);
    exit(9);
}

/* objcmp: {"function":"probe_mouse","address":"11c1:0442","size":191,"symbols":{"_mouse_regs":"DS:743e","_intr":"0000:32d8","_delay":"0000:29a6"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
int probe_mouse(unsigned int mask) {
    unsigned int status;
    unsigned int attempts;
    unsigned int buttons;
    mouse_regs.r_ax = 0;
    intr(0x33, &mouse_regs);
    status = attempts = 0;
    if (mouse_regs.r_ax)
        status = 1;
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
        } while (++attempts <= 50 &&
                 (buttons | mouse_regs.r_cx | mouse_regs.r_dx) != 0);
        if (attempts >= 50)
            status = 2;
    }
    return status & mask;
}

/* objcmp: {"function":"mouse_position","address":"11c1:0501","size":46,"symbols":{"_mouse_regs":"DS:743e","_intr":"0000:32d8"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void mouse_position(int buttons, int x, int y) {
    mouse_regs.r_cx = buttons;
    mouse_regs.r_es = y;
    mouse_regs.r_dx = x;
    mouse_regs.r_ax = 12;
    intr(0x33, &mouse_regs);
}

/* objcmp: {"function":"fatal_selfcheck_error","address":"11c1:052f","size":148,"symbols":{"_exit_requested":"DS:5d80","_computed_crc":"DS:6021","_expected_crc":"DS:6027","_serial_r1":"DS:602b","_serial_r2":"DS:602d","_selfcheck_format_with_crc":"DS:0e11","_selfcheck_format_without_crc":"DS:0e96","_selfcheck_reason_format":"DS:0ec6","_standard_error_stream":"DS:441c","_restore_video_state":"1899:0bcd","_fprintf":"0000:45d4","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void fatal_selfcheck_error(int show_crc, const char far *reason) {
    exit_requested = 1;
    restore_video_state(1);
    if (show_crc)
        fprintf(&standard_error_stream, "\r Program aborting in antiviral routines! cv=%08lX, r1=%05u, r2=%05u\nThis feature is for YOUR protection, write us for a good copy.\n", expected_crc, serial_r1,
                serial_r2, computed_crc);
    else
        fprintf(&standard_error_stream, "\r Program aborting! cv=%08lX, r1=%05u, r2=%05u\n", expected_crc, serial_r1,
                serial_r2);
    fprintf(&standard_error_stream, "\nREASON: %s \n\n\a", reason);
    print_order_info(0);
    exit(100);
}

/* objcmp: {"function":"read_mz_resource_base","address":"11c1:05c3","size":104,"symbols":{"_resource_base":"DS:5d68","_fread":"0000:46e4","F_LXMUL@":"0000:11e3","_fatal_selfcheck_error":"11c1:052f","_selfcheck_read_error":"DS:0ed6"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void read_mz_resource_base(FILE far *input) {
    int header[3];
    if (fread(header, 2, 3, input) != 3)
        fatal_selfcheck_error(1, "can't read EXE header");
    resource_base = header[2] - 1;
    resource_base *= 512;
    resource_base += header[1] + 1;
}

/* objcmp: {"function":"allocate","address":"11c1:062b","size":65,"symbols":{"_malloc":"0000:2c51"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void *allocate(unsigned *amount, unsigned minimum) {
    unsigned n;
    void *p;
    n = *amount;
    while (n >= minimum && (p = malloc(n)) == 0)
        n /= 2;
    *amount = n;
    return p;
}

/* objcmp: {"function":"genCRC","address":"11c1:066c","size":501,"symbols":{"_crc_polynomial":"DS:0c06","_allocate":"11c1:062b","_fseek":"0000:47f4","_fread":"0000:46e4","_free":"0000:2b47","_fatal_selfcheck_error":"11c1:052f","_crc_buffer_error":"DS:0f05","_crc_read_error":"DS:0eec"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
unsigned long genCRC(FILE far *input, long length) {
    unsigned int i;
    unsigned int got;
    unsigned char far *buffer;
    unsigned int amount;
    unsigned long far *table_entry;
    unsigned long crc;
    unsigned char far *cursor;
    long processed;
    long remaining;
    unsigned long table[256];

    amount = 0x2000;
    buffer = (unsigned char far *)allocate(&amount, 0x200);
    if (buffer) {
        i = 0;
        table_entry = table;
        table[0] = 0;
        for (; i < 256; i += 2, ++table_entry) {
            if (((unsigned char far *)table_entry)[3] & 0x80)
                table[i] = (table[i + 1] = *table_entry << 1) ^ crc_polynomial;
            else
                table[i + 1] = (table[i] = *table_entry << 1) ^ crc_polynomial;
        }

        crc = 0;
        if (fseek(input, 0L, 0) != 0) {
            free(buffer);
            fatal_selfcheck_error(1, "failure in genCRC fseek!");
        }
        processed = 0;
        while (processed < length) {
            remaining = length - processed - 1;
            got = amount < remaining ? amount : (unsigned int)remaining;
            if ((i = fread(buffer, 1, got, input)) == 0)
                break;
            cursor = buffer;
            while (i--) {
                crc = (crc << 8) ^ table[((unsigned char *)&crc)[3] ^ *cursor];
                ++cursor;
            }
            processed += got;
        }
        free(buffer);
    } else
        fatal_selfcheck_error(1, "memory unavailable in genCRC ");
    return crc;
}

/* objcmp: {"function":"deRead","address":"11c1:0861","size":232,"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"symbols":{"_selfcheck_stream":"DS:6002","_resource_base":"DS:5d68","_insecure_argument_index":"DS:5dc8","_executable_path":"DS:5d17","_selfcheck_open_error":"DS:0f26","_selfcheck_seek_error":"DS:0f3d","_selfcheck_metadata_error":"DS:0f54","_computed_crc":"DS:6021","_genCRC":"11c1:066c","_read_mz_resource_base":"11c1:05c3","_fatal_selfcheck_error":"11c1:052f","_fopen":"0000:459f","_fseek":"0000:47f4","_fread":"0000:46e4","_fclose":"0000:407f"}} */
int deRead(void far *metadata, unsigned int size) {
    int result;
    result = 0;
    if (!(selfcheck_stream = fopen(executable_path, "rb")))
        fatal_selfcheck_error(1, "deRead can't open file");
    read_mz_resource_base(selfcheck_stream);
    if (!insecure_argument_index)
        computed_crc = genCRC(selfcheck_stream, resource_base);
    if (fseek(selfcheck_stream, resource_base, 0) != 0)
        fatal_selfcheck_error(1, "deRead file seek error");
    if (fread(metadata, size, 1, selfcheck_stream) != 1)
        result = !insecure_argument_index;
    if (fclose(selfcheck_stream) != 0)
        fatal_selfcheck_error(1, "deRead can't close file");
    return result;
}

/* objcmp: {"function":"check_executable_integrity","address":"11c1:0949","size":365,"symbols":{"_insecure_argument_index":"DS:5dc8","_executable_path":"DS:5d17","_exit_requested":"DS:5d80","_expected_crc":"DS:6027","_computed_crc":"DS:6021","_deRead":"11c1:0861","_fatal_selfcheck_error":"11c1:052f","_textcolor":"0000:25fc","_cprintf":"0000:277a","_stricmp":"0000:59f5","_fnsplit":"0000:439f","_searchpath":"0000:1ea4","_strlen":"0000:5a36","_strncpy":"0000:5a79","_standard_error_stream":"DS:441c","_fprintf":"0000:45d4","_beep":"11c1:0e07","_integrity_check_intro":"DS:0f6c","_selfcheck_missing_name":"DS:0f82","_selfcheck_bad_program_name":"DS:0fa3","_selfcheck_bad_crc":"DS:0fca","_selfcheck_missing_data":"DS:1000","_selfcheck_done":"DS:1017","_help_program_name":"DS:009a","_executable_name_pointer":"DS:0096","_g_0098":"DS:0098"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void check_executable_integrity(void) {
    char far *resolved_path;
    char drive[4];
    char basename[10];
    char extension[6];
    int path_flags;
    char directory[66];

    if (!insecure_argument_index) {
        textcolor(14);
        cprintf(" antiviral checking \r");
        beep(2000, 4);
        textcolor(3);
    }

    if (strlen(executable_path) == 0) {
        resolved_path = searchpath(executable_name_pointer);
        if (!resolved_path) {
            fatal_selfcheck_error(0, "can't locate file for self check");
        } else {
            strncpy(executable_path, strupr(resolved_path), 80);
        }
    } else {
        path_flags = fnsplit(executable_path, drive, directory, basename, extension);
        if (!(path_flags & 4) || (stricmp(basename, help_program_name) != 0 &&
            !insecure_argument_index)) {
            fprintf(&standard_error_stream, "\nProgram name should be %s, not %s.%s\n",
                    executable_name_pointer, basename, extension);
            delay(500);
        }
    }

    if (!deRead(&expected_crc, 10)) {
        if (!insecure_argument_index && expected_crc != computed_crc)
            fatal_selfcheck_error(1, "bad ckval--program may have been altered, do not use!");
    } else
        fatal_selfcheck_error(1, "missing antiviral data");

    cprintf("                    \r");
    exit_requested = 0;
}

/* objcmp: {"function":"print_order_info","address":"11c1:0ab6","size":311,"symbols":{"_serial_r1":"DS:602b","_serial_r2":"DS:602d","_standard_output_stream":"DS:4408","_order_form_text":"DS:10f0","_registration_summary":"DS:1285","_order_form_notice":"DS:165a","_order_name_format":"DS:16a1","_order_address_format":"DS:16a4","_address_text":"DS:102d","_compuserve_text":"DS:1046","_company_text":"DS:1077","_phone_text":"DS:1086","_mailing_text":"DS:10c1","_help_program_name":"DS:009a","_program_revision":"DS:009e","_registration_notice_pointer":"DS:00a2","_textcolor":"0000:25fc","_cprintf":"0000:277a","_fprintf":"0000:45d4"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void print_order_info(int show_order_form) {
    char far *address_line;
    char far *compuserve_line;
    char far *company_line;
    char far *phone_line;
    char far *mailing_line;

    address_line = "Mailing address (1992) :";
    compuserve_line = "                         Compuserve   76236,1573";
    company_line = " MicroTronics ";
    phone_line = "                                   Voice/FAX (713)298-1698";
    mailing_line = " P.O.Box 8759 \r\n Spring, TX. U.S.A. 77387-8759";
    if (show_order_form) {
        fprintf(&standard_output_stream, "\r\nSoftware products can be ordered from MicroTronics at the\n\raddress below. Please include this completed form with your check.\n\rWe do not CURRENTLY take individual telephone orders, so please write.\n\r\n\rPlease PRINT your full name and address clearly.\n\r\n\rProgram name : %s \n\rRevision     : %s \n\rID codes     : r1=%05u,  r2=%05u\n\r\n\r%s\n\r%s\n\r\n\r *** MicroTronics product REGISTRATION and ORDER FORM ***\r\n%s\n\r",
                help_program_name, program_revision, serial_r1, serial_r2,
                company_line, mailing_line, registration_notice_pointer);
        fprintf(&standard_output_stream, "\n\rCheck your MSDOS floppy disk format : 5.25(__)  3.5(__)\n\r  DSHD [High Density] (__)  DSDD [Low Density] (__)\n\r\n\rWhen registering this shareware program you will receive a credit for\n\ran update disk. Registered users receive our regular mailings providing\n\rthe most current information about program enhancements and new products.\n\r\n\rCheck here if you want to receive a program disk now,\n\rinstead of receiving a credit for your future use : (__)\n\r\r\nCheck here if interested in our retail DOS and Windows self activating\r\n Screen Saver products : (__)\r\n\n\rCheck here if interested in Multi-Copy or Site License prices : (__)\n\r\n\rCheck here to receive information about MT-DAZZLE Music Videos, shirts,\n\r mugs, cards, prints, etc. : (__)\r\n\r\nPlease review the User's Manual for an abundance of useful information,\n\r and legal limitations on the use of this program and it's images.\r\nEnter \"%s -?\" at the DOS prompt for a short help screen, and instructions on extracting the manual.\r\n\014", help_program_name);
        textcolor(3);
        cprintf("\r\n An order form has been provided.\r\n Please address it as follows:\r\n\n");
    }
    textcolor(3);
    cprintf("%s", address_line);
    textcolor(7);
    cprintf("%s \r\n", compuserve_line);
    textcolor(15);
    cprintf("%s", company_line);
    textcolor(7);
    cprintf("%s \r\n", phone_line);
    textcolor(15);
    cprintf("%s \r\n", mailing_line);
}

/* objcmp: {"function":"command_line_help","address":"11c1:0bed","size":345,"symbols":{"_fcloseall":"0000:3f37","_textcolor":"0000:25fc","_cprintf":"0000:277a","_exit":"0000:11a8","_help_intro":"DS:16aa","_help_heading":"DS:16cb","_help_option_2":"DS:16f3","_help_option_a":"DS:173a","_help_option_more":"DS:176f","_help_option_toggle":"DS:19de","_help_option_suffix":"DS:1a18","_help_default_config_format":"DS:1b2a","_help_footer":"DS:1bcc","_help_palette_option":"DS:1abb","_help_cycle_option":"DS:1ac2","_help_misc_option":"DS:1add","_help_extra_option":"DS:1b0f","_help_program_name":"DS:009a","_help_order_form":"DS:1c7b","_default_config_name":"DS:5ffa","_print_order_info":"11c1:0ab6","_getch":"0000:2f7c"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void command_line_help(void) {
    fcloseall();
    textcolor(7);
    cprintf("\r Usage: %s -options inputfile\n\r", help_program_name);
    textcolor(11);
    cprintf("       -$  Display order information.\r\n");
    textcolor(3);
    cprintf("       -2  2 part palette update to fix flickering screens (slower).\r\n");
    cprintf("       -a  Alternate (faster) color cycling speed.\r\n");
    cprintf("       -b  Bashful, no closing credits & any key aborts.\r\n       -c  Compatible mode, use BIOS for better portability.\r\n       -d  Delay, slow down drawing for faster PC's.\r\n           \"-d=xxxx\" delay xxxx units.\r\n       -i  Insecure, no antiviral checking.\r\n       -m  Mad (fastest) color cycling speed.\r\n       -n  No retract of hard disk heads.\r\n       -q  Quiet, no tones (no effect on warning beeps).\r\n       -s  Show mode, quick single image.\r\n           \"-s=xxxx\" Show mode, duration xxxx seconds.\r\n       -w  Window environment, no messages & changes some defaults.\r\n       -x  XT compatible, inferior but works.\r\n");
    textcolor(11);
    cprintf("       -z  Create DAZZLE.DOC file on current disk path!\r\n");
    textcolor(7);
    cprintf("\r\nDuring the display the TAB key will FREEZE the display till SPACE is pressed.\r\nF1 activates on-line help during the program. F2 provides a configuration menu.\r\n");
    textcolor(12);
    cprintf("ESCAPE");
    textcolor(7);
    cprintf(" terminates the program.\r\n");
    textcolor(14);
    cprintf(" ** Refer to DAZZLE.DOC for far more details **\r\n");
    textcolor(7);
    cprintf("Press any key to continue\r");
    textcolor(3);
    getch();
    textcolor(7);
    cprintf("If an \"inputfile\" is not specified, the program will try to locate\r\nand load %s from the current path. To prevent this load\r\nspecify an inputfile name of \"*\".\r\n\n", default_config_name);
    textcolor(3);
    cprintf(" Additional Options\r\n       -e  EGA operation mode forced.\r\n       -h  Hide EGA border.\r\n       -k  Keyboard typematic unchanged.\r\n       -t  Tall video aspect ratio mode.\r\n\n");
    textcolor(2);
    cprintf("\376\376\376\376\376 limited use command line options \376\376\376\376\376\r\n       -+  Ignore TARGA+64, even if present.\r\n       -!  Monitor Screen Saver driver interface.\r\n       -4  Restrict the EGA mode to only 4 random colors.\r\n       -f  Fixed colors instead of randomized.\r\n       -v  VGA operation mode forced.\r\n\n");
    print_order_info(0);
    exit(10);
}

/* objcmp: {"function":"print_closing_text_and_exit","address":"11c1:0d46","size":63,"symbols":{"_bashful_mode":"DS:5dca","_serial_r1":"DS:602b","_serial_r2":"DS:602d","_closing_text":"DS:1d9e","_program_revision":"DS:009e","_registration_notice_pointer":"DS:00a2","_standard_output_stream":"DS:4408","_fprintf":"0000:45d4","_print_order_info":"11c1:0ab6","_exit":"0000:11a8"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void print_closing_text_and_exit(int status) {
    if (!bashful_mode) {
        fprintf(&standard_output_stream, "\nThis program may be distributed as is AT NO CHARGE by the distributor\n(excluding reasonable duplication cost). All commercial rights retained\nby the author! This program must not be altered in any way, and is offered\nas is without warranty. Refer to User's Manual for usage limitations and\nroyalty requirements.\n\nEnter \"DAZZLE -Z\" at the DOS prompt to generate the User's Manual.\nEnter \"DAZZLE -$ > PRN:\" at the DOS prompt to print an order form.\n\n%s\nWhen you write be sure to mention your floppy disk format and this program's\nversion of %s \n\n", registration_notice_pointer, program_revision);
        print_order_info(0);
    }
    exit(status);
}

/* objcmp: {"function":"reject_exec_filename","address":"11c1:0d85","size":130,"symbols":{"_strupr":"0000:5b18","_strstr":"0000:5aaa","_strcpy":"0000:59cc","_default_config_name":"DS:5ffa","_menu_error_message":"158d:2492","_exe_suffix":"DS:1fc0","_bat_suffix":"DS:1fc5","_com_suffix":"DS:1fca","_rejected_name_warning":"DS:1fcf"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
int reject_exec_filename(char far *filename) {
    int rejected;
    rejected = 0;
    strupr(filename);
    if (strstr(filename, ".EXE") || strstr(filename, ".BAT") ||
        strstr(filename, ".COM")) {
        menu_error_message("EXE, COM, and BAT are illegal extentions for File Name!");
        strcpy(filename, default_config_name);
        rejected = 1;
    }
    return rejected;
}

/* objcmp: {"function":"beep","address":"11c1:0e07","size":35,"symbols":{"_muted":"DS:5dbe","_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void beep(unsigned hz, unsigned ms) {
    if (!muted) {
        sound(hz);
        delay(ms);
        nosound();
    }
}

/* objcmp: {"function":"set_ega_raster_xor","address":"11c1:0e2a","size":27,"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"symbols":{}} */
/* Borland's outportb intrinsic reproduces this port sequence without assembly. */
void set_ega_raster_xor(int mode) {
    if (mode)
        mode = 0x18;
    outportb(0x3ce, 3);
    outportb(0x3cf, mode);
}

/* objcmp: {"function":"generate_palette","address":"11c1:0e45","size":884,"symbols":{"_palette_buffer":"DS:5d94","_color_count":"DS:5db6","_palette_entries":"DS:5dba","_plot_color":"DS:7488","_palette_random_mode":"DS:5dda","_palette_fixed_mode":"DS:5de2","_palette_alternate_mode":"DS:5de0","_palette_four_ega_colors":"DS:5dd0","_background_fill_word":"DS:5d8a","_palette_base_green":"DS:5d6c","_palette_base_blue":"DS:5d6e","_ega_red_limit":"DS:743c","_ega_green_limit":"DS:7438","_ega_blue_limit":"DS:743a","_palette_cycle_offset":"DS:5dec","_keyboard_led_flags":"11c1:000a","_randomize_palette_state":"11c1:11b9","_poll_graphics_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_movmem":"0000:4d55","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void generate_palette(void) {
    unsigned int i;
    int level[3];
    int phase[3];
    int period[3];
    int direction[3];

    palette_base_green = 10;
    palette_base_blue = 12;
    palette_buffer[0] = (unsigned char)(background_fill_word & 0xff);
    palette_buffer[1] = (unsigned char)palette_base_green;
    palette_buffer[2] = (unsigned char)palette_base_blue;

    if (palette_random_mode) {
        for (i = 3; i < palette_entries; ++i)
            palette_buffer[i] = (unsigned char)random(palette_entries + 1);
    } else {
        period[0] = random(color_count - 4) + 4;
        period[1] = random(color_count - 4) + 4;
        period[2] = random(color_count - 4) + 4;
        if (palette_alternate_mode) {
            if (period[0] >= 8) period[0] >>= 1;
            if (period[1] >= 8) period[1] >>= 1;
            if (period[2] >= 8) period[2] >>= 1;
        }
        phase[0] = phase[1] = phase[2] = 0;
        if (palette_fixed_mode) {
            period[0] = color_count;
            period[1] = color_count / 2;
            period[2] = level[0] = level[1] = level[2] = 0;
        } else {
            level[0] = random(4);
            level[1] = random(4);
            level[2] = random(4);
        }
        direction[0] = direction[1] = direction[2] = 1;
        for (i = 3; i < palette_entries; ++i) {
            if (++phase[0] > period[0]) {
                phase[0] = 0;
                if ((level[0] += direction[0]) >= 3 || level[0] <= 0)
                    direction[0] = -direction[0];
            }
            if (++phase[1] > period[1]) {
                phase[1] = 0;
                if ((level[1] += direction[1]) >= 3 || level[1] <= 0)
                    direction[1] = -direction[1];
            }
            if (++phase[2] > period[2]) {
                phase[2] = 0;
                if ((level[2] += direction[2]) >= 3 || level[2] <= 0)
                    direction[2] = -direction[2];
            }
            palette_buffer[i] = ((level[0] & 1) << 5) | ((level[0] & 2) << 1);
            palette_buffer[i] |= ((level[1] & 1) << 4) | (level[1] & 2);
            palette_buffer[i] |= ((level[2] & 1) << 3) | ((level[2] & 2) >> 1);
        }
    }
    if (palette_four_ega_colors) {
        palette_buffer[1] = (unsigned char)random(palette_entries + 1);
        palette_buffer[2] = (unsigned char)random(palette_entries + 1);
        palette_buffer[3] = (unsigned char)random(palette_entries + 1);
        for (i = 4; i < palette_entries; ++i)
            palette_buffer[i] = palette_buffer[i & 3];
    }
    movmem(palette_buffer + 3, palette_buffer + i, color_count);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = 3;
}

/* objcmp: {"function":"randomize_palette_state","address":"11c1:11b9","size":401,"symbols":{"_palette_buffer":"DS:5d94","_color_count":"DS:5db6","_palette_entries":"DS:5dba","_palette_random_mode":"DS:5dda","_background_fill_word":"DS:5d8a","_palette_base_green":"DS:5d6c","_palette_base_blue":"DS:5d6e","_ega_green_limit":"DS:7438","_ega_blue_limit":"DS:743a","_memset":"0000:4d36","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXLSH@":"0000:1395"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
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
    palette_buffer[4] = (unsigned char)random(ega_green_limit);
    palette_buffer[5] = (unsigned char)(ega_green_limit + random(ega_green_limit));
    palette_buffer[6] = (unsigned char)random(ega_blue_limit);
    palette_buffer[8] = (unsigned char)ega_green_limit;
}

/* objcmp: {"function":"change_background_palette","address":"11c1:134a","size":165,"symbols":{"_video_adapter":"DS:7484","_randomize_palette_state":"11c1:11b9","_upload_reserved_dac_entries":"17f5:030f","_palette_buffer":"DS:5d94","_palette_base_green":"DS:5d6c","_palette_base_blue":"DS:5d6e","_palette_cycle_offset":"DS:5dec","_palette_upload_callback":"DS:5c37","_rand":"0000:1694","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void change_background_palette(void) {
    if (video_adapter == 8) {
        randomize_palette_state(1);
        upload_reserved_dac_entries(3);
    } else {
        palette_buffer[0] = (unsigned char)(random(8) + 8);
        palette_buffer[1] = (unsigned char)(palette_base_green = random(8) + 8);
        palette_buffer[2] = (unsigned char)(palette_base_blue = random(8) + 8);
    }
    palette_upload_callback(palette_buffer + palette_cycle_offset);
}

/* objcmp: {"function":"build_ega_palette","address":"11c1:13ef","size":1089,"symbols":{"_palette_fixed_mode":"DS:5de2","_palette_alternate_mode":"DS:5de0","_palette_random_mode":"DS:5dda","_color_count":"DS:5db6","_palette_entries":"DS:5dba","_palette_buffer":"DS:5d94","_ega_green_limit":"DS:7438","_ega_red_limit":"DS:743c","_video_adapter":"DS:7484","_randomize_palette_state":"11c1:11b9","_keyboard_led_flags":"11c1:000a","_poll_graphics_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_movmem":"0000:4d55","_rand":"0000:1694","_cos":"0000:0f6e","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7","F_LXURSH@":"0000:13d6","F_FTOL@":"0000:0fbe"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void build_ega_palette(int update_cycle) {
    int i;
    unsigned int sample;
    unsigned int amplitude[3];
    unsigned int value[3];
    unsigned int phase[3];
    float frequency[3];

    keyboard_led_flags(0x40, 0);
    if (palette_fixed_mode) {
        for (i = 0; i <= 2; ++i) {
            phase[i] = 0;
            amplitude[i] = ega_red_limit;
        }
    } else {
        for (i = 0; i <= 2; ++i) {
            phase[i] = random(color_count);
            if (random(3))
                amplitude[i] = ega_red_limit;
            else
                amplitude[i] = ega_red_limit / 2 + random(ega_red_limit / 2);
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
        for (sample = 0, i = 9; i < palette_entries * 3; ++sample) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            value[0] = (float)amplitude[0] *
                cos((phase[0] + sample) * frequency[0]) + (amplitude[0] + 1);
            value[1] = (float)amplitude[1] *
                cos((phase[1] + sample) * frequency[1]) + (amplitude[1] + 1);
            value[2] = (float)amplitude[2] *
                cos((phase[2] + sample) * frequency[2]) + (amplitude[2] + 1);
            palette_buffer[(unsigned int)i++] = (unsigned char)(value[0] > ega_green_limit ? value[0] - ega_green_limit : 0);
            palette_buffer[(unsigned int)i++] = (unsigned char)(value[1] > ega_green_limit ? value[1] - ega_green_limit : 0);
            palette_buffer[(unsigned int)i++] = (unsigned char)(value[2] > ega_green_limit ? value[2] - ega_green_limit : 0);
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    } else {
        for (sample = 0, i = 9; i < palette_entries * 3; ++sample) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            value[0] = (float)ega_green_limit * cos(sample * frequency[0]) + ega_green_limit;
            palette_buffer[(unsigned int)i++] = value[0];
            palette_buffer[(unsigned int)i++] = value[0];
            palette_buffer[(unsigned int)i++] = value[0];
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    }
    movmem(palette_buffer + 9, palette_buffer + i, color_count * 3);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = 3;
    keyboard_led_flags(0x40, 1);
}

/* The scanner's starts at 15CB, 1626, 1681, and 1780 lie inside the
   floating-point expressions of build_ega_palette, not separate functions. */

/* objcmp: {"function":"freeze_display","address":"11c1:1830","size":183,"symbols":{"_g_0bfe":"DS:0bfe","_g_0c00":"DS:0c00","_freeze_led_state":"DS:5d82","_input_pending":"DS:5d7e","_exit_requested":"DS:5d80","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_menu_active":"DS:24a2","_keyboard_led_flags":"11c1:000a","_beep":"11c1:0e07","_poll_graphics_keys":"11c1:18e7","_cycle_palette":"11c1:2515"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
int freeze_display(unsigned int operation) {
    if (menu_active)
        operation = 2;

    switch (operation) {
    case 0:
        if (g_0bfe)
            return g_0c00 = !g_0c00;
        g_0bfe = 1;
        freeze_led_state = keyboard_led_flags(16, 0);
        while (g_0bfe) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            if (g_0c00 && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        return;
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

/* objcmp: {"function":"poll_graphics_keys","address":"11c1:18e7","size":1149,"symbols":{"_input_pending":"DS:5d7e","_mouse_abort_pending":"DS:5d7c","_exit_requested":"DS:5d80","_bashful_mode":"DS:5dca","_startup_active":"DS:5d8e","_g_5d90":"DS:5d90","_g_5d92":"DS:5d92","_g_5d93":"DS:5d93","_g_5d9e":"DS:5d9e","_semi_auto_mode":"DS:5da0","_palette_cycle_ticks":"DS:5da6","_palette_step":"DS:5da4","_menu_active":"DS:24a2","_video_adapter":"DS:7484","_show_mode":"DS:5dce","_alt_image_count":"DS:5d9c","_g_0c0a":"DS:0c0a","_palette_buffer":"DS:5d94","_palette_cycle_offset":"DS:5dec","_bioskey":"0000:2553","__bios_keybrd":"0000:2578","_tolower":"0000:1f66","_beep":"11c1:0e07","_freeze_display":"11c1:1830","_help_screen":"158d:05dc","_configuration_screen":"158d:2256","_save_pcx":"19f0:060b","_palette_build_callback":"DS:5c47","_palette_upload_callback":"DS:5c37","_change_background_palette":"11c1:134a"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
int poll_graphics_keys(void) {
    if (input_pending)
        --input_pending;
    if (mouse_abort_pending) {
        mouse_abort_pending = 0;
        if (startup_active)
            return exit_requested;
        beep(1200, 5);
        return exit_requested = 1;
    }
    if (exit_requested || bashful_mode)
        return exit_requested = 1;
    if (!bioskey(1))
        return exit_requested;

    g_5d92 = (unsigned int)_bios_keybrd(0);
    if (((unsigned char *)&g_5d92)[0]) {
        ((unsigned char *)&g_5d92)[0] = tolower(((unsigned char *)&g_5d92)[0] & 0x7f);
        g_5d93 = 0;
        switch (g_5d92) {
        case '+':
            g_5d92 = g_5d90;
            if (palette_cycle_ticks > 2)
                palette_cycle_ticks -= palette_cycle_ticks >> 1;
            else
                --palette_cycle_ticks;
            if (palette_cycle_ticks < 1) {
                palette_cycle_ticks = 1;
                beep(210, 20);
                return exit_requested;
            }
            return exit_requested;
        case '-':
            g_5d92 = g_5d90;
            if (palette_cycle_ticks > 2)
                palette_cycle_ticks += palette_cycle_ticks >> 1;
            else
                ++palette_cycle_ticks;
            if (palette_cycle_ticks > 0x40) {
                palette_cycle_ticks = 0x40;
                beep(190, 20);
                return exit_requested;
            }
            return exit_requested;
        case 9:
            freeze_display(0);
            g_5d92 = g_5d90;
            return exit_requested;
        case ' ':
            freeze_display(1);
            g_5d92 = g_5d90;
            return exit_requested;
        case 'n':
            freeze_display(1);
            g_5d90 = g_5d92;
            return exit_requested;
        default:
            g_5d90 = g_5d92;
            return exit_requested;
        case 10:
        case 13:
            g_5d90 = g_5d92;
            if (menu_active || startup_active)
                return exit_requested;
            /* fall through */
        case 27:
            g_5d90 = 0;
            return exit_requested = 1;
        }
    }
    if (show_mode || g_0c0a) {
        g_5d92 = g_5d90;
        return exit_requested;
    } else if (g_5d93 >= 0x78 && g_5d93 <= 0x81) {
        ((unsigned char *)&g_5d92)[0] = 0x80 | (g_5d93 + 0x88);
        g_5d90 = g_5d92;
    } else if (g_5d93 >= 0x10 && g_5d93 <= 0x19) {
        ((unsigned char *)&g_5d92)[0] = 0x80 | (g_5d93 + 0xfa);
        g_5d90 = g_5d92;
    } else if (g_5d93 >= 0x1e && g_5d93 <= 0x26) {
        ((unsigned char *)&g_5d92)[0] = 0x80 | (g_5d93 + 0xf6);
        g_5d90 = g_5d92;
    } else if (g_5d93 >= 0x2c && g_5d93 <= 0x32) {
        ((unsigned char *)&g_5d92)[0] = 0x80 | (g_5d93 + 0xf1);
        g_5d90 = g_5d92;
        if ((((unsigned char *)&g_5d92)[0] & 0x7f) >= alt_image_count)
            g_5d90 = g_5d92 = 0;
        return exit_requested;
    } else {
        switch (g_5d92) {
        case 0x3b00:
            g_5d90 = g_5d92 = 0;
            help_screen();
            freeze_display(2);
            return exit_requested;
        case 0x3c00:
            g_5d90 = g_5d92 = 0;
            configuration_screen();
            freeze_display(2);
            return exit_requested;
        case 0x3f00:
            if (startup_active || menu_active)
                g_5d92 = g_5d90;
            else {
                save_pcx();
                g_5d92 = g_5d90;
                return exit_requested;
            }
            /* fall through */
        case 0x4200:
            if (semi_auto_mode) {
                g_5d90 = g_5d92;
                g_5d9e = !g_5d9e;
                if (g_5d9e)
                    beep(3000, 2);
                else
                    beep(2200, 2);
            } else {
                g_5d92 = g_5d90;
                return exit_requested;
            }
            return exit_requested;
        case 0x4300:
            if (semi_auto_mode)
                g_5d90 = g_5d92;
            else
                g_5d92 = g_5d90;
            return exit_requested;
        case 0x4400:
            if (semi_auto_mode)
                g_5d90 = g_5d92;
            else
                g_5d92 = g_5d90;
            return exit_requested;
        case 0x4700:
        case 0x4b00:
            g_5d90 = g_5d92 = 0x4700;
            return exit_requested;
        case 0x4900:
            if (!menu_active && video_adapter == 8) {
                g_5d92 = g_5d90;
                if (palette_step < 0x39)
                    palette_step += 3;
                if (palette_step >= 0x39) {
                    palette_step = 0x39;
                    beep(210, 20);
                    return exit_requested;
                }
                return exit_requested;
            }
            /* fall through */
        case 0x4800:
            g_5d90 = g_5d92 = 0x4900;
            return exit_requested;
        case 0x4d00:
        case 0x4f00:
            g_5d90 = g_5d92 = 0x4f00;
            return exit_requested;
        case 0x5100:
            if (!menu_active) {
                g_5d92 = g_5d90;
                if (palette_step > 3)
                    palette_step -= 3;
                if (palette_step <= 3) {
                    palette_step = 3;
                    beep(190, 20);
                    return exit_requested;
                }
                return exit_requested;
            }
            /* fall through */
        case 0x5000:
            g_5d90 = g_5d92 = 0x5100;
            return exit_requested;
        case 0x6700:
            if (menu_active) {
                g_5d92 = g_5d90;
            } else {
                beep(1500, 10);
                change_background_palette();
                beep(1000, 5);
                g_5d92 = g_5d90;
            }
            return exit_requested;
        case 0x5d00:
            if (menu_active) {
                g_5d92 = g_5d90;
            } else {
                g_0c0a = 1;
                beep(1000, 10);
                palette_build_callback(1);
                palette_upload_callback(palette_buffer + palette_cycle_offset);
                beep(1500, 5);
                g_0c0a = 0;
                g_5d92 = g_5d90;
            }
            return exit_requested;
        case 0x7000:
            semi_auto_mode = !semi_auto_mode;
            g_5d90 = g_5d92 = 'n';
            if (semi_auto_mode) {
                freeze_display(2);
                beep(880, 20);
            } else
                beep(440, 20);
            return exit_requested;
        case 0x7100:
            if (semi_auto_mode) {
                g_5d90 = g_5d92;
                return exit_requested;
            }
            /* fall through */
        default:
            g_5d92 = g_5d90;
            break;
        }
    }
    return exit_requested;
}

/* objcmp: {"function":"initialize_math_tables","address":"11c1:1d64","size":113,"symbols":{"_trig_table":"DS:5df2","_cos":"0000:0f6e","_sin":"0000:0e75"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */

void initialize_math_tables(void) {
    float angle;
    register int i;

    angle = 0.0;
    for (i = 0; i < 64; ++i) {
        trig_table[i].cosine = cos(angle);
        trig_table[i].sine = sin(angle);
        angle += 0.098174770421875;
    }
}

/* objcmp: {"function":"initialize_palette_mapping","address":"11c1:1dd5","size":340,"symbols":{"_color_count":"DS:5db6","_sqrt":"0000:0e33","F_FTOL@":"0000:0fbe"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */

void far initialize_palette_mapping(PaletteMapSpec far *mapping) {
    int half_height, half_width;
    int limit_y, limit_x;
    unsigned int y_squared;
    unsigned int row_start;
    unsigned int radius_limit;
    float distance;
    register int y, x;
    register int dy, dx;
    register unsigned char color_index;

    half_height = mapping->height / 2;
    half_width = mapping->width / 2;
    limit_y = half_height + half_height / 4;
    limit_x = half_width + half_width / 4;
    radius_limit = half_height < half_width ? half_height - 1 : half_width - 1;
    radius_limit *= radius_limit;

    for (y = 0; y < mapping->height; ++y) {
        y_squared = abs(half_height - y);
        y_squared *= y_squared;
        row_start = mapping->width * y;
        for (x = 0; x < mapping->width; ++x) {
            dx = abs(half_width - x);
            dx *= dx;
            if (y_squared + dx > radius_limit) {
                mapping->pixels[row_start + x] = 0;
            } else {
                dx = abs(limit_y - y);
                dy = abs(limit_x - x);
                if (dx || dy) {
                    distance = sqrt((double)(unsigned int)(dx * dx + dy * dy));
                } else {
                    distance = 0.0;
                }
                color_index = (int)distance % 253;
                if (color_count < 0x80)
                    color_index >>= 1;
                mapping->pixels[row_start + x] = (unsigned char)(color_index + 3);
            }
        }
    }
}

/* objcmp: {"function":"initialize_program_defaults","address":"11c1:1f29","size":150,"symbols":{"_plot_color":"DS:7488","_draw_delay_count":"DS:5dc6","_draw_symmetry":"DS:5caf","_g_5c4b":"DS:5c4b","_g_5d90":"DS:5d90","_g_5d92":"DS:5d92","_g_5cb1":"DS:5cb1","_exit_requested":"DS:5d80","_initialize_math_tables":"11c1:1d64","_default_config_name":"DS:5ffa","_mode_setup_callback":"DS:5c43","_plot_callback":"DS:5c3f","_g_5c5f":"DS:5c5f","_strcpy":"0000:59cc","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8","_palette_step":"DS:5da4","_palette_alternate_mode":"DS:5de0","_delay":"0000:29a6","_ega_set_mode":"1899:0aa5","_ega_plot_symmetric_640x350":"1899:02e8"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
void initialize_program_defaults(void) {
    plot_color = 3;
    draw_delay_count = 1;
    draw_symmetry = 3;
    g_5c4b = alt_image_records[0].draw;
    g_5d92 = (g_5d90 = 112);
    exit_requested = (g_5cb1 = 1);
    initialize_math_tables();
    default_config_name = "MTDAZZLE.DZL";
    mode_setup_callback = ega_set_mode;
    plot_callback = ega_plot_symmetric_640x350;
    strcpy(g_5c5f, default_config_name);
    g_5cc5 = 3;
    g_5cc3 = 9;
    g_5da8 = 91;
    palette_step = 3;
    palette_alternate_mode = 0;
    delay(1);
}

/* objcmp: {"function":"beep_float","address":"11c1:1fbf","size":48,"symbols":{"_sound":"0000:3d88","_delay":"0000:29a6","_nosound":"0000:3db4","F_FTOL@":"0000:0fbe"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"absolute_symbols":{"FIDRQQ":"0x5c32"}} */
void delay(unsigned); void nosound(void);
void beep_float(float hz, unsigned ms) {
    int f;
    f = hz + 0.5;
    sound(f);
    delay(ms);
    nosound();
}

/* objcmp: {"function":"play_tone_sequence","address":"11c1:1fef","size":370,"symbols":{"_muted":"DS:5dbe","_mouse_abort_pending":"DS:5d7c","_tone_base_frequency":"DS:0c14","_g_0c16":"DS:0c16","_tone_chord":"DS:0c0c","_bioskey":"0000:2553","_beep_float":"11c1:1fbf"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"},"absolute_symbols":{"FIDRQQ":"0x5c32","FIWRQQ":"0xa23d"}} */
void play_tone_sequence(void) {
    float notes[8][12];
    int note;
    register int octave;
    register int melody;

    if (muted)
        return;
    for (octave = 0; octave < 8; ++octave) {
        for (note = 0; note < 12; ++note) {
            notes[octave][note] = tone_base_frequency;
            tone_base_frequency *= 1.0594645;
        }
    }
    if (mouse_abort_pending || bioskey(1)) {
        nosound();
        return;
    }

    for (octave = 7; octave > 1; --octave) {
        if (mouse_abort_pending || bioskey(1)) {
            nosound();
            return;
        }
        for (note = 0; note < 4; ++note)
            beep_float(notes[octave][tone_chord[note]], 80);
    }

    if (mouse_abort_pending || bioskey(1)) {
        nosound();
        return;
    }
    for (melody = 0; melody < 7; ++melody) {
        beep_float(notes[4][0], 80);
        beep_float(notes[4][2], 80);
    }
    if (!mouse_abort_pending && !bioskey(1))
        beep_float(notes[4][0], 600);
    nosound();
}

/* objcmp: {"function":"closing_graphic_and_music","address":"11c1:2161","size":69,"symbols":{"_draw_closing_graphic":"17f5:051c","_mouse_abort_pending":"DS:5d7c","_input_pending":"DS:5d7e","_countdown_ticks":"DS:5d78","_window_mode":"DS:5dbc","_play_tone_sequence":"11c1:1fef","_delay":"0000:29a6","_bioskey":"0000:2553"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void closing_graphic_and_music(void) {
    draw_closing_graphic();
    input_pending = (mouse_abort_pending = 0);
    countdown_ticks = 182;
    if (!window_mode) {
        play_tone_sequence();
        delay(1000);
    }
    while (!bioskey(1) && countdown_ticks > 0 && !mouse_abort_pending)
        ;
}

/* objcmp: {"function":"configure_video_backend","address":"11c1:21a6","size":879,"symbols":{"_freeze_led_state":"DS:5d82","_g_5c33":"DS:5c33","_ega_red_limit":"DS:743c","_ega_blue_limit":"DS:743a","_ega_green_limit":"DS:7438","_video_adapter":"DS:7484","_g_5da2":"DS:5da2","_g_5dd4":"DS:5dd4","_palette_cycle_ticks":"DS:5da6","_palette_entries":"DS:5dba","_color_count":"DS:5db6","_color_index_mask":"DS:5dc0","_screen_half_width":"DS:5db0","_screen_half_height":"DS:5db2","_g_5de4":"DS:5de4","_mode_setup_callback":"DS:5c43","_plot_callback":"DS:5c3f","_read_pixel_callback":"DS:5c3b","_g_5db4":"DS:5db4","_g_5dae":"DS:5dae","_palette_build_callback":"DS:5c47","_g_5dd8":"DS:5dd8","_palette_upload_callback":"DS:5c37","_g_5c49":"DS:5c49","_g_5d88":"DS:5d88","_g_5db8":"DS:5db8","_palette_buffer":"DS:5d94","_g_5d96":"DS:5d96","_g_5de8":"DS:5de8","_exit_requested":"DS:5d80","_g_5c57":"DS:5c57","_g_5c59":"DS:5c59","_g_5c4f":"DS:5c4f","_g_5c51":"DS:5c51","_g_5c53":"DS:5c53","_g_5c55":"DS:5c55","_standard_error_stream":"DS:441c","_g_2028":"DS:2028","_g_2047":"DS:2047","_g_2068":"DS:2068","_set_display_start":"1899:0ebf","_vga_set_mode_320x200":"1899:0b9e","_vga_set_mode_320x400":"1899:0afc","_vga_plot_symmetric_320x200":"1899:0622","_vga_plot_symmetric_320x400":"1899:048b","_vga_read_pixel_320x200":"1899:08f1","_vga_read_pixel_320x400":"1899:088b","_build_ega_palette":"11c1:13ef","_vga_upload_palette_bios_split":"1899:0d27","_vga_upload_palette_8086_split":"1899:0dee","_vga_upload_palette_rep_split":"1899:0d74","_vga_upload_palette_bios":"1899:0d14","_vga_upload_palette_8086":"1899:0dbf","_vga_upload_palette_rep":"1899:0d48","_ega_set_mode":"1899:0aa5","_ega_plot_symmetric_640x350":"1899:02e8","_ega_plot_symmetric_640x700":"1899:012d","_ega_read_pixel_640x350":"1899:07f5","_ega_read_pixel_640x700":"1899:075d","_ega_upload_palette_bios":"1899:0c13","_ega_upload_palette_ports":"1899:0c7c","_generate_palette":"11c1:0e45","_malloc":"0000:2c51","_rand":"0000:1694","_fprintf":"0000:45d4","_print_closing_text_and_exit":"11c1:0d46","_initialize_palette_mapping":"11c1:1dd5","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void configure_video_backend(int allocate) {
    register int divisor;

    if (allocate) freeze_led_state = 0;
    g_5c33 = set_display_start;
    ega_red_limit = 0x2f;
    ega_blue_limit = 0x3f;
    ega_green_limit = 0x1f;
    if (video_adapter == 8) {
        g_5da2 = 1;
        if (allocate) palette_cycle_ticks = g_5dd4 ? g_5da2 << 2 : g_5da2;
        palette_entries = 576;
        color_count = 256;
        color_index_mask = color_count - 1;
        screen_half_width = 160;
        if (g_5de4) {
            screen_half_height = 100;
            mode_setup_callback = (VideoCallback)vga_set_mode_320x200;
            plot_callback = (VideoCallback)vga_plot_symmetric_320x200;
            read_pixel_callback = vga_read_pixel_320x200;
            g_5dae = screen_half_width + 1;
        } else {
            screen_half_height = 200;
            mode_setup_callback = (VideoCallback)vga_set_mode_320x400;
            plot_callback = (VideoCallback)vga_plot_symmetric_320x400;
            read_pixel_callback = vga_read_pixel_320x400;
            if (g_5db4 || g_5dd8 || g_5dd4)
                g_5dae = screen_half_height - 28;
            else
                g_5dae = screen_half_height + 1;
        }
        palette_build_callback = build_ega_palette;
        if (g_5dd8) {
            if (g_5db4)
                palette_upload_callback = vga_upload_palette_bios_split;
            else if (g_5dd4)
                palette_upload_callback = vga_upload_palette_8086_split;
            else
                palette_upload_callback = vga_upload_palette_rep_split;
        } else if (g_5db4) {
            palette_upload_callback = vga_upload_palette_bios;
        } else if (g_5dd4) {
            palette_upload_callback = vga_upload_palette_8086;
        } else {
            palette_upload_callback = vga_upload_palette_rep;
        }
    }
    if (video_adapter == 4) {
        g_5da2 = 2;
        if (allocate) palette_cycle_ticks = g_5da2;
        palette_entries = 64;
        color_count = 16;
        color_index_mask = color_count - 1;
        screen_half_width = 320;
        mode_setup_callback = ega_set_mode;
        if (g_5de4) {
            plot_callback = (VideoCallback)ega_plot_symmetric_640x350;
            read_pixel_callback = ega_read_pixel_640x350;
            screen_half_height = 175;
            g_5dae = screen_half_width + 1;
        } else {
            plot_callback = (VideoCallback)ega_plot_symmetric_640x700;
            read_pixel_callback = ega_read_pixel_640x700;
            screen_half_height = 350;
            g_5dae = screen_half_height + 1;
        }
        if (g_5db4)
            palette_upload_callback = ega_upload_palette_bios;
        else
            palette_upload_callback = ega_upload_palette_ports;
        palette_build_callback = generate_palette;
    }
    g_5d88 = 0xffff / (g_5dae + (g_5dae >> 2)) - 1;
    g_5db8 = palette_entries * 3;
    if (!allocate) return;

    if (video_adapter == 4)
        g_5de8 = color_count + palette_entries;
    else
        g_5de8 = (color_count + palette_entries) * 3;
    palette_buffer = (unsigned char far *)malloc(g_5de8 + 3);
    if (!palette_buffer) {
        exit_requested = 1;
        fprintf(&standard_error_stream, "\r Memory error: EPAL - ABORT\n\a");
        print_closing_text_and_exit(0x3c);
    }
    g_5c57 = malloc(0x7d8);
    if (!g_5c57) {
        exit_requested = 1;
        fprintf(&standard_error_stream, "\r Memory error: WLINES - ABORT\n\a");
        print_closing_text_and_exit(0x46);
    }
    divisor = (int)((long)rand() * 5L / 32768L) + 2;
    g_5c4f = screen_half_height / divisor;
    g_5c51 = screen_half_width / divisor;
    g_5c53 = malloc(g_5c4f * g_5c51);
    if (!g_5c53) {
        exit_requested = 1;
        fprintf(&standard_error_stream, "\r Memory error: BALL - ABORT\n\a");
        print_closing_text_and_exit(0x47);
    }
    initialize_palette_mapping((PaletteMapSpec far *)&g_5c4f);
}

/* objcmp: {"function":"cycle_palette","address":"11c1:2515","size":89,"symbols":{"_g_0bfe":"DS:0bfe","_g_0c00":"DS:0c00","_video_adapter":"DS:7484","_palette_cycle_offset":"DS:5dec","_palette_entries":"DS:5dba","_palette_step":"DS:5da4","_g_5db8":"DS:5db8","_palette_buffer":"DS:5d94","_g_5d96":"DS:5d96","_palette_upload_callback":"DS:5c37","_palette_elapsed_ticks":"DS:5d7a"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void cycle_palette(void) {
    if (!g_0bfe || g_0c00) {
        if (video_adapter == 4) {
            ++palette_cycle_offset;
            if (palette_cycle_offset >= palette_entries) palette_cycle_offset = 3;
        } else {
            palette_cycle_offset = palette_cycle_offset + palette_step;
            if (palette_cycle_offset >= g_5db8) palette_cycle_offset = 9;
        }
        palette_upload_callback(palette_buffer + palette_cycle_offset);
    }
    palette_elapsed_ticks = 0;
}

/* objcmp: {"function":"parse_config_version","address":"11c1:256e","size":104,"symbols":{"_g_version_format":"DS:2087","_g_version_corrupt":"DS:208a","_g_version_warning":"DS:20b3","_strchr":"0000:598f","_sscanf":"0000:5909","_menu_error_message":"158d:2492"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

int parse_config_version(char far *line) {
    char far *mark;
    unsigned version;

    mark = strchr(line, '#');
    ++mark;
    if (sscanf(mark, "%u", &version) == -1) {
        menu_error_message("Corrupted version data in the input file");
        return 1;
    }
    if (version < 10410)
        menu_error_message("WARNING: bad version in the input file");
    return 0;
}

/* objcmp: {"function":"parse_alt_image_size","address":"11c1:25d6","size":244,"symbols":{"_g_format_char":"DS:20da","_g_format_decimal":"DS:20dd","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5d88":"DS:5d88","_alt_image_count":"DS:5d9c","_g_020c":"DS:020c","_g_0202":"DS:0202"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void parse_alt_image_size(char far *line) {
    int size;
    char far *p;
    char key;
    unsigned i;

    if (!(p = strchr(line, '~'))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, "%c", &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    if (sscanf(p, "%d", &size) == -1)
        return;
    if (size > g_5d88) size = g_5d88;
    if (size < 0) size = 0;
    for (i = 0; i < alt_image_count; ++i) {
        if ((char)alt_image_records[i].key == key) {
            alt_image_records[i].size = size;
            break;
        }
    }
}

/* objcmp: {"function":"parse_ctrl_image_size","address":"11c1:26ca","size":244,"symbols":{"_g_format_char":"DS:20da","_g_format_decimal":"DS:20dd","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5d88":"DS:5d88","_ctrl_image_count":"DS:5d9a","_g_0428":"DS:0428","_g_041e":"DS:041e"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void parse_ctrl_image_size(char far *line) {
    int size;
    char far *p;
    char key;
    unsigned i;

    if (!(p = strchr(line, '^'))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, "%c", &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    if (sscanf(p, "%d", &size) == -1)
        return;
    if (size > g_5d88) size = g_5d88;
    if (size < 0) size = 0;
    for (i = 0; i < ctrl_image_count; ++i) {
        if ((char)ctrl_image_records[i].key == key) {
            ctrl_image_records[i].size = size;
            break;
        }
    }
}

/* objcmp: {"function":"parse_fade_enabled","address":"11c1:27be","size":256,"symbols":{"_g_format_char":"DS:20da","_strchr":"0000:598f","_sscanf":"0000:5909","_fade_count":"DS:5d98","_fade_records":"DS:0474","_g_0479":"DS:0479"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void parse_fade_enabled(char far *line) {
    char far *p;
    char key;
    char enabled;
    unsigned i;

    p = line;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, "%c", &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, "%c", &enabled) == -1)
        return;
    if (enabled != 'n' && enabled != 'y') return;
    for (i = 0; i < fade_count; ++i) {
        if (fade_records[i].key == key) {
            fade_records[i].enabled = (enabled == 'y');
            break;
        }
    }
}

/* objcmp: {"function":"parse_auto_configuration","address":"11c1:28be","size":240,"symbols":{"_g_format_char":"DS:20da","_g_format_decimal":"DS:20dd","_strchr":"0000:598f","_sscanf":"0000:5909","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void parse_auto_configuration(char far *line) {
    int value;
    char far *p;
    char key;

    if (!(p = strchr(line, '$'))) return;
    ++p;
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    if (*p == 0) return;
    if (sscanf(p, "%c", &key) == -1)
        return;
    if (!(p = strchr(p, '['))) return;
    ++p;
    if (sscanf(p, "%d", &value) == -1)
        return;
    if (value > 256) value = 256;
    if (value < 0) value = 1;
    switch (key) {
    case 'i':
        g_5cc5 = value;
        break;
    case 'x':
        g_5cc3 = value;
        break;
    case 'f':
        g_5da8 = value * 18;
        break;
    }
}

/* objcmp: {"function":"load_configuration_file","address":"11c1:29ae","size":406,"symbols":{"_g_mode_rt":"DS:20e0","_g_config_close_error":"DS:20e3","_g_5ffe":"DS:5ffe","_fopen":"0000:459f","_fgets":"0000:4236","_clearerr":"0000:102f","_fclose":"0000:407f","_strlen":"0000:5a36","_strchr":"0000:598f","_strlwr":"0000:5a55","_menu_error_message":"158d:2492","_parse_config_version":"11c1:256e","_parse_alt_image_size":"11c1:25d6","_parse_ctrl_image_size":"11c1:26ca","_parse_fade_enabled":"11c1:27be","_parse_auto_configuration":"11c1:28be"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

int load_configuration_file(char far *path) {
    char line[80];
    char far *p;

    if (!(g_5ffe = fopen(path, "rt")))
        return 1;
    for (;;) {
        if (!fgets(line, 80, g_5ffe)) {
            clearerr(g_5ffe);
            if (fclose(g_5ffe) != 0)
                menu_error_message("Error closing input file");
            return 0;
        }
        if (strlen(line) < 1)
            continue;
        if ((p = strchr(line, ';')) != 0)
            *p = 0;
        if (strlen(line) < 1)
            continue;
        p = line;
        while (*p && (*p == ' ' || *p == '\t'))
            ++p;
        if (*p == 0)
            continue;
        if (!strlwr(p))
            continue;
        switch (*p) {
        case '#':
            if (parse_config_version(p)) {
                fclose(g_5ffe);
                return 1;
            }
            break;
        case '~':
            parse_alt_image_size(p);
            break;
        case '^':
            parse_ctrl_image_size(p);
            break;
        case '`':
            break;
        case '$':
            parse_auto_configuration(p);
            break;
        default:
            parse_fade_enabled(p);
            break;
        }
    }
}

/* objcmp: {"function":"save_configuration_file","address":"11c1:2b44","size":482,"symbols":{"_g_config_write_mode":"DS:20fc","_g_config_save_error":"DS:20ff","_g_5ffe":"DS:5ffe","_g_config_header":"DS:211d","_g_alt_write_format":"DS:223d","_g_ctrl_write_format":"DS:2259","_g_fade_write_format":"DS:2275","_g_min_write_format":"DS:228f","_g_max_write_format":"DS:22bd","_g_fade_delay_write_format":"DS:22eb","_alt_image_records":"DS:01fe","_ctrl_image_records":"DS:041a","_fade_records":"DS:0474","_alt_image_count":"DS:5d9c","_ctrl_image_count":"DS:5d9a","_fade_count":"DS:5d98","_g_5cc5":"DS:5cc5","_g_5cc3":"DS:5cc3","_g_5da8":"DS:5da8","_fopen":"0000:459f","_fprintf":"0000:45d4","_fclose":"0000:407f","_menu_error_message":"158d:2492"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

int save_configuration_file(char far *path) {
    unsigned i;
    register char enabled;

    if (!(g_5ffe = fopen(path, "wt"))) {
        menu_error_message("Unable to open file for write");
        return 1;
    }
    fprintf(g_5ffe, "; This file created by MTDAZZLE.  It is a text file of program\n; configuration information that can be edited, allowing customization\n; of the display. Refer to MTDAZZLE.DOC for full details.\n; Be sure not to delete the VERSION flag that starts with '#'!\n#10500;DO NOT MODIFY THIS LINE!\n");
    for (i = 0; i < alt_image_count; ++i) {
        if (fprintf(g_5ffe, "~%c [%03d] ; %s image size\n", (char)alt_image_records[i].key,
                    alt_image_records[i].size,
                    alt_image_records[i].name) == -1) {
            fclose(g_5ffe);
            return 1;
        }
    }
    for (i = 0; i < ctrl_image_count; ++i) {
        if (fprintf(g_5ffe, "^%c [%03d] ; %s image size\n", (char)ctrl_image_records[i].key,
                    ctrl_image_records[i].size,
                    ctrl_image_records[i].name) == -1) {
            fclose(g_5ffe);
            return 1;
        }
    }
    for (i = 0; i < fade_count; ++i) {
        if (fade_records[i].enabled)
            enabled = 'y';
        else
            enabled = 'n';
        if (fprintf(g_5ffe, "%c [%c] ; %s fade enable\n", (char)fade_records[i].key,
                    enabled,
                    fade_records[i].name) == -1) {
            fclose(g_5ffe);
            return 1;
        }
    }
    if (fprintf(g_5ffe, "$I [%03d] ; mInimum image count in Auto Mode\n", g_5cc5) == -1) {
        fclose(g_5ffe);
        return 1;
    }
    if (fprintf(g_5ffe, "$X [%03d] ; maXimum image count in Auto Mode\n", g_5cc3) == -1) {
        fclose(g_5ffe);
        return 1;
    }
    if (fprintf(g_5ffe, "$F [%03d] ; Fade delay time in Auto Mode\n", (int)g_5da8 / 18) == -1) {
        fclose(g_5ffe);
        return 1;
    }
    fclose(g_5ffe);
    return 0;
}

/* objcmp: {"function":"resolve_startup_configuration","address":"11c1:2d26","size":455,"symbols":{"_g_5c5f":"DS:5c5f","_default_config_name":"DS:5ffa","_g_5ffc":"DS:5ffc","_executable_path":"DS:5d17","_window_mode":"DS:5dbc","_bashful_mode":"DS:5dca","_g_startup_load_error":"DS:0c18","_g_illegal_config_name":"DS:2315","_g_loaded_config":"DS:2341","_standard_error_stream":"DS:441c","_load_configuration_file":"11c1:29ae","_findfirst":"0000:42d2","_fnsplit":"0000:439f","_fnmerge":"0000:4374","_searchpath":"0000:1ea4","_tolower":"0000:1f66","_setdisk":"0000:128e","_strcpy":"0000:59cc","_strcat":"0000:5950","_fprintf":"0000:45d4","_delay":"0000:29a6","_textcolor":"0000:25fc","_reject_exec_filename":"11c1:0d85","_cprintf":"0000:277a"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

int resolve_startup_configuration(int requested) {
    char far *found_path;
    int find_result;
    char drive[MAXDRIVE];
    char filename[MAXFILE];
    char extension[MAXEXT];
    struct ffblk file_info;
    char directory[MAXDIR];

    if (requested < 0)
        return 0;

    if ((find_result = findfirst(g_5c5f, &file_info, 0)) && requested) {
        fprintf(&standard_error_stream, config_load_error_text, g_5c5f);
        delay(1250);
        return 1;
    }

    if (find_result) {
        fnsplit(default_config_name, 0, 0, filename, extension);
        fnsplit(executable_path, drive, directory, 0, 0);
        fnmerge(g_5c5f, drive, directory, filename, extension);
        if (findfirst(g_5c5f, &file_info, 0)) {
            if (found_path = searchpath(default_config_name))
                strcpy(g_5c5f, found_path);
            else
                return 0;
        }
    } else {
        fnsplit(g_5c5f, drive, 0, 0, 0);
        /* The original tests the buffer pointer, rather than its first byte. */
        if (drive)
            setdisk(tolower(drive[0]) - 'a');
    }

    if (reject_exec_filename(g_5c5f)) {
        fprintf(&standard_error_stream, "\n Illegal configuration file name ignored.\n");
        strcpy(g_5c5f, default_config_name);
        delay(1250);
        return 1;
    }

    if (load_configuration_file(g_5c5f) && requested > 0) {
        fprintf(&standard_error_stream, config_load_error_text, g_5c5f);
        delay(1250);
        return 1;
    }

    textcolor(9);
    if (!window_mode && !bashful_mode)
        cprintf("\r Loaded %s\n\r", g_5c5f);
    return 0;
}

/* objcmp: {"function":"parse_command_line","address":"11c1:2eed","size":1202,"symbols":{"_getswitchar":"0000:12ad","_tolower":"0000:1f66","_cprintf":"0000:277a","_exit":"0000:11a8","_strchr":"0000:598f","_sscanf":"0000:5909","_strcpy":"0000:59cc","_strcat":"0000:5950","_g_5c5f":"DS:5c5f","_default_config_name":"DS:5ffa","_g_5ffc":"DS:5ffc","_g_5cb7":"DS:5cb7","_insecure_argument_index":"DS:5dc8","_g_5dc2":"DS:5dc2","_g_5dd8":"DS:5dd8","_palette_four_ega_colors":"DS:5dd0","_g_5dd4":"DS:5dd4","_palette_alternate_mode":"DS:5de0","_bashful_mode":"DS:5dca","_g_5db4":"DS:5db4","_draw_delay_count":"DS:5dc6","_palette_fixed_mode":"DS:5de2","_g_5dde":"DS:5dde","_keep_typematic":"DS:5dd2","_palette_random_mode":"DS:5dda","_no_disk_park":"DS:5dcc","_muted":"DS:5dbe","_show_mode":"DS:5dce","_g_5ddc":"DS:5ddc","_g_5d86":"DS:5d86","_g_5de4":"DS:5de4","_window_mode":"DS:5dbc","_force_ega":"DS:7480","_force_vga":"DS:7482","_g_unknown_option":"DS:234f","_g_unsigned_format":"DS:2366","_g_format_parse_error":"DS:236a","_g_delay_limit":"DS:2380","_g_showtime_limit":"DS:23a6","_g_force_xt_message":"DS:23cc","_g_dzl_suffix":"DS:2017","_command_line_help":"11c1:0bed","_check_executable_integrity":"11c1:0949","_print_order_info":"11c1:0ab6","_extract_manual":"1a72:11bb","_cpu_has_post_8086_shifts":"1899:0ff5"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */
/* Walks far argv strings, accepts concatenated switch characters, and handles
   optional numeric values, one input path, and the documented application flags. */

int parse_command_line(int argc, char far * far *argv,
                       int far *option_position) {
    register int argument;
    int result;
    int machine_mode;
    char previous;
    char switch_character;
    register int string_position;

    result = 0;
    *option_position = 0xffff;
    strcpy(g_5c5f, default_config_name);
    switch_character = (char)getswitchar();
    if (argc >= 1) {
        for (argument = 1; argument < argc; ++argument) {
            if (*argv[argument] == '-' || *argv[argument] == switch_character) {
                argv[argument][0] = '-';
                string_position = 1;
                while ((previous = argv[argument][string_position]) != 0) {
                    if (previous == switch_character)
                        previous = '-';
                    switch (tolower(previous)) {
                    default:
                        fprintf(&standard_error_stream, "\r unknown option /%c \n", previous);
                    case '?':
                        command_line_help();
                        break;
                    case '$':
                        insecure_argument_index = 1;
                        check_executable_integrity();
                        print_order_info(1);
                        exit(0);
                        break;
                    case '!':
                        g_5cb7 = 1;
                        break;
                    case '+':
                        g_5dc2 = 1;
                        break;
                    case '-':
                        break;
                    case '2':
                        g_5dd8 = 1;
                        break;
                    case '4':
                        palette_four_ega_colors = 1;
                        break;
                    case 'a':
                        palette_alternate_mode = 1;
                        break;
                    case 'b':
                        bashful_mode = 1;
                        break;
                    case 'c':
                        g_5db4 = 1;
                        break;
                    case 'd':
                        draw_delay_count = 0x100;
                        if (argv[argument][string_position + 1] == '=') {
                            string_position += 2;
                            if (sscanf(argv[argument] + string_position, "%ud", &draw_delay_count) != 1) {
                                fprintf(&standard_error_stream, "\r unable to parse %s\n", argv[argument][string_position - 2]);
                                command_line_help();
                            }
                            if (draw_delay_count < 1)
                                draw_delay_count = 1;
                            if (draw_delay_count > 0x1fff) {
                                fprintf(&standard_error_stream, "\r Delay time limited to 8191 units!\n\a");
                                command_line_help();
                            }
                            while ((previous = argv[argument][string_position + 1]) >= '0' && previous <= '9')
                                ++string_position;
                        }
                        break;
                    case 'e':
                        force_ega = 1;
                        break;
                    case 'f':
                        palette_fixed_mode = 1;
                        break;
                    case 'h':
                        g_5dde = 1;
                        break;
                    case 'i':
                        *option_position = string_position;
                        insecure_argument_index = argument;
                        break;
                    case 'k':
                        keep_typematic = 1;
                        break;
                    case 'm':
                        palette_alternate_mode = 0;
                        palette_random_mode = 1;
                        break;
                    case 'n':
                        no_disk_park = 1;
                        break;
                    case 'q':
                        muted = 1;
                        break;
                    case 's':
                        show_mode = 1;
                        if (argv[argument][string_position + 1] == '=') {
                            string_position += 2;
                            if (sscanf(argv[argument] + string_position, "%ud", &g_5d86) != 1) {
                                fprintf(&standard_error_stream, "\r unable to parse %s\n", argv[argument][string_position - 2]);
                                command_line_help();
                            }
                            if (g_5d86 > 3600) {
                                fprintf(&standard_error_stream, "\r ShowTime limited to 3600 seconds!\n\a");
                                command_line_help();
                            }
                            g_5d86 *= 18;
                            while ((previous = argv[argument][string_position + 1]) >= '0' && previous <= '9')
                                ++string_position;
                        }
                        break;
                    case 't':
                        g_5de4 = 1;
                        break;
                    case 'v':
                        force_vga = 1;
                        break;
                    case 'w':
                        window_mode = 1;
                        break;
                    case 'x':
                        g_5dd4 = 1;
                        break;
                    case 'y':
                        g_5ddc = 1;
                        break;
                    case 'z':
                        extract_manual();
                        exit(3);
                    case '*':
                        result = -1;
                        break;
                    }

                    ++string_position;
                }
            } else {
                strcpy(g_5c5f, argv[argument]);
                if (strchr(g_5c5f, '?'))
                    command_line_help();
                if (!strchr(g_5c5f, '*')) {
                    if (!strchr(g_5c5f, '.'))
                        strcat(g_5c5f, ".DZL");
                    result = 1;
                } else {
                    strcpy(g_5c5f, default_config_name);
                    result = -1;
                }
            }
        }
    }
    if (g_5dd4) {
        return result;
    } else {
        machine_mode = cpu_has_post_8086_shifts();
        if (machine_mode == 0) {
            fprintf(&standard_error_stream, "\r Forcing the /X option for the detected CPU\n");
            g_5dd4 = 1;
        }
    }
    return result;
}

/* objcmp: {"function":"print_startup_information","address":"11c1:339f","size":410,"symbols":{"_cprintf":"0000:277a","_textcolor":"0000:25fc","_strlen":"0000:5a36","_highvideo":"0000:2635","_lowvideo":"0000:263b","_video_adapter":"DS:7484","_window_mode":"DS:5dbc","_bashful_mode":"DS:5dca","_no_disk_park":"DS:5dcc","_show_mode":"DS:5dce","_g_5dc4":"DS:5dc4","_mouse_callback_installed":"DS:5cb9","_mouse_available":"DS:5cb5","_current_image_header":"DS:5c5b","_help_program_name":"DS:009a","_program_revision":"DS:009e","_g_00a0":"DS:00a0","_g_23fa":"DS:23fa","_g_2400":"DS:2400","_g_2405":"DS:2405","_g_0c34":"DS:0c34","_g_0c3b":"DS:0c3b","_g_0cba":"DS:0cba","_order_name_format":"DS:16a1","_g_2415":"DS:2415","_g_241a":"DS:241a","_g_2424":"DS:2424","_g_2429":"DS:2429","_g_2433":"DS:2433","_g_243f":"DS:243f","_g_245e":"DS:245e","_g_246f":"DS:246f"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void print_startup_information(void) {
    int width;

    if (show_mode || window_mode || (bashful_mode && no_disk_park))
        return;
    textcolor(13);
    cprintf("\r %s ", help_program_name);
    width = strlen(help_program_name) + 2;
    textcolor(15);
    cprintf("%s%s", revision_label, program_revision);
    width += strlen(revision_label) + strlen(program_revision);
    width = width + strlen(copyright_text);
    while (width < 0x4f) {
        cprintf(" ");
        ++width;
    }
    cprintf("%s", copyright_text);
    textcolor(7);
    cprintf("\r\n video mode (");
    if (video_adapter == 4)
        cprintf("EGA)");
    if (g_5dc4)
        cprintf("TARGA+64)");
    else if (video_adapter == 8)
        cprintf("VGA)");
    if (mouse_callback_installed) {
        textcolor(6);
        highvideo();
        if (current_image_header[5] & 0x8000)
            cprintf("\r\n ACTIVE");
        else
            cprintf("\r\n INACTIVE");
        lowvideo();
        cprintf(" Screen Saver Driver detected.");
        if (mouse_available)
            cprintf(" Mouse Disabled.");
    }
    textcolor(7);
    cprintf("\r\n Type '%s -?' at the DOS prompt for help.\n\r", help_program_name);
}

/* objcmp: {"function":"semi_auto_loop","address":"11c1:3539","size":906,"symbols":{"_ctrl_image_count":"DS:5d9a","_alt_image_count":"DS:5d9c","_g_5d92":"DS:5d92","_g_5d90":"DS:5d90","_g_5d9e":"DS:5d9e","_semi_auto_mode":"DS:5da0","_input_pending":"DS:5d7e","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_exit_requested":"DS:5d80","_g_5cc1":"DS:5cc1","_g_5cb1":"DS:5cb1","_g_5cb3":"DS:5cb3","_draw_symmetry":"DS:5caf","_g_5cbf":"DS:5cbf","_g_5cbd":"DS:5cbd","_g_5de6":"DS:5de6","_g_5c4b":"DS:5c4b","_g_5c4d":"DS:5c4d","_plot_callback":"DS:5c3f","_rand":"0000:1694","_poll_graphics_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_set_ega_raster_xor":"11c1:0e2a","_beep":"11c1:0e07","_mode_ready_chirp":"11c1:006f","_display_boundary_fade_and_pan":"0ccd:4a82","_alt_image_records":"DS:01fe","_ctrl_image_records":"DS:041a","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void semi_auto_loop(void) {
    register unsigned old_index;

    old_index = 0xffff;
    g_5cc1 = (long)rand() * (alt_image_count + ctrl_image_count + 1) / 32768L;
    if (!g_5cb1) {
        g_5d90 = g_5d92 = 110;
        display_boundary_fade_and_pan();
    }
    g_5d9e = 0;
    while (input_pending) {
        if (poll_graphics_keys())
            return;
    }
    g_5d92 = g_5d90 = 0;
    if (semi_auto_mode)
        mode_ready_chirp();

    while (semi_auto_mode && !exit_requested) {
        while (!input_pending && !g_5d9e) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        if (poll_graphics_keys())
            return;

        if (g_5d92 && (unsigned)g_5d92 <= ctrl_image_count) {
            g_5cc1 = g_5d92 + alt_image_count;
            g_5c4b = ctrl_image_records[((unsigned)g_5d92 - 1)].draw;
            g_5d92 = 0x7100;
        } else if (((unsigned char)g_5d92 & 0x7f) < alt_image_count) {
            if (g_5d92 & 0x80) {
                g_5cc1 = (unsigned char)g_5d92 & 0x7f;
                g_5c4b = alt_image_records[g_5cc1].draw;
                g_5d92 = 0x7100;
            }
        }
        if (!g_5d92 && g_5d9e)
            g_5d92 = 0x4200;

        switch (g_5d92) {
        case 0x4300:
            g_5cc1 = (long)rand() * (alt_image_count + ctrl_image_count + 1) / 32768L;
            if (g_5cc1 < alt_image_count)
                g_5c4b = alt_image_records[g_5cc1].draw;
            else
                g_5c4b = ctrl_image_records[(g_5cc1 - alt_image_count - 1)].draw;
            /* Fall through to the common new-image setup. */

        case 0x7100:
            g_5cbf = (long)rand() * 2L / 32768L;
            if (g_5cc1 < alt_image_count) {
                g_5cb3 = alt_image_records[g_5cc1].continuous &&
                    (int)((long)rand() * 5L / 32768L) == 0 ? 1 : 0;
            } else {
                g_5cb3 = ctrl_image_records[g_5cc1 - alt_image_count - 1].continuous &&
                    (int)((long)rand() * 5L / 32768L) == 0 ? 1 : 0;
            }
            set_ega_raster_xor(g_5cb3);
            g_5c4b(1);
            old_index = g_5cc1;
            g_5cb1 = 0;
            if (g_5d92 == 0x4300)
                beep(1200, 10);
            else
                beep(2000, 3);
            g_5d90 = g_5d92 = 0;
            continue;

        case 0x4200:
        case 0x4400:
            g_5cb1 = 0;
            if (g_5cc1 < alt_image_count) {
                draw_symmetry = alt_image_records[g_5cc1].enabled &&
                         g_5cbf ? 3 : 1;
            } else {
                draw_symmetry = ctrl_image_records[g_5cc1 - alt_image_count - 1].enabled &&
                         g_5cbf ? 3 : 1;
            }
            if (old_index != g_5cc1) {
                if (g_5cc1 < alt_image_count) {
                    g_5c4b = alt_image_records[g_5cc1].draw;
                } else {
                    g_5c4b = ctrl_image_records[(g_5cc1 - alt_image_count - 1)].draw;
                }
                g_5c4b(1);
                old_index = g_5cc1;
            }
            while (!input_pending) {
                g_5c4b(0);
                plot_callback(draw_symmetry);
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
            break;

        case 0x68:
            /* Clear the ignored Hold command; BC30 removes the dead store. */
            g_5d92 = 0;
            break;

        case 0:
            continue;

        default:
            display_boundary_fade_and_pan();
            if (semi_auto_mode)
                mode_ready_chirp();
            break;
        }
        g_5d90 = g_5d92 = 0;
    }
}

/* objcmp: {"function":"auto_loop","address":"11c1:38c3","size":1025,"symbols":{"_g_5cc1":"DS:5cc1","_g_5cbb":"DS:5cbb","_g_5cc3":"DS:5cc3","_g_5cc5":"DS:5cc5","_g_5c4b":"DS:5c4b","_plot_callback":"DS:5c3f","_draw_symmetry":"DS:5caf","_exit_requested":"DS:5d80","_startup_config_failure":"DS:5d84","_semi_auto_mode":"DS:5da0","_g_5d92":"DS:5d92","_input_pending":"DS:5d7e","_palette_elapsed_ticks":"DS:5d7a","_palette_cycle_ticks":"DS:5da6","_ctrl_image_count":"DS:5d9a","_alt_image_count":"DS:5d9c","_g_5dae":"DS:5dae","_g_5cb1":"DS:5cb1","_g_5cb3":"DS:5cb3","_g_5cbf":"DS:5cbf","_g_5cbd":"DS:5cbd","_g_5de6":"DS:5de6","_palette_alternate_mode":"DS:5de0","_palette_random_mode":"DS:5dda","_standard_output_stream":"DS:4408","_g_0f80":"DS:0f80","_rand":"0000:1694","_fprintf":"0000:45d4","_poll_graphics_keys":"11c1:18e7","_cycle_palette":"11c1:2515","_set_ega_raster_xor":"11c1:0e2a","_mode_ready_chirp":"11c1:006f","_display_boundary_fade_and_pan":"0ccd:4a82","_alt_image_records":"DS:01fe","_ctrl_image_records":"DS:041a","F_LXMUL@":"0000:11e3","F_LDIV@":"0000:12e7"},"segments":{"_DATA":"29bf:0bfe","_BSS":"29bf:7438"}} */

void auto_loop(void) {
    register int overlay_count;
    register unsigned attempts;

    g_5cc1 = 0;
    g_5cbb = 1;
    if (g_5cc5 < 1)
        g_5cc5 = 1;
    if (g_5cc3 < g_5cc5)
        g_5cc3 = g_5cc5;
    overlay_count = g_5cc5 + (long)rand() *
        (g_5cc3 + 1 - g_5cc5) / 32768L;
    while (!exit_requested && !semi_auto_mode) {
        if (g_5cbb++ > g_5cbd || g_5d92 == 110) {
            g_5cbb = 0;

            g_5cc1 = (long)rand() * (alt_image_count + ctrl_image_count + 1) / 32768L;
            attempts = 0;
            for (;;) {
                if (g_5cc1 < alt_image_count) {
                    if (alt_image_records[g_5cc1].size > 0)
                        break;
                } else if (ctrl_image_records[g_5cc1 - alt_image_count - 1].size > 0) {
                    break;
                }
                ++g_5cc1;
                g_5cc1 %= alt_image_count + ctrl_image_count + 1;
                if (attempts++ > alt_image_count + ctrl_image_count + 2) {
                    startup_config_failure = exit_requested = 1;
                    return;
                }
            }

            g_5cbf = (long)rand() * 2L / 32768L;
            if (g_5cc1 < alt_image_count) {
                g_5cbd = (long)rand() *
                        (alt_image_records[g_5cc1].size * g_5dae) / 32768L +
                    ((unsigned)(alt_image_records[g_5cc1].size * g_5dae) >> 2);
            } else {
                g_5cbd = (long)rand() *
                        (ctrl_image_records[g_5cc1 - alt_image_count - 1].size * g_5dae) /
                        32768L +
                    ((unsigned)(ctrl_image_records[g_5cc1 - alt_image_count - 1].size *
                                g_5dae) >> 2);
            }
            if (--overlay_count <= 0 || g_5d92 == 110) {
                display_boundary_fade_and_pan();
                overlay_count = g_5cc5 + (long)rand() *
                    (g_5cc3 + 1 - g_5cc5) / 32768L;
                if (palette_alternate_mode)
                    overlay_count += (int)((long)rand() * g_5cc5 / 32768L);
                if (palette_random_mode)
                    overlay_count += (int)((long)rand() * g_5cc3 / 32768L);
                if (g_5de6 == 2 && overlay_count > 8)
                    overlay_count = 9;
            }

            if (g_5cc1 < alt_image_count) {
                g_5cb3 = alt_image_records[g_5cc1].continuous &&
                    (int)((long)rand() * (overlay_count + 1) / 32768L) == 0 ? 1 : 0;
            } else {
                g_5cb3 = ctrl_image_records[g_5cc1 - alt_image_count - 1].continuous &&
                    (int)((long)rand() * (overlay_count + 1) / 32768L) == 0 ? 1 : 0;
            }
            if (input_pending && (poll_graphics_keys() || semi_auto_mode))
                return;
            set_ega_raster_xor(g_5cb3);
            if (g_5cc1 < alt_image_count) {
                g_5c4b = alt_image_records[g_5cc1].draw;
                draw_symmetry = alt_image_records[g_5cc1].enabled &&
                         g_5cbf ? 3 : 1;
            } else {
                g_5c4b = ctrl_image_records[(g_5cc1 - alt_image_count - 1)].draw;
                /* The original also indexes the Alt table in this Ctrl branch. */
                draw_symmetry = alt_image_records[g_5cc1].enabled &&
                         g_5cbf ? 3 : 1;
            }
            g_5c4b(1);
            plot_callback(draw_symmetry);
            g_5cb1 = 0;
            fprintf(&standard_output_stream, "\r");

        }
        if (input_pending && (poll_graphics_keys() || semi_auto_mode))
            return;
        g_5c4b(0);
        plot_callback(draw_symmetry);
        if (palette_elapsed_ticks >= palette_cycle_ticks)
            cycle_palette();
    }
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
struct REGPACK mouse_regs; /* DS:743E */
unsigned int ega_red_limit; /* DS:743C */
unsigned int ega_blue_limit; /* DS:743A */
unsigned int ega_green_limit; /* DS:7438 */
