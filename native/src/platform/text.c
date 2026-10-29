/* Minimal 80x25 conio output. Cells are CP437 bytes with DOS text attributes. */
#include "platform.h"
#include <string.h>
static uint8_t text_attribute = 7;

void dz_text_attribute(uint8_t attribute) { text_attribute = attribute; }
void dz_text_putc(uint8_t character)
{
    if (character == '\r') dz_video.cursor_x = 0;
    else if (character == '\n') ++dz_video.cursor_y;
    else if (character == '\b') {
        if (dz_video.cursor_x > 0) --dz_video.cursor_x;
    } else if (character == '\t') {
        unsigned spaces = 8u - ((unsigned)dz_video.cursor_x & 7u);
        while (spaces--) dz_text_putc(' ');
    } else {
        dz_video.text[dz_video.cursor_y][dz_video.cursor_x][0] = character;
        dz_video.text[dz_video.cursor_y][dz_video.cursor_x][1] = text_attribute;
        if (++dz_video.cursor_x == 80) { dz_video.cursor_x = 0; ++dz_video.cursor_y; }
    }
    if (dz_video.cursor_y == 25) {
        memmove(dz_video.text[0], dz_video.text[1], sizeof dz_video.text[0] * 24u);
        --dz_video.cursor_y;
        for (unsigned x = 0; x < 80; ++x) {
            dz_video.text[24][x][0] = ' ';
            dz_video.text[24][x][1] = text_attribute;
        }
    }
    dz_charge(1);
}
void dz_text_write(const char *text)
{
    while (*text) dz_text_putc((uint8_t)*text++);
}
