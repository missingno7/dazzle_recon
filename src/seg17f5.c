#pragma option -d
#include <dos.h>

/* objcmp: {"function":"current_code_segment","address":"17f5:0008","size":3,"level":"PROVISIONAL","blocker":"static-no-pubdef"} */
static unsigned current_code_segment(void)
{
    return _CS;
}

/* The following low level calls are external Borland/DAZZLE helpers. */
extern void far *open_image_file(char far *path, char far *mode);
extern void far *open_image_source(char near *path, char near *mode);
extern unsigned far strlen(const char far *text);
extern int far seek_image_file(void far *stream, unsigned long offset,
                               unsigned origin);
extern unsigned far read_image_file(void far *buffer, unsigned size,
                                    unsigned count, void far *stream);
extern int far close_image_file(void far *stream);
extern int far validate_image_header(void far *stream);
extern void far show_image_error(unsigned fatal, const char far *message);
extern unsigned far copy_image_header(void far *destination,
                                      unsigned length,
                                      unsigned stream_offset,
                                      unsigned stream_segment);
extern char far *copy_path_from_stream(void far *stream);
extern char far *copy_path_to_name(char far *destination,
                                   const char far *source, unsigned length);
extern unsigned far parse_image_path(const char far *path, char far *drive,
                                     char far *directory, char far *name,
                                     char far *extension);
extern char near *default_open_path;
extern char near *default_open_mode;

extern char near executable_path[];
extern unsigned char near expected_crc[];
extern void far *image_stream;
extern unsigned long image_file_position;
extern unsigned image_data_offset;
extern char far image_header_buffer[];
extern unsigned image_header_flags;
extern unsigned image_frame_count;
extern unsigned image_frame_size;
extern unsigned image_start_offset;
extern unsigned image_plane_count;
extern unsigned image_source_offset;
extern unsigned long image_source_bytes;
extern unsigned image_source_segment;
extern unsigned image_dest_offset;
extern unsigned image_dest_segment;
extern unsigned long image_buffer_size;
extern unsigned image_logo_enabled;
extern unsigned display_adapter_kind;
extern unsigned g_39aa;
extern char far *g_745a[2];

/* objcmp: {"function":"load_logo_image","segments":{"_DATA":"DS:39ae"},"address":"17f5:000b","size":580,"symbols":{"_open_image_file":"0000:459f","_validate_image_header":"11c1:05c3","_seek_image_file":"0000:47f4","_read_image_file":"0000:46e4","_close_image_file":"0000:407f","_show_image_error":"11c1:052f","_executable_path":"DS:5d17","_image_file_mode":"DS:39ae","_expected_crc":"DS:6027","_image_stream":"DS:6002","_image_file_position":"DS:5d68","_image_data_offset":"DS:602f","_image_header_buffer":"DS:5d17","_image_frame_count":"DS:7470","_image_frame_size":"DS:7466","_image_start_offset":"DS:7468","_image_plane_count":"DS:7458","_image_source_offset":"DS:7456","_image_source_bytes":"DS:7456","_image_header_flags":"DS:5d6a","_g_39aa":"DS:39aa","_g_745a":"DS:745a","_image_open_error":"DS:39b1","_image_seek_error":"DS:39c9","_image_read_error":"DS:39fc","_image_offset_error":"DS:3a1e"},"level":"PROVISIONAL","blocker":"relocation-coverage at 17f5:0036"} */
int far load_logo_image(unsigned options)
{
    unsigned char header[0x80];
    register unsigned flags = options;

    image_frame_size = image_frame_count = 0;
    image_source_bytes = image_start_offset = 0;
    if ((image_stream = open_image_file(executable_path, "rb")) == 0)
        show_image_error(0, "deIRead can't open file");

    if (flags & 1) {
        validate_image_header(image_stream);
        if (seek_image_file(image_stream, image_file_position, 0) != 0)
            show_image_error(0, "deIRead file seek error looking for serialize data");
        if (read_image_file(expected_crc, 10, 1, image_stream) != 1)
            show_image_error(0, "deIRead can't read serialize data");
        image_file_position += 10UL;
        image_file_position += image_data_offset;
    }

    if (seek_image_file(image_stream, image_file_position, 0) != 0 ||
        read_image_file(header, 0x80, 1, image_stream) != 1) {
        close_image_file(image_stream);
        return 1;
    }

    image_source_bytes = *(unsigned long *)(header + 0x46);
    if (image_source_bytes > (unsigned long)g_39aa) {
        image_start_offset = image_source_offset - g_39aa;
        image_frame_size = 1;
    } else {
        image_start_offset = image_source_offset;
        image_frame_size = 0;
    }

    image_frame_count = *(unsigned *)(header + 0x4a);
    if (flags & 4) {
        if (seek_image_file(image_stream, image_file_position, 0) != 0)
            show_image_error(0, "deIRead file seek error, pass 2");
        if (image_frame_size) {
            if (read_image_file(g_745a[0], g_39aa, 1, image_stream) != 1) {
                close_image_file(image_stream);
                return 1;
            }
            if (read_image_file(g_745a[1], image_start_offset, 1,
                                image_stream) != 1) {
                close_image_file(image_stream);
                return 1;
            }
        } else {
            if (read_image_file(g_745a[0], image_start_offset, 1,
                                image_stream) != 1) {
                close_image_file(image_stream);
                return 1;
            }
        }
    }

    close_image_file(image_stream);
    return 0;
}

/* objcmp: {"function":"load_image_from_path","address":"17f5:024f","size":192,"symbols":{"_strlen":"0000:5a36","_open_image_source":"0000:1ea4","_copy_path_from_stream":"0000:5b18","_copy_path_to_name":"0000:5a79","_parse_image_path":"0000:439f","_load_logo_image":"17f5:000b","_show_image_error":"11c1:052f","_executable_path":"DS:5d17","_default_open_path":"DS:0096","_default_open_mode":"DS:0098","_empty_filename_error":"DS:3a3e","_invalid_resource_error":"DS:3a6c"},"level":"PROVISIONAL","blocker":"full-extent path buffer/register layout at 17f5:024f"} */
int far load_image_from_path(unsigned options)
{
    struct {
        char scratch[0x56];
        void far *stream;
    } local;
    register unsigned flags = options;
    register unsigned result;

    if (strlen(executable_path) == 0) {
        local.stream = open_image_source(default_open_path, default_open_mode);
        if (local.stream == 0) {
            show_image_error(0, "deGetIData: can't locate file for image load.");
        } else {
            copy_path_to_name(executable_path,
                              copy_path_from_stream(local.stream), 0x50);
        }
    } else {
        result = parse_image_path(executable_path, local.scratch + 82,
                                  local.scratch, local.scratch + 72,
                                  local.scratch + 66);
        if ((result & 4) == 0)
            show_image_error(0, "deGetIData: Program name corrupted.");
    }

    result = (unsigned)load_logo_image(flags);
    if (result != 0 && (flags & 1))
        show_image_error(0, "deGetIData: missing image data.");
    return result;
}

extern unsigned g_5d94;
extern unsigned g_5d96;
extern unsigned char far *logo_palette;

/* objcmp: {"function":"upload_reserved_dac_entries","address":"17f5:030f","size":62,"symbols":{"_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_int86x":"0000:323f","_expected_crc":"DS:6027","_logo_allocation_error":"DS:3ab0","_logo_mode_flags":"DS:5d8a","_logo_plane_bytes":"DS:746c","_logo_previous_aspect":"DS:7478"}} */
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

struct image_header {
    char reserved[0x42];
    int output_size;
};

extern struct image_header far *g_747c;
extern unsigned g_39aa;
extern unsigned g_39ac;
extern unsigned g_746c;
extern unsigned g_746e;
extern char far *g_745a[2];

extern unsigned g_7456;
extern unsigned g_7458;
extern unsigned g_7466;
extern unsigned g_7468;
extern unsigned g_746e;
extern unsigned g_7470;
extern unsigned g_7478;
extern unsigned g_747a;
extern unsigned g_747e;
extern unsigned g_7476;
extern unsigned g_7484;
extern unsigned g_745c;
extern unsigned g_745e;
extern unsigned g_7460;
extern unsigned g_39aa;
extern unsigned g_39a8;
extern unsigned g_5de4;
extern unsigned g_5d8a;
extern unsigned g_5d94;
extern unsigned g_5d96;
extern unsigned g_5d68;
extern unsigned g_5d6a;
extern unsigned g_5c43;
extern void far *malloc(unsigned size);
extern void far free(void far *block);
extern void far logo_video_setup(unsigned mode);
extern void far logo_bitmap_start(unsigned mode);
extern void far logo_bitmap_finish(void);
extern void far composite_logo_plane(unsigned char far *source, unsigned width,
                                     unsigned frame);
extern void far copy_logo_plane(unsigned char far *destination,
                                unsigned char far *source,
                                unsigned count);
extern void (far *far_callback)(unsigned value);
extern void far setup_display_geometry(unsigned value);
extern void far report_logo_problem(unsigned fatal, const char far *message);
extern void far configure_logo_dac(void);

/* objcmp: {"function":"dim_logo_pixels","address":"17f5:034d","size":168,"symbols":{"_g_745a":"DS:745a","_g_7456":"DS:7456","_g_7466":"DS:7466","_g_7468":"DS:7468","_g_39aa":"DS:39aa"}} */
void far dim_logo_pixels(void)
{
    unsigned frame = 0;
    unsigned index;
    register unsigned pixel;
    char far *source;

    if (g_7466) {
        if (g_7468 < 0x301) {
            index = g_7456 - 0x300 + g_7468;
            for (pixel = 0; pixel < 0x300; ++pixel) {
                g_745a[frame][index] >>= 2;
                ++index;
                if (index >= g_39aa) {
                    index = 0;
                    frame = 1;
                }
            }
        } else {
            source = g_745a[1] + (g_7468 - 0x300);
            for (pixel = 0; pixel < 0x300; ++pixel) {
                source[pixel] >>= 2;
            }
        }
    } else {
        source = g_745a[0] + (g_7468 - 0x300);
        for (pixel = 0; pixel < 0x300; ++pixel) {
            source[pixel] >>= 2;
        }
    }
}

extern void far draw_logo_part(unsigned char far *destination,
                               unsigned char far *source, unsigned mask);
extern void far copy_logo_part(unsigned char far *source,
                               unsigned char far *destination, unsigned mask);

/* objcmp: {"function":"prepare_logo_chunk","address":"17f5:03f5","size":92,"symbols":{"_g_747c":"DS:747c","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_draw_logo_part":"1899:0006","_copy_logo_part":"0000:4cee"}} */
void far prepare_logo_chunk(void)
{
    draw_logo_part((unsigned char far *)MK_FP(g_5d96, g_5d94),
                   (unsigned char far *)g_747c + 0x10,
                   1 << g_747c->reserved[0x41]);
    copy_logo_part((unsigned char far *)g_747c + 0x10,
                   (unsigned char far *)MK_FP(g_5d96, g_5d94),
                   1 << g_747c->reserved[0x41]);
    g_747c->reserved[0x20] = 0;
}

/* objcmp: {"function":"decode_logo_run","address":"17f5:0451","size":203,"symbols":{"_g_747c":"DS:747c","_g_39aa":"DS:39aa","_g_39ac":"DS:39ac","_g_746c":"DS:746c","_g_746e":"DS:746e","_g_745a":"DS:745a"},"level":"PROVISIONAL","blocker":"branch-layout at 17f5:045c"} */
void far decode_logo_run(unsigned char far *destination)
{
    register int output_offset = 0;
    register unsigned char code;
    register unsigned repeat;

    if (g_39ac) {
        output_offset = g_39ac - g_747c->output_size;
        g_39ac = 0;
    }

    do {
        code = g_745a[g_746e][g_746c++];
        if (g_746c >= g_39aa) {
            g_746e = 1;
            g_746c = 0;
        }
        if ((code & 0xc0) == 0xc0) {
            repeat = code & 0x3f;
            code = g_745a[g_746e][g_746c++];
            if (g_746c >= g_39aa) {
                g_746e = 1;
                g_746c = 0;
            }
            while (repeat--) {
                destination[output_offset++] = code;
            }
        } else {
            destination[output_offset++] = code;
        }
    } while (output_offset < g_747c->output_size);

    if (g_747c->output_size >= output_offset)
        g_39ac = 0;
    else
        g_39ac = output_offset;
}

extern unsigned far logo_frame_width;
extern unsigned far logo_frame_height;
extern unsigned logo_mode_flags;
extern unsigned far logo_storage_handle;
extern unsigned far logo_storage_segment;
extern unsigned far logo_resource_bytes;
extern unsigned logo_plane_bytes;
extern unsigned far logo_current_offset;
extern unsigned far logo_current_segment;
extern unsigned logo_previous_aspect;
extern unsigned far logo_dac_count;
extern unsigned far logo_dac_buffer_offset;
extern unsigned far logo_dac_buffer_segment;

/* objcmp: {"function":"draw_closing_graphic","address":"17f5:051c","size":467,"symbols":{"_g_5d8a":"DS:5d8a","_far_callback":"DS:5c43","_logo_palette":"DS:5d94","_g_746e":"DS:746e","_g_7478":"DS:7478","_g_7484":"DS:7484","_g_7470":"DS:7470","_g_5de4":"DS:5de4","_setup_display_geometry":"11c1:21a6","_logo_video_setup":"1899:0b9e","_g_7466":"DS:7466","_g_7468":"DS:7468","_g_745a":"DS:745a","_g_745c":"DS:745c","_g_39aa":"DS:39aa","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_copy_logo_plane":"0000:4cee","_upload_reserved_dac_entries":"17f5:030f","_logo_bitmap_start":"1899:0aa5","_logo_bitmap_finish":"1899:010c","_configure_logo_dac":"17f5:09e1","_g_747c":"DS:747c","_malloc":"0000:2c51","_decode_logo_run":"17f5:0451","_composite_logo_plane":"1899:008c","_g_747a":"DS:747a","_g_5c43":"DS:5c43","_free":"0000:2b47"},"level":"PROVISIONAL","blocker":"full-extent control-flow layout at 17f5:051c"} */
void far draw_closing_graphic(void)
{
    register int pixel;
    register int buffer = 0;
    register unsigned source_position;
    unsigned char far *temp;

    g_746e = logo_mode_flags = 0;
    logo_previous_aspect = g_5de4;
    if (g_7484 == 8 && g_7470 != 0) {
        g_5de4 = 1;
        setup_display_geometry(0);
        logo_video_setup(1);
        if (g_7466 != 0) {
            if (g_7468 >= 0x301) {
                copy_logo_plane((unsigned char far *)MK_FP(g_5d96, g_5d94),
                                g_745a[1] + (g_7468 - 0x300),
                                0x300);
            } else {
                source_position = g_7456 - 0x300 + g_7468;
                for (pixel = 0; pixel < 0x300; ++pixel) {
                    logo_palette[pixel] = g_745a[buffer][source_position];
                    ++source_position;
                    if (source_position >= g_39aa) {
                        source_position = 0;
                        buffer = 1;
                    }
                }
            }
        } else {
            copy_logo_plane((unsigned char far *)MK_FP(g_5d96, g_5d94),
                            g_745a[0] + (g_7468 - 0x300), 0x300);
        }
        upload_reserved_dac_entries(0x100);
    } else {
        if (g_7484 != 4) {
            g_39a8 = 1;
            g_7476 = g_7484;
            g_7484 = 4;
            setup_display_geometry(0);
            logo_bitmap_start(1);
        }
        logo_bitmap_finish();
        configure_logo_dac();
        if ((temp = (unsigned char far *)malloc(
            (g_747c->reserved[0x41] + 4) * g_747c->output_size)) == 0)
            report_logo_problem(0, "\nMemory exhausted during image processing.");
    }

    logo_plane_bytes = 0x80;
    if (g_7470 != 0) {
        for (pixel = 0; pixel < g_747a; ++pixel) {
            decode_logo_run((unsigned char far *)MK_FP(
                0xa000, pixel * 0x140));
        }
        return;
    } else {
        for (pixel = 0; pixel < g_747a; ++pixel) {
            for (buffer = 0; buffer < g_747c->reserved[0x41]; ++buffer) {
                decode_logo_run((unsigned char far *)MK_FP(
                    FP_SEG(temp), FP_OFF(temp) + buffer * g_747c->output_size));
            }
            composite_logo_plane(temp, g_747c->output_size, pixel);
        }
    }
    far_callback(2);
    free(temp);
}

/* objcmp: {"function":"prepare_logo_image_resources","address":"17f5:06ef","size":638,"symbols":{"_g_7460":"DS:7460","_g_745e":"DS:745e","_g_745c":"DS:745c","_g_745a":"DS:745a","_load_image_from_path":"17f5:024f","_g_7470":"DS:7470","_g_7484":"DS:7484","_g_7458":"DS:7458","_g_7456":"DS:7456","_g_39aa":"DS:39aa","_g_7468":"DS:7468","_g_7466":"DS:7466","_g_39a8":"DS:39a8","_g_5d68":"DS:5d68","_g_5d6a":"DS:5d6a","_g_5de4":"DS:5de4","_g_7476":"DS:7476","_g_747c":"DS:747c","_g_747e":"DS:747e","_g_747a":"DS:747a","_g_746c":"DS:746c","_dim_logo_pixels":"17f5:034d","_prepare_logo_chunk":"17f5:03f5","_malloc":"0000:2c51","_free":"0000:2b47","_configure_logo_dac":"17f5:09e1","_report_logo_problem":"11c1:052f","_logo_size_error":"DS:3adb","_logo_width_error":"DS:3af1","_logo_header_alloc_error":"DS:3b07","_logo_header_error":"DS:3b44","_logo_embedded_width_error":"DS:3b29","_logo_embedded_plane_error":"DS:3b6a","_logo_plane_flag_error":"DS:3b89","_logo_pixel_mode_error":"DS:3b9f"},"level":"PROVISIONAL","blocker":"extent gap 24 bytes at 17f5:06ef; added omitted fallback predicate and post-header width check"} */
void far prepare_logo_image_resources(void)
{
    g_745a[0] = g_745a[1] = (char far *)0;
    load_image_from_path(1);
    if ((g_7470 != 0 && g_7484 != 8) ||
        (g_7470 == 0 && g_7484 == 8)) {
        image_file_position += image_source_bytes;
        if (load_image_from_path(2) != 0)
            load_image_from_path(1);
    }

    if ((unsigned long)g_39aa * 2UL <= image_source_bytes)
        report_logo_problem(1, "Image size too large.");
    if (image_source_bytes <= 0x80UL)
        report_logo_problem(1, "Image size too small.");

    if (g_7466) {
        g_745a[0] = (char far *)malloc(g_39aa);
        g_745a[1] = (char far *)malloc(g_7468 + 0x15e);
    } else {
        g_745a[0] = (char far *)malloc(g_7468 + 0x15e);
    }
    g_747c = (struct image_header far *)(void far *)g_745a[0];
    if (g_745a[0] == 0 || (g_7466 && g_745a[1] == 0))
        report_logo_problem(0, "Not enough memory for image file!");

    load_image_from_path(4);
    if (image_source_bytes <= 0x80UL)
        report_logo_problem(1, "Invalid image file header.");
    if (g_747c->reserved[0] != 10 || g_747c->reserved[1] < 5)
        report_logo_problem(1, "Not a usable color image file format.");

    if (g_747c->reserved[0x41] == 1) {
        if (g_7466) {
            if (g_7468 >= 0x301) {
                if (g_745a[1][g_7468 - 0x301] == 0x0c)
                    g_7470 = 1;
                else
                    report_logo_problem(1, "Illegal VGA format image file.");
            } else {
                if (g_745a[0][g_7456 - 0x301 + g_7468] == 0x0c)
                    g_7470 = 1;
                else
                    report_logo_problem(1, "Illegal VGA format image file.");
            }
        } else if (image_source_bytes - 0x80UL - 0x301UL > 0UL &&
                   g_745a[0][g_7468 - 0x301] == 0x0c)
            g_7470 = 1;
        else {
            report_logo_problem(1, "Illegal VGA format image file.");
        }
    } else
        g_7470 = 0;

    if ((g_7470 && *(unsigned far *)((unsigned char far *)g_747c + 0x4a) == 0) ||
        (g_7470 == 0 && *(unsigned far *)((unsigned char far *)g_747c + 0x4a) != 0))
        report_logo_problem(1, "Image Data Corrupted.");
    if (g_7470 && g_7484 != 8)
        report_logo_problem(0, "Image not compatible with your display");

    if (g_7470)
        dim_logo_pixels();
    else
        prepare_logo_chunk();
    g_747a = *(unsigned far *)((unsigned char far *)g_747c + 0x0a) -
             *(unsigned far *)((unsigned char far *)g_747c + 0x06) + 1;
}

extern unsigned palette_bytes;
extern unsigned palette_mode;
extern unsigned palette_offset;
extern void far setmem(void far *destination, unsigned count, char value);
extern void (far *upload_logo_palette)(unsigned char far *palette);

/* objcmp: {"function":"restore_logo_video_state","address":"17f5:096d","size":116,"symbols":{"_g_39a8":"DS:39a8","_g_7484":"DS:7484","_g_7470":"DS:7470","_g_7476":"DS:7476","_g_5de4":"DS:5de4","_logo_previous_aspect":"DS:7478","_setup_display_geometry":"11c1:21a6","_setmem":"0000:4d12","_logo_palette":"DS:5d94","_palette_bytes":"DS:5de8","_logo_mode_flags":"DS:5d8a","_palette_mode":"DS:5d8c","_palette_offset":"DS:5dec","_upload_logo_palette":"DS:5c37","_far_callback":"DS:5c43"}} */
void far restore_logo_video_state(void)
{
    if (g_39a8 || (g_7484 == 8 && g_7470)) {
        if (g_39a8)
            g_7484 = g_7476;
        g_5de4 = logo_previous_aspect;
        setup_display_geometry(0);
    }
    setmem(logo_palette, palette_bytes, 0);
    palette_mode = logo_mode_flags = 0;
    palette_offset = 0;
    upload_logo_palette(logo_palette);
    far_callback(1);
    g_39a8 = 0;
}

/* objcmp: {"function":"configure_logo_dac","address":"17f5:09e1","size":100,"symbols":{"_g_747c":"DS:747c","_g_5d94":"DS:5d94","_g_5d96":"DS:5d96","_int86x":"0000:323f","_copy_logo_plane":"0000:4cee"},"level":"PROVISIONAL","blocker":"duplicate-register-address at 17f5:0a31"} */
/* This routine retains a repeated LEA for the input/output register block.
   BC30's redundant-load elimination must be disabled to reproduce it. */
#pragma option -Z-
void far configure_logo_dac(void)
{
    union REGS regs;
    struct SREGS segs;

    copy_logo_plane((unsigned char far *)MK_FP(g_5d96, g_5d94),
                    (unsigned char far *)g_747c + 0x10, 0x11);
    regs.x.ax = 0x1002;
    regs.x.bx = 0;
    regs.x.dx = FP_OFF(g_747c) + 0x10;
    segs.es = FP_SEG((unsigned char far *)g_747c + 0x10);
    int86x(0x10, (union REGS *)&regs.h, &regs, &segs);
}
