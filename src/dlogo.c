/* DAZZLE 5.0 DLOGO; BC++ 3.00 -ml -O1, merged literal pool. */
#pragma option -d
#include "dazzle.h"


/* The static getter is covered by the complete 17F5 TU proof. */
/* 17F5:0008, 3 bytes */
unsigned dlogo_seg(void)
{
    return _CS;
}

/* The following low level calls are external Borland/DAZZLE helpers. */


/* 17F5:000B, 580 bytes */
int far load_logo_image(unsigned options)
{
    unsigned char header[0x80];
    register unsigned flags = options;

    image_frame_size = image_frame_count = 0;
    image_source_bytes = image_start_offset = 0;
    if ((image_stream = fopen(executable_path, "rb")) == 0)
        fatal_selfcheck_error(0, "deIRead can't open file");

    if (flags & 1) {
        read_mz_resource_base(image_stream);
        if (fseek(image_stream, image_file_position, 0) != 0)
            fatal_selfcheck_error(0, "deIRead file seek error looking for serialize data");
        if (fread(&expected_crc, 10, 1, image_stream) != 1)
            fatal_selfcheck_error(0, "deIRead can't read serialize data");
        image_file_position += 10UL;
        image_file_position += image_data_offset;
    }

    if (fseek(image_stream, image_file_position, 0) != 0 ||
        fread(header, 0x80, 1, image_stream) != 1) {
        fclose(image_stream);
        return 1;
    }

    image_source_bytes = *(unsigned long *)(header + 0x46);
    if (image_source_bytes > (unsigned long)logo_input_buffer_size) {
        image_start_offset = image_source_offset - logo_input_buffer_size;
        image_frame_size = 1;
    } else {
        image_start_offset = image_source_offset;
        image_frame_size = 0;
    }

    image_frame_count = *(unsigned *)(header + 0x4a);
    if (flags & 4) {
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

/* 17F5:024F, 192 bytes */
int far load_image_from_path(unsigned options)
{
    struct {
        char scratch[0x56];
        void far *stream;
    } local;
    register unsigned flags = options;
    register unsigned result;

    if (strlen(executable_path) == 0) {
        local.stream = searchpath(executable_name_pointer);
        if (local.stream == 0) {
            fatal_selfcheck_error(0, "deGetIData: can't locate file for image load.");
        } else {
            strncpy(executable_path,
                              strupr(local.stream), 0x50);
        }
    } else {
        result = fnsplit(executable_path, local.scratch + 82,
                                  local.scratch, local.scratch + 72,
                                  local.scratch + 66);
        if ((result & 4) == 0)
            fatal_selfcheck_error(0, "deGetIData: Program name corrupted.");
    }

    result = (unsigned)load_logo_image(flags);
    if (result != 0 && (flags & 1))
        fatal_selfcheck_error(0, "deGetIData: missing image data.");
    return result;
}


/* 17F5:030F, 62 bytes */
void far upload_reserved_dac_entries(unsigned count)
{
    union REGS regs;
    struct SREGS segs;

    regs.x.ax = 0x1012;
    regs.x.bx = 0;
    regs.x.cx = count;
    regs.x.dx = palette_pointer_offset;
    segs.es = palette_pointer_segment;
    int86x(0x10, &regs, &regs, &segs);
}


/* 17F5:034D, 168 bytes */
void far dim_logo_pixels(void)
{
    unsigned frame = 0;
    unsigned index;
    register unsigned pixel;
    char far *source;

    if (image_frame_size) {
        if (image_start_offset < 0x301) {
            index = image_source_bytes_low - 0x300 + image_start_offset;
            for (pixel = 0; pixel < 0x300; ++pixel) {
                logo_input_buffers[frame][index] >>= 2;
                ++index;
                if (index >= logo_input_buffer_size) {
                    index = 0;
                    frame = 1;
                }
            }
        } else {
            source = logo_input_buffers[1] + (image_start_offset - 0x300);
            for (pixel = 0; pixel < 0x300; ++pixel) {
                source[pixel] >>= 2;
            }
        }
    } else {
        source = logo_input_buffers[0] + (image_start_offset - 0x300);
        for (pixel = 0; pixel < 0x300; ++pixel) {
            source[pixel] >>= 2;
        }
    }
}


/* 17F5:03F5, 92 bytes */
void far prepare_logo_chunk(void)
{
    rgb8_to_ega_palette((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                   (unsigned char far *)logo_image_header + 0x10,
                   1 << logo_image_header->reserved[0x41]);
    memcpy((unsigned char far *)logo_image_header + 0x10,
                   (unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                   1 << logo_image_header->reserved[0x41]);
    logo_image_header->reserved[0x20] = 0;
}

/* 17F5:0451, 203 bytes */
void far decode_logo_run(unsigned char far *destination)
{
    register int output_offset = 0;
    register unsigned char code;
    register unsigned repeat;

    if (logo_pending_output_offset) {
        output_offset = logo_pending_output_offset - logo_image_header->output_size;
        logo_pending_output_offset = 0;
    }

    do {
        code = logo_input_buffers[logo_input_buffer_index][logo_input_buffer_offset++];
        if (logo_input_buffer_offset >= logo_input_buffer_size) {
            logo_input_buffer_index = 1;
            logo_input_buffer_offset = 0;
        }
        if ((code & 0xc0) == 0xc0) {
            repeat = code & 0x3f;
            code = logo_input_buffers[logo_input_buffer_index][logo_input_buffer_offset++];
            if (logo_input_buffer_offset >= logo_input_buffer_size) {
                logo_input_buffer_index = 1;
                logo_input_buffer_offset = 0;
            }
            while (repeat--) {
                destination[output_offset++] = code;
            }
        } else {
            destination[output_offset++] = code;
        }
    } while (output_offset < logo_image_header->output_size);

    if (logo_image_header->output_size >= output_offset)
        logo_pending_output_offset = 0;
    else
        logo_pending_output_offset = output_offset;
}


/* 17F5:051C, 467 bytes */
void far draw_closing_graphic(void)
{
    register int pixel;
    register int buffer = 0;
    register unsigned source_position;
    unsigned char far *temp;

    logo_input_buffer_index = background_fill_word = 0;
    logo_previous_aspect = bios_resolution_mode;
    if (video_adapter == 8 && image_frame_count != 0) {
        bios_resolution_mode = 1;
        configure_video_backend(0);
        vga_set_mode_320x200(1);
        if (image_frame_size != 0) {
            if (image_start_offset >= 0x301) {
                memcpy((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                                logo_input_buffers[1] + (image_start_offset - 0x300),
                                0x300);
            } else {
                source_position = image_source_bytes_low - 0x300 + image_start_offset;
                for (pixel = 0; pixel < 0x300; ++pixel) {
                    palette_buffer[pixel] = logo_input_buffers[buffer][source_position];
                    ++source_position;
                    if (source_position >= logo_input_buffer_size) {
                        source_position = 0;
                        buffer = 1;
                    }
                }
            }
        } else {
            memcpy((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                            logo_input_buffers[0] + (image_start_offset - 0x300), 0x300);
        }
        upload_reserved_dac_entries(0x100);
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
        if ((temp = (unsigned char far *)malloc(
            (logo_image_header->reserved[0x41] + 4) * logo_image_header->output_size)) == 0)
            fatal_selfcheck_error(0, "\nMemory exhausted during image processing.");
    }

    logo_input_buffer_offset = 0x80;
    if (image_frame_count != 0) {
        for (pixel = 0; pixel < logo_image_height; ++pixel) {
            decode_logo_run((unsigned char far *)MK_FP(
                0xa000, pixel * 0x140));
        }
        return;
    } else {
        for (pixel = 0; pixel < logo_image_height; ++pixel) {
            for (buffer = 0; buffer < logo_image_header->reserved[0x41]; ++buffer) {
                decode_logo_run((unsigned char far *)MK_FP(
                    FP_SEG(temp), FP_OFF(temp) + buffer * logo_image_header->output_size));
            }
            ega_write_planar_row(temp, logo_image_header->output_size, pixel);
        }
    }
    far_callback(2);
    free(temp);
}

/* 17F5:06EF, 638 bytes */
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
        logo_input_buffers[1] = (char far *)malloc(image_start_offset + 0x15e);
    } else {
        logo_input_buffers[0] = (char far *)malloc(image_start_offset + 0x15e);
    }
    logo_image_header = (struct image_header far *)(void far *)logo_input_buffers[0];
    if (logo_input_buffers[0] == 0 || (image_frame_size && logo_input_buffers[1] == 0))
        fatal_selfcheck_error(0, "Not enough memory for image file!");

    load_image_from_path(4);
    if (image_source_bytes <= 0x80UL)
        fatal_selfcheck_error(1, "Invalid image file header.");
    if (logo_image_header->reserved[0] != 10 || logo_image_header->reserved[1] < 5)
        fatal_selfcheck_error(1, "Not a usable color image file format.");

    if (logo_image_header->reserved[0x41] == 1) {
        if (image_frame_size) {
            if (image_start_offset >= 0x301) {
                if (logo_input_buffers[1][image_start_offset - 0x301] == 0x0c)
                    image_frame_count = 1;
                else
                    fatal_selfcheck_error(1, "Illegal VGA format image file.");
            } else {
                if (logo_input_buffers[0][image_source_bytes_low - 0x301 + image_start_offset] == 0x0c)
                    image_frame_count = 1;
                else
                    fatal_selfcheck_error(1, "Illegal VGA format image file.");
            }
        } else if (image_source_bytes - 0x80UL - 0x301UL > 0UL &&
                   logo_input_buffers[0][image_start_offset - 0x301] == 0x0c)
            image_frame_count = 1;
        else {
            fatal_selfcheck_error(1, "Illegal VGA format image file.");
        }
    } else
        image_frame_count = 0;

    if ((image_frame_count && *(unsigned far *)((unsigned char far *)logo_image_header + 0x4a) == 0) ||
        (image_frame_count == 0 && *(unsigned far *)((unsigned char far *)logo_image_header + 0x4a) != 0))
        fatal_selfcheck_error(1, "Image Data Corrupted.");
    if (image_frame_count && video_adapter != 8)
        fatal_selfcheck_error(0, "Image not compatible with your display");

    if (image_frame_count)
        dim_logo_pixels();
    else
        prepare_logo_chunk();
    logo_image_height = *(unsigned far *)((unsigned char far *)logo_image_header + 0x0a) -
             *(unsigned far *)((unsigned char far *)logo_image_header + 0x06) + 1;
}


/* 17F5:096D, 116 bytes */
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

/* 17F5:09E1, 100 bytes */
/* This routine retains a repeated LEA for the input/output register block.
   BC30's redundant-load elimination must be disabled to reproduce it. */
#pragma option -Z-
void far configure_logo_dac(void)
{
    union REGS regs;
    struct SREGS segs;

    memcpy((unsigned char far *)MK_FP(palette_pointer_segment, palette_pointer_offset),
                    (unsigned char far *)logo_image_header + 0x10, 0x11);
    regs.x.ax = 0x1002;
    regs.x.bx = 0;
    regs.x.dx = FP_OFF(logo_image_header) + 0x10;
    segs.es = FP_SEG((unsigned char far *)logo_image_header + 0x10);
    int86x(0x10, (union REGS *)&regs.h, &regs, &segs);
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
