#pragma once

/*  The number of columns. */
#define COLUMNS 80
/*  The number of lines. */
#define LINES 25
/*  The attribute of an character. */
#define ATTRIBUTE 7
/*  The video memory address. */
#define VIDEO 0xB8000

void init_tty();
void scroll();
void itoa(char *buf, int base, int d);
void putchar(int c);
void printf(const char *format, ...);