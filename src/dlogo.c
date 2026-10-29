/* DAZZLE 5.0 DLOGO; BC++ 3.00 -ml -O1, merged literal pool. */
#pragma option -d
#include "dazzle.h"
#include "dzconst.h"

#define BIOS_SET_DAC_BLOCK_FUNCTION 0x1012
#define BIOS_SET_EGA_PALETTE_FUNCTION 0x1002
#define LOGO_PALETTE_BYTES 0x300
#define LOGO_PALETTE_MARKER_OFFSET 0x301
#define LOGO_PALETTE_MARKER 0x0c
#define LOGO_SCANLINE_BYTES 0x140
#define LOGO_PALETTE_FIELD_OFFSET 0x10
#define LOGO_BACKGROUND_FIELD_OFFSET 0x20
#define LOGO_PLANE_COUNT_OFFSET 0x41
#define LOGO_FRAME_COUNT_OFFSET 0x4a
#define LOGO_YMAX_FIELD_OFFSET 0x0a
#define LOGO_YMIN_FIELD_OFFSET 0x06
#define LOGO_IMAGE_BUFFER_EXTRA 0x15e
#define LOGO_EGA_PALETTE_BYTES 0x11
#define VGA_DAC_ENTRY_COUNT 0x100


/* The static getter is covered by the complete 17F5 TU proof. */
/* Returns the DLOGO code segment.
 * Original 17F5:0008, 3 bytes. */
unsigned dlogo_seg(void)
{
    return _CS;
}

/* The following low level calls are external Borland/DAZZLE helpers. */


/* Reads embedded image metadata and loads the requested header or image-data chunks into
 * buffers.
 * Original 17F5:000B, 580 bytes. */
int far load_logo_image(unsigned options)
{
    unsigned char pcx_header_bytes[PCX_HEADER_BYTES];
    register unsigned load_flags = options;

    image_frame_size = image_frame_count = 0;
    image_source_bytes = image_start_offset = 0;
    if ((image_stream = fopen(executable_path, "rb")) == 0)
        fatal_selfcheck_error(0, "deIRead can't open file");

    if (load_flags & 1) {
        read_mz_resource_base(image_stream);
        if (fseek(image_stream, image_file_position, 0) != 0)
            fatal_selfcheck_error(0, "deIRead file seek error looking for serialize data");
        if (fread(&expected_crc, 10, 1, image_stream) != 1)
            fatal_selfcheck_error(0, "deIRead can't read serialize data");
        image_file_position += 10UL;
        image_file_position += image_data_offset;
    }

    if (fseek(image_stream, image_file_position, 0) != 0 ||
        fread(pcx_header_bytes, PCX_HEADER_BYTES, 1, image_stream) != 1) {
        fclose(image_stream);
        return 1;
    }

    image_source_bytes = *(unsigned long *)(pcx_header_bytes + 0x46);
    if (image_source_bytes > (unsigned long)logo_input_buffer_size) {
        image_start_offset = image_source_offset - logo_input_buffer_size;
        image_frame_size = 1;
    } else {
        image_start_offset = image_source_offset;
        image_frame_size = 0;
    }

    image_frame_count = *(unsigned *)(pcx_header_bytes + LOGO_FRAME_COUNT_OFFSET);
    if (load_flags & 4) {
        if (fseek(image_stream, image_file_position, 0) != 0)
            fatal_selfcheck_error(0, "deIRead file seek error, pass 2");
        if (image_frame_size) {
            if (fread(logo_input_buffers[0], logo_input_buffer_size, 1, image_stream) != 1) {
                fclose(image_stream);
                return 1;
            }
            if (fread(logo_input_buffers[1], image_start_offset, 1,
                                image_stream) != 1) {
                fclose(image_stream);
                return 1;
            }
        } else {
            if (fread(logo_input_buffers[0], image_start_offset, 1,
                                image_stream) != 1) {
                fclose(image_stream);
                return 1;
            }
        }
    }

    fclose(image_stream);
    return 0;
}

/* Resolves the executable path if needed, then loads the requested image chunks and reports
 * missing data.
 * Original 17F5:024F, 192 bytes. */
int far load_image_from_path(unsigned options)
{
    struct {
        char path_scratch[0x56];
        void far *resolved_path;
    } path_lookup;
    register unsigned load_flags = options;
    register unsigned result;

    if (strlen(executable_path) == 0) {
        path_lookup.resolved_path = searchpath(executable_name_pointer);
        if (path_lookup.resolved_path == 0) {
            fatal_selfcheck_error(0, "deGetIData: can't locate file for image load.");
        } else {
            strncpy(executable_path,
                              strupr(path_lookup.resolved_path), 0x50);
        }
    } else {
        result = fnsplit(executable_path, path_lookup.path_scratch + 82,
                                  path_lookup.path_scratch, path_lookup.path_scratch + 72,
                                  path_lookup.path_scratch + 66);
        if ((result & 4) == 0)
            fatal_selfcheck_error(0, "deGetIData: Program name corrupted.");
    }

    result = (unsigned)load_logo_image(load_flags);
    if (result != 0 && (load_flags & 1))
        fatal_selfcheck_error(0, "deGetIData: missing image data.");
    return result;
}


/* Uploads the supplied number of palette entries from the shared far palette buffer through
 * BIOS.
 * Original 17F5:030F, 62 bytes. */
void far upload_reserved_dac_entries(unsigned count)
{
    union REGS regs;
    struct SREGS segs;

    regs.x.ax = BIOS_SET_DAC_BLOCK_FUNCTION;
    regs.x.bx = 0;
    regs.x.cx = count;
    regs.x.dx = palette_pointer_offset;
    segs.es = palette_pointer_segment;
    int86x(LOGO_PALETTE_FIELD_OFFSET, &regs, &regs, &segs);
}


/* Shifts the closing image's 768-byte VGA palette to six-bit values across one or two input
 * buffers.
 * Original 17F5:034D, 168 bytes. */
void far dim_logo_pixels(void)
{
    unsigned buffer_index = 0;
    unsigned palette_index;
    register unsigned palette_byte_index;
    char far *palette_source;

    if (image_frame_size) {
        if (image_start_offset < LOGO_PALETTE_MARKER_OFFSET) {
            palette_index = image_source_bytes_low - LOGO_PALETTE_BYTES + image_start_offset;
            for (palette_byte_index = 0; palette_byte_index < LOGO_PALETTE_BYTES; ++palette_byte_index) {
                logo_input_buffers[buffer_index][palette_index] >>= 2;
                ++palette_index;
                if (palette_index >= logo_input_buffer_size) {
                    palette_index = 0;
                    buffer_index = 1;
                }
            }
        } else {
            palette_source = logo_input_buffers[1] + (image_start_offset - LOGO_PALETTE_BYTES);
            for (palette_byte_index = 0; palette_byte_index < LOGO_PALETTE_BYTES; ++palette_byte_index) {
                palette_source[palette_byte_index] >>= 2;
            }
        }
    } else {
        palette_source = logo_input_buffers[0] + (image_start_offset - LOGO_PALETTE_BYTES);
        for (palette_byte_index = 0; palette_byte_index < LOGO_PALETTE_BYTES; ++palette_byte_index) {
            palette_source[palette_byte_index] >>= 2;
        }
    }
}


/* Converts the image header's EGA palette, copies the converted entries back, and clears its
 * background field.
 * Original 17F5:03F5, 92 bytes. */
void far prepare_logo_chunk(void)
{
    rgb8_to_ega_palette((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                   (unsigned char far *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET,
                   1 << logo_image_header->reserved[LOGO_PLANE_COUNT_OFFSET]);
    memcpy((unsigned char far *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET,
                   (unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                   1 << logo_image_header->reserved[LOGO_PLANE_COUNT_OFFSET]);
    logo_image_header->reserved[LOGO_BACKGROUND_FIELD_OFFSET] = 0;
}

/* Expands PCX run-length bytes into one output row while preserving spill beyond the row size.
 * Original 17F5:0451, 203 bytes. */
void far decode_logo_run(unsigned char far *destination)
{
    register int decoded_byte_count = 0;
    register unsigned char encoded_byte;
    register unsigned repeat_count;

    if (logo_pending_output_offset) {
        decoded_byte_count = logo_pending_output_offset - logo_image_header->output_size;
        logo_pending_output_offset = 0;
    }

    do {
        encoded_byte = logo_input_buffers[logo_input_buffer_index][logo_input_buffer_offset++];
        if (logo_input_buffer_offset >= logo_input_buffer_size) {
            logo_input_buffer_index = 1;
            logo_input_buffer_offset = 0;
        }
    if ((encoded_byte & PCX_RLE_MARKER_MASK) == PCX_RLE_MARKER) {
            repeat_count = encoded_byte & PCX_MAX_RUN_LENGTH;
            encoded_byte = logo_input_buffers[logo_input_buffer_index][logo_input_buffer_offset++];
            if (logo_input_buffer_offset >= logo_input_buffer_size) {
                logo_input_buffer_index = 1;
                logo_input_buffer_offset = 0;
            }
            while (repeat_count--) {
                destination[decoded_byte_count++] = encoded_byte;
            }
        } else {
            destination[decoded_byte_count++] = encoded_byte;
        }
    } while (decoded_byte_count < logo_image_header->output_size);

    if (logo_image_header->output_size >= decoded_byte_count)
        logo_pending_output_offset = 0;
    else
        logo_pending_output_offset = decoded_byte_count;
}


/* Selects VGA/EGA setup and decodes the closing image rows into video memory or an EGA plane
 * buffer.
 * Original 17F5:051C, 467 bytes. */
void far draw_closing_graphic(void)
{
    register int scanline;
    register int plane_index = 0;
    register unsigned palette_source_offset;
    unsigned char far *planar_row_buffer;

    logo_input_buffer_index = background_fill_word = 0;
    logo_previous_aspect = bios_resolution_mode;
    if (video_adapter == 8 && image_frame_count != 0) {
        bios_resolution_mode = 1;
        configure_video_backend(0);
        vga_set_mode_320x200(1);
        if (image_frame_size != 0) {
            if (image_start_offset >= LOGO_PALETTE_MARKER_OFFSET) {
                memcpy((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                                logo_input_buffers[1] + (image_start_offset - LOGO_PALETTE_BYTES),
                                LOGO_PALETTE_BYTES);
            } else {
                palette_source_offset = image_source_bytes_low - LOGO_PALETTE_BYTES + image_start_offset;
                for (scanline = 0; scanline < LOGO_PALETTE_BYTES; ++scanline) {
                    palette_buffer[scanline] = logo_input_buffers[plane_index][palette_source_offset];
                    ++palette_source_offset;
                    if (palette_source_offset >= logo_input_buffer_size) {
                        palette_source_offset = 0;
                        plane_index = 1;
                    }
                }
            }
        } else {
            memcpy((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                            logo_input_buffers[0] + (image_start_offset - LOGO_PALETTE_BYTES), LOGO_PALETTE_BYTES);
        }
        upload_reserved_dac_entries(VGA_DAC_ENTRY_COUNT);
    } else {
        if (video_adapter != 4) {
            logo_restore_video_adapter = 1;
            logo_previous_video_adapter = video_adapter;
            video_adapter = 4;
            configure_video_backend(0);
            ega_set_mode(1);
        }
        ega_init_row_table_and_mode();
        configure_logo_dac();
        if ((planar_row_buffer = (unsigned char far *)malloc(
            (logo_image_header->reserved[LOGO_PLANE_COUNT_OFFSET] + 4) * logo_image_header->output_size)) == 0)
            fatal_selfcheck_error(0, "\nMemory exhausted during image processing.");
    }

    logo_input_buffer_offset = PCX_HEADER_BYTES;
    /* VGA decodes directly to video memory; EGA routes each plane through a row buffer. */
    if (image_frame_count != 0) {
        for (scanline = 0; scanline < logo_image_height; ++scanline) {
            decode_logo_run((unsigned char far *)MK_FP(
                VGA_VIDEO_SEGMENT, scanline * LOGO_SCANLINE_BYTES));
        }
        return;
    } else {
        for (scanline = 0; scanline < logo_image_height; ++scanline) {
            for (plane_index = 0; plane_index < logo_image_header->reserved[LOGO_PLANE_COUNT_OFFSET]; ++plane_index) {
                decode_logo_run((unsigned char far *)MK_FP(
                    FP_SEG(planar_row_buffer), FP_OFF(planar_row_buffer) + plane_index * logo_image_header->output_size));
            }
            ega_write_planar_row(planar_row_buffer, logo_image_header->output_size, scanline);
        }
    }
    far_callback(2);
    free(planar_row_buffer);
}

/* Loads and validates the embedded image, allocates input buffers, selects VGA/EGA palette
 * handling, and sets image height.
 * Original 17F5:06EF, 638 bytes. */
void far prepare_logo_image_resources(void)
{
    logo_input_buffers[0] = logo_input_buffers[1] = (char far *)0;
    load_image_from_path(1);
    if ((image_frame_count != 0 && video_adapter != 8) ||
        (image_frame_count == 0 && video_adapter == 8)) {
        image_file_position += image_source_bytes;
        if (load_image_from_path(2) != 0)
            load_image_from_path(1);
    }

    if ((unsigned long)logo_input_buffer_size * 2UL <= image_source_bytes)
        fatal_selfcheck_error(1, "Image size too large.");
    if (image_source_bytes <= 0x80UL)
        fatal_selfcheck_error(1, "Image size too small.");

    if (image_frame_size) {
        logo_input_buffers[0] = (char far *)malloc(logo_input_buffer_size);
        logo_input_buffers[1] = (char far *)malloc(image_start_offset + LOGO_IMAGE_BUFFER_EXTRA);
    } else {
        logo_input_buffers[0] = (char far *)malloc(image_start_offset + LOGO_IMAGE_BUFFER_EXTRA);
    }
    logo_image_header = (struct image_header far *)(void far *)logo_input_buffers[0];
    if (logo_input_buffers[0] == 0 || (image_frame_size && logo_input_buffers[1] == 0))
        fatal_selfcheck_error(0, "Not enough memory for image file!");

    load_image_from_path(4);
    if (image_source_bytes <= 0x80UL)
        fatal_selfcheck_error(1, "Invalid image file header.");
    if (logo_image_header->reserved[0] != 10 || logo_image_header->reserved[1] < 5)
        fatal_selfcheck_error(1, "Not a usable color image file format.");

    if (logo_image_header->reserved[LOGO_PLANE_COUNT_OFFSET] == 1) {
        if (image_frame_size) {
            if (image_start_offset >= LOGO_PALETTE_MARKER_OFFSET) {
                if (logo_input_buffers[1][image_start_offset - LOGO_PALETTE_MARKER_OFFSET] == LOGO_PALETTE_MARKER)
                    image_frame_count = 1;
                else
                    fatal_selfcheck_error(1, "Illegal VGA format image file.");
            } else {
                if (logo_input_buffers[0][image_source_bytes_low - LOGO_PALETTE_MARKER_OFFSET + image_start_offset] == LOGO_PALETTE_MARKER)
                    image_frame_count = 1;
                else
                    fatal_selfcheck_error(1, "Illegal VGA format image file.");
            }
        } else if (image_source_bytes - 0x80UL - 0x301UL > 0UL &&
                   logo_input_buffers[0][image_start_offset - LOGO_PALETTE_MARKER_OFFSET] == LOGO_PALETTE_MARKER)
            image_frame_count = 1;
        else {
            fatal_selfcheck_error(1, "Illegal VGA format image file.");
        }
    } else
        image_frame_count = 0;

    if ((image_frame_count && *(unsigned far *)((unsigned char far *)logo_image_header + LOGO_FRAME_COUNT_OFFSET) == 0) ||
        (image_frame_count == 0 && *(unsigned far *)((unsigned char far *)logo_image_header + LOGO_FRAME_COUNT_OFFSET) != 0))
        fatal_selfcheck_error(1, "Image Data Corrupted.");
    if (image_frame_count && video_adapter != 8)
        fatal_selfcheck_error(0, "Image not compatible with your display");

    if (image_frame_count)
        dim_logo_pixels();
    else
        prepare_logo_chunk();
    logo_image_height = *(unsigned far *)((unsigned char far *)logo_image_header + LOGO_YMAX_FIELD_OFFSET) -
             *(unsigned far *)((unsigned char far *)logo_image_header + LOGO_YMIN_FIELD_OFFSET) + 1;
}


/* Restores the previous adapter/aspect state, clears and uploads the shared palette, and resets
 * the display.
 * Original 17F5:096D, 116 bytes. */
void far restore_logo_video_state(void)
{
    if (logo_restore_video_adapter || (video_adapter == 8 && image_frame_count)) {
        if (logo_restore_video_adapter)
            video_adapter = logo_previous_video_adapter;
        bios_resolution_mode = logo_previous_aspect;
        configure_video_backend(0);
    }
    setmem(palette_buffer, palette_buffer_bytes, 0);
    ega_border_color = background_fill_word = 0;
    palette_cycle_offset = 0;
    palette_upload_callback(palette_buffer);
    far_callback(1);
    logo_restore_video_adapter = 0;
}

/* Copies the image's 17-byte EGA palette and installs it through BIOS INT 10h/1002h.
 * Original 17F5:09E1, 100 bytes. */
/* This routine retains a repeated LEA for the input/output register block.
   BC30's redundant-load elimination must be disabled to reproduce it. */
#pragma option -Z-
void far configure_logo_dac(void)
{
    union REGS regs;
    struct SREGS segs;

    memcpy((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                    (unsigned char far *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET, LOGO_EGA_PALETTE_BYTES);
    regs.x.ax = BIOS_SET_EGA_PALETTE_FUNCTION;
    regs.x.bx = 0;
    regs.x.dx = FP_OFF(logo_image_header) + LOGO_PALETTE_FIELD_OFFSET;
    segs.es = FP_SEG((unsigned char far *)logo_image_header + LOGO_PALETTE_FIELD_OFFSET);
    int86x(LOGO_PALETTE_FIELD_OFFSET, (union REGS *)&regs.h, &regs, &segs);
}

#pragma option -Z

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
struct image_header far *logo_image_header; /* DS:747C */
unsigned logo_image_height; /* DS:747A */
unsigned logo_previous_aspect; /* DS:7478 */
unsigned logo_previous_video_adapter; /* DS:7476 */
unsigned logo_frame_state[2]; /* DS:7472 */
unsigned image_frame_count; /* DS:7470 */
unsigned logo_input_buffer_index; /* DS:746E */
unsigned logo_input_buffer_offset; /* DS:746C */
unsigned logo_decode_state; /* DS:746A */
unsigned image_start_offset; /* DS:7468 */
unsigned image_frame_size; /* DS:7466 */
unsigned logo_plane_state[2]; /* DS:7462 */
char far *logo_input_buffers[2]; /* DS:745A */
unsigned long image_source_bytes; /* DS:7456 */

/* DLOGO initialized state, preceding its merged text pool. */
unsigned logo_reserved = 0;
unsigned logo_restore_video_adapter = 0;
unsigned logo_input_buffer_size = 0xFE9E;
unsigned logo_pending_output_offset = 0;
