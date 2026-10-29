/* Text output for startup/diagnostic strings shares the Borland conio state. */
#include "dzconio.h"
#include <stdint.h>

void dz_text_attribute(uint8_t attribute)
{
    textattr(attribute);
}

void dz_text_putc(uint8_t character)
{
    putch(character);
}

void dz_text_write(const char *text)
{
    cputs(text);
}
