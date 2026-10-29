/* DAZPCX: Borland C++ 3.0, large model, -O1. Port I/O uses DOS.H builtins.
 * Extents include the complete RETF paths; 0820 is inside the final epilogue.
 */
#include <dos.h>
#define SELECT_PLANE(p) outportb(VGA_GC_DATA_PORT,p)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include "dzconst.h"

#define VGA_GC_READ_MAP_SELECT_REGISTER 4

struct pcx_header {
    char manufacturer, version, encoding, bits_per_pixel;
    int xmin, ymin, xmax, ymax, horizontal_res, vertical_res;
    char palette[48], reserved, planes;
    int bytes_per_line;
    char remainder[60];
};
extern unsigned char *palette_buffer;
extern int palette_cycle_offset;
extern unsigned background_fill_word, palette_base_green;
extern int video_adapter, bios_resolution_mode, display_pan_state, screen_half_height;
extern unsigned page_paragraph;
extern int pcx_number;
extern int _fmode;
extern char pcx_name_format[], binary_mode[], memory_error[], create_error[];
extern char number_error[], stream_error[], header_error[], palette_error[];
void restore_video_state(int);
void beep(int,int);
extern void (*display_start_callback)(int,int);

extern struct pcx_header *pcx;
extern char *video_memory;
extern FILE *pcx_file;
extern char *pcx_palette;

#define reserved_colors ((unsigned near *)&palette_base_green)

/* Returns the DAZPCX code segment.
 * Original 19F0:0004, 3 bytes. */
unsigned dazpcx_seg(void) { return _CS; }

/* Expands the shared palette bytes to VGA DAC values in the PCX palette buffer.
 * Original 19F0:0007, 70 bytes. */
void pcx_prepare_vga_palette(void)
{
    int palette_byte_index;
    for(palette_byte_index=0;palette_byte_index<9;palette_byte_index++) pcx_palette[palette_byte_index]=palette_buffer[palette_byte_index]<<2;
    for(;palette_byte_index<PCX_VGA_PALETTE_BYTES;palette_byte_index++) pcx_palette[palette_byte_index]=*(palette_cycle_offset+palette_byte_index+palette_buffer)<<2;
}

/* Converts the background, reserved colors, and cycling palette into the EGA PCX header palette.
 * Original 19F0:004D, 405 bytes. */
void pcx_prepare_ega_palette(void)
{
    int palette_index,source_entry_index;
    palette_index=0; source_entry_index=0;
    pcx->palette[palette_index++]=((background_fill_word&32)>>5)|((background_fill_word&4)>>1);
    pcx->palette[palette_index++]=((background_fill_word&16)>>4)|(background_fill_word&2);
    pcx->palette[palette_index++]=((background_fill_word&8)>>3)|((background_fill_word&1)<<1);
    for(;palette_index<9;palette_index+=3,source_entry_index++) {
        pcx->palette[palette_index]=((reserved_colors[source_entry_index]&32)>>5)|((reserved_colors[source_entry_index]&4)>>1);
        pcx->palette[palette_index+1]=((reserved_colors[source_entry_index]&16)>>4)|(reserved_colors[source_entry_index]&2);
        pcx->palette[palette_index+2]=((reserved_colors[source_entry_index]&8)>>3)|((reserved_colors[source_entry_index]&1)<<1);
    }
    for(source_entry_index=palette_cycle_offset+3;palette_index<48;source_entry_index++) {
        pcx->palette[palette_index++]=((palette_buffer[source_entry_index]&32)>>5)|((palette_buffer[source_entry_index]&4)>>1);
        pcx->palette[palette_index++]=((palette_buffer[source_entry_index]&16)>>4)|(palette_buffer[source_entry_index]&2);
        pcx->palette[palette_index++]=((palette_buffer[source_entry_index]&8)>>3)|((palette_buffer[source_entry_index]&1)<<1);
    }
    /* Reset both indices after the RGB passes. BC30 removes the unused source-index
     * store, but retains the initial jump to the conversion loop's test.
     */
    source_entry_index=palette_index=0;
    while(palette_index<48) {
        switch(pcx->palette[palette_index]) {
        case 1: pcx->palette[palette_index]=0x55; break;
        case 2: pcx->palette[palette_index]=0xaa; break;
        case 3: pcx->palette[palette_index]=0xff; break;
        }
        palette_index++;
    }
}

/* Initializes and writes a 128-byte PCX header for the active EGA or VGA display
 * dimensions.
 * Original 19F0:01E2, 196 bytes. */
void pcx_write_header(void)
{
    memset(pcx,0,PCX_HEADER_BYTES);
    pcx->manufacturer=10; pcx->version=5; pcx->encoding=1;
    if(video_adapter==4) {
        pcx->bits_per_pixel=1;
        pcx->xmax=639; pcx->ymax=349;
        pcx->horizontal_res=640; pcx->vertical_res=350;
        pcx->planes=4; pcx->bytes_per_line=80;
        pcx_prepare_ega_palette();
    } else {
        pcx->bits_per_pixel=8;
        pcx->xmax=319; pcx->horizontal_res=320;
        pcx->planes=1; pcx->bytes_per_line=320;
        if(bios_resolution_mode) { pcx->ymax=199; pcx->vertical_res=200; }
        else { pcx->ymax=399; pcx->vertical_res=400; }
    }
    fwrite(pcx,1,PCX_HEADER_BYTES,pcx_file);
}

/* Encodes one PCX scanline with run-length markers and escapes literal bytes in the marker
 * range.
 * Original 19F0:02A6, 216 bytes. */
int pcx_encode_row(char *row)
{
    unsigned byte_index, repeat_count;
    int encoded_byte_count;
    encoded_byte_count = 0;
    byte_index = 0;
    do {
        repeat_count = 0;
        /* PCX runs use a six-bit count; marker-valued literals need a one-byte run. */
        while (row[byte_index + repeat_count] == row[byte_index + repeat_count + 1] &&
               byte_index + repeat_count < pcx->bytes_per_line &&
               repeat_count < PCX_MAX_RUN_LENGTH)
            repeat_count++;
        if (repeat_count > 0) {
            fputc(repeat_count | PCX_RLE_MARKER, pcx_file);
            fputc(row[byte_index], pcx_file);
            byte_index += repeat_count;
            encoded_byte_count += 2;
        } else {
            if ((row[byte_index] & PCX_RLE_MARKER_MASK) == PCX_RLE_MARKER) {
                fputc(PCX_SINGLE_LITERAL_RUN, pcx_file);
                encoded_byte_count++;
            }
            fputc(row[byte_index++], pcx_file);
            encoded_byte_count++;
        }
    } while (byte_index < pcx->bytes_per_line);
    return encoded_byte_count;
}

/* Selects each EGA plane in turn and encodes its scanlines to the PCX stream.
 * Original 19F0:037E, 169 bytes. */
void pcx_write_ega_rows(void)
{
    int row_index,plane_index;
    plane_index=row_index=0;
    for(;row_index<pcx->vertical_res;row_index++) {
        outportb(VGA_GC_INDEX_PORT,VGA_GC_READ_MAP_SELECT_REGISTER);
        SELECT_PLANE(plane_index++); pcx_encode_row(video_memory+pcx->bytes_per_line*row_index);
        SELECT_PLANE(plane_index++); pcx_encode_row(video_memory+pcx->bytes_per_line*row_index);
        SELECT_PLANE(plane_index++); pcx_encode_row(video_memory+pcx->bytes_per_line*row_index);
        SELECT_PLANE(plane_index++); pcx_encode_row(video_memory+pcx->bytes_per_line*row_index);
        /* BC30 removes the last increment before this reset, while retaining
         * the word load for the post-increment expression's old value.
         */
        plane_index=0;
    }
}

/* Encodes linear VGA scanlines and appends the 256-color palette marker and data.
 * Original 19F0:0427, 97 bytes. */
void pcx_write_linear_rows(void)
{
    int row_index;
    for(row_index=0;row_index<pcx->vertical_res;row_index++) pcx_encode_row(video_memory+pcx->bytes_per_line*row_index);
    row_index=PCX_VGA_PALETTE_MARKER; fputc(row_index,pcx_file);
    fwrite(pcx_palette,PCX_VGA_PALETTE_BYTES,1,pcx_file);
}

#define PCX_FATAL(message,code) { restore_video_state(1); fprintf(stderr,message); exit(code); }

/* Gathers enhanced VGA planes into a scanline buffer, encodes each row, and appends the palette.
 * Original 19F0:0488, 387 bytes. */
void pcx_write_planar_rows(void)
{
    int row_index,plane_byte_index,packed_column,plane_index,bytes_per_plane;
    char *row;
    bytes_per_plane=pcx->bytes_per_line/4;
    if(!(row=malloc(pcx->bytes_per_line))) PCX_FATAL(memory_error,903);
    for(row_index=0;row_index<pcx->vertical_res;row_index++) {
        plane_index=packed_column=plane_byte_index=0;
        for(;plane_byte_index<bytes_per_plane;plane_byte_index++) {
            outportb(VGA_GC_INDEX_PORT,VGA_GC_READ_MAP_SELECT_REGISTER);
            SELECT_PLANE(plane_index++); row[packed_column++]=video_memory[plane_byte_index+row_index*80];
            SELECT_PLANE(plane_index++); row[packed_column++]=video_memory[plane_byte_index+row_index*80];
            SELECT_PLANE(plane_index++); row[packed_column++]=video_memory[plane_byte_index+row_index*80];
            SELECT_PLANE(plane_index++); row[packed_column++]=video_memory[plane_byte_index+row_index*80];
            plane_index=0;
        }
        pcx_encode_row(row);
    }
    free(row);
    row_index=PCX_VGA_PALETTE_MARKER; fputc(row_index,pcx_file);
    fwrite(pcx_palette,PCX_VGA_PALETTE_BYTES,1,pcx_file);
}

/* Creates the next PCX file, writes its header and display rows, then releases buffers and
 * closes files.
 * Original 19F0:060B, 540 bytes. */
void save_pcx(void)
{
    int file_handle;
    char output_filename[14];
    _fmode=0x8000;
    do {
        sprintf(output_filename,pcx_name_format,pcx_number++);
        file_handle=creatnew(output_filename,0);
        if(file_handle==-1 && errno!=35) PCX_FATAL(create_error,904);
    } while(file_handle==-1 && pcx_number<9999);
    if(pcx_number>9999) PCX_FATAL(number_error,905);
    if(!(pcx_file=fdopen(file_handle,binary_mode))) PCX_FATAL(stream_error,900);
    if(!(pcx=malloc(PCX_HEADER_BYTES))) PCX_FATAL(header_error,901);
    pcx_write_header();
    beep(600,55);
    video_memory=MK_FP(VGA_VIDEO_SEGMENT,0);
    if(display_pan_state<=1) {
        display_start_callback(0,0);
        FP_SEG(video_memory)=VGA_VIDEO_SEGMENT;
    } else {
        display_start_callback(0,video_adapter==4?350:screen_half_height*2);
        FP_SEG(video_memory)=VGA_VIDEO_SEGMENT+page_paragraph;
    }
    FP_OFF(video_memory)=0;
    if(video_adapter==4) pcx_write_ega_rows();
    else {
        if(!(pcx_palette=malloc(PCX_VGA_PALETTE_BYTES))) PCX_FATAL(palette_error,902);
        pcx_prepare_vga_palette();
        if(bios_resolution_mode) pcx_write_linear_rows(); else pcx_write_planar_rows();
        free(pcx_palette);
    }
    free(pcx);
    fcloseall();
    _fmode=0x4000;
    beep(300,40);
}

/* Uninitialized module state. BC++ 3.0 emits BSS in reverse declaration order.
 * Offsets below are the independently recovered DGROUP addresses. */
struct pcx_header *pcx; /* DS:7762 */
char *video_memory; /* DS:775E */
FILE *pcx_file; /* DS:775A */
char *pcx_palette; /* DS:7756 */

/* PCX serial number and diagnostic strings in original storage order. */
int pcx_number = 0;
char memory_error[] = "malloc error for hptr in DAZPCX.\a\n";
char pcx_name_format[] = "daz_%04d.pcx";
char create_error[] = "Sorry, can't create output PCX file.\a\n";
char number_error[] = "Sorry, PCX filename range exceeded.\a\n";
char binary_mode[] = "wb";
char stream_error[] = "Sorry, can't open output PCX file.\a\n";
char header_error[] = "malloc error for stdhdr in PackPcxFile.\a\n";
char palette_error[] = "malloc error for vga_palette in PackPcxFile.\a\n";
