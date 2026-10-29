/* DAZZLE 5.0 DLOGO; BC++ 3.00 -ml -O1, merged literal pool. */
#pragma option -d
#include "dazzle.h"


/* The static getter is covered by the complete 17F5 TU proof. */
/* objcmp: {"function":"dlogo_seg","address":"17f5:0008","size":3,"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"},"symbols":{"_executable_name_pointer":"DS:0096","_fade_records":"DS:0474","_background_fill_word":"DS:5d8a","_current_graphics_key":"DS:5d92","_palette_buffer":"DS:5d94","_image_source_bytes":"DS:7456","_resource_base":"DS:5d68","_selfcheck_stream":"DS:6002","_mode_setup_callback":"DS:5c43","_saved_text_info":"DS:6016","__streams":"DS:43f4","_saved_keyboard_vector":"DS:5ff2","_saved_timer_vector":"DS:5ff6","_palette_base_green":"DS:5d6c"}} */
unsigned dlogo_seg(void)
{
    return _CS;
}

/* The following low level calls are external Borland/DAZZLE helpers. */


/* objcmp: {"function":"load_logo_image","segments":{"_DATA":"29bf:39a6","_BSS":"29bf:7456"},"address":"17f5:000b","size":580,"symbols":{"_fopen":"0000:459f","_read_mz_resource_base":"11c1:05c3","_fseek":"0000:47f4","_fread":"0000:46e4","_fclose":"0000:407f","_fatal_selfcheck_error":"11c1:052f","_executable_path":"DS:5d17","_image_file_mode":"DS:39ae","_expected_crc":"DS:6027","_image_stream":"DS:6002","_image_file_position":"DS:5d68","_image_data_offset":"DS:602f","_image_header_buffer":"DS:5d17","_image_frame_count":"DS:7470","_image_frame_size":"DS:7466","_image_start_offset":"DS:7468","_image_plane_count":"DS:7458","_image_source_offset":"DS:7456","_image_source_bytes":"DS:7456","_image_header_flags":"DS:5d6a","_logo_input_buffer_size":"DS:39aa","_logo_input_buffers":"DS:745a","_image_open_error":"DS:39b1","_image_seek_error":"DS:39c9","_image_read_error":"DS:39fc","_image_offset_error":"DS:3a1e"}} */
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

/* objcmp: {"function":"load_image_from_path","address":"17f5:024f","size":192,"symbols":{"_strlen":"0000:5a36","_searchpath":"0000:1ea4","_strupr":"0000:5b18","_strncpy":"0000:5a79","_fnsplit":"0000:439f","_load_logo_image":"17f5:000b","_fatal_selfcheck_error":"11c1:052f","_executable_path":"DS:5d17","_default_open_path":"DS:0096","_default_open_mode":"DS:0098","_empty_filename_error":"DS:3a3e","_invalid_resource_error":"DS:3a6c"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
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


/* objcmp: {"function":"upload_reserved_dac_entries","address":"17f5:030f","size":62,"symbols":{"_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_int86x":"0000:323f","_expected_crc":"DS:6027","_logo_allocation_error":"DS:3ab0","_background_fill_word":"DS:5d8a","_logo_input_buffer_offset":"DS:746c","_logo_previous_aspect":"DS:7478"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
void far upload_reserved_dac_entries(unsigned count)
{
    union REGS regs;
    struct SREGS segs;

    regs.x.ax = 0x1012;
    regs.x.bx = 0;
    regs.x.cx = count;
    regs.x.dx = g_5d94;
    segs.es = g_5d96;
    int86x(0x10, &regs, &regs, &segs);
}


/* objcmp: {"function":"dim_logo_pixels","address":"17f5:034d","size":168,"symbols":{"_logo_input_buffers":"DS:745a","_g_7456":"DS:7456","_image_frame_size":"DS:7466","_image_start_offset":"DS:7468","_logo_input_buffer_size":"DS:39aa"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
void far dim_logo_pixels(void)
{
    unsigned frame = 0;
    unsigned index;
    register unsigned pixel;
    char far *source;

    if (image_frame_size) {
        if (image_start_offset < 0x301) {
            index = g_7456 - 0x300 + image_start_offset;
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


/* objcmp: {"function":"prepare_logo_chunk","address":"17f5:03f5","size":92,"symbols":{"_logo_image_header":"DS:747c","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_rgb8_to_ega_palette":"1899:0006","_memcpy":"0000:4cee"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
void far prepare_logo_chunk(void)
{
    rgb8_to_ega_palette((unsigned char far *)MK_FP(g_5d96, g_5d94),
                   (unsigned char far *)logo_image_header + 0x10,
                   1 << logo_image_header->reserved[0x41]);
    memcpy((unsigned char far *)logo_image_header + 0x10,
                   (unsigned char far *)MK_FP(g_5d96, g_5d94),
                   1 << logo_image_header->reserved[0x41]);
    logo_image_header->reserved[0x20] = 0;
}

/* objcmp: {"function":"decode_logo_run","address":"17f5:0451","size":203,"symbols":{"_logo_image_header":"DS:747c","_logo_input_buffer_size":"DS:39aa","_logo_pending_output_offset":"DS:39ac","_logo_input_buffer_offset":"DS:746c","_logo_input_buffer_index":"DS:746e","_logo_input_buffers":"DS:745a"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
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


/* objcmp: {"function":"draw_closing_graphic","address":"17f5:051c","size":467,"symbols":{"_g_5d8a":"DS:5d8a","_far_callback":"DS:5c43","_palette_buffer":"DS:5d94","_logo_input_buffer_index":"DS:746e","_g_7478":"DS:7478","_video_adapter":"DS:7484","_image_frame_count":"DS:7470","_bios_resolution_mode":"DS:5de4","_configure_video_backend":"11c1:21a6","_vga_set_mode_320x200":"1899:0b9e","_image_frame_size":"DS:7466","_image_start_offset":"DS:7468","_logo_input_buffers":"DS:745a","_g_745c":"DS:745c","_logo_input_buffer_size":"DS:39aa","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_memcpy":"0000:4cee","_upload_reserved_dac_entries":"17f5:030f","_ega_set_mode":"1899:0aa5","_ega_init_row_table_and_mode":"1899:010c","_configure_logo_dac":"17f5:09e1","_logo_image_header":"DS:747c","_malloc":"0000:2c51","_decode_logo_run":"17f5:0451","_ega_write_planar_row":"1899:008c","_logo_image_height":"DS:747a","_g_5c43":"DS:5c43","_free":"0000:2b47"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
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
                memcpy((unsigned char far *)MK_FP(g_5d96, g_5d94),
                                logo_input_buffers[1] + (image_start_offset - 0x300),
                                0x300);
            } else {
                source_position = g_7456 - 0x300 + image_start_offset;
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
            memcpy((unsigned char far *)MK_FP(g_5d96, g_5d94),
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

/* objcmp: {"function":"prepare_logo_image_resources","address":"17f5:06ef","size":638,"symbols":{"_g_7460":"DS:7460","_g_745e":"DS:745e","_g_745c":"DS:745c","_logo_input_buffers":"DS:745a","_load_image_from_path":"17f5:024f","_image_frame_count":"DS:7470","_video_adapter":"DS:7484","_g_7458":"DS:7458","_g_7456":"DS:7456","_logo_input_buffer_size":"DS:39aa","_image_start_offset":"DS:7468","_image_frame_size":"DS:7466","_logo_restore_video_adapter":"DS:39a8","_g_5d68":"DS:5d68","_g_5d6a":"DS:5d6a","_bios_resolution_mode":"DS:5de4","_logo_previous_video_adapter":"DS:7476","_logo_image_header":"DS:747c","_g_747e":"DS:747e","_logo_image_height":"DS:747a","_logo_input_buffer_offset":"DS:746c","_dim_logo_pixels":"17f5:034d","_prepare_logo_chunk":"17f5:03f5","_malloc":"0000:2c51","_free":"0000:2b47","_configure_logo_dac":"17f5:09e1","_fatal_selfcheck_error":"11c1:052f","_logo_size_error":"DS:3adb","_logo_width_error":"DS:3af1","_logo_header_alloc_error":"DS:3b07","_logo_header_error":"DS:3b44","_logo_embedded_width_error":"DS:3b29","_logo_embedded_plane_error":"DS:3b6a","_logo_plane_flag_error":"DS:3b89","_logo_pixel_mode_error":"DS:3b9f"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
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
                if (logo_input_buffers[0][g_7456 - 0x301 + image_start_offset] == 0x0c)
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


/* objcmp: {"function":"restore_logo_video_state","address":"17f5:096d","size":116,"symbols":{"_logo_restore_video_adapter":"DS:39a8","_video_adapter":"DS:7484","_image_frame_count":"DS:7470","_logo_previous_video_adapter":"DS:7476","_bios_resolution_mode":"DS:5de4","_logo_previous_aspect":"DS:7478","_configure_video_backend":"11c1:21a6","_setmem":"0000:4d12","_palette_buffer":"DS:5d94","_g_5de8":"DS:5de8","_background_fill_word":"DS:5d8a","_g_5d8c":"DS:5d8c","_palette_cycle_offset":"DS:5dec","_palette_upload_callback":"DS:5c37","_far_callback":"DS:5c43"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
void far restore_logo_video_state(void)
{
    if (logo_restore_video_adapter || (video_adapter == 8 && image_frame_count)) {
        if (logo_restore_video_adapter)
            video_adapter = logo_previous_video_adapter;
        bios_resolution_mode = logo_previous_aspect;
        configure_video_backend(0);
    }
    setmem(palette_buffer, g_5de8, 0);
    g_5d8c = background_fill_word = 0;
    palette_cycle_offset = 0;
    palette_upload_callback(palette_buffer);
    far_callback(1);
    logo_restore_video_adapter = 0;
}

/* objcmp: {"function":"configure_logo_dac","address":"17f5:09e1","size":100,"symbols":{"_logo_image_header":"DS:747c","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_int86x":"0000:323f","_memcpy":"0000:4cee"},"segments":{"_BSS":"29bf:7456","_DATA":"29bf:39a6"}} */
/* This routine retains a repeated LEA for the input/output register block.
   BC30's redundant-load elimination must be disabled to reproduce it. */
#pragma option -Z-
void far configure_logo_dac(void)
{
    union REGS regs;
    struct SREGS segs;

    memcpy((unsigned char far *)MK_FP(g_5d96, g_5d94),
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
