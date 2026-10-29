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

/* 19F0:0004, 3 bytes */
unsigned dazpcx_seg(void) { return _CS; }

/* 19F0:0007, 70 bytes */
void pcx_prepare_vga_palette(void)
{
    int i;
    for(i=0;i<9;i++) pcx_palette[i]=palette_buffer[i]<<2;
    for(;i<769;i++) pcx_palette[i]=*(palette_cycle_offset+i+palette_buffer)<<2;
}

/* 19F0:004D, 405 bytes */
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

/* 19F0:01E2, 196 bytes */
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
        if(bios_resolution_mode) { pcx->ymax=199; pcx->vertical_res=200; }
        else { pcx->ymax=399; pcx->vertical_res=400; }
    }
    fwrite(pcx,1,128,pcx_file);
}

/* 19F0:02A6, 216 bytes */
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

/* 19F0:037E, 169 bytes */
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

/* 19F0:0427, 97 bytes */
void pcx_write_linear_rows(void)
{
    int y;
    for(y=0;y<pcx->vertical_res;y++) pcx_encode_row(video_memory+pcx->bytes_per_line*y);
    y=12; fputc(y,pcx_file);
    fwrite(pcx_palette,769,1,pcx_file);
}

#define PCX_FATAL(message,code) { restore_video_state(1); fprintf(stderr,message); exit(code); }

/* 19F0:0488, 387 bytes */
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

/* 19F0:060B, 540 bytes */
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
    if(display_pan_state<=1) {
        display_start_callback(0,0);
        FP_SEG(video_memory)=0xa000;
    } else {
        display_start_callback(0,video_adapter==4?350:screen_half_height*2);
        FP_SEG(video_memory)=0xa000+page_paragraph;
    }
    FP_OFF(video_memory)=0;
    if(video_adapter==4) pcx_write_ega_rows();
    else {
        if(!(pcx_palette=malloc(769))) PCX_FATAL(palette_error,902);
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
