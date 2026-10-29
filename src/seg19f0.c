/* DAZPCX: Borland C++ 3.0, large model, -O1. Port I/O uses DOS.H builtins.
 * Extents include the complete RETF paths; 0820 is inside the final epilogue.
 */
#include <dos.h>
#define SELECT_PLANE(p) outportb(0x3cf,p)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>

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
extern int video_adapter, g_5de4, g_5de6, screen_half_height;
extern unsigned page_paragraph;
extern int pcx_number;
extern int _fmode;
extern char pcx_name_format[], binary_mode[], memory_error[], create_error[];
extern char number_error[], stream_error[], header_error[], palette_error[];
void restore_video_state(int);
void beep(int,int);
extern void (*g_5c33)(int,int);

extern struct pcx_header *pcx;
extern char *video_memory;
extern FILE *pcx_file;
extern char *pcx_palette;

#define reserved_colors ((unsigned near *)&palette_base_green)

/* objcmp: {"function":"pcx_code_segment","address":"19f0:0004","size":3,"symbols":{"_palette_buffer":"DS:5d94","_palette_cycle_offset":"DS:5dec","_background_fill_word":"DS:5d8a","_reserved_colors":"DS:5d6c","_video_adapter":"DS:7484","_g_5de4":"DS:5de4","_g_5de6":"DS:5de6","_screen_half_height":"DS:5db2","_page_paragraph":"DS:7490","_pcx_palette":"DS:7756","_pcx_file":"DS:775a","_video_memory":"DS:775e","_pcx":"DS:7762","_pcx_number":"DS:3c66","__fmode":"DS:45ae","__streams":"DS:43f4","__errno":"DS:007f","_errno":"DS:007f","_pcx_name_format":"DS:3c8b","_binary_mode":"DS:3ce5","_memory_error":"DS:3c68","_create_error":"DS:3c98","_number_error":"DS:3cbf","_stream_error":"DS:3ce8","_header_error":"DS:3d0d","_palette_error":"DS:3d37","_restore_video_state":"1899:0bcd","_beep":"11c1:0e07","_g_5c33":"DS:5c33","_memset":"0000:4d36","_fwrite":"0000:48cc","_fputc":"0000:4fea","_malloc":"0000:2c51","_free":"0000:2b47","_fprintf":"0000:45d4","_exit":"0000:11a8","_sprintf":"0000:5893","_creatnew":"0000:3fed","_fdopen":"0000:412a","_fcloseall":"0000:3f37","_executable_name_pointer":"DS:0096","_fade_records":"DS:0474","_g_5d92":"DS:5d92","_image_source_bytes":"DS:7456","_resource_base":"DS:5d68","_selfcheck_stream":"DS:6002","_mode_setup_callback":"DS:5c43","_saved_text_info":"DS:6016","_saved_keyboard_vector":"DS:5ff2","_saved_timer_vector":"DS:5ff6","_palette_base_green":"DS:5d6c"},"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
unsigned pcx_code_segment(void) { return _CS; }

/* objcmp: {"function":"pcx_prepare_vga_palette","address":"19f0:0007","size":70,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void pcx_prepare_vga_palette(void)
{
    int i;
    for(i=0;i<9;i++) pcx_palette[i]=palette_buffer[i]<<2;
    for(;i<769;i++) pcx_palette[i]=*(palette_cycle_offset+i+palette_buffer)<<2;
}

/* objcmp: {"function":"pcx_prepare_ega_palette","address":"19f0:004d","size":405,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void pcx_prepare_ega_palette(void)
{
    int i,j;
    i=0; j=0;
    pcx->palette[i++]=((background_fill_word&32)>>5)|((background_fill_word&4)>>1);
    pcx->palette[i++]=((background_fill_word&16)>>4)|(background_fill_word&2);
    pcx->palette[i++]=((background_fill_word&8)>>3)|((background_fill_word&1)<<1);
    for(;i<9;i+=3,j++) {
        pcx->palette[i]=((reserved_colors[j]&32)>>5)|((reserved_colors[j]&4)>>1);
        pcx->palette[i+1]=((reserved_colors[j]&16)>>4)|(reserved_colors[j]&2);
        pcx->palette[i+2]=((reserved_colors[j]&8)>>3)|((reserved_colors[j]&1)<<1);
    }
    for(j=palette_cycle_offset+3;i<48;j++) {
        pcx->palette[i++]=((palette_buffer[j]&32)>>5)|((palette_buffer[j]&4)>>1);
        pcx->palette[i++]=((palette_buffer[j]&16)>>4)|(palette_buffer[j]&2);
        pcx->palette[i++]=((palette_buffer[j]&8)>>3)|((palette_buffer[j]&1)<<1);
    }
    /* Reset both indices after the RGB passes. BC30 removes the unused j
     * store, but retains the initial jump to the conversion loop's test.
     */
    j=i=0;
    while(i<48) {
        switch(pcx->palette[i]) {
        case 1: pcx->palette[i]=0x55; break;
        case 2: pcx->palette[i]=0xaa; break;
        case 3: pcx->palette[i]=0xff; break;
        }
        i++;
    }
}

/* objcmp: {"function":"pcx_write_header","address":"19f0:01e2","size":196,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void pcx_write_header(void)
{
    memset(pcx,0,128);
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
        if(g_5de4) { pcx->ymax=199; pcx->vertical_res=200; }
        else { pcx->ymax=399; pcx->vertical_res=400; }
    }
    fwrite(pcx,1,128,pcx_file);
}

/* objcmp: {"function":"pcx_encode_row","address":"19f0:02a6","size":216,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
int pcx_encode_row(char *row)
{
    unsigned i,run;
    int written;
    written=0; i=0;
    do {
        run=0;
        while(row[i+run]==row[i+run+1] && i+run<pcx->bytes_per_line && run<63) run++;
        if(run>0) {
            fputc(run|0xc0,pcx_file);
            fputc(row[i],pcx_file);
            i+=run;
            written+=2;
        } else {
            if((row[i]&0xc0)==0xc0) { fputc(0xc1,pcx_file); written++; }
            fputc(row[i++],pcx_file);
            written++;
        }
    } while(i<pcx->bytes_per_line);
    return written;
}

/* objcmp: {"function":"pcx_write_ega_rows","address":"19f0:037e","size":169,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void pcx_write_ega_rows(void)
{
    int scanline,plane;
    plane=scanline=0;
    for(;scanline<pcx->vertical_res;scanline++) {
        outportb(0x3ce,4);
        SELECT_PLANE(plane++); pcx_encode_row(video_memory+pcx->bytes_per_line*scanline);
        SELECT_PLANE(plane++); pcx_encode_row(video_memory+pcx->bytes_per_line*scanline);
        SELECT_PLANE(plane++); pcx_encode_row(video_memory+pcx->bytes_per_line*scanline);
        SELECT_PLANE(plane++); pcx_encode_row(video_memory+pcx->bytes_per_line*scanline);
        /* BC30 removes the last increment before this reset, while retaining
         * the word load for the post-increment expression's old value.
         */
        plane=0;
    }
}

/* objcmp: {"function":"pcx_write_linear_rows","address":"19f0:0427","size":97,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void pcx_write_linear_rows(void)
{
    int y;
    for(y=0;y<pcx->vertical_res;y++) pcx_encode_row(video_memory+pcx->bytes_per_line*y);
    y=12; fputc(y,pcx_file);
    fwrite(pcx_palette,769,1,pcx_file);
}

#define PCX_FATAL(message,code) { restore_video_state(1); fprintf(stderr,message); exit(code); }

/* objcmp: {"function":"pcx_write_planar_rows","address":"19f0:0488","size":387,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void pcx_write_planar_rows(void)
{
    int y,x,column,plane,width;
    char *row;
    width=pcx->bytes_per_line/4;
    if(!(row=malloc(pcx->bytes_per_line))) PCX_FATAL(memory_error,903);
    for(y=0;y<pcx->vertical_res;y++) {
        plane=column=x=0;
        for(;x<width;x++) {
            outportb(0x3ce,4);
            SELECT_PLANE(plane++); row[column++]=video_memory[x+y*80];
            SELECT_PLANE(plane++); row[column++]=video_memory[x+y*80];
            SELECT_PLANE(plane++); row[column++]=video_memory[x+y*80];
            SELECT_PLANE(plane++); row[column++]=video_memory[x+y*80];
            plane=0;
        }
        pcx_encode_row(row);
    }
    free(row);
    y=12; fputc(y,pcx_file);
    fwrite(pcx_palette,769,1,pcx_file);
}

/* objcmp: {"function":"save_pcx","address":"19f0:060b","size":540,"segments":{"_BSS":"29bf:7756","_DATA":"29bf:3c66"}} */
void save_pcx(void)
{
    int handle;
    char filename[14];
    _fmode=0x8000;
    do {
        sprintf(filename,pcx_name_format,pcx_number++);
        handle=creatnew(filename,0);
        if(handle==-1 && errno!=35) PCX_FATAL(create_error,904);
    } while(handle==-1 && pcx_number<9999);
    if(pcx_number>9999) PCX_FATAL(number_error,905);
    if(!(pcx_file=fdopen(handle,binary_mode))) PCX_FATAL(stream_error,900);
    if(!(pcx=malloc(128))) PCX_FATAL(header_error,901);
    pcx_write_header();
    beep(600,55);
    video_memory=MK_FP(0xa000,0);
    if(g_5de6<=1) {
        g_5c33(0,0);
        FP_SEG(video_memory)=0xa000;
    } else {
        g_5c33(0,video_adapter==4?350:screen_half_height*2);
        FP_SEG(video_memory)=0xa000+page_paragraph;
    }
    FP_OFF(video_memory)=0;
    if(video_adapter==4) pcx_write_ega_rows();
    else {
        if(!(pcx_palette=malloc(769))) PCX_FATAL(palette_error,902);
        pcx_prepare_vga_palette();
        if(g_5de4) pcx_write_linear_rows(); else pcx_write_planar_rows();
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
