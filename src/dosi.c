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
#include "dzconst.h"

#define BIOS_TIMER_VECTOR 0x1c
#define BIOS_KEYBOARD_VECTOR 9
#define MOUSE_INTERRUPT 0x33
#define MOUSE_FUNCTION_RESET 0
#define MOUSE_FUNCTION_BUTTON_PRESS 5
#define MOUSE_FUNCTION_BUTTON_RELEASE 6
#define MOUSE_FUNCTION_MOTION_COUNTER 11
#define MOUSE_FUNCTION_SET_CALLBACK 12
#define EGA_GRAPHICS_CONTROLLER_MODE_REGISTER 3
#define EGA_RASTER_XOR_MODE 0x18

#define OPTION_ORDER_INFO '$'
#define OPTION_DRIVER_INTERFACE '!'
#define OPTION_IGNORE_TARGA '+'
#define OPTION_SEPARATOR '-'
#define OPTION_HELP '?'
#define OPTION_SPLIT_PALETTE '2'
#define OPTION_FOUR_EGA_COLORS '4'
#define OPTION_ALTERNATE_PALETTE 'a'
#define OPTION_BASHFUL 'b'
#define OPTION_BIOS_PALETTE 'c'
#define OPTION_DRAW_DELAY 'd'
#define OPTION_FORCE_EGA 'e'
#define OPTION_FIXED_COLORS 'f'
#define OPTION_HIDE_EGA_BORDER 'h'
#define OPTION_INSECURE 'i'
#define OPTION_KEEP_TYPEMATIC 'k'
#define OPTION_RANDOM_PALETTE 'm'
#define OPTION_NO_DISK_PARK 'n'
#define OPTION_QUIET 'q'
#define OPTION_SHOW 's'
#define OPTION_TALL_ASPECT 't'
#define OPTION_FORCE_VGA 'v'
#define OPTION_WINDOW 'w'
#define OPTION_8086_PALETTE_IO 'x'
#define OPTION_Y 'y'
#define OPTION_EXTRACT_MANUAL 'z'
#define OPTION_NO_AUTOLOAD '*'

/* Initialized DOSI state. Text and numeric literals follow in source order. */
unsigned display_freeze_active = 0, display_freeze_cycle_requested = 0;
unsigned far *bios_keyboard_flags = (unsigned far *)0x00000417UL;
unsigned long crc_polynomial = 0x00102100UL;
unsigned palette_callback_active = 0;
unsigned tone_chord[4] = {0, 4, 7, 10};
float tone_base_frequency = 16.35f;
char config_load_error_text[] = "\n Unable to load file : %s\n";
char revision_label[] = " rev. ";
char copyright_text[] = "Copyright(c)1989-92 MicroTronics";

/* Reconstructed 11C1 startup, integrity, and input routines. */

/* Updates selected BIOS keyboard LED flags unless Window or Bashful mode suppresses it; returns
 * the old flags.
 * Original 11C1:000A, 101 bytes. */
unsigned int keyboard_led_flags(unsigned int flag_mask, unsigned int update_mode) {
    unsigned int old_flags;
    old_flags = *bios_keyboard_flags;
    if (!window_mode && !bashful_mode) {
        switch (update_mode) {
        case 0:
            *bios_keyboard_flags |= flag_mask;
            bioskey(1);
            break;
        case 1:
            *bios_keyboard_flags &= ~flag_mask;
            bioskey(1);
            break;
        default:
            *bios_keyboard_flags |= flag_mask;
            *bios_keyboard_flags &= (unsigned int)(flag_mask | 0xff8f);
            break;
        }
    }
    return old_flags;
}

/* Plays a rising three-note readiness chirp unless sound is muted.
 * Original 11C1:006F, 73 bytes. */
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

/* Supplies the default Ctrl-Break callback result.
 * Original 11C1:00B8, 4 bytes. */
int default_signal_result(void) {
    return 1;
}

/* Resumes the active hard-error operation and returns its callback result.
 * Original 11C1:00BC, 13 bytes. */
int interrupt_signal_result(void) {
    hardresume(0);
    return 2;
}

/* Installs Ctrl-Break, timer, keyboard, optional mouse, and hard-error callbacks.
 * Original 11C1:00C9, 236 bytes. */
void install_runtime_callbacks(void) {
    volatile int mouse_probe_status;
    unsigned int mouse_callback_mask;
    ctrlbrk((int (far *)(void))default_signal_result);
    if (!skip_ctrlbreak_control) {
        original_ctrlbreak_setting = getcbrk();
        setcbrk(0);
    }
    if (!timer_hook_installed) {
        saved_timer_vector = getvect(BIOS_TIMER_VECTOR);
        setvect(BIOS_TIMER_VECTOR, graphics_timer_tick_isr);
        timer_hook_installed = 1;
    }
    if (!keyboard_hook_installed) {
        saved_keyboard_vector = getvect(BIOS_KEYBOARD_VECTOR);
        setvect(BIOS_KEYBOARD_VECTOR, keyboard_pending_isr);
        keyboard_hook_installed = 1;
    }
    if (!mouse_available || (current_image_header[6] & 0x0800)) {
        if ((mouse_probe_status = probe_mouse(1)) != 0) {
            mouse_callback_mask = 0x2a;
            if (bashful_mode && mouse_probe_status != 2)
                mouse_callback_mask |= 1;
            mouse_position(mouse_callback_mask, FP_OFF(mouse_pending_callback), FP_SEG(mouse_pending_callback));
        }
    }
    harderr(interrupt_signal_result);
}

/* Resets text video attributes and restores the saved text attribute.
 * Original 11C1:01B5, 18 bytes. */
void restore_text_attributes(void) {
    normvideo();
    textattr(original_text_attribute);
}

/* Restores interrupt, Ctrl-Break, mouse, and typematic state, then closes open files.
 * Original 11C1:01C7, 173 bytes. */
void restore_runtime_callbacks(void) {
    if (timer_hook_installed) {
        setvect(BIOS_TIMER_VECTOR, saved_timer_vector);
        timer_hook_installed = 0;
    }
    if (keyboard_hook_installed) {
        setvect(BIOS_KEYBOARD_VECTOR, saved_keyboard_vector);
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
        int86(BIOS_KEYBOARD_INTERRUPT, &keyboard_regs, &keyboard_regs);
    }
    fcloseall();
    if (mouse_callback_installed)
        current_image_header[5] &= 0xf7ff;
}

/* Ignores two math exception types; otherwise restores runtime/video state, reports the error,
 * and returns zero.
 * Original 11C1:0274, 100 bytes. */
int matherr(struct exception *exception_frame) {
    if (((unsigned int far *)exception_frame)[0] == 5 ||
        ((unsigned int far *)exception_frame)[0] == 4)
        return 1;
    restore_runtime_callbacks();
    restore_video_state(1);
    print_order_info(0);
    printf("\nmatherr -- program aborted. Contact us for assistance.\a\n");
    fprintf(&standard_error_stream, "Program revision level : %s\n\n", program_revision);
    printf("REASON ");
    return 0;
}

/* Prints the interrupted registers, module segment map, and revision details, then exits.
 * Original 11C1:02D8, 362 bytes. */
/* Table pairs, in order: 11C1:034C->066A:000B DZLE, 0364->0B84:0000 DZL2,
   037C->0CCD:000A DZFD, 0394->1899:1571 DVDT, 03C1->1A72:0007 DHUF,
   03D9->17F5:0008 DLOGO, 03F1->19F0:0004 DAZPCX. DOSI uses CS captured at
   02DE; 158D's menu/UI segment has no table entry or identified module label. */
void signal_diagnostic(int signal_number, int signal_type, unsigned int far *interrupt_registers) {
    unsigned int dosi_segment;
    dosi_segment = _CS;
    restore_runtime_callbacks();
    restore_video_state(1);
    fprintf(&standard_error_stream, "sigerr -- program aborted. Contact us for assistance\n");
    fprintf(&standard_error_stream, "Signal %d, Type %d,  Please print this error message!\a\n\n", signal_number, signal_type);
    fprintf(&standard_error_stream, "Error at %04X:%04X\t FLAGS = %04X\n\n",
            interrupt_registers[0x14 / 2], interrupt_registers[0x12 / 2], interrupt_registers[0x16 / 2]);
    fprintf(&standard_error_stream, "SEGMENT MAP : \n");
    fprintf(&standard_error_stream, "DZLE_Seg  = %04X\n", dzle_seg());
    fprintf(&standard_error_stream, "DZL2_Seg  = %04X\n", dzl2_seg());
    fprintf(&standard_error_stream, "DZFD_Seg  = %04X\n", dzfd_seg());
    fprintf(&standard_error_stream, "DVDT_Seg  = %04X\n", dvdt_seg());
    fprintf(&standard_error_stream, "DOSI_Seg  = %04X\n", dosi_segment);
    fprintf(&standard_error_stream, "DHUF_Seg  = %04X\n", dhuf_seg());
    fprintf(&standard_error_stream, "DLOGO_Seg = %04X\n", dlogo_seg());
    fprintf(&standard_error_stream, "DAZPCX_Seg = %04X\n", dazpcx_seg());
    fprintf(&standard_error_stream, "\nProgram revision level :%s, %s\n\n",
            help_program_name, program_revision);
    print_order_info(0);
    exit(9);
}

/* Resets the mouse and optionally waits for button or motion activity; returns status masked by
 * the request.
 * Original 11C1:0442, 191 bytes. */
int probe_mouse(unsigned int requested_status_mask) {
    unsigned int mouse_status;
    unsigned int poll_attempts;
    unsigned int button_activity;
    mouse_regs.r_ax = MOUSE_FUNCTION_RESET;
    intr(MOUSE_INTERRUPT, &mouse_regs);
    mouse_status = poll_attempts = 0;
    if (mouse_regs.r_ax)
        mouse_status = 1;
    if (mouse_status && requested_status_mask) {
        do {
            delay(10);
            mouse_regs.r_ax = MOUSE_FUNCTION_BUTTON_PRESS;
            intr(MOUSE_INTERRUPT, &mouse_regs);
            button_activity = mouse_regs.r_bx;
            mouse_regs.r_ax = MOUSE_FUNCTION_BUTTON_RELEASE;
            intr(MOUSE_INTERRUPT, &mouse_regs);
            button_activity |= mouse_regs.r_bx;
            mouse_regs.r_ax = MOUSE_FUNCTION_MOTION_COUNTER;
            intr(MOUSE_INTERRUPT, &mouse_regs);
        } while (++poll_attempts <= 50 &&
                 (button_activity | mouse_regs.r_cx | mouse_regs.r_dx) != 0);
        if (poll_attempts >= 50)
            mouse_status = 2;
    }
    return mouse_status & requested_status_mask;
}

/* Registers the mouse callback and its button mask through INT 33h.
 * Original 11C1:0501, 46 bytes. */
void mouse_position(int callback_mask, int callback_offset, int callback_segment) {
    mouse_regs.r_cx = callback_mask;
    mouse_regs.r_es = callback_segment;
    mouse_regs.r_dx = callback_offset;
    mouse_regs.r_ax = MOUSE_FUNCTION_SET_CALLBACK;
    intr(MOUSE_INTERRUPT, &mouse_regs);
}

/* Restores video, prints the self-check reason and optional CRC details, then exits with status
 * 100.
 * Original 11C1:052F, 148 bytes. */
void fatal_selfcheck_error(int include_crc_details, const char far *reason) {
    exit_requested = 1;
    restore_video_state(1);
    if (include_crc_details)
        fprintf(&standard_error_stream, "\r Program aborting in antiviral routines! cv=%08lX, r1=%05u, r2=%05u\nThis feature is for YOUR protection, write us for a good copy.\n", expected_crc, serial_r1,
                serial_r2, computed_crc);
    else
        fprintf(&standard_error_stream, "\r Program aborting! cv=%08lX, r1=%05u, r2=%05u\n", expected_crc, serial_r1,
                serial_r2);
    fprintf(&standard_error_stream, "\nREASON: %s \n\n\a", reason);
    print_order_info(0);
    exit(100);
}

/* Reads the first three MZ header words and computes the executable's resource base.
 * Original 11C1:05C3, 104 bytes. */
void read_mz_resource_base(FILE far *input) {
    int mz_header_words[3];
    if (fread(mz_header_words, 2, 3, input) != 3)
        fatal_selfcheck_error(1, "can't read EXE header");
    resource_base = mz_header_words[2] - 1;
    resource_base *= 512;
    resource_base += mz_header_words[1] + 1;
}

/* Tries progressively smaller allocations down to the requested minimum and returns the
 * successful size.
 * Original 11C1:062B, 65 bytes. */
void *allocate(unsigned *amount, unsigned minimum) {
    unsigned attempt_size;
    void *allocation;
    attempt_size = *amount;
    while (attempt_size >= minimum && (allocation = malloc(attempt_size)) == 0)
        attempt_size /= 2;
    *amount = attempt_size;
    return allocation;
}

/* Builds a CRC table and checks the requested executable bytes in allocated chunks.
 * Original 11C1:066C, 501 bytes. */
unsigned long genCRC(FILE far *input, long length) {
    unsigned int index;
    unsigned int chunk_size;
    unsigned char far *buffer;
    unsigned int buffer_capacity;
    unsigned long far *crc_table_entry;
    unsigned long crc;
    unsigned char far *buffer_cursor;
    long bytes_processed;
    long bytes_remaining;
    unsigned long table[256];

    buffer_capacity = 0x2000;
    buffer = (unsigned char far *)allocate(&buffer_capacity, 0x200);
    if (buffer) {
        index = 0;
        crc_table_entry = table;
        table[0] = 0;
        for (; index < 256; index += 2, ++crc_table_entry) {
            if (((unsigned char far *)crc_table_entry)[3] & 0x80)
                table[index] = (table[index + 1] = *crc_table_entry << 1) ^ crc_polynomial;
            else
                table[index + 1] = (table[index] = *crc_table_entry << 1) ^ crc_polynomial;
        }

        crc = 0;
        if (fseek(input, 0L, 0) != 0) {
            free(buffer);
            fatal_selfcheck_error(1, "failure in genCRC fseek!");
        }
        bytes_processed = 0;
        while (bytes_processed < length) {
            bytes_remaining = length - bytes_processed - 1;
            chunk_size = buffer_capacity < bytes_remaining ? buffer_capacity : (unsigned int)bytes_remaining;
            if ((index = fread(buffer, 1, chunk_size, input)) == 0)
                break;
            buffer_cursor = buffer;
            while (index--) {
                crc = (crc << 8) ^ table[((unsigned char *)&crc)[3] ^ *buffer_cursor];
                ++buffer_cursor;
            }
            bytes_processed += chunk_size;
        }
        free(buffer);
    } else
        fatal_selfcheck_error(1, "memory unavailable in genCRC ");
    return crc;
}

/* Opens the executable, optionally calculates its CRC, reads metadata at the MZ end, and reports
 * read failure.
 * Original 11C1:0861, 232 bytes. */
int deRead(void far *metadata, unsigned int metadata_size) {
    int result;
    result = 0;
    if (!(selfcheck_stream = fopen(executable_path, "rb")))
        fatal_selfcheck_error(1, "deRead can't open file");
    read_mz_resource_base(selfcheck_stream);
    if (!insecure_argument_index)
        computed_crc = genCRC(selfcheck_stream, resource_base);
    if (fseek(selfcheck_stream, resource_base, 0) != 0)
        fatal_selfcheck_error(1, "deRead file seek error");
    if (fread(metadata, metadata_size, 1, selfcheck_stream) != 1)
        result = !insecure_argument_index;
    if (fclose(selfcheck_stream) != 0)
        fatal_selfcheck_error(1, "deRead can't close file");
    return result;
}

/* Resolves and validates the executable path and self-check metadata, skipping CRC comparison
 * for /I.
 * Original 11C1:0949, 365 bytes. */
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

/* Prints MicroTronics contact information and, when requested, the registration/order form.
 * Original 11C1:0AB6, 311 bytes. */
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

/* Displays command-line help and waits between its main and additional option lists.
 * Original 11C1:0BED, 345 bytes. */
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

/* Prints closing and registration text unless Bashful mode is set, then exits with the supplied
 * status.
 * Original 11C1:0D46, 63 bytes. */
void print_closing_text_and_exit(int status) {
    if (!bashful_mode) {
        fprintf(&standard_output_stream, "\nThis program may be distributed as is AT NO CHARGE by the distributor\n(excluding reasonable duplication cost). All commercial rights retained\nby the author! This program must not be altered in any way, and is offered\nas is without warranty. Refer to User's Manual for usage limitations and\nroyalty requirements.\n\nEnter \"DAZZLE -Z\" at the DOS prompt to generate the User's Manual.\nEnter \"DAZZLE -$ > PRN:\" at the DOS prompt to print an order form.\n\n%s\nWhen you write be sure to mention your floppy disk format and this program's\nversion of %s \n\n", registration_notice_pointer, program_revision);
        print_order_info(0);
    }
    exit(status);
}

/* Uppercases a proposed file name, rejects EXE/BAT/COM extensions, and substitutes the default
 * config name.
 * Original 11C1:0D85, 130 bytes. */
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

/* Sounds a tone for the requested duration unless sound is muted.
 * Original 11C1:0E07, 35 bytes. */
void beep(unsigned hz, unsigned ms) {
    if (!muted) {
        sound(hz);
        delay(ms);
        nosound();
    }
}

/* Selects the EGA set/reset write mode used for XOR drawing.
 * Original 11C1:0E2A, 27 bytes. */
/* Borland's outportb intrinsic reproduces this port sequence without assembly. */
void set_ega_raster_xor(int raster_mode) {
    if (raster_mode)
        raster_mode = EGA_RASTER_XOR_MODE;
    outportb(VGA_GC_INDEX_PORT, EGA_GRAPHICS_CONTROLLER_MODE_REGISTER);
    outportb(VGA_GC_DATA_PORT, raster_mode);
}

/* Fills the EGA/VGA palette buffer using random, fixed, or cycling color rules, then advances
 * the plot color.
 * Original 11C1:0E45, 884 bytes. */
void generate_palette(void) {
    unsigned int palette_index;
    int channel_level[3];
    int channel_phase[3];
    int channel_period[3];
    int channel_direction[3];

    palette_base_green = 10;
    palette_base_blue = 12;
    palette_buffer[0] = (unsigned char)(background_fill_word & 0xff);
    palette_buffer[1] = (unsigned char)palette_base_green;
    palette_buffer[2] = (unsigned char)palette_base_blue;

    if (palette_random_mode) {
        for (palette_index = 3; palette_index < palette_entries; ++palette_index)
            palette_buffer[palette_index] = (unsigned char)random(palette_entries + 1);
    } else {
        channel_period[0] = random(color_count - 4) + 4;
        channel_period[1] = random(color_count - 4) + 4;
        channel_period[2] = random(color_count - 4) + 4;
        if (palette_alternate_mode) {
            if (channel_period[0] >= 8) channel_period[0] >>= 1;
            if (channel_period[1] >= 8) channel_period[1] >>= 1;
            if (channel_period[2] >= 8) channel_period[2] >>= 1;
        }
        channel_phase[0] = channel_phase[1] = channel_phase[2] = 0;
        if (palette_fixed_mode) {
            channel_period[0] = color_count;
            channel_period[1] = color_count / 2;
            channel_period[2] = channel_level[0] = channel_level[1] = channel_level[2] = 0;
        } else {
            channel_level[0] = random(4);
            channel_level[1] = random(4);
            channel_level[2] = random(4);
        }
        channel_direction[0] = channel_direction[1] = channel_direction[2] = 1;
        for (palette_index = 3; palette_index < palette_entries; ++palette_index) {
            if (++channel_phase[0] > channel_period[0]) {
                channel_phase[0] = 0;
                if ((channel_level[0] += channel_direction[0]) >= 3 || channel_level[0] <= 0)
                    channel_direction[0] = -channel_direction[0];
            }
            if (++channel_phase[1] > channel_period[1]) {
                channel_phase[1] = 0;
                if ((channel_level[1] += channel_direction[1]) >= 3 || channel_level[1] <= 0)
                    channel_direction[1] = -channel_direction[1];
            }
            if (++channel_phase[2] > channel_period[2]) {
                channel_phase[2] = 0;
                if ((channel_level[2] += channel_direction[2]) >= 3 || channel_level[2] <= 0)
                    channel_direction[2] = -channel_direction[2];
            }
            palette_buffer[palette_index] = ((channel_level[0] & 1) << 5) | ((channel_level[0] & 2) << 1);
            palette_buffer[palette_index] |= ((channel_level[1] & 1) << 4) | (channel_level[1] & 2);
            palette_buffer[palette_index] |= ((channel_level[2] & 1) << 3) | ((channel_level[2] & 2) >> 1);
        }
    }
    if (palette_four_ega_colors) {
        palette_buffer[1] = (unsigned char)random(palette_entries + 1);
        palette_buffer[2] = (unsigned char)random(palette_entries + 1);
        palette_buffer[3] = (unsigned char)random(palette_entries + 1);
        for (palette_index = 4; palette_index < palette_entries; ++palette_index)
            palette_buffer[palette_index] = palette_buffer[palette_index & 3];
    }
    movmem(palette_buffer + 3, palette_buffer + palette_index, color_count);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
}

/* Seeds the reserved EGA palette bytes and optionally replaces the background colors.
 * Original 11C1:11B9, 401 bytes. */
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

/* Chooses a new background palette and uploads it through the active adapter callback.
 * Original 11C1:134A, 165 bytes. */
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

/* Builds a changing EGA palette from channel phases and amplitudes while servicing input and
 * palette timing.
 * Original 11C1:13EF, 1089 bytes. */
void build_ega_palette(int update_cycle) {
    int palette_index;
    unsigned int sample_index;
    unsigned int channel_amplitude[3];
    unsigned int channel_value[3];
    unsigned int channel_phase[3];
    float channel_frequency[3];

    keyboard_led_flags(0x40, 0);
    if (palette_fixed_mode) {
        for (palette_index = 0; palette_index <= 2; ++palette_index) {
            channel_phase[palette_index] = 0;
            channel_amplitude[palette_index] = ega_red_limit;
        }
    } else {
        for (palette_index = 0; palette_index <= 2; ++palette_index) {
            channel_phase[palette_index] = random(color_count);
            if (random(3))
                channel_amplitude[palette_index] = ega_red_limit;
            else
                channel_amplitude[palette_index] = ega_red_limit / 2 + random(ega_red_limit / 2);
        }
    }
    if (palette_alternate_mode) {
        channel_frequency[0] = 0.075f; channel_frequency[1] = 0.087f; channel_frequency[2] = 0.100f;
    } else {
        channel_frequency[0] = 0.025f; channel_frequency[1] = 0.033f; channel_frequency[2] = 0.040f;
    }
    if (palette_random_mode) {
        channel_frequency[0] = 0.150f; channel_frequency[1] = 0.175f; channel_frequency[2] = 0.200f;
    }
    randomize_palette_state(0);
    if (random(50)) {
        for (sample_index = 0, palette_index = 9; palette_index < palette_entries * 3; ++sample_index) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            channel_value[0] = (float)channel_amplitude[0] *
                cos((channel_phase[0] + sample_index) * channel_frequency[0]) + (channel_amplitude[0] + 1);
            channel_value[1] = (float)channel_amplitude[1] *
                cos((channel_phase[1] + sample_index) * channel_frequency[1]) + (channel_amplitude[1] + 1);
            channel_value[2] = (float)channel_amplitude[2] *
                cos((channel_phase[2] + sample_index) * channel_frequency[2]) + (channel_amplitude[2] + 1);
            palette_buffer[(unsigned int)palette_index++] = (unsigned char)(channel_value[0] > ega_green_limit ? channel_value[0] - ega_green_limit : 0);
            palette_buffer[(unsigned int)palette_index++] = (unsigned char)(channel_value[1] > ega_green_limit ? channel_value[1] - ega_green_limit : 0);
            palette_buffer[(unsigned int)palette_index++] = (unsigned char)(channel_value[2] > ega_green_limit ? channel_value[2] - ega_green_limit : 0);
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    } else {
        for (sample_index = 0, palette_index = 9; palette_index < palette_entries * 3; ++sample_index) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            channel_value[0] = (float)ega_green_limit * cos(sample_index * channel_frequency[0]) + ega_green_limit;
            palette_buffer[(unsigned int)palette_index++] = channel_value[0];
            palette_buffer[(unsigned int)palette_index++] = channel_value[0];
            palette_buffer[(unsigned int)palette_index++] = channel_value[0];
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    }
    movmem(palette_buffer + 9, palette_buffer + palette_index, color_count * 3);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    keyboard_led_flags(0x40, 1);
}

/* The scanner's starts at 15CB, 1626, 1681, and 1780 lie inside the
   floating-point expressions of build_ega_palette, not separate functions. */

/* Freezes or resumes display work and palette cycling, using menu state and keyboard LED state.
 * Original 11C1:1830, 183 bytes. */
int freeze_display(unsigned int operation) {
    if (menu_active)
        operation = 2;

    switch (operation) {
    case 0:
        if (display_freeze_active)
            return display_freeze_cycle_requested = !display_freeze_cycle_requested;
        display_freeze_active = 1;
        freeze_led_state = keyboard_led_flags(16, 0);
        while (display_freeze_active) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            if (display_freeze_cycle_requested && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        return;
    case 1:
        if (display_freeze_active && !exit_requested)
            beep(440, 10);
        /* fall through */
    default:
        if (display_freeze_active)
            keyboard_led_flags(freeze_led_state, 2);
        return display_freeze_cycle_requested = display_freeze_active = 0;
    }
}

/* Dispatches pending graphics keyboard and mouse actions, updating menu, palette, image, mode,
 * and exit state.
 * Original 11C1:18E7, 1149 bytes. */
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

    current_graphics_key = (unsigned int)_bios_keybrd(0);
    if (((unsigned char *)&current_graphics_key)[0]) {
        ((unsigned char *)&current_graphics_key)[0] = tolower(((unsigned char *)&current_graphics_key)[0] & 0x7f);
        bios_key_scan_code = 0;
        switch (current_graphics_key) {
        case '+':
            current_graphics_key = previous_graphics_key;
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
            current_graphics_key = previous_graphics_key;
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
            current_graphics_key = previous_graphics_key;
            return exit_requested;
        case ' ':
            freeze_display(1);
            current_graphics_key = previous_graphics_key;
            return exit_requested;
        case 'n':
            freeze_display(1);
            previous_graphics_key = current_graphics_key;
            return exit_requested;
        default:
            previous_graphics_key = current_graphics_key;
            return exit_requested;
        case 10:
        case 13:
            previous_graphics_key = current_graphics_key;
            if (menu_active || startup_active)
                return exit_requested;
            /* fall through */
        case 27:
            previous_graphics_key = 0;
            return exit_requested = 1;
        }
    }
    /* Normalize scan-code ranges to the compact image-selection key byte. */
    /* When ASCII is unavailable, map scan-code ranges to compact image-selection bytes. */
    if (show_mode || palette_callback_active) {
        current_graphics_key = previous_graphics_key;
        return exit_requested;
    } else if (bios_key_scan_code >= 0x78 && bios_key_scan_code <= 0x81) {
        ((unsigned char *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0x88);
        previous_graphics_key = current_graphics_key;
    } else if (bios_key_scan_code >= 0x10 && bios_key_scan_code <= 0x19) {
        ((unsigned char *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0xfa);
        previous_graphics_key = current_graphics_key;
    } else if (bios_key_scan_code >= 0x1e && bios_key_scan_code <= 0x26) {
        ((unsigned char *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0xf6);
        previous_graphics_key = current_graphics_key;
    } else if (bios_key_scan_code >= 0x2c && bios_key_scan_code <= 0x32) {
        ((unsigned char *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0xf1);
        previous_graphics_key = current_graphics_key;
        if ((((unsigned char *)&current_graphics_key)[0] & 0x7f) >= alt_image_count)
            previous_graphics_key = current_graphics_key = 0;
        return exit_requested;
    } else {
        switch (current_graphics_key) {
        case BIOS_KEY_F1:
            previous_graphics_key = current_graphics_key = 0;
            help_screen();
            freeze_display(2);
            return exit_requested;
        case BIOS_KEY_F2:
            previous_graphics_key = current_graphics_key = 0;
            configuration_screen();
            freeze_display(2);
            return exit_requested;
        case BIOS_KEY_F5:
            if (startup_active || menu_active)
                current_graphics_key = previous_graphics_key;
            else {
                save_pcx();
                current_graphics_key = previous_graphics_key;
                return exit_requested;
            }
            /* fall through */
        case BIOS_KEY_F8:
            if (semi_auto_mode) {
                previous_graphics_key = current_graphics_key;
                semi_auto_continuous = !semi_auto_continuous;
                if (semi_auto_continuous)
                    beep(3000, 2);
                else
                    beep(2200, 2);
            } else {
                current_graphics_key = previous_graphics_key;
                return exit_requested;
            }
            return exit_requested;
        case BIOS_KEY_F9:
            if (semi_auto_mode)
                previous_graphics_key = current_graphics_key;
            else
                current_graphics_key = previous_graphics_key;
            return exit_requested;
        case BIOS_KEY_F10:
            if (semi_auto_mode)
                previous_graphics_key = current_graphics_key;
            else
                current_graphics_key = previous_graphics_key;
            return exit_requested;
        case BIOS_KEY_HOME:
        case BIOS_KEY_LEFT:
            previous_graphics_key = current_graphics_key = BIOS_KEY_HOME;
            return exit_requested;
        case BIOS_KEY_PAGE_UP:
            if (!menu_active && video_adapter == 8) {
                current_graphics_key = previous_graphics_key;
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
        case BIOS_KEY_UP:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_UP;
            return exit_requested;
        case BIOS_KEY_RIGHT:
        case BIOS_KEY_END:
            previous_graphics_key = current_graphics_key = BIOS_KEY_END;
            return exit_requested;
        case BIOS_KEY_PAGE_DOWN:
            if (!menu_active) {
                current_graphics_key = previous_graphics_key;
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
        case BIOS_KEY_DOWN:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_DOWN;
            return exit_requested;
        case BIOS_KEY_CTRL_F10:
            if (menu_active) {
                current_graphics_key = previous_graphics_key;
            } else {
                beep(1500, 10);
                change_background_palette();
                beep(1000, 5);
                current_graphics_key = previous_graphics_key;
            }
            return exit_requested;
        case BIOS_KEY_SHIFT_F10:
            if (menu_active) {
                current_graphics_key = previous_graphics_key;
            } else {
                palette_callback_active = 1;
                beep(1000, 10);
                palette_build_callback(1);
                palette_upload_callback(palette_buffer + palette_cycle_offset);
                beep(1500, 5);
                palette_callback_active = 0;
                current_graphics_key = previous_graphics_key;
            }
            return exit_requested;
        case BIOS_KEY_ALT_F9:
            semi_auto_mode = !semi_auto_mode;
            previous_graphics_key = current_graphics_key = 'n';
            if (semi_auto_mode) {
                freeze_display(2);
                beep(880, 20);
            } else
                beep(440, 20);
            return exit_requested;
        case BIOS_KEY_ALT_F10:
            if (semi_auto_mode) {
                previous_graphics_key = current_graphics_key;
                return exit_requested;
            }
            /* fall through */
        default:
            current_graphics_key = previous_graphics_key;
            break;
        }
    }
    return exit_requested;
}

/* Fills the 64-entry sine and cosine lookup table used by drawing routines.
 * Original 11C1:1D64, 113 bytes. */

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

/* Fills a supplied pixel map with a circular mask and distance-based color indices starting at
 * three.
 * Original 11C1:1DD5, 340 bytes. */

void far initialize_palette_mapping(PaletteMapSpec far *mapping) {
    int map_half_height, map_half_width;
    int map_limit_y, map_limit_x;
    unsigned int y_squared;
    unsigned int pixel_row_offset;
    unsigned int map_radius_squared;
    float color_distance;
    register int y, x;
    register int dy, dx;
    register unsigned char color_index;

    map_half_height = mapping->height / 2;
    map_half_width = mapping->width / 2;
    map_limit_y = map_half_height + map_half_height / 4;
    map_limit_x = map_half_width + map_half_width / 4;
    map_radius_squared = map_half_height < map_half_width ? map_half_height - 1 : map_half_width - 1;
    map_radius_squared *= map_radius_squared;

    for (y = 0; y < mapping->height; ++y) {
        y_squared = abs(map_half_height - y);
        y_squared *= y_squared;
        pixel_row_offset = mapping->width * y;
        for (x = 0; x < mapping->width; ++x) {
            dx = abs(map_half_width - x);
            dx *= dx;
            if (y_squared + dx > map_radius_squared) {
                mapping->pixels[pixel_row_offset + x] = 0;
            } else {
                dx = abs(map_limit_y - y);
                dy = abs(map_limit_x - x);
                if (dx || dy) {
                    color_distance = sqrt((double)(unsigned int)(dx * dx + dy * dy));
                } else {
                    color_distance = 0.0;
                }
                color_index = (int)color_distance % 253;
                if (color_count < 0x80)
                    color_index >>= 1;
                mapping->pixels[pixel_row_offset + x] = (unsigned char)(color_index + FIRST_CYCLING_COLOR);
            }
        }
    }
}

/* Sets initial drawing, palette, callback, image-count, and Auto pause state, then builds math
 * tables.
 * Original 11C1:1F29, 150 bytes. */
void initialize_program_defaults(void) {
    plot_color = FIRST_CYCLING_COLOR;
    draw_delay_count = 1;
    draw_symmetry = 3;
    current_image_draw = alt_image_records[0].draw;
    current_graphics_key = (previous_graphics_key = 112);
    exit_requested = (display_cleared = 1);
    initialize_math_tables();
    default_config_name = "MTDAZZLE.DZL";
    mode_setup_callback = ega_set_mode;
    plot_callback = ega_plot_symmetric_640x350;
    strcpy(configuration_filename, default_config_name);
    minimum_images_per_display = 3;
    maximum_images_per_display = 9;
    auto_fade_pause_ticks = 91;
    palette_step = 3;
    palette_alternate_mode = 0;
    delay(1);
}

/* Rounds a floating-point frequency and sounds it for the supplied duration.
 * Original 11C1:1FBF, 48 bytes. */
void delay(unsigned); void nosound(void);
void beep_float(float hz, unsigned ms) {
    int frequency_value;
    frequency_value = hz + 0.5;
    sound(frequency_value);
    delay(ms);
    nosound();
}

/* Builds a frequency table and plays the startup chord and melody until input or mouse abort.
 * Original 11C1:1FEF, 370 bytes. */
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

/* Draws the closing image, optionally plays music, and waits for a key, timeout, or mouse abort.
 * Original 11C1:2161, 69 bytes. */

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

/* Selects adapter modes, drawing/palette callbacks, dimensions, and allocates palette and image
 * work buffers.
 * Original 11C1:21A6, 879 bytes. */

void configure_video_backend(int allocate) {
    register int ball_scale_divisor;

    if (allocate) freeze_led_state = 0;
    display_start_callback = set_display_start;
    ega_red_limit = 0x2f;
    ega_blue_limit = 0x3f;
    ega_green_limit = 0x1f;
    if (video_adapter == 8) {
        default_palette_cycle_ticks = 1;
        if (allocate) palette_cycle_ticks = use_8086_palette_io ? default_palette_cycle_ticks << 2 : default_palette_cycle_ticks;
        palette_entries = 576;
        color_count = 256;
        color_index_mask = color_count - 1;
        screen_half_width = 160;
        if (bios_resolution_mode) {
            screen_half_height = 100;
            mode_setup_callback = (VideoCallback)vga_set_mode_320x200;
            plot_callback = (VideoCallback)vga_plot_symmetric_320x200;
            read_pixel_callback = vga_read_pixel_320x200;
            screen_extent = screen_half_width + 1;
        } else {
            screen_half_height = 200;
            mode_setup_callback = (VideoCallback)vga_set_mode_320x400;
            plot_callback = (VideoCallback)vga_plot_symmetric_320x400;
            read_pixel_callback = vga_read_pixel_320x400;
            if (use_bios_palette || split_vga_palette_updates || use_8086_palette_io)
                screen_extent = screen_half_height - 28;
            else
                screen_extent = screen_half_height + 1;
        }
        palette_build_callback = build_ega_palette;
        if (split_vga_palette_updates) {
            if (use_bios_palette)
                palette_upload_callback = vga_upload_palette_bios_split;
            else if (use_8086_palette_io)
                palette_upload_callback = vga_upload_palette_8086_split;
            else
                palette_upload_callback = vga_upload_palette_rep_split;
        } else if (use_bios_palette) {
            palette_upload_callback = vga_upload_palette_bios;
        } else if (use_8086_palette_io) {
            palette_upload_callback = vga_upload_palette_8086;
        } else {
            palette_upload_callback = vga_upload_palette_rep;
        }
    }
    if (video_adapter == 4) {
        default_palette_cycle_ticks = 2;
        if (allocate) palette_cycle_ticks = default_palette_cycle_ticks;
        palette_entries = 64;
        color_count = 16;
        color_index_mask = color_count - 1;
        screen_half_width = 320;
        mode_setup_callback = ega_set_mode;
        if (bios_resolution_mode) {
            plot_callback = (VideoCallback)ega_plot_symmetric_640x350;
            read_pixel_callback = ega_read_pixel_640x350;
            screen_half_height = 175;
            screen_extent = screen_half_width + 1;
        } else {
            plot_callback = (VideoCallback)ega_plot_symmetric_640x700;
            read_pixel_callback = ega_read_pixel_640x700;
            screen_half_height = 350;
            screen_extent = screen_half_height + 1;
        }
        if (use_bios_palette)
            palette_upload_callback = ega_upload_palette_bios;
        else
            palette_upload_callback = ega_upload_palette_ports;
        palette_build_callback = generate_palette;
    }
    maximum_image_size = 0xffff / (screen_extent + (screen_extent >> 2)) - 1;
    palette_cycle_wrap_bytes = palette_entries * 3;
    if (!allocate) return;

    if (video_adapter == 4)
        palette_buffer_bytes = color_count + palette_entries;
    else
        palette_buffer_bytes = (color_count + palette_entries) * 3;
    palette_buffer = (unsigned char far *)malloc(palette_buffer_bytes + 3);
    if (!palette_buffer) {
        exit_requested = 1;
        fprintf(&standard_error_stream, "\r Memory error: EPAL - ABORT\n\a");
        print_closing_text_and_exit(0x3c);
    }
    linewipe_history = malloc(0x7d8);
    if (!linewipe_history) {
        exit_requested = 1;
        fprintf(&standard_error_stream, "\r Memory error: WLINES - ABORT\n\a");
        print_closing_text_and_exit(0x46);
    }
    ball_scale_divisor = random(5) + 2;
    ball_bitmap_height = screen_half_height / ball_scale_divisor;
    ball_bitmap_width = screen_half_width / ball_scale_divisor;
    ball_bitmap_pixels = malloc(ball_bitmap_height * ball_bitmap_width);
    if (!ball_bitmap_pixels) {
        exit_requested = 1;
        fprintf(&standard_error_stream, "\r Memory error: BALL - ABORT\n\a");
        print_closing_text_and_exit(0x47);
    }
    initialize_palette_mapping((PaletteMapSpec far *)&ball_bitmap_height);
}

/* Advances and uploads the palette window unless the frozen display is static, then resets its
 * timer.
 * Original 11C1:2515, 89 bytes. */

void cycle_palette(void) {
    if (!display_freeze_active || display_freeze_cycle_requested) {
        if (video_adapter == 4) {
            ++palette_cycle_offset;
            if (palette_cycle_offset >= palette_entries) palette_cycle_offset = 3;
        } else {
            palette_cycle_offset = palette_cycle_offset + palette_step;
            if (palette_cycle_offset >= palette_cycle_wrap_bytes) palette_cycle_offset = 9;
        }
        palette_upload_callback(palette_buffer + palette_cycle_offset);
    }
    palette_elapsed_ticks = 0;
}

/* Reads and checks the configuration version marker, warning on versions older than 10410.
 * Original 11C1:256E, 104 bytes. */

int parse_config_version(char far *line) {
    char far *version_text;
    unsigned version;

    version_text = strchr(line, '#');
    ++version_text;
    if (sscanf(version_text, "%u", &version) == -1) {
        menu_error_message("Corrupted version data in the input file");
        return 1;
    }
    if (version < 10410)
        menu_error_message("WARNING: bad version in the input file");
    return 0;
}

/* Reads a '~' image key and bracketed size and updates the matching Alt-image record.
 * Original 11C1:25D6, 244 bytes. */

void parse_alt_image_size(char far *line) {
    int configured_size;
    char far *setting_text;
    char image_key;
    unsigned image_index;

    if (!(setting_text = strchr(line, '~'))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &image_key) == -1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    if (sscanf(setting_text, "%d", &configured_size) == -1)
        return;
    if (configured_size > maximum_image_size) configured_size = maximum_image_size;
    if (configured_size < 0) configured_size = 0;
    for (image_index = 0; image_index < alt_image_count; ++image_index) {
        if ((char)alt_image_records[image_index].key == image_key) {
            alt_image_records[image_index].size = configured_size;
            break;
        }
    }
}

/* Reads a '^' image key and bracketed size and updates the matching Ctrl-image record.
 * Original 11C1:26CA, 244 bytes. */

void parse_ctrl_image_size(char far *line) {
    int configured_size;
    char far *setting_text;
    char image_key;
    unsigned image_index;

    if (!(setting_text = strchr(line, '^'))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &image_key) == -1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    if (sscanf(setting_text, "%d", &configured_size) == -1)
        return;
    if (configured_size > maximum_image_size) configured_size = maximum_image_size;
    if (configured_size < 0) configured_size = 0;
    for (image_index = 0; image_index < ctrl_image_count; ++image_index) {
        if ((char)ctrl_image_records[image_index].key == image_key) {
            ctrl_image_records[image_index].size = configured_size;
            break;
        }
    }
}

/* Reads a fade key and bracketed y/n value and updates the matching fade enable flag.
 * Original 11C1:27BE, 256 bytes. */

void parse_fade_enabled(char far *line) {
    char far *setting_text;
    char fade_key;
    char enable_letter;
    unsigned fade_index;

    setting_text = line;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &fade_key) == -1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &enable_letter) == -1)
        return;
    if (enable_letter != 'n' && enable_letter != 'y') return;
    for (fade_index = 0; fade_index < fade_count; ++fade_index) {
        if (fade_records[fade_index].key == fade_key) {
            fade_records[fade_index].enabled = (enable_letter == 'y');
            break;
        }
    }
}

/* Reads an Auto setting key and bracketed value and updates image counts or fade pause time.
 * Original 11C1:28BE, 240 bytes. */

void parse_auto_configuration(char far *line) {
    int setting_value;
    char far *setting_text;
    char setting_key;

    if (!(setting_text = strchr(line, '$'))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &setting_key) == -1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    if (sscanf(setting_text, "%d", &setting_value) == -1)
        return;
    if (setting_value > 256) setting_value = 256;
    if (setting_value < 0) setting_value = 1;
    switch (setting_key) {
    case 'i':
        minimum_images_per_display = setting_value;
        break;
    case 'x':
        maximum_images_per_display = setting_value;
        break;
    case 'f':
        auto_fade_pause_ticks = setting_value * 18;
        break;
    }
}

/* Reads a text configuration file, removes comments, normalizes each line, and dispatches its
 * settings.
 * Original 11C1:29AE, 406 bytes. */

int load_configuration_file(char far *path) {
    char line[80];
    char far *line_cursor;

    if (!(configuration_file_stream = fopen(path, "rt")))
        return 1;
    for (;;) {
        if (!fgets(line, 80, configuration_file_stream)) {
            clearerr(configuration_file_stream);
            if (fclose(configuration_file_stream) != 0)
                menu_error_message("Error closing input file");
            return 0;
        }
        if (strlen(line) < 1)
            continue;
        if ((line_cursor = strchr(line, ';')) != 0)
            *line_cursor = 0;
        if (strlen(line) < 1)
            continue;
        line_cursor = line;
        while (*line_cursor && (*line_cursor == ' ' || *line_cursor == '\t'))
            ++line_cursor;
        if (*line_cursor == 0)
            continue;
        if (!strlwr(line_cursor))
            continue;
        switch (*line_cursor) {
        case '#':
            if (parse_config_version(line_cursor)) {
                fclose(configuration_file_stream);
                return 1;
            }
            break;
        case '~':
            parse_alt_image_size(line_cursor);
            break;
        case '^':
            parse_ctrl_image_size(line_cursor);
            break;
        case '`':
            break;
        case '$':
            parse_auto_configuration(line_cursor);
            break;
        default:
            parse_fade_enabled(line_cursor);
            break;
        }
    }
}

/* Writes version, image sizes, fade enables, and Auto settings to a text configuration file.
 * Original 11C1:2B44, 482 bytes. */

int save_configuration_file(char far *path) {
    unsigned record_index;
    register char enabled;

    if (!(configuration_file_stream = fopen(path, "wt"))) {
        menu_error_message("Unable to open file for write");
        return 1;
    }
    fprintf(configuration_file_stream, "; This file created by MTDAZZLE.  It is a text file of program\n; configuration information that can be edited, allowing customization\n; of the display. Refer to MTDAZZLE.DOC for full details.\n; Be sure not to delete the VERSION flag that starts with '#'!\n#10500;DO NOT MODIFY THIS LINE!\n");
    for (record_index = 0; record_index < alt_image_count; ++record_index) {
        if (fprintf(configuration_file_stream, "~%c [%03d] ; %s image size\n", (char)alt_image_records[record_index].key,
                    alt_image_records[record_index].size,
                    alt_image_records[record_index].name) == -1) {
            fclose(configuration_file_stream);
            return 1;
        }
    }
    for (record_index = 0; record_index < ctrl_image_count; ++record_index) {
        if (fprintf(configuration_file_stream, "^%c [%03d] ; %s image size\n", (char)ctrl_image_records[record_index].key,
                    ctrl_image_records[record_index].size,
                    ctrl_image_records[record_index].name) == -1) {
            fclose(configuration_file_stream);
            return 1;
        }
    }
    for (record_index = 0; record_index < fade_count; ++record_index) {
        if (fade_records[record_index].enabled)
            enabled = 'y';
        else
            enabled = 'n';
        if (fprintf(configuration_file_stream, "%c [%c] ; %s fade enable\n", (char)fade_records[record_index].key,
                    enabled,
                    fade_records[record_index].name) == -1) {
            fclose(configuration_file_stream);
            return 1;
        }
    }
    if (fprintf(configuration_file_stream, "$I [%03d] ; mInimum image count in Auto Mode\n", minimum_images_per_display) == -1) {
        fclose(configuration_file_stream);
        return 1;
    }
    if (fprintf(configuration_file_stream, "$X [%03d] ; maXimum image count in Auto Mode\n", maximum_images_per_display) == -1) {
        fclose(configuration_file_stream);
        return 1;
    }
    if (fprintf(configuration_file_stream, "$F [%03d] ; Fade delay time in Auto Mode\n", (int)auto_fade_pause_ticks / 18) == -1) {
        fclose(configuration_file_stream);
        return 1;
    }
    fclose(configuration_file_stream);
    return 0;
}

/* Resolves the requested/default config path, rejects executable names, and loads settings when
 * available.
 * Original 11C1:2D26, 455 bytes. */

int resolve_startup_configuration(int requested) {
    char far *located_config_path;
    int config_search_result;
    char drive[MAXDRIVE];
    char filename[MAXFILE];
    char extension[MAXEXT];
    struct ffblk file_info;
    char directory[MAXDIR];

    if (requested < 0)
        return 0;

    if ((config_search_result = findfirst(configuration_filename, &file_info, 0)) && requested) {
        fprintf(&standard_error_stream, config_load_error_text, configuration_filename);
        delay(1250);
        return 1;
    }

    if (config_search_result) {
        fnsplit(default_config_name, 0, 0, filename, extension);
        fnsplit(executable_path, drive, directory, 0, 0);
        fnmerge(configuration_filename, drive, directory, filename, extension);
        if (findfirst(configuration_filename, &file_info, 0)) {
            if (located_config_path = searchpath(default_config_name))
                strcpy(configuration_filename, located_config_path);
            else
                return 0;
        }
    } else {
        fnsplit(configuration_filename, drive, 0, 0, 0);
        /* The original tests the buffer pointer, rather than its first byte. */
        if (drive)
            setdisk(tolower(drive[0]) - 'a');
    }

    if (reject_exec_filename(configuration_filename)) {
        fprintf(&standard_error_stream, "\n Illegal configuration file name ignored.\n");
        strcpy(configuration_filename, default_config_name);
        delay(1250);
        return 1;
    }

    if (load_configuration_file(configuration_filename) && requested > 0) {
        fprintf(&standard_error_stream, config_load_error_text, configuration_filename);
        delay(1250);
        return 1;
    }

    textcolor(9);
    if (!window_mode && !bashful_mode)
        cprintf("\r Loaded %s\n\r", configuration_filename);
    return 0;
}

/* Parses concatenated command-line options, optional numeric values, and a config path, then
 * checks CPU mode.
 * Original 11C1:2EED, 1202 bytes. */
/* Walks far argv strings, accepts concatenated switch characters, and handles
   optional numeric values, one input path, and the documented application flags. */

int parse_command_line(int argc, char far * far *argv,
                       int far *option_position) {
    register int argument_index;
    int config_result;
    int machine_mode;
    char option_character;
    char configured_switch_character;
    register int option_character_index;

    config_result = 0;
    *option_position = 0xffff;
    strcpy(configuration_filename, default_config_name);
    configured_switch_character = (char)getswitchar();
    if (argc >= 1) {
        for (argument_index = 1; argument_index < argc; ++argument_index) {
            if (*argv[argument_index] == '-' || *argv[argument_index] == configured_switch_character) {
                argv[argument_index][0] = '-';
                option_character_index = 1;
                while ((option_character = argv[argument_index][option_character_index]) != 0) {
                    if (option_character == configured_switch_character)
                        option_character = '-';
                    switch (tolower(option_character)) {
                    default:
                        fprintf(&standard_error_stream, "\r unknown option /%c \n", option_character);
                    case OPTION_HELP:
                        command_line_help();
                        break;
                    case OPTION_ORDER_INFO:
                        insecure_argument_index = 1;
                        check_executable_integrity();
                        print_order_info(1);
                        exit(0);
                        break;
                    case OPTION_DRIVER_INTERFACE:
                        option_bang_requested = 1;
                        break;
                    case OPTION_IGNORE_TARGA:
                        option_plus_requested = 1;
                        break;
                    case OPTION_SEPARATOR:
                        break;
                    case OPTION_SPLIT_PALETTE:
                        split_vga_palette_updates = 1;
                        break;
                    case OPTION_FOUR_EGA_COLORS:
                        palette_four_ega_colors = 1;
                        break;
                    case OPTION_ALTERNATE_PALETTE:
                        palette_alternate_mode = 1;
                        break;
                    case OPTION_BASHFUL:
                        bashful_mode = 1;
                        break;
                    case OPTION_BIOS_PALETTE:
                        use_bios_palette = 1;
                        break;
                    case OPTION_DRAW_DELAY:
                        draw_delay_count = 0x100;
                        if (argv[argument_index][option_character_index + 1] == '=') {
                            option_character_index += 2;
                            if (sscanf(argv[argument_index] + option_character_index, "%ud", &draw_delay_count) != 1) {
                                fprintf(&standard_error_stream, "\r unable to parse %s\n", argv[argument_index][option_character_index - 2]);
                                command_line_help();
                            }
                            if (draw_delay_count < 1)
                                draw_delay_count = 1;
                            if (draw_delay_count > 0x1fff) {
                                fprintf(&standard_error_stream, "\r Delay time limited to 8191 units!\n\a");
                                command_line_help();
                            }
                            while ((option_character = argv[argument_index][option_character_index + 1]) >= '0' && option_character <= '9')
                                ++option_character_index;
                        }
                        break;
                    case OPTION_FORCE_EGA:
                        force_ega = 1;
                        break;
                    case OPTION_FIXED_COLORS:
                        palette_fixed_mode = 1;
                        break;
                    case OPTION_HIDE_EGA_BORDER:
                        hide_ega_border = 1;
                        break;
                    case OPTION_INSECURE:
                        *option_position = option_character_index;
                        insecure_argument_index = argument_index;
                        break;
                    case OPTION_KEEP_TYPEMATIC:
                        keep_typematic = 1;
                        break;
                    case OPTION_RANDOM_PALETTE:
                        palette_alternate_mode = 0;
                        palette_random_mode = 1;
                        break;
                    case OPTION_NO_DISK_PARK:
                        no_disk_park = 1;
                        break;
                    case OPTION_QUIET:
                        muted = 1;
                        break;
                    case OPTION_SHOW:
                        show_mode = 1;
                        if (argv[argument_index][option_character_index + 1] == '=') {
                            option_character_index += 2;
                            if (sscanf(argv[argument_index] + option_character_index, "%ud", &show_duration_ticks) != 1) {
                                fprintf(&standard_error_stream, "\r unable to parse %s\n", argv[argument_index][option_character_index - 2]);
                                command_line_help();
                            }
                            if (show_duration_ticks > 3600) {
                                fprintf(&standard_error_stream, "\r ShowTime limited to 3600 seconds!\n\a");
                                command_line_help();
                            }
                            show_duration_ticks *= 18;
                            while ((option_character = argv[argument_index][option_character_index + 1]) >= '0' && option_character <= '9')
                                ++option_character_index;
                        }
                        break;
                    case OPTION_TALL_ASPECT:
                        bios_resolution_mode = 1;
                        break;
                    case OPTION_FORCE_VGA:
                        force_vga = 1;
                        break;
                    case OPTION_WINDOW:
                        window_mode = 1;
                        break;
                    case OPTION_8086_PALETTE_IO:
                        use_8086_palette_io = 1;
                        break;
                    case OPTION_Y:
                        option_y_requested = 1;
                        break;
                    case OPTION_EXTRACT_MANUAL:
                        extract_manual();
                        exit(3);
                    case OPTION_NO_AUTOLOAD:
                        config_result = -1;
                        break;
                    }

                    ++option_character_index;
                }
            } else {
                strcpy(configuration_filename, argv[argument_index]);
                if (strchr(configuration_filename, '?'))
                    command_line_help();
                if (!strchr(configuration_filename, '*')) {
                    if (!strchr(configuration_filename, '.'))
                        strcat(configuration_filename, ".DZL");
                    config_result = 1;
                } else {
                    strcpy(configuration_filename, default_config_name);
                    config_result = -1;
                }
            }
        }
    }
    if (use_8086_palette_io) {
        return config_result;
    } else {
        machine_mode = cpu_has_post_8086_shifts();
        if (machine_mode == 0) {
            fprintf(&standard_error_stream, "\r Forcing the /X option for the detected CPU\n");
            use_8086_palette_io = 1;
        }
    }
    return config_result;
}

/* Prints program revision and copyright, adapter mode, screen-saver status, and the help hint
 * when enabled.
 * Original 11C1:339F, 410 bytes. */

void print_startup_information(void) {
    int line_width;

    if (show_mode || window_mode || (bashful_mode && no_disk_park))
        return;
    textcolor(13);
    cprintf("\r %s ", help_program_name);
    line_width = strlen(help_program_name) + 2;
    textcolor(15);
    cprintf("%s%s", revision_label, program_revision);
    line_width += strlen(revision_label) + strlen(program_revision);
    line_width = line_width + strlen(copyright_text);
    while (line_width < 0x4f) {
        cprintf(" ");
        ++line_width;
    }
    cprintf("%s", copyright_text);
    textcolor(7);
    cprintf("\r\n video mode (");
    if (video_adapter == 4)
        cprintf("EGA)");
    if (targa64_mode)
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

/* Runs Semi-Auto selection and drawing, handling image reinitialization, symmetry, XOR, and
 * continuous mode.
 * Original 11C1:3539, 906 bytes. */

void semi_auto_loop(void) {
    register unsigned previous_image_index;

    previous_image_index = 0xffff;
    current_image_index = random(alt_image_count + ctrl_image_count + 1);
    if (!display_cleared) {
        previous_graphics_key = current_graphics_key = 110;
        display_boundary_fade_and_pan();
    }
    semi_auto_continuous = 0;
    while (input_pending) {
        if (poll_graphics_keys())
            return;
    }
    current_graphics_key = previous_graphics_key = 0;
    if (semi_auto_mode)
        mode_ready_chirp();

    while (semi_auto_mode && !exit_requested) {
        while (!input_pending && !semi_auto_continuous) {
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        if (poll_graphics_keys())
            return;

        if (current_graphics_key && (unsigned)current_graphics_key <= ctrl_image_count) {
            current_image_index = current_graphics_key + alt_image_count;
            current_image_draw = ctrl_image_records[((unsigned)current_graphics_key - 1)].draw;
            current_graphics_key = BIOS_KEY_ALT_F10;
        } else if (((unsigned char)current_graphics_key & 0x7f) < alt_image_count) {
            if (current_graphics_key & 0x80) {
                current_image_index = (unsigned char)current_graphics_key & 0x7f;
                current_image_draw = alt_image_records[current_image_index].draw;
                current_graphics_key = BIOS_KEY_ALT_F10;
            }
        }
        if (!current_graphics_key && semi_auto_continuous)
            current_graphics_key = BIOS_KEY_F8;

        switch (current_graphics_key) {
        case BIOS_KEY_F9:
            current_image_index = random(alt_image_count + ctrl_image_count + 1);
            if (current_image_index < alt_image_count)
                current_image_draw = alt_image_records[current_image_index].draw;
            else
                current_image_draw = ctrl_image_records[(current_image_index - alt_image_count - 1)].draw;
            /* Fall through to the common new-image setup. */

        case BIOS_KEY_ALT_F10:
            diagonal_symmetry_selected = random(2);
            if (current_image_index < alt_image_count) {
                image_xor_mode = alt_image_records[current_image_index].continuous &&
                    random(5) == 0 ? 1 : 0;
            } else {
                image_xor_mode = ctrl_image_records[current_image_index - alt_image_count - 1].continuous &&
                    random(5) == 0 ? 1 : 0;
            }
            set_ega_raster_xor(image_xor_mode);
            current_image_draw(1);
            previous_image_index = current_image_index;
            display_cleared = 0;
            if (current_graphics_key == BIOS_KEY_F9)
                beep(1200, 10);
            else
                beep(2000, 3);
            previous_graphics_key = current_graphics_key = 0;
            continue;

        case BIOS_KEY_F8:
        case BIOS_KEY_F10:
            display_cleared = 0;
            if (current_image_index < alt_image_count) {
                draw_symmetry = alt_image_records[current_image_index].enabled &&
                         diagonal_symmetry_selected ? 3 : 1;
            } else {
                draw_symmetry = ctrl_image_records[current_image_index - alt_image_count - 1].enabled &&
                         diagonal_symmetry_selected ? 3 : 1;
            }
            if (previous_image_index != current_image_index) {
                if (current_image_index < alt_image_count) {
                    current_image_draw = alt_image_records[current_image_index].draw;
                } else {
                    current_image_draw = ctrl_image_records[(current_image_index - alt_image_count - 1)].draw;
                }
                current_image_draw(1);
                previous_image_index = current_image_index;
            }
            while (!input_pending) {
                current_image_draw(0);
                plot_callback(draw_symmetry);
                if (palette_elapsed_ticks >= palette_cycle_ticks)
                    cycle_palette();
            }
            break;

        case 0x68:
            /* Clear the ignored Hold command; BC30 removes the dead store. */
            current_graphics_key = 0;
            break;

        case 0:
            continue;

        default:
            display_boundary_fade_and_pan();
            if (semi_auto_mode)
                mode_ready_chirp();
            break;
        }
        previous_graphics_key = current_graphics_key = 0;
    }
}

/* Runs Auto image selection and overlays, services input and palette timing, and starts display
 * transitions.
 * Original 11C1:38C3, 1025 bytes. */

void auto_loop(void) {
    register int remaining_overlays;
    register unsigned image_search_attempts;

    current_image_index = 0;
    image_work_count = 1;
    if (minimum_images_per_display < 1)
        minimum_images_per_display = 1;
    if (maximum_images_per_display < minimum_images_per_display)
        maximum_images_per_display = minimum_images_per_display;
    remaining_overlays = minimum_images_per_display +
        random(maximum_images_per_display + 1 - minimum_images_per_display);
    while (!exit_requested && !semi_auto_mode) {
        if (image_work_count++ > image_work_limit || current_graphics_key == 110) {
            image_work_count = 0;

            current_image_index = random(alt_image_count + ctrl_image_count + 1);
            /* Skip disabled image records, but bound the search if none remain configured. */
            image_search_attempts = 0;
            for (;;) {
                if (current_image_index < alt_image_count) {
                    if (alt_image_records[current_image_index].size > 0)
                        break;
                } else if (ctrl_image_records[current_image_index - alt_image_count - 1].size > 0) {
                    break;
                }
                ++current_image_index;
                current_image_index %= alt_image_count + ctrl_image_count + 1;
                if (image_search_attempts++ > alt_image_count + ctrl_image_count + 2) {
                    startup_config_failure = exit_requested = 1;
                    return;
                }
            }

            diagonal_symmetry_selected = random(2);
            if (current_image_index < alt_image_count) {
                image_work_limit = random(alt_image_records[current_image_index].size * screen_extent) +
                    ((unsigned)(alt_image_records[current_image_index].size * screen_extent) >> 2);
            } else {
                image_work_limit = random(ctrl_image_records[current_image_index - alt_image_count - 1].size * screen_extent) +
                    ((unsigned)(ctrl_image_records[current_image_index - alt_image_count - 1].size *
                                screen_extent) >> 2);
            }
            if (--remaining_overlays <= 0 || current_graphics_key == 110) {
                display_boundary_fade_and_pan();
                remaining_overlays = minimum_images_per_display +
                    random(maximum_images_per_display + 1 - minimum_images_per_display);
                if (palette_alternate_mode)
                    remaining_overlays += random(minimum_images_per_display);
                if (palette_random_mode)
                    remaining_overlays += random(maximum_images_per_display);
                if (display_pan_state == 2 && remaining_overlays > 8)
                    remaining_overlays = 9;
            }

            if (current_image_index < alt_image_count) {
                image_xor_mode = alt_image_records[current_image_index].continuous &&
                    random(remaining_overlays + 1) == 0 ? 1 : 0;
            } else {
                image_xor_mode = ctrl_image_records[current_image_index - alt_image_count - 1].continuous &&
                    random(remaining_overlays + 1) == 0 ? 1 : 0;
            }
            if (input_pending && (poll_graphics_keys() || semi_auto_mode))
                return;
            set_ega_raster_xor(image_xor_mode);
            if (current_image_index < alt_image_count) {
                current_image_draw = alt_image_records[current_image_index].draw;
                draw_symmetry = alt_image_records[current_image_index].enabled &&
                         diagonal_symmetry_selected ? 3 : 1;
            } else {
                current_image_draw = ctrl_image_records[(current_image_index - alt_image_count - 1)].draw;
                /* The original also indexes the Alt table in this Ctrl branch. */
                draw_symmetry = alt_image_records[current_image_index].enabled &&
                         diagonal_symmetry_selected ? 3 : 1;
            }
            current_image_draw(1);
            plot_callback(draw_symmetry);
            display_cleared = 0;
            fprintf(&standard_output_stream, "\r");

        }
        if (input_pending && (poll_graphics_keys() || semi_auto_mode))
            return;
        current_image_draw(0);
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

/* Reserved startup-cleared storage at DS:7452..7456; ownership provisional. */
unsigned char reserved_program_state[4];
