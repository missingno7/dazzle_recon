/* DAZZLE 5.0 DLOGO; native resource and framebuffer port.
 * Original function order and recovered addresses are retained. */
#include "dazzle.h"
#include "dzconst.h"

#include <limits.h>

#define LOGO_PALETTE_BYTES 0x300u
#define LOGO_PALETTE_MARKER_OFFSET 0x301u
#define LOGO_PALETTE_MARKER 0x0cu
#define LOGO_PALETTE_FIELD_OFFSET 0x10u
#define LOGO_BACKGROUND_FIELD_OFFSET 0x20u
#define LOGO_PLANE_COUNT_OFFSET 0x41u
#define LOGO_FRAME_COUNT_OFFSET 0x4au
#define LOGO_XMIN_OFFSET 0x04u
#define LOGO_YMIN_OFFSET 0x06u
#define LOGO_XMAX_OFFSET 0x08u
#define LOGO_YMAX_OFFSET 0x0au
#define LOGO_PLANES_OFFSET 0x41u
#define LOGO_BYTES_PER_LINE_OFFSET 0x42u
#define LOGO_RESOURCE_SIZE_OFFSET 0x46u
#define LOGO_FRAME_COUNT_FIELD_OFFSET 0x4au
#define LOGO_IMAGE_BUFFER_EXTRA 0x15eu
#define LOGO_EGA_PALETTE_BYTES 0x11u
#define VGA_DAC_ENTRY_COUNT 0x100u
#define LOGO_HEADER_BYTES 128u

static uint8_t *logo_resource;
static size_t logo_resource_size;
static size_t logo_stream_end;
static size_t logo_stream_position;
static uint8_t logo_vga_palette[LOGO_PALETTE_BYTES];
static enum dz_video_mode logo_previous_mode;

static uint16_t read_u16le(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8));
}

static uint32_t read_u32le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int read_resource_base(FILE *input, uint32_t *base)
{
    uint8_t mz_header[0x1c];
    if (fseek(input, 0, SEEK_SET) != 0 || fread(mz_header, sizeof mz_header, 1, input) != 1 ||
        mz_header[0] != 'M' || mz_header[1] != 'Z')
        return 0;
    uint16_t last_page_bytes = read_u16le(mz_header + 2);
    uint16_t page_count = read_u16le(mz_header + 4);
    if (page_count == 0)
        return 0;
    /* DAZZLE's read_mz_resource_base formula includes the separator byte. */
    *base = (uint32_t)(page_count - 1u) * 512u + last_page_bytes + 1u;
    return 1;
}

/* The source diagnostic returned the DOS code segment.
 * Native builds have no DOS segment value. Original 17F5:0008, 3 bytes. */
uint16_t dlogo_seg(void) { return 0; }

/* Reads embedded image metadata and loads the requested image resource.
 * Original 17F5:000B, 580 bytes. */
int16_t load_logo_image(uint16_t options)
{
    (void)options;
    if (logo_resource != NULL)
        return 0;
    if (dz_options.original_exe == NULL)
        return 1;

    FILE *input = fopen(dz_options.original_exe, "rb");
    if (input == NULL)
        return 1;
    uint32_t base;
    uint8_t metadata[10];
    int result = 1;
    if (!read_resource_base(input, &base) || base > (uint32_t)LONG_MAX ||
        fseek(input, (long)base, SEEK_SET) != 0 || fread(metadata, sizeof metadata, 1, input) != 1)
        goto done;

    uint32_t image_offset = base + 10u + read_u16le(metadata + 8);
    if (image_offset > (uint32_t)LONG_MAX || fseek(input, (long)image_offset, SEEK_SET) != 0)
        goto done;

    uint8_t header[LOGO_HEADER_BYTES];
    if (fread(header, sizeof header, 1, input) != 1)
        goto done;
    uint32_t resource_size = read_u32le(header + LOGO_RESOURCE_SIZE_OFFSET);
    if (resource_size < LOGO_HEADER_BYTES || resource_size > 0x100000u)
        goto done;

    logo_resource = (uint8_t *)malloc(resource_size);
    if (logo_resource == NULL)
        goto done;
    memcpy(logo_resource, header, sizeof header);
    if (fread(logo_resource + sizeof header, resource_size - sizeof header, 1, input) != 1) {
        free(logo_resource);
        logo_resource = NULL;
        goto done;
    }
    logo_resource_size = resource_size;
    result = 0;

done:
    fclose(input);
    return (int16_t)result;
}

/* Resolves the original executable path, loads image data and reports missing data.
 * Original 17F5:024F, 192 bytes. */
int16_t load_image_from_path(uint16_t options)
{
    return load_logo_image(options);
}

/* Uploads the requested VGA DAC entries from the native palette buffer.
 * Original 17F5:030F, 62 bytes. */
void upload_reserved_dac_entries(uint16_t count)
{
    if (count > VGA_DAC_ENTRY_COUNT)
        count = VGA_DAC_ENTRY_COUNT;
    if (palette_buffer != NULL) {
        for (uint16_t i = 0; i < count; ++i) {
            dz_video.dac[i][0] = (uint8_t)(palette_buffer[(uint16_t)(i * 3u)] & 63u);
            dz_video.dac[i][1] = (uint8_t)(palette_buffer[(uint16_t)(i * 3u + 1u)] & 63u);
            dz_video.dac[i][2] = (uint8_t)(palette_buffer[(uint16_t)(i * 3u + 2u)] & 63u);
        }
    }
}

/* Shifts the closing image's 768-byte VGA palette to six-bit values.
 * Original 17F5:034D, 168 bytes. */
void dim_logo_pixels(void)
{
    if (logo_resource == NULL || logo_resource_size < LOGO_PALETTE_MARKER_OFFSET + LOGO_PALETTE_BYTES) {
        exit_requested = 1;
        return;
    }
    size_t palette_offset = logo_resource_size - LOGO_PALETTE_BYTES;
    if (logo_resource[palette_offset - 1u] != LOGO_PALETTE_MARKER) {
        exit_requested = 1;
        return;
    }
    for (size_t i = 0; i < LOGO_PALETTE_BYTES; ++i)
        logo_vga_palette[i] = (uint8_t)(logo_resource[palette_offset + i] >> 2);
    logo_stream_end = palette_offset - 1u;
}

/* Converts the image header's EGA palette and clears its background field.
 * Original 17F5:03F5, 92 bytes. */
void prepare_logo_chunk(void)
{
    if (logo_image_header == NULL)
        return;
    uint8_t ega_palette[16];
    rgb8_to_ega_palette(ega_palette,
        (uint8_t *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET, 16);
    memcpy((uint8_t *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET, ega_palette, sizeof ega_palette);
    logo_image_header->reserved[LOGO_BACKGROUND_FIELD_OFFSET] = 0;
}

/* Expands PCX run-length bytes into one plane row while preserving row spill.
 * Original 17F5:0451, 203 bytes. */
void decode_logo_run(uint8_t *destination)
{
    if (destination == NULL || logo_resource == NULL || logo_image_header == NULL)
        return;

    uint16_t decoded_count = 0;
    uint16_t row_bytes = (uint16_t)logo_image_header->output_size;
    if (logo_pending_output_offset != 0) {
        decoded_count = (uint16_t)(logo_pending_output_offset - row_bytes);
        logo_pending_output_offset = 0;
    }
    do {
        if (logo_stream_position >= logo_stream_end) {
            exit_requested = 1;
            return;
        }
        uint8_t encoded = logo_resource[logo_stream_position++];
        if ((encoded & PCX_RLE_MARKER_MASK) == PCX_RLE_MARKER) {
            uint8_t repeat_count = (uint8_t)(encoded & PCX_MAX_RUN_LENGTH);
            if (logo_stream_position >= logo_stream_end) {
                exit_requested = 1;
                return;
            }
            uint8_t value = logo_resource[logo_stream_position++];
            while (repeat_count-- != 0)
                destination[decoded_count++] = value;
        } else {
            destination[decoded_count++] = encoded;
        }
    } while (decoded_count < row_bytes);

    logo_pending_output_offset = decoded_count > row_bytes ? decoded_count : 0;
    logo_input_buffer_offset = (uint16_t)logo_stream_position;
}

/* Selects VGA/EGA setup and decodes the closing image to native video memory.
 * Original 17F5:051C, 467 bytes. */
void draw_closing_graphic(void)
{
    if (logo_resource == NULL || logo_image_header == NULL || logo_image_height == 0)
        return;

    logo_previous_aspect = bios_resolution_mode;
    logo_previous_mode = dz_video.mode;
    logo_input_buffer_index = 0;
    logo_input_buffer_offset = LOGO_HEADER_BYTES;
    logo_pending_output_offset = 0;
    logo_stream_position = LOGO_HEADER_BYTES;

    if (image_frame_count != 0 && video_adapter == ADAPTER_VGA) {
        bios_resolution_mode = 1;
        dz_video_set_mode(logo_image_height == 200 ? DZ_MODE_VGA_200 : DZ_MODE_VGA_400, 1);
        for (uint16_t row = 0; row < logo_image_height; ++row) {
            decode_logo_run(dz_video.vram + (uint32_t)row * (uint32_t)dz_video.width);
            if (exit_requested)
                return;
        }
        for (uint16_t i = 0; i < VGA_DAC_ENTRY_COUNT; ++i) {
            dz_video.dac[i][0] = logo_vga_palette[(size_t)i * 3u];
            dz_video.dac[i][1] = logo_vga_palette[(size_t)i * 3u + 1u];
            dz_video.dac[i][2] = logo_vga_palette[(size_t)i * 3u + 2u];
        }
        return;
    }

    if (video_adapter != ADAPTER_EGA) {
        logo_restore_video_adapter = 1;
        logo_previous_video_adapter = video_adapter;
        video_adapter = ADAPTER_EGA;
        configure_video_backend(0);
    }
    ega_init_row_table_and_mode();
    configure_logo_dac();

    uint16_t bytes_per_plane = (uint16_t)logo_image_header->output_size;
    size_t row_size = (size_t)bytes_per_plane * 4u;
    uint8_t *planar_row = (uint8_t *)malloc(row_size);
    if (planar_row == NULL) {
        exit_requested = 1;
        return;
    }
    for (uint16_t row = 0; row < logo_image_height; ++row) {
        for (uint16_t plane = 0; plane < 4; ++plane)
            decode_logo_run(planar_row + (size_t)plane * bytes_per_plane);
        if (exit_requested)
            break;
        ega_write_planar_row(planar_row, bytes_per_plane, row);
    }
    free(planar_row);
}

/* Loads and validates the embedded image and selects VGA/EGA palette handling.
 * Original 17F5:06EF, 638 bytes. */
void prepare_logo_image_resources(void)
{
    if (load_image_from_path(1) != 0) {
        fprintf(stderr, "DAZZLE: cannot load logo from original executable\n");
        exit_requested = 1;
        return;
    }
    if (logo_resource_size < LOGO_HEADER_BYTES || logo_resource[0] != 10 ||
        logo_resource[1] < 5 || logo_resource[2] != 1) {
        fprintf(stderr, "DAZZLE: invalid logo image header\n");
        exit_requested = 1;
        return;
    }

    uint16_t width = (uint16_t)(read_u16le(logo_resource + LOGO_XMAX_OFFSET) -
        read_u16le(logo_resource + LOGO_XMIN_OFFSET) + 1u);
    uint16_t height = (uint16_t)(read_u16le(logo_resource + LOGO_YMAX_OFFSET) -
        read_u16le(logo_resource + LOGO_YMIN_OFFSET) + 1u);
    uint8_t planes = logo_resource[LOGO_PLANES_OFFSET];
    uint16_t bytes_per_line = read_u16le(logo_resource + LOGO_BYTES_PER_LINE_OFFSET);
    uint32_t declared_size = read_u32le(logo_resource + LOGO_RESOURCE_SIZE_OFFSET);
    if (width == 0 || height == 0 || declared_size != logo_resource_size ||
        ((planes == 4 && read_u16le(logo_resource + LOGO_FRAME_COUNT_FIELD_OFFSET) != 0) ||
         (planes == 1 && read_u16le(logo_resource + LOGO_FRAME_COUNT_FIELD_OFFSET) != 1)) ||
        ((uint32_t)bytes_per_line * 8u != width && planes == 4) ||
        (planes != 1 && planes != 4)) {
        fprintf(stderr, "DAZZLE: unsupported logo image dimensions or planes\n");
        exit_requested = 1;
        return;
    }

    logo_image_header = (struct image_header *)logo_resource;
    logo_image_height = height;
    image_source_bytes = (uint32_t)logo_resource_size;
    image_frame_count = planes == 1 ? 1u : 0u;
    image_frame_size = 0;
    image_start_offset = (uint16_t)logo_resource_size;
    logo_input_buffers[0] = (char *)logo_resource;
    logo_input_buffers[1] = NULL;
    logo_input_buffer_index = 0;
    logo_input_buffer_offset = LOGO_HEADER_BYTES;
    logo_input_buffer_size = (uint16_t)(logo_resource_size > UINT16_MAX ? UINT16_MAX : logo_resource_size);
    logo_pending_output_offset = 0;
    logo_stream_position = LOGO_HEADER_BYTES;
    logo_stream_end = logo_resource_size;

    if (planes == 1) {
        if (width != 320 || bytes_per_line != 320 || (height != 200 && height != 400)) {
            fprintf(stderr, "DAZZLE: unsupported VGA logo dimensions\n");
            exit_requested = 1;
            return;
        }
        if (logo_resource_size < LOGO_HEADER_BYTES + LOGO_PALETTE_MARKER_OFFSET + LOGO_PALETTE_BYTES) {
            exit_requested = 1;
            return;
        }
        dim_logo_pixels();
    } else {
        if (width != 640 || height != 350 || bytes_per_line != 80) {
            fprintf(stderr, "DAZZLE: unsupported EGA logo dimensions\n");
            exit_requested = 1;
            return;
        }
        prepare_logo_chunk();
    }
}

/* Restores the previous adapter/aspect, clears and uploads the shared palette.
 * Original 17F5:096D, 116 bytes. */
void restore_logo_video_state(void)
{
    if (logo_restore_video_adapter || (video_adapter == ADAPTER_VGA && image_frame_count != 0)) {
        if (logo_restore_video_adapter)
            video_adapter = logo_previous_video_adapter;
        bios_resolution_mode = logo_previous_aspect;
        configure_video_backend(0);
        dz_video_set_mode(logo_previous_mode, 0);
    }
    if (palette_buffer != NULL)
        memset(palette_buffer, 0, palette_buffer_bytes);
    ega_border_color = 0;
    background_fill_word = 0;
    palette_cycle_offset = 0;
    if (palette_upload_callback != NULL && palette_buffer != NULL)
        palette_upload_callback(palette_buffer);
    logo_restore_video_adapter = 0;
}

/* Copies the image's 17-byte EGA palette into the native attribute palette.
 * Original 17F5:09E1, 100 bytes. */
void configure_logo_dac(void)
{
    if (logo_image_header == NULL)
        return;
    const uint8_t *palette = (const uint8_t *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET;
    memcpy(dz_video.ega_attr, palette, 16);
    dz_video.ega_border = palette[16] & 15u;
}

/* Uninitialized module state. Original storage order is retained. */
struct image_header *logo_image_header; /* DS:747C */
uint16_t logo_image_height; /* DS:747A */
uint16_t logo_previous_aspect; /* DS:7478 */
uint16_t logo_previous_video_adapter; /* DS:7476 */
uint16_t logo_frame_state[2]; /* DS:7472 */
uint16_t image_frame_count; /* DS:7470 */
uint16_t logo_input_buffer_index; /* DS:746E */
uint16_t logo_input_buffer_offset; /* DS:746C */
uint16_t logo_decode_state; /* DS:746A */
uint16_t image_start_offset; /* DS:7468 */
uint16_t image_frame_size; /* DS:7466 */
uint16_t logo_plane_state[2]; /* DS:7462 */
char *logo_input_buffers[2]; /* DS:745A */
uint32_t image_source_bytes; /* DS:7456 */

/* DLOGO initialized state. */
uint16_t logo_reserved = 0;
uint16_t logo_restore_video_adapter = 0;
uint16_t logo_input_buffer_size = 0xFE9E;
uint16_t logo_pending_output_offset = 0;
