/* Borland conio subset used by DAZZLE, implemented on the native 80x25 text console
 * (native/src/platform/conio.c). Coordinates are 1-based and relative to the current window,
 * as in Borland C++ 3.0. */
#ifndef DAZZLE_DZCONIO_H
#define DAZZLE_DZCONIO_H

#include <stdint.h>

struct text_info;

void gotoxy(int x, int y);
int wherex(void);
int wherey(void);
int cprintf(const char *format, ...);
int cputs(const char *text);
int putch(int character);
void textcolor(int color);
void textbackground(int color);
void textattr(int attribute);
void highvideo(void);
void lowvideo(void);
void normvideo(void);
void window(int left, int top, int right, int bottom);
void clrscr(void);
void clreol(void);
void gettextinfo(struct text_info *info);
int getch(void);
int kbhit(void);

#endif
