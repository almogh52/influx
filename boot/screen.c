#include "screen.h"

#include <stdarg.h>

/*  Save the X position. */
static int xpos;

/*  Save the Y position. */
static int ypos;

/*  Point to the video memory. */
static volatile unsigned char *video;

void init_tty()
{
    int i;

    // Set video to point at the video memory
    video = (unsigned char *)VIDEO;

    // Clear the screen
    for (i = 0; i < COLUMNS * LINES * 2; i++)
        *(video + i) = 0;

    // Set initial (x, y) pos of the cursor
    xpos = 0;
    ypos = 0;
}

void itoa(char *buf, int base, int d)
{
    char *p = buf;
    char *p1, *p2;
    long ud = d;
    unsigned int divisor = 10;

    /*  If %d is specified and D is minus, put `-' in the head. */
    if (base == 'd' && d < 0)
    {
        *p++ = '-';
        buf++;
        ud = -d;
    }
    else if (base == 'x')
        divisor = 16;

    /*  Divide UD by DIVISOR until UD == 0. */
    do
    {
        unsigned int remainder = ud % divisor;

        *p++ = (remainder < 10) ? (char)remainder + '0' : (char)remainder + 'a' - 10;
    } while (ud /= divisor);

    /*  Terminate BUF. */
    *p = 0;

    /*  Reverse BUF. */
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2)
    {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

void scroll()
{
    // Move all lines from line 2, 1 up
    for (int i = COLUMNS * 2; i < COLUMNS * (LINES) * 2; i++)
    {
        *(video + i - COLUMNS * 2) = *(video + i);
    }
}

void putchar(int c)
{
    if (c == '\n' || c == '\r')
    {
    newline:
        xpos = 0;
        if (ypos + 1 >= LINES)
            scroll();
        else
            ypos++;
        return;
    }

    *(video + (xpos + ypos * COLUMNS) * 2) = c & 0xFF;
    *(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

    xpos++;
    if (xpos >= COLUMNS)
        goto newline;
}

void printf(const char *format, ...)
{
    va_list ap;
    int c;
    char buf[20];

    va_start(ap, format);

    while ((c = *format++) != 0)
    {
        if (c != '%')
            putchar(c);
        else
        {
            char *p, *p2;
            int pad0 = 0, pad = 0;

            c = *format++;
            if (c == '0')
            {
                pad0 = 1;
                c = *format++;
            }

            if (c >= '0' && c <= '9')
            {
                pad = c - '0';
                c = *format++;
            }

            switch (c)
            {
            case 'd':
            case 'u':
            case 'x':
                itoa(buf, c, va_arg(ap, int));
                p = buf;
                goto string;
                break;

            case 's':
                p = va_arg(ap, char *);
                if (!p)
                    p = "(null)";

            string:
                for (p2 = p; *p2; p2++)
                    ;
                for (; p2 < p + pad; p2++)
                    putchar(pad0 ? '0' : ' ');
                while (*p)
                    putchar(*p++);
                break;

            default:
                putchar(va_arg(ap, int));
                break;
            }
        }
    }

    va_end(ap);
}