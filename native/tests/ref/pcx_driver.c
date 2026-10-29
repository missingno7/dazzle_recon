/* Deliberately supply two sentinel bytes beyond the row, so the DOS encoder's
 * comparison-before-bound-check is observable without unallocated memory. */
#include "compat.h"
#include <stdio.h>
#include <string.h>

struct pcx_header {
    unsigned char manufacturer, version, encoding, bits_per_pixel;
    RefWord xmin, ymin, xmax, ymax, horizontal_res, vertical_res;
    unsigned char palette[48], reserved, planes;
    RefWord bytes_per_line;
    unsigned char remainder[60];
};
extern struct pcx_header *pcx;
extern FILE *pcx_file;
#ifdef REF_NATIVE
extern int16_t pcx_encode_row(uint8_t *row, uint8_t after_row);
#else
extern int pcx_encode_row(char *row);
unsigned video_adapter, page_paragraph, background_fill_word, palette_base_green;
unsigned bios_resolution_mode, display_pan_state;
int screen_half_height;
unsigned palette_cycle_offset;
unsigned char *palette_buffer;
void restore_video_state(int mode) { (void)mode; }
void beep(unsigned hz, unsigned ms) { (void)hz; (void)ms; }
void (*display_start_callback)(unsigned, unsigned);
#endif

static void pcx_cases(FILE *out, const char *scratch)
{
    static struct pcx_header header;
    static unsigned char row[324];
    static unsigned lengths[] = {0, 1, 2, 3, 4, 62, 63, 64, 65, 80, 320};
    unsigned i, value, following, j, length;
    int result, byte;
    pcx = &header;
    for (i = 0; i < sizeof(lengths)/sizeof(lengths[0]); ++i) {
        length = lengths[i];
        for (value = 7; value <= 199; value += 192) {
            for (following = 0; following < 2; ++following) {
                memset(row, value, sizeof row);
                row[length] = (unsigned char)(following ? value + 1 : value);
                row[length + 1] = (unsigned char)(value + 2);
                header.bytes_per_line = length;
                pcx_file = fopen(scratch, "wb");
                if (!pcx_file) exit(2);
#ifdef REF_NATIVE
                result = pcx_encode_row(row, row[length]);
#else
                result = pcx_encode_row((char *)row);
#endif
                fclose(pcx_file);
                fprintf(out, "pcx,%u,%u,%u,%d,", length, value, following, result);
                pcx_file = fopen(scratch, "rb");
                if (!pcx_file) exit(3);
                j = 0;
                while ((byte = fgetc(pcx_file)) != EOF) {
                    fprintf(out, "%02X", (unsigned)byte); ++j;
                }
                fclose(pcx_file);
                fprintf(out, ",%u\n", j);
            }
        }
    }
    pcx = 0; pcx_file = 0;
    remove(scratch);
}

#ifndef REF_NATIVE
void main(int argc, char far * far *argv)
{
    FILE *out = fopen("PCX.TXT", "w");
    (void)argc; (void)argv;
    if (!out) exit(2);
    pcx_cases(out, "ROW.BIN");
    fclose(out);
    exit(0);
}
#endif
