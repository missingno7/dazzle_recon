/* Borland conio subset on the native text console (dz_video.text). Borland semantics: 1-based window
 * coordinates, cprintf/cputs do not translate '\n' to "\r\n", output scrolls inside the window. */
#include "dazzle.h"
#include "dzconio.h"
#include "platform.h"
#include <stdarg.h>
#include <stdio.h>

static int win_left = 1, win_top = 1, win_right = 80, win_bottom = 25;
static uint8_t attribute = 7, normal_attribute = 7;

static void scroll_window(void)
{
    for (int y = win_top; y < win_bottom; ++y)
        for (int x = win_left; x <= win_right; ++x) {
            dz_video.text[y - 1][x - 1][0] = dz_video.text[y][x - 1][0];
            dz_video.text[y - 1][x - 1][1] = dz_video.text[y][x - 1][1];
        }
    for (int x = win_left; x <= win_right; ++x) {
        dz_video.text[win_bottom - 1][x - 1][0] = ' ';
        dz_video.text[win_bottom - 1][x - 1][1] = attribute;
    }
}

int putch(int character)
{
    int x = dz_video.cursor_x + 1, y = dz_video.cursor_y + 1;   /* absolute, 1-based */
    switch (character) {
    case '\r': x = win_left; break;
    case '\n': ++y; break;
    case '\b': if (x > win_left) --x; break;
    case '\a': break;
    default:
        dz_video.text[y - 1][x - 1][0] = (uint8_t)character;
        dz_video.text[y - 1][x - 1][1] = attribute;
        if (++x > win_right) { x = win_left; ++y; }
    }
    if (y > win_bottom) { scroll_window(); y = win_bottom; }
    dz_video.cursor_x = x - 1;
    dz_video.cursor_y = y - 1;
    dz_charge(1);
    return (uint8_t)character;
}

int cputs(const char *text)
{
    int last = 0;
    while (*text) last = putch((uint8_t)*text++);
    return last;
}

int cprintf(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    cputs(buffer);
    return length;
}

void gotoxy(int x, int y)
{
    if (x < 1 || y < 1 || x > win_right - win_left + 1 || y > win_bottom - win_top + 1) return;
    dz_video.cursor_x = win_left + x - 2;
    dz_video.cursor_y = win_top + y - 2;
}
int wherex(void) { return dz_video.cursor_x + 2 - win_left; }
int wherey(void) { return dz_video.cursor_y + 2 - win_top; }

void textcolor(int color) { attribute = (uint8_t)((attribute & 0x70) | (color & 0x8f)); }
void textbackground(int color) { attribute = (uint8_t)((attribute & 0x8f) | ((color & 7) << 4)); }
void textattr(int value) { attribute = (uint8_t)value; }
void highvideo(void) { attribute |= 0x08; }
void lowvideo(void) { attribute &= (uint8_t)~0x08; }
void normvideo(void) { attribute = normal_attribute; }

void window(int left, int top, int right, int bottom)
{
    if (left < 1 || top < 1 || right > 80 || bottom > 25 || left > right || top > bottom) return;
    win_left = left; win_top = top; win_right = right; win_bottom = bottom;
    gotoxy(1, 1);
}

void clreol(void)
{
    for (int x = dz_video.cursor_x; x < win_right; ++x) {
        dz_video.text[dz_video.cursor_y][x][0] = ' ';
        dz_video.text[dz_video.cursor_y][x][1] = attribute;
    }
}

void clrscr(void)
{
    for (int y = win_top; y <= win_bottom; ++y)
        for (int x = win_left; x <= win_right; ++x) {
            dz_video.text[y - 1][x - 1][0] = ' ';
            dz_video.text[y - 1][x - 1][1] = attribute;
        }
    gotoxy(1, 1);
}

void gettextinfo(struct text_info *info)
{
    info->winleft = (uint8_t)win_left; info->wintop = (uint8_t)win_top;
    info->winright = (uint8_t)win_right; info->winbottom = (uint8_t)win_bottom;
    info->attribute = attribute; info->normattr = normal_attribute;
    info->currmode = 3; info->screenheight = 25; info->screenwidth = 80;
    info->curx = (uint8_t)wherex(); info->cury = (uint8_t)wherey();
}

/* Borland getch: extended keys return 0 first, then the scan code. */
static int pending_scan = -1;
int getch(void)
{
    if (pending_scan >= 0) { int scan = pending_scan; pending_scan = -1; return scan; }
    uint16_t key = dz_bioskey(0);
    if ((key & 0xff) == 0) { pending_scan = key >> 8; return 0; }
    return key & 0xff;
}
int kbhit(void) { return pending_scan >= 0 || dz_kbhit(); }
