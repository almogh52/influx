#include "screen.h"

#include <stdarg.h>
#include <stdbool.h>

void scroll();

/*  Save the X position. */
static int xpos;

/*  Save the Y position. */
static int ypos;

/*  Point to the video memory. */
static volatile unsigned char *video;

void init_tty() {
    int i;

    // Disable cursor using hardware ports
    __asm__ __volatile__(
        "mov dx, 0x3D4;"
        "mov al, 0xA;"  // Low cursor shape register
        "out dx, al;"
        "inc dx;"
        "mov al, 0x20;"  // Bits 6-7 unused, bit 5 disables the cursor, bits 0-4 control the cursor
                         // shape
        "out dx, al"
        :
        :
        : "dx", "al");

    // Set video to point at the video memory
    video = (unsigned char *)VIDEO;

    // Clear the screen
    for (i = 0; i < COLUMNS * LINES * 2; i++) *(video + i) = 0;

    // Set initial (x, y) pos of the cursor
    xpos = 0;
    ypos = 0;
}

void itoa(char *buf, char base, int64_t d) {
    char *p = buf;
    char *p1, *p2;
    uint64_t ud = (uint64_t)d;
    uint64_t divisor = 10;

    /*  If %d is specified and D is minus, put `-' in the head. */
    if (base == 'd' && d < 0) {
        *p++ = '-';
        buf++;
        ud = (uint64_t)-d;
    } else if (base == 'x')
        divisor = 16;

    /*  Divide UD by DIVISOR until UD == 0. */
    do {
        uint64_t remainder = ud % divisor;

        *p++ = (char)((remainder < 10) ? (char)remainder + '0' : (char)remainder + 'a' - 10);
    } while (ud /= divisor);

    /*  Terminate BUF. */
    *p = 0;

    /*  Reverse BUF. */
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

void scroll() {
    // Move all lines from line 2, 1 up
    for (int i = COLUMNS * 2; i < COLUMNS * LINES * 2; i++) {
        *(video + i - COLUMNS * 2) = *(video + i);
    }

    // Reset the last row
    for (int i = COLUMNS * (LINES - 1) * 2; i < COLUMNS * (LINES + 1) * 2; i++) {
        *(video + i) = 0;
    }
}

void putchar(int c) {
    if (c == '\n' || c == '\r') {
    newline:
        xpos = 0;

        // If reached the last line, just scroll
        if (ypos == LINES - 1) {
            scroll();
        } else {
            ypos++;
        }

        return;
    }

    *(video + (xpos + ypos * COLUMNS) * 2) = (unsigned char)c & 0xFF;
    *(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

    xpos++;
    if (xpos >= COLUMNS) goto newline;  // Handle line overflow by breaking a line
}

void printf(const char *format, ...) {
    va_list ap;
    char c;
    char buf[MAX_ARG_BUF_SIZE];

    va_start(ap, format);

    while ((c = *format++) != 0) {
        if (c != '%')
            putchar(c);
        else {
            char *p, *p2;
            bool pad0 = false, l64 = false;
            int pad = 0;

            c = *format++;
            if (c == '0') {
                pad0 = true;
                c = *format++;
            }

            if (c >= '0' && c <= '9') {
                pad = c - '0';
                c = *format++;
            }

            if (c == 'l') {
                l64 = true;
                c = *format++;
            }

            switch (c) {
                case 'd':
                case 'u':
                case 'x':
                    itoa(buf, c, l64 ? va_arg(ap, int64_t) : va_arg(ap, int32_t));
                    p = buf;
                    goto string;
                    break;

                case 's':
                    p = va_arg(ap, char *);
                    if (!p) p = "(null)";

                string:
                    for (p2 = p; *p2; p2++)
                        ;
                    for (; p2 < p + pad; p2++) putchar(pad0 ? '0' : ' ');
                    while (*p) putchar(*p++);
                    break;

                default:
                    putchar(va_arg(ap, int));
                    break;
            }
        }
    }

    va_end(ap);
}
