/* Borland C++ 3.0 conio subset for the native 80x25 text console. */
#include "dazzle.h"
#include "dzconio.h"
#include "platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int win_left = 1, win_top = 1, win_right = 80, win_bottom = 25;
static uint8_t attribute = 7, normal_attribute = 7;
static int cursor_start = 14, cursor_end = 15;
static int pending_scan = -1;

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

void dz_conio_reset(void)
{
    win_left = win_top = 1;
    win_right = 80;
    win_bottom = 25;
    attribute = normal_attribute = 7;
    cursor_start = 14;
    cursor_end = 15;
    pending_scan = -1;
    dz_video.cursor_x = dz_video.cursor_y = 0;
    dz_video.cursor_visible = 1;
}

int putch(int character)
{
    int x = dz_video.cursor_x + 1;
    int y = dz_video.cursor_y + 1;
    unsigned char byte = (unsigned char)character;

    if (x < win_left || x > win_right || y < win_top || y > win_bottom) {
        x = win_left;
        y = win_top;
    }
    switch (byte) {
    case '\r':
        x = win_left;
        break;
    case '\n':
        ++y;
        break;
    case '\b':
        if (x > win_left) --x;
        break;
    case '\a':
        break;
    default:
        dz_video.text[y - 1][x - 1][0] = byte;
        dz_video.text[y - 1][x - 1][1] = attribute;
        if (++x > win_right) {
            x = win_left;
            ++y;
        }
        break;
    }
    if (y > win_bottom) {
        scroll_window();
        y = win_bottom;
    }
    dz_video.cursor_x = x - 1;
    dz_video.cursor_y = y - 1;
    dz_charge(DZ_PLOT_UNITS);
    return byte;
}

int cputs(const char *text)
{
    int last = 0;
    while (*text) last = putch((unsigned char)*text++);
    return last;
}

int cprintf(const char *format, ...)
{
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0) {
        va_end(args);
        return length;
    }
    char *buffer = (char *)malloc((size_t)length + 1u);
    if (!buffer) {
        va_end(args);
        return -1;
    }
    vsnprintf(buffer, (size_t)length + 1u, format, args);
    va_end(args);
    cputs(buffer);
    free(buffer);
    return length;
}

void gotoxy(int x, int y)
{
    if (x < 1 || y < 1 || x > win_right - win_left + 1 || y > win_bottom - win_top + 1)
        return;
    dz_video.cursor_x = win_left + x - 2;
    dz_video.cursor_y = win_top + y - 2;
}

int wherex(void) { return dz_video.cursor_x + 2 - win_left; }
int wherey(void) { return dz_video.cursor_y + 2 - win_top; }

void textcolor(int color)
{
    attribute = (uint8_t)((attribute & 0x70u) | ((unsigned)color & 0x8fu));
}

void textbackground(int color)
{
    attribute = (uint8_t)((attribute & 0x8fu) | (((unsigned)color & 7u) << 4));
}

void textattr(int value) { attribute = (uint8_t)value; }
void highvideo(void) { attribute |= 0x08u; }
void lowvideo(void) { attribute &= (uint8_t)~0x08u; }
void normvideo(void) { attribute = normal_attribute; }

void window(int left, int top, int right, int bottom)
{
    if (left < 1 || top < 1 || right > 80 || bottom > 25 || left > right || top > bottom)
        return;
    win_left = left;
    win_top = top;
    win_right = right;
    win_bottom = bottom;
    gotoxy(1, 1);
}

void clreol(void)
{
    if (dz_video.cursor_y < win_top - 1 || dz_video.cursor_y >= win_bottom ||
        dz_video.cursor_x < win_left - 1 || dz_video.cursor_x >= win_right)
        return;
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

static int valid_rect(int left, int top, int right, int bottom)
{
    return left >= 1 && top >= 1 && right <= 80 && bottom <= 25 &&
        left <= right && top <= bottom;
}

int gettext(int left, int top, int right, int bottom, void *destination)
{
    if (!destination || !valid_rect(left, top, right, bottom)) return 0;
    uint8_t *bytes = (uint8_t *)destination;
    size_t offset = 0;
    for (int y = top - 1; y < bottom; ++y)
        for (int x = left - 1; x < right; ++x) {
            bytes[offset++] = dz_video.text[y][x][0];
            bytes[offset++] = dz_video.text[y][x][1];
        }
    return 1;
}

int puttext(int left, int top, int right, int bottom, const void *source)
{
    if (!source || !valid_rect(left, top, right, bottom)) return 0;
    const uint8_t *bytes = (const uint8_t *)source;
    size_t offset = 0;
    for (int y = top - 1; y < bottom; ++y)
        for (int x = left - 1; x < right; ++x) {
            dz_video.text[y][x][0] = bytes[offset++];
            dz_video.text[y][x][1] = bytes[offset++];
        }
    return 1;
}

void gettextinfo(struct text_info *info)
{
    if (!info) return;
    info->winleft = (uint8_t)win_left;
    info->wintop = (uint8_t)win_top;
    info->winright = (uint8_t)win_right;
    info->winbottom = (uint8_t)win_bottom;
    info->attribute = attribute;
    info->normattr = normal_attribute;
    info->currmode = 3;
    info->screenheight = 25;
    info->screenwidth = 80;
    info->curx = (uint8_t)wherex();
    info->cury = (uint8_t)wherey();
}

void _setcursortype(int type)
{
    switch (type) {
    case _NOCURSOR:
        dz_video.cursor_visible = 0;
        break;
    case _SOLIDCURSOR:
        cursor_start = 0;
        cursor_end = 15;
        dz_video.cursor_visible = 1;
        break;
    case _NORMALCURSOR:
    default:
        cursor_start = 14;
        cursor_end = 15;
        dz_video.cursor_visible = 1;
        break;
    }
}

void dzconio_cursor_rows(int *start, int *end)
{
    if (start) *start = cursor_start;
    if (end) *end = cursor_end;
}

/* Borland getch: extended keys return 0 first, then the scan code. */
int getch(void)
{
    if (pending_scan >= 0) {
        int scan = pending_scan;
        pending_scan = -1;
        return scan;
    }
    uint16_t key = dz_bioskey(0);
    if ((key & 0xffu) == 0) {
        pending_scan = key >> 8;
        return 0;
    }
    return key & 0xffu;
}

int kbhit(void) { return pending_scan >= 0 || dz_kbhit(); }
