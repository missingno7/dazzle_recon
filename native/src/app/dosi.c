/* DAZZLE 5.0 DOSI: native startup, configuration, input and program loops. */
#include "dazzle.h"
#include "platform/testctl.h"
#include "platform/platform.h"
#include "dzconst.h"
#include "dzconio.h"
#include "platform/dosi_support.h"
#include <math.h>
/* MinGW aliases matherr to its CRT _matherr; keep this callable DOS handler local to DAZZLE. */
#ifdef matherr
#undef matherr
#endif
#ifdef exception
#undef exception
#endif
#include <ctype.h>
#include <inttypes.h>

static void dz_build_ega_adapter(int16_t unused) { (void)unused; generate_palette(); }
enum video_backend_kind video_backend;


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

static int16_t dos_word(uint16_t value)
{
    return value <= INT16_MAX ? (int16_t)value : (int16_t)((int32_t)value - 65536);
}
/* scanf's original destinations were words, even for out-of-range text. */
static int parse_word(const char *text, uint16_t *word)
{
    char *end;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text) return 0;
    *word = (uint16_t)value;
    return 1;
}

/* The DOS arrays are contiguous: Ctrl[-1] aliases the last Alt record.
 * The Auto Ctrl symmetry read instead uses Alt[index], hence Ctrl[index-36].
 * Alt[42].enabled reads the nonzero DGROUP segment of a fade-name pointer. */
static ImageConfigRow *dz_dosi_image_record(uint16_t index)
{
    if (index <= alt_image_count) return &alt_image_records[index == alt_image_count ? alt_image_count - 1u : index];
    return &ctrl_image_records[index - alt_image_count - 1u];
}
static uint16_t dz_dosi_auto_symmetry(uint16_t index)
{
    if (index < alt_image_count) return alt_image_records[index].enabled;
    if (index < (uint16_t)(alt_image_count + ctrl_image_count)) return ctrl_image_records[index - alt_image_count].enabled;
    return 1; /* DOS segment word is nonzero. */
}
static uint16_t dz_dosi_next_work_count(void)
{
    uint16_t previous = (uint16_t)image_work_count;
    uint16_t next = (uint16_t)(previous + 1u);
    image_work_count = next <= INT16_MAX ? (int16_t)next : (int16_t)((int32_t)next - 65536);
    return previous; /* comparison with unsigned image_work_limit used unsigned promotion */
}

/* Initialized DOSI state. Text and numeric literals follow in source order. */
uint16_t display_freeze_active = 0, display_freeze_cycle_requested = 0;
static uint16_t led_flags;
uint32_t crc_polynomial = 0x00102100UL;
uint16_t palette_callback_active = 0;
uint16_t tone_chord[4] = {0, 4, 7, 10};
float tone_base_frequency = 16.35f;
char config_load_error_text[] = "\n Unable to load file : %s\n";
char revision_label[] = " rev. ";
char copyright_text[] = "Copyright(c)1989-92 MicroTronics";

/* Reconstructed 11C1 startup, integrity, and input routines. */

/* Updates selected BIOS keyboard LED flags unless Window or Bashful mode suppresses it; returns
 * the old flags.
 * Original 11C1:000A, 101 bytes. */
uint16_t keyboard_led_flags(uint16_t flag_mask, uint16_t update_mode) {
    uint16_t old_flags = led_flags;
    if (!window_mode && !bashful_mode) {
        switch (update_mode) {
        case 0: led_flags |= flag_mask; break;
        case 1: led_flags &= (uint16_t)~flag_mask; break;
        default:
            led_flags |= flag_mask;
            led_flags &= (uint16_t)(flag_mask | 0xff8fu);
            break;
        }
    }
    return old_flags;
}

/* Plays a rising three-note readiness chirp unless sound is muted.
 * Original 11C1:006F, 73 bytes. */
void mode_ready_chirp(void) {
    if (!muted) {
        dz_sound(220);
        dz_delay_ms(30);
        dz_sound(440);
        dz_delay_ms(40);
        dz_sound(880);
        dz_delay_ms(50);
        dz_nosound();
    }
}

/* Supplies the default Ctrl-Break callback result.
 * Original 11C1:00B8, 4 bytes. */
int16_t default_signal_result(void) {
    return 1;
}

/* Resumes the active hard-error operation and returns its callback result.
 * Original 11C1:00BC, 13 bytes. */
int16_t interrupt_signal_result(void) { return 2; }

/* Installs Ctrl-Break, timer, keyboard, optional mouse, and hard-error callbacks.
 * Original 11C1:00C9, 236 bytes. */
void install_runtime_callbacks(void) {
    int16_t mouse_probe_status;
    dz_dosi_install_signals();
    if (!timer_hook_installed) {
        dz_timer_tick_handler = graphics_timer_tick_isr;
        timer_hook_installed = 1;
    }
    if (!keyboard_hook_installed) {
        dz_keyboard_irq_handler = keyboard_pending_isr;
        keyboard_hook_installed = 1;
    }
    if (!mouse_available || (current_image_header && (current_image_header[6] & 0x0800u))) {
        if ((mouse_probe_status = probe_mouse(1)) != 0) {
            int16_t mask = 0x2a;
            if (bashful_mode && mouse_probe_status != 2) mask |= 1;
            mouse_position(mask, 0, 0);
        }
    }
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
    dz_dosi_restore_signals();
    dz_timer_tick_handler = dz_keyboard_irq_handler = dz_mouse_event_handler = NULL;
    timer_hook_installed = keyboard_hook_installed = 0;
    if (configuration_file_stream) {
        fclose(configuration_file_stream); configuration_file_stream = NULL;
        configuration_file_stream = NULL;
    }
    if (selfcheck_stream) { fclose(selfcheck_stream); selfcheck_stream = NULL; }
    if (mouse_callback_installed && current_image_header)
        current_image_header[5] &= 0xf7ffu;
}

/* Ignores two math exception types; otherwise restores runtime/video state, reports the error,
 * and returns zero.
 * Original 11C1:0274, 100 bytes. */
int16_t matherr(struct exception *exception_frame) {
    if (exception_frame && (exception_frame->type == 5 || exception_frame->type == 4)) return 1;
    restore_runtime_callbacks();
    restore_video_state(1);
    print_order_info(0);
    dz_dosi_printf("\nmatherr -- program aborted. Contact us for assistance.\a\n");
    dz_dosi_printf("Program revision level : %s\n\nREASON ", program_revision);
    return 0;
}

/* Prints the interrupted registers, module segment map, and revision details, then exits.
 * Original 11C1:02D8, 362 bytes. */
/* Table pairs, in order: 11C1:034C->066A:000B DZLE, 0364->0B84:0000 DZL2,
   037C->0CCD:000A DZFD, 0394->1899:1571 DVDT, 03C1->1A72:0007 DHUF,
   03D9->17F5:0008 DLOGO, 03F1->19F0:0004 DAZPCX. DOSI uses CS captured at
   02DE; 158D's menu/UI segment has no table entry or identified module label. */
void signal_diagnostic(int16_t signal_number, int16_t signal_type, uint16_t *interrupt_registers) {
    restore_runtime_callbacks();
    restore_video_state(1);
    dz_dosi_printf("sigerr -- program aborted. Contact us for assistance\n");
    dz_dosi_printf("Signal %d, Type %d,  Please print this error message!\a\n\n", signal_number, signal_type);
    if (interrupt_registers)
        dz_dosi_printf("Error at %04X:%04X\t FLAGS = %04X\n\n",
            interrupt_registers[10], interrupt_registers[9], interrupt_registers[11]);
    dz_dosi_printf("Native program: DOS segment map unavailable.\nProgram revision level :%s, %s\n\n",
        help_program_name, program_revision);
    print_order_info(0);
    exit(9);
}

/* Resets the mouse and optionally waits for button or motion activity; returns status masked by
 * the request.
 * Original 11C1:0442, 191 bytes. */
int16_t probe_mouse(uint16_t requested_status_mask) {
    uint16_t mouse_status = (uint16_t)(dz_mouse_present() ? 1 : 0), poll_attempts = 0;
    if (!requested_status_mask) dz_mouse_event_handler = NULL;
    if (mouse_status && requested_status_mask) {
        int dx, dy;
        unsigned buttons;
        do {
            dz_delay_ms(10);
            dz_mouse_counters(&dx, &dy, &buttons);
        } while (++poll_attempts <= 50 && (buttons || dx || dy));
        if (poll_attempts >= 50) mouse_status = 2;
    }
    return (int16_t)(mouse_status & requested_status_mask);
}

/* Registers the mouse callback and its button mask through INT 33h.
 * Original 11C1:0501, 46 bytes. */
void mouse_position(int16_t callback_mask, int16_t callback_offset, int16_t callback_segment) {
    (void)callback_offset; (void)callback_segment;
    dz_dosi_register_mouse((uint16_t)callback_mask);
}

/* Restores video, prints the self-check reason and optional CRC details, then exits with status
 * 100.
 * Original 11C1:052F, 148 bytes. */
void fatal_selfcheck_error(int16_t include_crc_details, const char *reason) {
    exit_requested = 1;
    restore_video_state(1);
    if (include_crc_details)
        dz_dosi_printf("\r Program aborting in antiviral routines! cv=%08" PRIX32
            ", r1=%05u, r2=%05u\nThis feature is for YOUR protection, write us for a good copy.\n",
            expected_crc, serial_r1, serial_r2);
    else
        dz_dosi_printf("\r Program aborting! cv=%08" PRIX32 ", r1=%05u, r2=%05u\n",
            expected_crc, serial_r1, serial_r2);
    dz_dosi_printf("\nREASON: %s \n\n\a", reason);
    print_order_info(0);
    exit(100);
}

/* Reads the first three MZ header words and computes the executable's resource base.
 * Original 11C1:05C3, 104 bytes. */
void read_mz_resource_base(FILE *input) {
    uint8_t header[6];
    if (fread(header, 1, sizeof header, input) != sizeof header)
        fatal_selfcheck_error(1, "can't read EXE header");
    resource_base = ((int32_t)dz_dosi_le16(header + 4) - 1) * 512 + dz_dosi_le16(header + 2) + 1;
}

/* Tries progressively smaller allocations down to the requested minimum and returns the
 * successful size.
 * Original 11C1:062B, 65 bytes. */
void *allocate(uint16_t *amount, uint16_t minimum) {
    uint16_t attempt_size;
    void *allocation = NULL;
    attempt_size = *amount;
    while (attempt_size >= minimum && (allocation = malloc(attempt_size)) == 0)
        attempt_size /= 2;
    *amount = attempt_size;
    return allocation;
}

/* Builds a CRC table and checks the requested executable bytes in allocated chunks.
 * Original 11C1:066C, 501 bytes. */
uint32_t genCRC(FILE *input, int32_t length) {
    uint16_t buffer_capacity = 0x2000;
    uint8_t *buffer = allocate(&buffer_capacity, 0x200);
    uint32_t crc = 0, table[256];
    int32_t bytes_processed = 0;
    if (!buffer) fatal_selfcheck_error(1, "memory unavailable in genCRC ");
    for (unsigned index = 0; index < 256; ++index) {
        uint32_t value = (uint32_t)index << 24;
        for (unsigned bit = 0; bit < 8; ++bit)
            value = (value << 1) ^ ((value & UINT32_C(0x80000000)) ? crc_polynomial : 0);
        table[index] = value;
    }
    if (fseek(input, 0, SEEK_SET)) { free(buffer); fatal_selfcheck_error(1, "failure in genCRC fseek!"); }
    while (bytes_processed < length - 1) {
        int32_t remaining = length - bytes_processed - 1;
        size_t count = remaining < buffer_capacity ? (size_t)remaining : buffer_capacity;
        size_t received = fread(buffer, 1, count, input);
        if (!received) break;
        for (size_t index = 0; index < received; ++index)
            crc = (crc << 8) ^ table[(crc >> 24) ^ buffer[index]];
        bytes_processed += (int32_t)count;
    }
    free(buffer);
    return crc;
}

/* Opens the executable, optionally calculates its CRC, reads metadata at the MZ end, and reports
 * read failure.
 * Original 11C1:0861, 232 bytes. */
int16_t deRead(void *metadata, uint16_t metadata_size) {
    uint8_t frame[10] = {0};
    /* The native executable is not an MZ. Optional original resources supply
    * serial/logo metadata; the native integrity path always passes, as /I. */
    const char *path = dz_options.original_exe ? dz_options.original_exe : "assets/DAZZLE.EXE";
    selfcheck_stream = fopen(path, "rb");
    if (selfcheck_stream) {
        uint8_t header[6];
        if (fread(header, 1, sizeof header, selfcheck_stream) == sizeof header &&
            header[0] == 'M' && header[1] == 'Z') {
            rewind(selfcheck_stream);
            read_mz_resource_base(selfcheck_stream);
            if (!fseek(selfcheck_stream, resource_base, SEEK_SET))
                (void)fread(frame, 1, sizeof frame, selfcheck_stream);
            snprintf(executable_path, sizeof executable_path, "%s", path);
        }
        fclose(selfcheck_stream); selfcheck_stream = NULL;
    }
    if (metadata == &expected_crc) {
        expected_crc = (uint32_t)dz_dosi_le16(frame) | ((uint32_t)dz_dosi_le16(frame + 2) << 16);
        serial_r1 = dz_dosi_le16(frame + 4); serial_r2 = dz_dosi_le16(frame + 6);
        image_data_offset = dz_dosi_le16(frame + 8);
    } else if (metadata && metadata_size) {
        memcpy(metadata, frame, metadata_size < sizeof frame ? metadata_size : sizeof frame);
    }
    return 0;
}

/* Resolves and validates the executable path and self-check metadata, skipping CRC comparison
 * for /I.
 * Original 11C1:0949, 365 bytes. */
void check_executable_integrity(void) {
    if (!insecure_argument_index) {
        textcolor(14);
        dz_dosi_printf(" antiviral checking \r");
        beep(2000, 4);
        textcolor(3);
    }
    /* Keep metadata loading and the original completion state; no native CRC
     * generation/comparison and no RNG calls are added or removed. */
    if (deRead(&expected_crc, 10)) fatal_selfcheck_error(1, "missing antiviral data");
    dz_dosi_printf("                    \r");
    exit_requested = 0;
}

/* Prints MicroTronics contact information and, when requested, the registration/order form.
 * Original 11C1:0AB6, 311 bytes. */
void print_order_info(int16_t show_order_form) {
    char *address_line;
    char *compuserve_line;
    char *company_line;
    char *phone_line;
    char *mailing_line;

    address_line = "Mailing address (1992) :";
    compuserve_line = "                         Compuserve   76236,1573";
    company_line = " MicroTronics ";
    phone_line = "                                   Voice/FAX (713)298-1698";
    mailing_line = " P.O.Box 8759 \r\n Spring, TX. U.S.A. 77387-8759";
    if (show_order_form) {
        dz_dosi_printf("\r\nSoftware products can be ordered from MicroTronics at the\n\raddress below. Please include this completed form with your check.\n\rWe do not CURRENTLY take individual telephone orders, so please write.\n\r\n\rPlease PRINT your full name and address clearly.\n\r\n\rProgram name : %s \n\rRevision     : %s \n\rID codes     : r1=%05u,  r2=%05u\n\r\n\r%s\n\r%s\n\r\n\r *** MicroTronics product REGISTRATION and ORDER FORM ***\r\n%s\n\r",
                help_program_name, program_revision, serial_r1, serial_r2,
                company_line, mailing_line, registration_notice_pointer);
        dz_dosi_printf("\n\rCheck your MSDOS floppy disk format : 5.25(__)  3.5(__)\n\r  DSHD [High Density] (__)  DSDD [Low Density] (__)\n\r\n\rWhen registering this shareware program you will receive a credit for\n\ran update disk. Registered users receive our regular mailings providing\n\rthe most current information about program enhancements and new products.\n\r\n\rCheck here if you want to receive a program disk now,\n\rinstead of receiving a credit for your future use : (__)\n\r\r\nCheck here if interested in our retail DOS and Windows self activating\r\n Screen Saver products : (__)\r\n\n\rCheck here if interested in Multi-Copy or Site License prices : (__)\n\r\n\rCheck here to receive information about MT-DAZZLE Music Videos, shirts,\n\r mugs, cards, prints, etc. : (__)\r\n\r\nPlease review the User's Manual for an abundance of useful information,\n\r and legal limitations on the use of this program and it's images.\r\nEnter \"%s -?\" at the DOS prompt for a short help screen, and instructions on extracting the manual.\r\n\014", help_program_name);
        textcolor(3);
        dz_dosi_printf("\r\n An order form has been provided.\r\n Please address it as follows:\r\n\n");
    }
    textcolor(3);
    dz_dosi_printf("%s", address_line);
    textcolor(7);
    dz_dosi_printf("%s \r\n", compuserve_line);
    textcolor(15);
    dz_dosi_printf("%s", company_line);
    textcolor(7);
    dz_dosi_printf("%s \r\n", phone_line);
    textcolor(15);
    dz_dosi_printf("%s \r\n", mailing_line);
}

/* Displays command-line help and waits between its main and additional option lists.
 * Original 11C1:0BED, 345 bytes. */
void command_line_help(void) {
    /* DOS fcloseall has no process-wide native equivalent. */
    textcolor(7);
    dz_dosi_printf("\r Usage: %s -options inputfile\n\r", help_program_name);
    textcolor(11);
    dz_dosi_printf("       -$  Display order information.\r\n");
    textcolor(3);
    dz_dosi_printf("       -2  2 part palette update to fix flickering screens (slower).\r\n");
    dz_dosi_printf("       -a  Alternate (faster) color cycling speed.\r\n");
    dz_dosi_printf("       -b  Bashful, no closing credits & any key aborts.\r\n       -c  Compatible mode, use BIOS for better portability.\r\n       -d  Delay, slow down drawing for faster PC's.\r\n           \"-d=xxxx\" delay xxxx units.\r\n       -i  Insecure, no antiviral checking.\r\n       -m  Mad (fastest) color cycling speed.\r\n       -n  No retract of hard disk heads.\r\n       -q  Quiet, no tones (no effect on warning beeps).\r\n       -s  Show mode, quick single image.\r\n           \"-s=xxxx\" Show mode, duration xxxx seconds.\r\n       -w  Window environment, no messages & changes some defaults.\r\n       -x  XT compatible, inferior but works.\r\n");
    textcolor(11);
    dz_dosi_printf("       -z  Create DAZZLE.DOC file on current disk path!\r\n");
    textcolor(7);
    dz_dosi_printf("\r\nDuring the display the TAB key will FREEZE the display till SPACE is pressed.\r\nF1 activates on-line help during the program. F2 provides a configuration menu.\r\n");
    textcolor(12);
    dz_dosi_printf("ESCAPE");
    textcolor(7);
    dz_dosi_printf(" terminates the program.\r\n");
    textcolor(14);
    dz_dosi_printf(" ** Refer to DAZZLE.DOC for far more details **\r\n");
    textcolor(7);
    dz_dosi_printf("Press any key to continue\r");
    textcolor(3);
    if (!dz_options.headless) getch();
    textcolor(7);
    dz_dosi_printf("If an \"inputfile\" is not specified, the program will try to locate\r\nand load %s from the current path. To prevent this load\r\nspecify an inputfile name of \"*\".\r\n\n", default_config_name);
    textcolor(3);
    dz_dosi_printf(" Additional Options\r\n       -e  EGA operation mode forced.\r\n       -h  Hide EGA border.\r\n       -k  Keyboard typematic unchanged.\r\n       -t  Tall video aspect ratio mode.\r\n\n");
    textcolor(2);
    dz_dosi_printf("\376\376\376\376\376 limited use command line options \376\376\376\376\376\r\n       -+  Ignore TARGA+64, even if present.\r\n       -!  Monitor Screen Saver driver interface.\r\n       -4  Restrict the EGA mode to only 4 random colors.\r\n       -f  Fixed colors instead of randomized.\r\n       -v  VGA operation mode forced.\r\n\n");
    print_order_info(0);
    exit(10);
}

/* Prints closing and registration text unless Bashful mode is set, then exits with the supplied
 * status.
 * Original 11C1:0D46, 63 bytes. */
void print_closing_text_and_exit(int16_t status) {
    if (!bashful_mode) {
        dz_dosi_printf("\nThis program may be distributed as is AT NO CHARGE by the distributor\n(excluding reasonable duplication cost). All commercial rights retained\nby the author! This program must not be altered in any way, and is offered\nas is without warranty. Refer to User's Manual for usage limitations and\nroyalty requirements.\n\nEnter \"DAZZLE -Z\" at the DOS prompt to generate the User's Manual.\nEnter \"DAZZLE -$ > PRN:\" at the DOS prompt to print an order form.\n\n%s\nWhen you write be sure to mention your floppy disk format and this program's\nversion of %s \n\n", registration_notice_pointer, program_revision);
        print_order_info(0);
    }
    dz_testctl_finish();
    exit(status ? status : dz_testctl_failed());
}

/* Uppercases a proposed file name, rejects EXE/BAT/COM extensions, and substitutes the default
 * config name.
 * Original 11C1:0D85, 130 bytes. */
int16_t reject_exec_filename(char *filename) {
    int16_t rejected;
    rejected = 0;
    char upper_name[80];
    snprintf(upper_name, sizeof upper_name, "%s", filename);
    dz_dosi_upper(upper_name);
    if (strstr(upper_name, ".EXE") || strstr(upper_name, ".BAT") ||
        strstr(upper_name, ".COM")) {
        menu_error_message("EXE, COM, and BAT are illegal extentions for File Name!");
        strcpy(filename, default_config_name);
        rejected = 1;
    }
    return rejected;
}

/* Sounds a tone for the requested duration unless sound is muted.
 * Original 11C1:0E07, 35 bytes. */
void beep(uint16_t hz, uint16_t ms) {
    if (!muted) {
        dz_sound(hz);
        dz_delay_ms(ms);
        dz_nosound();
    }
}

/* Selects the EGA set/reset write mode used for XOR drawing.
 * Original 11C1:0E2A, 27 bytes. */
/* Native raster operation is represented by dz_video.ega_xor. */
void set_ega_raster_xor(int16_t raster_mode) {
    dz_video.ega_xor = raster_mode != 0;
}

/* Fills the EGA/VGA palette buffer using random, fixed, or cycling color rules, then advances
 * the plot color.
 * Original 11C1:0E45, 884 bytes. */
void generate_palette(void) {
    uint16_t palette_index;
    int16_t channel_level[3];
    int16_t channel_phase[3];
    int16_t channel_period[3];
    int16_t channel_direction[3];

    palette_base_green = 10;
    palette_base_blue = 12;
    palette_buffer[0] = (uint8_t)(background_fill_word & 0xff);
    palette_buffer[1] = (uint8_t)palette_base_green;
    palette_buffer[2] = (uint8_t)palette_base_blue;

    if (palette_random_mode) {
        for (palette_index = 3; palette_index < palette_entries; ++palette_index)
            palette_buffer[palette_index] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
    } else {
        channel_period[0] = dz_random((uint16_t)(color_count - 4)) + 4;
        channel_period[1] = dz_random((uint16_t)(color_count - 4)) + 4;
        channel_period[2] = dz_random((uint16_t)(color_count - 4)) + 4;
        if (palette_alternate_mode) {
            if (channel_period[0] >= 8) channel_period[0] >>= 1;
            if (channel_period[1] >= 8) channel_period[1] >>= 1;
            if (channel_period[2] >= 8) channel_period[2] >>= 1;
        }
        channel_phase[0] = channel_phase[1] = channel_phase[2] = 0;
        if (palette_fixed_mode) {
            channel_period[0] = (int16_t)color_count;
            channel_period[1] = (int16_t)(color_count / 2);
            channel_period[2] = channel_level[0] = channel_level[1] = channel_level[2] = 0;
        } else {
            channel_level[0] = dz_random(4);
            channel_level[1] = dz_random(4);
            channel_level[2] = dz_random(4);
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
        palette_buffer[1] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
        palette_buffer[2] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
        palette_buffer[3] = (uint8_t)dz_random((uint16_t)(palette_entries + 1));
        for (palette_index = 4; palette_index < palette_entries; ++palette_index)
            palette_buffer[palette_index] = palette_buffer[palette_index & 3];
    }
    memmove(palette_buffer + palette_index, palette_buffer + 3, color_count);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
}

/* Seeds the reserved EGA palette bytes and optionally replaces the background colors.
 * Original 11C1:11B9, 401 bytes. */
void randomize_palette_state(uint16_t replace_base_colors) {
    uint8_t seed;

    if (dz_random(20) && !replace_base_colors)
        seed = 0;
    else
        seed = (uint8_t)(dz_random(16) + 0xef);
    memset(palette_buffer, seed, 9);
    if (replace_base_colors) {
        palette_buffer[0] = (uint8_t)dz_random(seed);
        palette_buffer[1] = (uint8_t)dz_random(seed);
        palette_buffer[2] = (uint8_t)dz_random(seed);
    }
    palette_buffer[4] = (uint8_t)dz_random(ega_green_limit);
    palette_buffer[5] = (uint8_t)(ega_green_limit + dz_random(ega_green_limit));
    palette_buffer[6] = (uint8_t)dz_random(ega_blue_limit);
    palette_buffer[8] = (uint8_t)ega_green_limit;
}

/* Chooses a new background palette and uploads it through the active adapter callback.
 * Original 11C1:134A, 165 bytes. */
void change_background_palette(void) {
    if (video_adapter == 8) {
        randomize_palette_state(1);
        upload_reserved_dac_entries(3);
    } else {
        palette_buffer[0] = (uint8_t)(dz_random(8) + 8);
        palette_buffer[1] = (uint8_t)(palette_base_green = (uint16_t)(dz_random(8) + 8));
        palette_buffer[2] = (uint8_t)(palette_base_blue = (uint16_t)(dz_random(8) + 8));
    }
    palette_upload_callback(palette_buffer + palette_cycle_offset);
}

/* Builds a changing EGA palette from channel phases and amplitudes while servicing input and
 * palette timing.
 * Original 11C1:13EF, 1089 bytes. */
void build_ega_palette(int16_t update_cycle) {
    int16_t palette_index;
    uint16_t sample_index;
    uint16_t channel_amplitude[3];
    uint16_t channel_value[3];
    uint16_t channel_phase[3];
    float channel_frequency[3];

    keyboard_led_flags(0x40, 0);
    if (palette_fixed_mode) {
        for (palette_index = 0; palette_index <= 2; ++palette_index) {
            channel_phase[palette_index] = 0;
            channel_amplitude[palette_index] = ega_red_limit;
        }
    } else {
        for (palette_index = 0; palette_index <= 2; ++palette_index) {
            channel_phase[palette_index] = (uint16_t)dz_random((uint16_t)color_count);
            if (dz_random(3))
                channel_amplitude[palette_index] = ega_red_limit;
            else
                channel_amplitude[palette_index] = (uint16_t)(ega_red_limit / 2 + dz_random((uint16_t)(ega_red_limit / 2)));
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
    if (dz_random(50)) {
        for (sample_index = 0, palette_index = 9; palette_index < palette_entries * 3; ++sample_index) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            channel_value[0] = (uint16_t)((float)channel_amplitude[0] *
                cos((channel_phase[0] + sample_index) * channel_frequency[0]) + (channel_amplitude[0] + 1));
            channel_value[1] = (uint16_t)((float)channel_amplitude[1] *
                cos((channel_phase[1] + sample_index) * channel_frequency[1]) + (channel_amplitude[1] + 1));
            channel_value[2] = (uint16_t)((float)channel_amplitude[2] *
                cos((channel_phase[2] + sample_index) * channel_frequency[2]) + (channel_amplitude[2] + 1));
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)(channel_value[0] > ega_green_limit ? channel_value[0] - ega_green_limit : 0);
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)(channel_value[1] > ega_green_limit ? channel_value[1] - ega_green_limit : 0);
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)(channel_value[2] > ega_green_limit ? channel_value[2] - ega_green_limit : 0);
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    } else {
        for (sample_index = 0, palette_index = 9; palette_index < palette_entries * 3; ++sample_index) {
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return;
            }
            channel_value[0] = (uint16_t)((float)ega_green_limit * cos(sample_index * channel_frequency[0]) + ega_green_limit);
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)channel_value[0];
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)channel_value[0];
            palette_buffer[(uint16_t)palette_index++] = (uint8_t)channel_value[0];
            if (update_cycle && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
    }
    memmove(palette_buffer + palette_index, palette_buffer + 9, (size_t)color_count * 3u);
    ++plot_color;
    if (plot_color >= color_count)
        plot_color = FIRST_CYCLING_COLOR;
    keyboard_led_flags(0x40, 1);
}

/* The scanner's starts at 15CB, 1626, 1681, and 1780 lie inside the
   floating-point expressions of build_ega_palette, not separate functions. */

/* Freezes or resumes display work and palette cycling, using menu state and keyboard LED state.
 * Original 11C1:1830, 183 bytes. */
int16_t freeze_display(uint16_t operation) {
    if (menu_active)
        operation = 2;

    switch (operation) {
    case 0:
        if (display_freeze_active)
            return (int16_t)(display_freeze_cycle_requested = !display_freeze_cycle_requested);
        display_freeze_active = 1;
        freeze_led_state = keyboard_led_flags(16, 0);
        while (display_freeze_active) {
            dz_service();
            dz_dosi_checkpoint();
            if (input_pending || exit_requested) {
                if (poll_graphics_keys())
                    return (int16_t)exit_requested;
            }
            if (display_freeze_cycle_requested && palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        return (int16_t)exit_requested;
    case 1:
        if (display_freeze_active && !exit_requested)
            beep(440, 10);
        /* fall through */
    default:
        if (display_freeze_active)
            keyboard_led_flags(freeze_led_state, 2);
        return (int16_t)(display_freeze_cycle_requested = display_freeze_active = 0);
    }
}

/* Dispatches pending graphics keyboard and mouse actions, updating menu, palette, image, mode,
 * and exit state.
 * Original 11C1:18E7, 1149 bytes. */
int16_t poll_graphics_keys(void) {
    dz_dosi_checkpoint();
    if (input_pending)
        --input_pending;
    if (mouse_abort_pending) {
        mouse_abort_pending = 0;
        if (startup_active)
            return dos_word(exit_requested);
        beep(1200, 5);
        return (int16_t)(exit_requested = 1);
    }
    if (exit_requested || bashful_mode)
        return (int16_t)(exit_requested = 1);
    if (!dz_bioskey(1))
        return dos_word(exit_requested);

    current_graphics_key = (uint16_t)dz_bioskey(0);
    if (((uint8_t *)&current_graphics_key)[0]) {
        ((uint8_t *)&current_graphics_key)[0] = (uint8_t)tolower(((uint8_t *)&current_graphics_key)[0] & 0x7f);
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
                return dos_word(exit_requested);
            }
            return dos_word(exit_requested);
        case '-':
            current_graphics_key = previous_graphics_key;
            if (palette_cycle_ticks > 2)
                palette_cycle_ticks += palette_cycle_ticks >> 1;
            else
                ++palette_cycle_ticks;
            if (palette_cycle_ticks > 0x40) {
                palette_cycle_ticks = 0x40;
                beep(190, 20);
                return dos_word(exit_requested);
            }
            return dos_word(exit_requested);
        case 9:
            freeze_display(0);
            current_graphics_key = previous_graphics_key;
            return dos_word(exit_requested);
        case ' ':
            freeze_display(1);
            current_graphics_key = previous_graphics_key;
            return dos_word(exit_requested);
        case 'n':
            freeze_display(1);
            previous_graphics_key = current_graphics_key;
            return dos_word(exit_requested);
        default:
            previous_graphics_key = current_graphics_key;
            return dos_word(exit_requested);
        case 10:
        case 13:
            previous_graphics_key = current_graphics_key;
            if (menu_active || startup_active)
                return dos_word(exit_requested);
            /* fall through */
        case 27:
            previous_graphics_key = 0;
            return (int16_t)(exit_requested = 1);
        }
    }
    /* Normalize scan-code ranges to the compact image-selection key byte. */
    /* When ASCII is unavailable, map scan-code ranges to compact image-selection bytes. */
    if (show_mode || palette_callback_active) {
        current_graphics_key = previous_graphics_key;
        return dos_word(exit_requested);
    } else if (bios_key_scan_code >= 0x78 && bios_key_scan_code <= 0x81) {
        ((uint8_t *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0x88);
        previous_graphics_key = current_graphics_key;
    } else if (bios_key_scan_code >= 0x10 && bios_key_scan_code <= 0x19) {
        ((uint8_t *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0xfa);
        previous_graphics_key = current_graphics_key;
    } else if (bios_key_scan_code >= 0x1e && bios_key_scan_code <= 0x26) {
        ((uint8_t *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0xf6);
        previous_graphics_key = current_graphics_key;
    } else if (bios_key_scan_code >= 0x2c && bios_key_scan_code <= 0x32) {
        ((uint8_t *)&current_graphics_key)[0] = 0x80 | (bios_key_scan_code + 0xf1);
        previous_graphics_key = current_graphics_key;
        if ((((uint8_t *)&current_graphics_key)[0] & 0x7f) >= alt_image_count)
            previous_graphics_key = current_graphics_key = 0;
        return dos_word(exit_requested);
    } else {
        switch (current_graphics_key) {
        case BIOS_KEY_F1:
            previous_graphics_key = current_graphics_key = 0;
            help_screen();
            freeze_display(2);
            return dos_word(exit_requested);
        case BIOS_KEY_F2:
            previous_graphics_key = current_graphics_key = 0;
            configuration_screen();
            freeze_display(2);
            return dos_word(exit_requested);
        case BIOS_KEY_F5:
            if (startup_active || menu_active)
                current_graphics_key = previous_graphics_key;
            else {
                save_pcx();
                current_graphics_key = previous_graphics_key;
                return dos_word(exit_requested);
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
                return dos_word(exit_requested);
            }
            return dos_word(exit_requested);
        case BIOS_KEY_F9:
            if (semi_auto_mode)
                previous_graphics_key = current_graphics_key;
            else
                current_graphics_key = previous_graphics_key;
            return dos_word(exit_requested);
        case BIOS_KEY_F10:
            if (semi_auto_mode)
                previous_graphics_key = current_graphics_key;
            else
                current_graphics_key = previous_graphics_key;
            return dos_word(exit_requested);
        case BIOS_KEY_HOME:
        case BIOS_KEY_LEFT:
            previous_graphics_key = current_graphics_key = BIOS_KEY_HOME;
            return dos_word(exit_requested);
        case BIOS_KEY_PAGE_UP:
            if (!menu_active && video_adapter == 8) {
                current_graphics_key = previous_graphics_key;
                if (palette_step < 0x39)
                    palette_step += 3;
                if (palette_step >= 0x39) {
                    palette_step = 0x39;
                    beep(210, 20);
                    return dos_word(exit_requested);
                }
                return dos_word(exit_requested);
            }
            /* fall through */
        case BIOS_KEY_UP:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_UP;
            return dos_word(exit_requested);
        case BIOS_KEY_RIGHT:
        case BIOS_KEY_END:
            previous_graphics_key = current_graphics_key = BIOS_KEY_END;
            return dos_word(exit_requested);
        case BIOS_KEY_PAGE_DOWN:
            if (!menu_active) {
                current_graphics_key = previous_graphics_key;
                if (palette_step > 3)
                    palette_step -= 3;
                if (palette_step <= 3) {
                    palette_step = 3;
                    beep(190, 20);
                    return dos_word(exit_requested);
                }
                return dos_word(exit_requested);
            }
            /* fall through */
        case BIOS_KEY_DOWN:
            previous_graphics_key = current_graphics_key = BIOS_KEY_PAGE_DOWN;
            return dos_word(exit_requested);
        case BIOS_KEY_CTRL_F10:
            if (menu_active) {
                current_graphics_key = previous_graphics_key;
            } else {
                beep(1500, 10);
                change_background_palette();
                beep(1000, 5);
                current_graphics_key = previous_graphics_key;
            }
            return dos_word(exit_requested);
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
            return dos_word(exit_requested);
        case BIOS_KEY_ALT_F9:
            semi_auto_mode = !semi_auto_mode;
            previous_graphics_key = current_graphics_key = 'n';
            if (semi_auto_mode) {
                freeze_display(2);
                beep(880, 20);
            } else
                beep(440, 20);
            return dos_word(exit_requested);
        case BIOS_KEY_ALT_F10:
            if (semi_auto_mode) {
                previous_graphics_key = current_graphics_key;
                return dos_word(exit_requested);
            }
            /* fall through */
        default:
            current_graphics_key = previous_graphics_key;
            break;
        }
    }
    return dos_word(exit_requested);
}

/* Fills the 64-entry sine and cosine lookup table used by drawing routines.
 * Original 11C1:1D64, 113 bytes. */

void initialize_math_tables(void) {
    float angle;
    register int16_t i;

    angle = 0.0;
    for (i = 0; i < 64; ++i) {
        trig_table[i].cosine = (float)cos(angle);
        trig_table[i].sine = (float)sin(angle);
        angle = (float)(angle + 0.098174770421875);
    }
}

/* Fills a supplied pixel map with a circular mask and distance-based color indices starting at
 * three.
 * Original 11C1:1DD5, 340 bytes. */

void initialize_palette_mapping(PaletteMapSpec *mapping) {
    int16_t map_half_height = (int16_t)(mapping->height / 2);
    int16_t map_half_width = (int16_t)(mapping->width / 2);
    int16_t map_limit_y = (int16_t)(map_half_height + map_half_height / 4);
    int16_t map_limit_x = (int16_t)(map_half_width + map_half_width / 4);
    uint16_t radius = (uint16_t)((map_half_height < map_half_width ? map_half_height : map_half_width) - 1);
    uint16_t map_radius_squared = (uint16_t)((uint32_t)radius * radius);
    for (uint16_t y = 0; y < mapping->height; ++y) {
        uint16_t distance_y = (uint16_t)abs(map_half_height - y);
        uint16_t y_squared = (uint16_t)((uint32_t)distance_y * distance_y);
        uint16_t pixel_row_offset = (uint16_t)((uint32_t)mapping->width * y);
        for (uint16_t x = 0; x < mapping->width; ++x) {
            int16_t dx = dos_word((uint16_t)abs(map_half_width - x));
            dx = dos_word((uint16_t)((int32_t)dx * dx));
            uint16_t offset = (uint16_t)(pixel_row_offset + x);
            if ((uint16_t)(y_squared + (uint16_t)dx) > map_radius_squared) {
                mapping->pixels[offset] = 0;
            } else {
                dx = dos_word((uint16_t)abs(map_limit_y - y));
                int16_t dy = dos_word((uint16_t)abs(map_limit_x - x));
                float color_distance = 0;
                if (dx || dy)
                    color_distance = (float)sqrt((double)(uint16_t)((uint32_t)((int32_t)dx * dx) +
                        (uint32_t)((int32_t)dy * dy)));
                uint8_t color_index = (uint8_t)((int16_t)color_distance % 253);
                if (color_count < 0x80) color_index >>= 1;
                mapping->pixels[offset] = (uint8_t)(color_index + FIRST_CYCLING_COLOR);
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
    exit_requested = 1;
    display_cleared = 1;
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
    dz_delay_ms(1);
}

/* Rounds a floating-point frequency and sounds it for the supplied duration.
 * Original 11C1:1FBF, 48 bytes. */

void beep_float(float hz, uint16_t ms) {
    int16_t frequency_value;
    if (muted) return;
    frequency_value = dos_word((uint16_t)(uint32_t)(hz + 0.5));
    dz_sound((uint16_t)frequency_value);
    dz_delay_ms(ms);
    dz_nosound();
}

/* Builds a frequency table and plays the startup chord and melody until input or mouse abort.
 * Original 11C1:1FEF, 370 bytes. */
void play_tone_sequence(void) {
    float notes[8][12];
    int16_t note;
    register int16_t octave;
    register int16_t melody;

    if (muted)
        return;
    for (octave = 0; octave < 8; ++octave) {
        for (note = 0; note < 12; ++note) {
            notes[octave][note] = tone_base_frequency;
            tone_base_frequency = (float)(tone_base_frequency * 1.0594645);
        }
    }
    if (mouse_abort_pending || dz_bioskey(1)) {
        dz_nosound();
        return;
    }

    for (octave = 7; octave > 1; --octave) {
        if (mouse_abort_pending || dz_bioskey(1)) {
            dz_nosound();
            return;
        }
        for (note = 0; note < 4; ++note)
            beep_float(notes[octave][tone_chord[note]], 80);
    }

    if (mouse_abort_pending || dz_bioskey(1)) {
        dz_nosound();
        return;
    }
    for (melody = 0; melody < 7; ++melody) {
        beep_float(notes[4][0], 80);
        beep_float(notes[4][2], 80);
    }
    if (!mouse_abort_pending && !dz_bioskey(1))
        beep_float(notes[4][0], 600);
    dz_nosound();
}

/* Draws the closing image, optionally plays music, and waits for a key, timeout, or mouse abort.
 * Original 11C1:2161, 69 bytes. */

void closing_graphic_and_music(void) {
    if (!logo_image_header) {
        dz_dosi_printf("DAZZLE: closing logo unavailable; skipping.\n");
        return;
    }
    draw_closing_graphic();
    dz_present();
    if (dz_testctl_stopped()) return;
    input_pending = (mouse_abort_pending = 0);
    countdown_ticks = 182;
    if (!window_mode) {
        play_tone_sequence();
        dz_delay_ms(1000);
    }
    while (!dz_bioskey(1) && countdown_ticks > 0 && !mouse_abort_pending)
        dz_service();
}

/* Selects adapter modes, drawing/palette callbacks, dimensions, and allocates palette and image
 * work buffers.
 * Original 11C1:21A6, 879 bytes. */

void configure_video_backend(int16_t allocate) {
    register int16_t ball_scale_divisor;

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
            mode_setup_callback = vga_set_mode_320x200;
            plot_callback = vga_plot_symmetric_320x200;
            video_backend = BACKEND_VGA_320X200;
            read_pixel_callback = vga_read_pixel_320x200;
            screen_extent = screen_half_width + 1;
        } else {
            screen_half_height = 200;
            mode_setup_callback = vga_set_mode_320x400;
            plot_callback = vga_plot_symmetric_320x400;
            video_backend = BACKEND_VGA_320X400;
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
            plot_callback = ega_plot_symmetric_640x350;
            video_backend = BACKEND_EGA_640X350;
            read_pixel_callback = ega_read_pixel_640x350;
            screen_half_height = 175;
            screen_extent = screen_half_width + 1;
        } else {
            plot_callback = ega_plot_symmetric_640x700;
            video_backend = BACKEND_EGA_640X700;
            read_pixel_callback = ega_read_pixel_640x700;
            screen_half_height = 350;
            screen_extent = screen_half_height + 1;
        }
        if (use_bios_palette)
            palette_upload_callback = ega_upload_palette_bios;
        else
            palette_upload_callback = ega_upload_palette_ports;
        palette_build_callback = dz_build_ega_adapter;
    }
    maximum_image_size = (uint16_t)(0xffffu / (uint16_t)(screen_extent + (screen_extent >> 2)) - 1u);
    palette_cycle_wrap_bytes = palette_entries * 3;
    if (!allocate) return;

    if (video_adapter == 4)
        palette_buffer_bytes = color_count + palette_entries;
    else
        palette_buffer_bytes = (uint16_t)((color_count + palette_entries) * 3);
    palette_buffer = (uint8_t *)malloc(palette_buffer_bytes + 3);
    if (!palette_buffer) {
        exit_requested = 1;
        dz_dosi_printf("\r Memory error: EPAL - ABORT\n\a");
        print_closing_text_and_exit(0x3c);
    }
    linewipe_history = malloc(0x7d8);
    if (!linewipe_history) { exit_requested = 1; print_closing_text_and_exit(0x46); }
    ball_scale_divisor = (int16_t)(dz_random(5) + 2);
    ball_bitmap_height = (uint16_t)(screen_half_height / ball_scale_divisor);
    ball_bitmap_width = (uint16_t)(screen_half_width / ball_scale_divisor);
    ball_bitmap_pixels = malloc((size_t)ball_bitmap_height * ball_bitmap_width);
    if (!ball_bitmap_pixels) { exit_requested = 1; print_closing_text_and_exit(0x47); }
    /* Original overlay starts at DS:5C4F (height), then DS:5C51 (width).
     * BALL samples x*height+y, so this intentionally builds the transposed map. */
    PaletteMapSpec mapping = {ball_bitmap_height, ball_bitmap_width, ball_bitmap_pixels};
    initialize_palette_mapping(&mapping);
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

int16_t parse_config_version(char *line) {
    char *version_text;
    uint16_t version;

    version_text = strchr(line, '#');
    if (!version_text) return 1;
    ++version_text;
    if (!parse_word(version_text, &version)) {
        menu_error_message("Corrupted version data in the input file");
        return 1;
    }
    if (version < 10410)
        menu_error_message("WARNING: bad version in the input file");
    return 0;
}

/* Reads a '~' image key and bracketed size and updates the matching Alt-image record.
 * Original 11C1:25D6, 244 bytes. */

void parse_alt_image_size(char *line) {
    int16_t configured_size;
    uint16_t parsed_size;
    char *setting_text;
    char image_key;
    uint16_t image_index;

    if (!(setting_text = strchr(line, '~'))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &image_key) != 1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    if (!parse_word(setting_text, &parsed_size)) return;
    configured_size = dos_word(parsed_size);
    if ((uint16_t)configured_size > maximum_image_size) configured_size = dos_word(maximum_image_size);
    if (configured_size < 0) configured_size = 0;
    for (image_index = 0; image_index < alt_image_count; ++image_index) {
        if ((char)alt_image_records[image_index].key == image_key) {
            alt_image_records[image_index].size = (uint16_t)configured_size;
            break;
        }
    }
}

/* Reads a '^' image key and bracketed size and updates the matching Ctrl-image record.
 * Original 11C1:26CA, 244 bytes. */

void parse_ctrl_image_size(char *line) {
    int16_t configured_size;
    uint16_t parsed_size;
    char *setting_text;
    char image_key;
    uint16_t image_index;

    if (!(setting_text = strchr(line, '^'))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &image_key) != 1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    if (!parse_word(setting_text, &parsed_size)) return;
    configured_size = dos_word(parsed_size);
    if ((uint16_t)configured_size > maximum_image_size) configured_size = dos_word(maximum_image_size);
    if (configured_size < 0) configured_size = 0;
    for (image_index = 0; image_index < ctrl_image_count; ++image_index) {
        if ((char)ctrl_image_records[image_index].key == image_key) {
            ctrl_image_records[image_index].size = (uint16_t)configured_size;
            break;
        }
    }
}

/* Reads a fade key and bracketed y/n value and updates the matching fade enable flag.
 * Original 11C1:27BE, 256 bytes. */

void parse_fade_enabled(char *line) {
    char *setting_text;
    char fade_key;
    char enable_letter;
    uint16_t fade_index;

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

void parse_auto_configuration(char *line) {
    int16_t setting_value;
    uint16_t parsed_value;
    char *setting_text;
    char setting_key;

    if (!(setting_text = strchr(line, '$'))) return;
    ++setting_text;
    while (*setting_text && (*setting_text == ' ' || *setting_text == '\t')) ++setting_text;
    if (*setting_text == 0) return;
    if (sscanf(setting_text, "%c", &setting_key) != 1)
        return;
    if (!(setting_text = strchr(setting_text, '['))) return;
    ++setting_text;
    if (!parse_word(setting_text, &parsed_value)) return;
    setting_value = dos_word(parsed_value);
    if (setting_value > 256) setting_value = 256;
    if (setting_value < 0) setting_value = 1;
    switch (setting_key) {
    case 'i':
        minimum_images_per_display = (uint16_t)setting_value;
        break;
    case 'x':
        maximum_images_per_display = (uint16_t)setting_value;
        break;
    case 'f':
        auto_fade_pause_ticks = (uint16_t)(setting_value * 18);
        break;
    }
}

/* Reads a text configuration file, removes comments, normalizes each line, and dispatches its
 * settings.
 * Original 11C1:29AE, 406 bytes. */

int16_t load_configuration_file(char *path) {
    char line[80];
    char *line_cursor;

    if (!(configuration_file_stream = fopen(path, "rt")))
        return 1;
    for (;;) {
        if (!fgets(line, 80, configuration_file_stream)) {
            clearerr(configuration_file_stream);
            if (dz_dosi_close_configuration() != 0)
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
        if (!dz_dosi_lower(line_cursor))
            continue;
        switch (*line_cursor) {
        case '#':
            if (parse_config_version(line_cursor)) {
                fclose(configuration_file_stream); configuration_file_stream = NULL;
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

int16_t save_configuration_file(char *path) {
    uint16_t record_index;
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
            fclose(configuration_file_stream); configuration_file_stream = NULL;
            return 1;
        }
    }
    for (record_index = 0; record_index < ctrl_image_count; ++record_index) {
        if (fprintf(configuration_file_stream, "^%c [%03d] ; %s image size\n", (char)ctrl_image_records[record_index].key,
                    ctrl_image_records[record_index].size,
                    ctrl_image_records[record_index].name) == -1) {
            fclose(configuration_file_stream); configuration_file_stream = NULL;
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
            fclose(configuration_file_stream); configuration_file_stream = NULL;
            return 1;
        }
    }
    if (fprintf(configuration_file_stream, "$I [%03d] ; mInimum image count in Auto Mode\n", minimum_images_per_display) == -1) {
        fclose(configuration_file_stream); configuration_file_stream = NULL;
        return 1;
    }
    if (fprintf(configuration_file_stream, "$X [%03d] ; maXimum image count in Auto Mode\n", maximum_images_per_display) == -1) {
        fclose(configuration_file_stream); configuration_file_stream = NULL;
        return 1;
    }
    if (fprintf(configuration_file_stream, "$F [%03d] ; Fade delay time in Auto Mode\n", (int16_t)auto_fade_pause_ticks / 18) == -1) {
        fclose(configuration_file_stream); configuration_file_stream = NULL;
        return 1;
    }
    fclose(configuration_file_stream); configuration_file_stream = NULL;
    return 0;
}

/* Resolves the requested/default config path, rejects executable names, and loads settings when
 * available.
 * Original 11C1:2D26, 455 bytes. */

int16_t resolve_startup_configuration(int16_t requested) {
    char located_config_path[80];
    int config_search_result;
    if (requested < 0) return 0;
    config_search_result = !dz_dosi_file_exists(configuration_filename);
    if (config_search_result && requested) {
        dz_dosi_printf(config_load_error_text, configuration_filename);
        dz_delay_ms(1250); return 1;
    }
    if (config_search_result) {
        if (!dz_dosi_find_config(default_config_name, located_config_path, sizeof located_config_path)) return 0;
        strcpy(configuration_filename, located_config_path);
    }
    if (reject_exec_filename(configuration_filename)) {
        dz_dosi_printf("\n Illegal configuration file name ignored.\n");
        strcpy(configuration_filename, default_config_name);
        dz_delay_ms(1250); return 1;
    }
    if (load_configuration_file(configuration_filename) && requested > 0) {
        dz_dosi_printf(config_load_error_text, configuration_filename);
        dz_delay_ms(1250); return 1;
    }
    textcolor(9);
    if (!window_mode && !bashful_mode) dz_dosi_printf("\r Loaded %s\n\r", configuration_filename);
    return 0;
}

/* Parses concatenated command-line options, optional numeric values, and a config path, then
 * checks CPU mode.
 * Original 11C1:2EED, 1202 bytes. */
/* Walks far argv strings, accepts concatenated switch characters, and handles
   optional numeric values, one input path, and the documented application flags. */

int16_t parse_command_line(int16_t argc, char * *argv,
                       int16_t *option_position) {
    register int16_t argument_index;
    int16_t config_result;
    int16_t machine_mode;
    uint16_t numeric_value;
    char option_character;
    char configured_switch_character;
    register int16_t option_character_index;

    config_result = 0;
    *option_position = -1;
    strcpy(configuration_filename, default_config_name);
    configured_switch_character = '/';
    if (argc >= 1) {
        for (argument_index = 1; argument_index < argc; ++argument_index) {
            if (*argv[argument_index] == '-' || *argv[argument_index] == configured_switch_character) {
                /* Leave caller argv storage untouched. */
                option_character_index = 1;
                while ((option_character = argv[argument_index][option_character_index]) != 0) {
                    if (option_character == configured_switch_character)
                        option_character = '-';
                    switch (tolower(option_character)) {
                    default:
                        dz_dosi_printf("\r unknown option /%c \n", option_character);
                        /* fall through */
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
                            if (!parse_word(argv[argument_index] + option_character_index, &numeric_value)) {
                                dz_dosi_printf("\r unable to parse %s\n", argv[argument_index]);
                                command_line_help();
                            }
                            draw_delay_count = (uint16_t)numeric_value;
                            if (draw_delay_count < 1)
                                draw_delay_count = 1;
                            if (draw_delay_count > 0x1fff) {
                                dz_dosi_printf("\r Delay time limited to 8191 units!\n\a");
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
                        insecure_argument_index = (uint16_t)argument_index;
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
                            if (!parse_word(argv[argument_index] + option_character_index, &numeric_value)) {
                                dz_dosi_printf("\r unable to parse %s\n", argv[argument_index]);
                                command_line_help();
                            }
                            show_duration_ticks = (uint16_t)numeric_value;
                            if (show_duration_ticks > 3600) {
                                dz_dosi_printf("\r ShowTime limited to 3600 seconds!\n\a");
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
                snprintf(configuration_filename, sizeof configuration_filename, "%s", argv[argument_index]);
                if (strchr(configuration_filename, '?'))
                    command_line_help();
                if (!strchr(configuration_filename, '*')) {
                    if (!strchr(configuration_filename, '.'))
                        strncat(configuration_filename, ".DZL", sizeof configuration_filename - strlen(configuration_filename) - 1);
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
            dz_dosi_printf("\r Forcing the /X option for the detected CPU\n");
            use_8086_palette_io = 1;
        }
    }
    return config_result;
}

/* Prints program revision and copyright, adapter mode, screen-saver status, and the help hint
 * when enabled.
 * Original 11C1:339F, 410 bytes. */

void print_startup_information(void) {
    size_t line_width;

    if (show_mode || window_mode || (bashful_mode && no_disk_park))
        return;
    textcolor(13);
    dz_dosi_printf("\r %s ", help_program_name);
    line_width = strlen(help_program_name) + 2;
    textcolor(15);
    dz_dosi_printf("%s%s", revision_label, program_revision);
    line_width += strlen(revision_label) + strlen(program_revision);
    line_width = line_width + strlen(copyright_text);
    while (line_width < 0x4f) {
        dz_dosi_printf(" ");
        ++line_width;
    }
    dz_dosi_printf("%s", copyright_text);
    textcolor(7);
    dz_dosi_printf("\r\n video mode (");
    if (video_adapter == 4)
        dz_dosi_printf("EGA)");
    if (targa64_mode)
        dz_dosi_printf("TARGA+64)");
    else if (video_adapter == 8)
        dz_dosi_printf("VGA)");
    if (mouse_callback_installed) {
        textcolor(6);
        highvideo();
        if (current_image_header[5] & 0x8000)
            dz_dosi_printf("\r\n ACTIVE");
        else
            dz_dosi_printf("\r\n INACTIVE");
        lowvideo();
        dz_dosi_printf(" Screen Saver Driver detected.");
        if (mouse_available)
            dz_dosi_printf(" Mouse Disabled.");
    }
    textcolor(7);
    dz_dosi_printf("\r\n Type '%s -?' at the DOS prompt for help.\n\r", help_program_name);
}

/* Runs Semi-Auto selection and drawing, handling image reinitialization, symmetry, XOR, and
 * continuous mode.
 * Original 11C1:3539, 906 bytes. */

void semi_auto_loop(void) {
    register uint16_t previous_image_index;

    previous_image_index = 0xffff;
    current_image_index = (uint16_t)dz_random((uint16_t)(alt_image_count + ctrl_image_count + 1));
    if (!display_cleared) {
        previous_graphics_key = current_graphics_key = 110;
        display_boundary_fade_and_pan();
    }
    semi_auto_continuous = 0;
    while (input_pending) {
        dz_dosi_checkpoint();
        if (poll_graphics_keys())
            return;
    }
    current_graphics_key = previous_graphics_key = 0;
    if (semi_auto_mode)
        mode_ready_chirp();

    while (semi_auto_mode && !exit_requested) {
        while (!input_pending && !semi_auto_continuous && !exit_requested) {
            dz_service();
            dz_dosi_checkpoint();
            if (palette_elapsed_ticks >= palette_cycle_ticks)
                cycle_palette();
        }
        dz_dosi_checkpoint();
        if (poll_graphics_keys())
            return;

        if (current_graphics_key && (uint16_t)current_graphics_key <= ctrl_image_count) {
            current_image_index = current_graphics_key + alt_image_count;
            current_image_draw = ctrl_image_records[(uint16_t)(current_graphics_key - 1)].draw;
            current_graphics_key = BIOS_KEY_ALT_F10;
        } else if (((uint8_t)current_graphics_key & 0x7f) < alt_image_count) {
            if (current_graphics_key & 0x80) {
                current_image_index = (uint8_t)current_graphics_key & 0x7f;
                current_image_draw = alt_image_records[current_image_index].draw;
                current_graphics_key = BIOS_KEY_ALT_F10;
            }
        }
        if (!current_graphics_key && semi_auto_continuous)
            current_graphics_key = BIOS_KEY_F8;


        switch (current_graphics_key) {
        case BIOS_KEY_F9:
            current_image_index = (uint16_t)dz_random((uint16_t)(alt_image_count + ctrl_image_count + 1));
            if (current_image_index < alt_image_count)
                current_image_draw = alt_image_records[current_image_index].draw;
            else
                current_image_draw = dz_dosi_image_record(current_image_index)->draw;
            /* fall through */

        case BIOS_KEY_ALT_F10:
            diagonal_symmetry_selected = (uint16_t)dz_random(2);
            if (current_image_index < alt_image_count) {
                image_xor_mode = alt_image_records[current_image_index].continuous &&
                    dz_random(5) == 0 ? 1 : 0;
            } else {
                image_xor_mode = dz_dosi_image_record(current_image_index)->continuous &&
                    dz_random(5) == 0 ? 1 : 0;
            }
            set_ega_raster_xor((int16_t)image_xor_mode);
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
                draw_symmetry = dz_dosi_image_record(current_image_index)->enabled &&
                         diagonal_symmetry_selected ? 3 : 1;
            }
            if (previous_image_index != current_image_index) {
                if (current_image_index < alt_image_count) {
                    current_image_draw = alt_image_records[current_image_index].draw;
                } else {
                    current_image_draw = dz_dosi_image_record(current_image_index)->draw;
                }
                current_image_draw(1);
                previous_image_index = current_image_index;
            }
            while (!input_pending && !exit_requested) {
                dz_dosi_checkpoint();
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
    register int16_t remaining_overlays;
    register uint16_t image_search_attempts;

    current_image_index = 0;
    image_work_count = 1;
    if (minimum_images_per_display < 1)
        minimum_images_per_display = 1;
    if (maximum_images_per_display < minimum_images_per_display)
        maximum_images_per_display = minimum_images_per_display;
    remaining_overlays = dos_word((uint16_t)(minimum_images_per_display +
        dz_random((uint16_t)(maximum_images_per_display + 1 - minimum_images_per_display))));
    while (!exit_requested && !semi_auto_mode) {
        dz_dosi_checkpoint();
        if (exit_requested) return;
        if (dz_dosi_next_work_count() > image_work_limit || current_graphics_key == 110) {
            image_work_count = 0;

            current_image_index = (uint16_t)dz_random((uint16_t)(alt_image_count + ctrl_image_count + 1));
            /* Skip disabled image records, but bound the search if none remain configured. */
            image_search_attempts = 0;
            for (;;) {
                if (current_image_index < alt_image_count) {
                    if (alt_image_records[current_image_index].size > 0)
                        break;
                } else if (dz_dosi_image_record(current_image_index)->size > 0) {
                    break;
                }
                ++current_image_index;
                current_image_index = (uint16_t)(current_image_index % (uint16_t)(alt_image_count + ctrl_image_count + 1));
                if (image_search_attempts++ > alt_image_count + ctrl_image_count + 2) {
                    startup_config_failure = exit_requested = 1;
                    return;
                }
            }

            diagonal_symmetry_selected = (uint16_t)dz_random(2);
            if (current_image_index < alt_image_count) {
                image_work_limit = (uint16_t)(dz_random((uint16_t)((uint32_t)alt_image_records[current_image_index].size * (uint16_t)screen_extent)) +
                    ((uint16_t)((uint32_t)alt_image_records[current_image_index].size * (uint16_t)screen_extent) >> 2));
            } else {
                image_work_limit = (uint16_t)(dz_random((uint16_t)((uint32_t)dz_dosi_image_record(current_image_index)->size * (uint16_t)screen_extent)) +
                    ((uint16_t)((uint32_t)dz_dosi_image_record(current_image_index)->size *
                                (uint16_t)screen_extent) >> 2));
            }
            if (--remaining_overlays <= 0 || current_graphics_key == 110) {
                display_boundary_fade_and_pan();
                remaining_overlays = dos_word((uint16_t)(minimum_images_per_display +
                    dz_random((uint16_t)(maximum_images_per_display + 1 - minimum_images_per_display))));
                if (palette_alternate_mode)
                    remaining_overlays += dz_random(minimum_images_per_display);
                if (palette_random_mode)
                    remaining_overlays += dz_random(maximum_images_per_display);
                if (display_pan_state == 2 && remaining_overlays > 8)
                    remaining_overlays = 9;
            }

            if (current_image_index < alt_image_count) {
                image_xor_mode = alt_image_records[current_image_index].continuous &&
                    dz_random((uint16_t)(remaining_overlays + 1)) == 0 ? 1 : 0;
            } else {
                image_xor_mode = dz_dosi_image_record(current_image_index)->continuous &&
                    dz_random((uint16_t)(remaining_overlays + 1)) == 0 ? 1 : 0;
            }
            if (input_pending && (poll_graphics_keys() || semi_auto_mode))
                return;
            set_ega_raster_xor((int16_t)image_xor_mode);
            if (current_image_index < alt_image_count) {
                current_image_draw = alt_image_records[current_image_index].draw;
                draw_symmetry = alt_image_records[current_image_index].enabled &&
                         diagonal_symmetry_selected ? 3 : 1;
            } else {
                current_image_draw = dz_dosi_image_record(current_image_index)->draw;
                /* The original also indexes the Alt table in this Ctrl branch. */
                draw_symmetry = dz_dosi_auto_symmetry(current_image_index) &&
                         diagonal_symmetry_selected ? 3 : 1;
            }
            current_image_draw(1);
            plot_callback(draw_symmetry);
            display_cleared = 0;
            dz_dosi_printf("\r");

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
uint16_t ega_red_limit; /* DS:743C */
uint16_t ega_blue_limit; /* DS:743A */
uint16_t ega_green_limit; /* DS:7438 */

/* Reserved startup-cleared storage at DS:7452..7456; ownership provisional. */
uint8_t reserved_program_state[4];
