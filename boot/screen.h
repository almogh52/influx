#pragma once

#include <stdint.h>

#include "main.h"

/*  The number of columns. */
#define COLUMNS 80
/*  The number of lines. */
#define LINES 25
/*  The attribute of an character. */
#define ATTRIBUTE 7
/*  The video memory address. */
#define VIDEO (HIGHER_HALF_OFFSET + 0xB8000)
/*  The max size of argument buffer. */
#define MAX_ARG_BUF_SIZE 50

void init_tty();
void itoa(char *buf, char base, int64_t d);
void putchar(int c);
void printf(const char *format, ...);