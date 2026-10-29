/* DAZPCX: native PCX save from the modeled framebuffer.
 * Original function order and recovered addresses are retained. */
#include "dazzle.h"
#include "dzconst.h"

#include <errno.h>

struct pcx_header {
    uint8_t manufacturer, version, encoding, bits_per_pixel;
    uint16_t xmin, ymin, xmax, ymax, horizontal_res, vertical_res;
    uint8_t palette[48], reserved, planes;
    uint16_t bytes_per_line;
    uint8_t remainder[60];
};

_Static_assert(sizeof(struct pcx_header) == PCX_HEADER_BYTES, "PCX header must be 128 bytes");

extern struct pcx_header *pcx;
extern uint8_t *video_memory, *pcx_palette;
extern FILE *pcx_file;
extern int16_t pcx_number;
extern char pcx_name_format[], binary_mode[], create_error[], number_error[];
extern char stream_error[], header_error[], palette_error[];

static int pcx_write_failed;

static uint8_t rgb_channel(uint8_t value)
{
    switch (value) {
    case 1: return 0x55;
    case 2: return 0xaa;
    case 3: return 0xff;
    default: return value;
    }
}

static void pcx_put_byte(uint8_t value)
{
    if (pcx_file == NULL || fputc(value, pcx_file) == EOF)
        pcx_write_failed = 1;
}

static void pcx_write_bytes(const void *bytes, size_t length)
{
    if (pcx_file == NULL || fwrite(bytes, 1, length, pcx_file) != length)
        pcx_write_failed = 1;
}

static void pcx_beep(uint16_t hz, uint16_t milliseconds)
{
    dz_sound(hz);
    dz_delay_ms(milliseconds);
    dz_nosound();
}

/* Returns the DAZPCX code segment. Native builds have no DOS segment value.
 * Original 19F0:0004, 3 bytes. */
uint16_t dazpcx_seg(void) { return 0; }

/* Expands the shared palette bytes to VGA DAC values in the PCX palette buffer.
 * Original 19F0:0007, 70 bytes. */
void pcx_prepare_vga_palette(void)
{
    if (pcx_palette == NULL)
        return;
    for (uint16_t i = 0; i < PCX_VGA_PALETTE_BYTES - 1u; ++i) {
        uint8_t value;
        if (palette_buffer != NULL) {
            uint16_t source_index = i < 9u ? i : (uint16_t)(palette_cycle_offset + i);
            value = palette_buffer[source_index];
        } else {
            uint16_t entry = (uint16_t)(i / 3u);
            uint16_t channel = (uint16_t)(i % 3u);
            value = dz_video.dac[entry][channel];
        }
        pcx_palette[i] = (uint8_t)(value << 2);
    }
}

/* Converts the background, reserved colors and cycling palette to EGA PCX RGB.
 * Original 19F0:004D, 405 bytes. */
void pcx_prepare_ega_palette(void)
{
    if (pcx == NULL)
        return;
    uint16_t index = 0;
    uint16_t source_index = 0;
    pcx->palette[index++] = (uint8_t)(((background_fill_word & 32u) >> 5) |
        ((background_fill_word & 4u) >> 1));
    pcx->palette[index++] = (uint8_t)(((background_fill_word & 16u) >> 4) |
        (background_fill_word & 2u));
    pcx->palette[index++] = (uint8_t)(((background_fill_word & 8u) >> 3) |
        ((background_fill_word & 1u) << 1));

    const uint16_t reserved_colors[2] = {palette_base_green, palette_base_blue};
    for (; index < 9; index = (uint16_t)(index + 3u), ++source_index) {
        uint16_t color = reserved_colors[source_index];
        pcx->palette[index] = (uint8_t)(((color & 32u) >> 5) | ((color & 4u) >> 1));
        pcx->palette[index + 1u] = (uint8_t)(((color & 16u) >> 4) | (color & 2u));
        pcx->palette[index + 2u] = (uint8_t)(((color & 8u) >> 3) | ((color & 1u) << 1));
    }
    for (source_index = (uint16_t)(palette_cycle_offset + 3u); index < 48; ++source_index) {
        uint8_t color = palette_buffer != NULL ? palette_buffer[source_index] : 0;
        pcx->palette[index++] = (uint8_t)(((color & 32u) >> 5) | ((color & 4u) >> 1));
        pcx->palette[index++] = (uint8_t)(((color & 16u) >> 4) | (color & 2u));
        pcx->palette[index++] = (uint8_t)(((color & 8u) >> 3) | ((color & 1u) << 1));
    }
    for (index = 0; index < 48; ++index)
        pcx->palette[index] = rgb_channel(pcx->palette[index]);
}

/* Initializes and writes the 128-byte PCX header for the active display.
 * Original 19F0:01E2, 196 bytes. */
void pcx_write_header(void)
{
    memset(pcx, 0, sizeof *pcx);
    pcx->manufacturer = 10;
    pcx->version = 5;
    pcx->encoding = 1;
    if (video_backend == BACKEND_EGA_640X350 || video_backend == BACKEND_EGA_640X700) {
        pcx->bits_per_pixel = 1;
        pcx->xmax = 639;
        pcx->ymax = 349;
        pcx->horizontal_res = 640;
        pcx->vertical_res = 350;
        pcx->planes = 4;
        pcx->bytes_per_line = 80;
        pcx_prepare_ega_palette();
    } else {
        pcx->bits_per_pixel = 8;
        pcx->xmax = 319;
        pcx->horizontal_res = 320;
        pcx->planes = 1;
        pcx->bytes_per_line = 320;
        if (video_backend == BACKEND_VGA_320X200) {
            pcx->ymax = 199;
            pcx->vertical_res = 200;
        } else {
            pcx->ymax = 399;
            pcx->vertical_res = 400;
        }
    }
    pcx_write_bytes(pcx, sizeof *pcx);
}

/* Encodes one PCX row with DAZZLE's observed run choices.
 * Original 19F0:02A6, 216 bytes. */
int16_t pcx_encode_row(uint8_t *row, uint8_t after_row)
{
    uint16_t byte_index = 0;
    uint16_t encoded_count = 0;
    do {
        uint16_t repeat_count = 0;
        while ((uint32_t)byte_index + repeat_count < pcx->bytes_per_line && repeat_count < PCX_MAX_RUN_LENGTH) {
            uint32_t left_index = (uint32_t)byte_index + repeat_count;
            uint8_t next = left_index + 1u < pcx->bytes_per_line ? row[left_index + 1u] : after_row;
            if (row[left_index] != next)
                break;
            ++repeat_count;
        }
        if (repeat_count > 0) {
            pcx_put_byte((uint8_t)(repeat_count | PCX_RLE_MARKER));
            pcx_put_byte(row[byte_index]);
            byte_index = (uint16_t)(byte_index + repeat_count);
            encoded_count = (uint16_t)(encoded_count + 2u);
        } else {
            if ((row[byte_index] & PCX_RLE_MARKER_MASK) == PCX_RLE_MARKER) {
                pcx_put_byte(PCX_SINGLE_LITERAL_RUN);
                ++encoded_count;
            }
            pcx_put_byte(row[byte_index++]);
            ++encoded_count;
        }
    } while (byte_index < pcx->bytes_per_line);
    return (int16_t)encoded_count;
}

static uint8_t plane_byte(const uint8_t *row, uint16_t byte_index, uint16_t plane)
{
    uint8_t bits = 0;
    for (uint16_t bit = 0; bit < 8; ++bit) {
        uint8_t pixel = row[(size_t)byte_index * 8u + bit];
        if ((pixel & (uint8_t)(1u << plane)) != 0)
            bits |= (uint8_t)(0x80u >> bit);
    }
    return bits;
}

/* Selects each EGA plane in turn and encodes its scanlines to the PCX stream.
 * Original 19F0:037E, 169 bytes. */
void pcx_write_ega_rows(void)
{
    uint8_t plane_row[80 + 1];
    for (uint16_t y = 0; y < pcx->vertical_res; ++y) {
        const uint8_t *pixel_row = (const uint8_t *)video_memory + (size_t)y * 640u;
        for (uint16_t plane = 0; plane < 4; ++plane) {
            for (uint16_t b = 0; b < 80; ++b)
                plane_row[b] = plane_byte(pixel_row, b, plane);
            plane_row[80] = plane_byte(pixel_row + 640u, 0, plane);
            (void)pcx_encode_row(plane_row, plane_row[80]);
        }
    }
}

/* Encodes linear VGA scanlines and appends the 256-color palette marker/data.
 * Original 19F0:0427, 97 bytes. */
void pcx_write_linear_rows(void)
{
    for (uint16_t y = 0; y < pcx->vertical_res; ++y) {
        const uint8_t *row = (const uint8_t *)video_memory + (size_t)y * 320u;
        uint8_t next_row = row[320];
        (void)pcx_encode_row((uint8_t *)row, next_row);
    }
    pcx_put_byte(PCX_VGA_PALETTE_MARKER);
    pcx_write_bytes(pcx_palette, PCX_VGA_PALETTE_BYTES - 1u);
}

/* Gathers VGA scanlines in pixel order and appends the palette.
 * Original 19F0:0488, 387 bytes. */
void pcx_write_planar_rows(void)
{
    for (uint16_t y = 0; y < pcx->vertical_res; ++y) {
        uint8_t *row = (uint8_t *)video_memory + (size_t)y * 320u;
        /* The DOS helper read beyond its malloc row at this boundary. */
        (void)pcx_encode_row(row, 0);
    }
    pcx_put_byte(PCX_VGA_PALETTE_MARKER);
    pcx_write_bytes(pcx_palette, PCX_VGA_PALETTE_BYTES - 1u);
}

static void pcx_report_failure(const char *message)
{
    restore_video_state(1);
    fprintf(stderr, "%s", message);
    exit_requested = 1;
}

/* Creates the next PCX file from the active native framebuffer.
 * Original 19F0:060B, 540 bytes. */
void save_pcx(void)
{
    char output_filename[14];
    pcx_write_failed = 0;
    for (;;) {
        (void)snprintf(output_filename, sizeof output_filename, pcx_name_format, pcx_number++);
        FILE *existing = fopen(output_filename, "rb");
        if (existing != NULL) {
            fclose(existing);
            if (pcx_number >= 9999) {
                pcx_report_failure(number_error);
                return;
            }
            continue;
        }
        pcx_file = fopen(output_filename, binary_mode);
        break;
    }
    if (pcx_file == NULL) {
        pcx_report_failure(create_error);
        return;
    }
    pcx = (struct pcx_header *)malloc(sizeof *pcx);
    if (pcx == NULL) {
        fclose(pcx_file);
        pcx_file = NULL;
        pcx_report_failure(header_error);
        return;
    }
    pcx_write_header();
    pcx_beep(600, 55);

    uint32_t base_index = 0;
    if (display_pan_state <= 1u) {
        display_start_callback(0, 0);
    } else {
        uint16_t y = video_backend == BACKEND_EGA_640X350 || video_backend == BACKEND_EGA_640X700
            ? 350u : (uint16_t)(screen_half_height * 2);
        display_start_callback(0, y);
        uint16_t plane_offset = (uint16_t)(page_paragraph * 16u);
        base_index = dz_vram_index(plane_offset);
    }
    video_memory = dz_video.vram + base_index;

    if (video_backend == BACKEND_EGA_640X350 || video_backend == BACKEND_EGA_640X700) {
        pcx_write_ega_rows();
    } else {
        pcx_palette = (uint8_t *)malloc(PCX_VGA_PALETTE_BYTES - 1u);
        if (pcx_palette == NULL) {
            free(pcx);
            pcx = NULL;
            fclose(pcx_file);
            pcx_file = NULL;
            pcx_report_failure(palette_error);
            return;
        }
        pcx_prepare_vga_palette();
        if (video_backend == BACKEND_VGA_320X200)
            pcx_write_linear_rows();
        else
            pcx_write_planar_rows();
        free(pcx_palette);
        pcx_palette = NULL;
    }

    free(pcx);
    pcx = NULL;
    if (fclose(pcx_file) == EOF)
        pcx_write_failed = 1;
    pcx_file = NULL;
    pcx_beep(300, 40);
    if (pcx_write_failed)
        pcx_report_failure(stream_error);
}

/* Uninitialized module state, then PCX serial and diagnostics. */
struct pcx_header *pcx; /* DS:7762 */
uint8_t *video_memory; /* DS:775E */
FILE *pcx_file; /* DS:775A */
uint8_t *pcx_palette; /* DS:7756 */
int16_t pcx_number = 0;
char memory_error[] = "malloc error for hptr in DAZPCX.\a\n";
char pcx_name_format[] = "daz_%04d.pcx";
char create_error[] = "Sorry, can't create output PCX file.\a\n";
char number_error[] = "Sorry, PCX filename range exceeded.\a\n";
char binary_mode[] = "wb";
char stream_error[] = "Sorry, can't open output PCX file.\a\n";
char header_error[] = "malloc error for stdhdr in PackPcxFile.\a\n";
char palette_error[] = "malloc error for vga_palette in PackPcxFile.\a\n";
