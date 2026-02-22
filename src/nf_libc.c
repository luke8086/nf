/*
 * Copyright (c) 2019 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * x86/nf_libc.c - standard library
 */

#include "nf_cmmn.h"

/* software interrupt trigger */
struct nf_regs {
	int ax, bx, cx, dx, bp, di, si, flags;
};

void nf_intr(int, struct nf_regs *);

/* heap pointers */
extern void *nf_heap_start;
static char *nf_heap_ptr = (char *)&nf_heap_start;

/* buffer for nf_readline */
#define NF_READLINE_BUF_SIZE 64
static char nf_readline_buf[NF_READLINE_BUF_SIZE];

/* local functions */
static void nf_putc(char);
static void nf_puts(char *);

/* allocate chunk of memory on the heap */
void *
nf_malloc(size_t size)
{
    void *ret = (void *)nf_heap_ptr;
    nf_heap_ptr += size;
    return ret;
}

/* deallocate given chunk of memory */
void
nf_free(void *ptr)
{
    (void)ptr;
}

/* print a single character to the screen */
static void
nf_putc(char c)
{
    struct nf_regs regs;
    regs.ax = 0x0e00 | c;
    regs.bx = 0x0000;
    nf_intr(0x10, &regs);
}

/* print a string to the screen */
static void
nf_puts(char *s)
{
    while (*s) {
        if (*s == '\n')
            nf_putc('\r');
        nf_putc(*s);
        s++;
    }
}

/* read a single character */
char
nf_getc(void)
{
    struct nf_regs regs;
    regs.ax = 0x0000;
    nf_intr(0x16, &regs);
    return regs.ax & 0xff;
}

/* read a line of text */
char *
nf_readline(void)
{
    char ch;
    char *start = nf_readline_buf;
    char *end = start + NF_READLINE_BUF_SIZE - 1;
    char *buf = start;
    *buf = 0;

    for (;;) {
        ch = nf_getc();

        if (ch == 0x08 && buf > start) {
            nf_putc(ch);
            nf_putc(' ');
            nf_putc(ch);
            buf--;
        } else if (ch == 0x08) {
            /* pass */
        } else if (ch == 0x0d) {
            nf_printf("\n");
            *buf = 0;
            return start;
        } else if (buf < end) {
            nf_putc(ch);
            *buf = ch;
            buf++;
        }
    }

    /* NOTREACHED */
}

/* formatted print */
int
nf_printf(const char *format, ...)
{
    va_list ap;
    char buf[1024];
    int ret;

    va_start(ap, format);
    ret = nf_vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (ret >= 0)
        nf_puts(buf);

    return ret;
}

/* exit interpreter with given status code */
void
nf_exit(char code)
{
    if (*((unsigned *)0) == 0x20CD) {
        struct nf_regs regs;
        regs.ax = 0x4c00 | code;
        nf_intr(0x21, &regs);
    }

    for (;;) { };
    /* NOTREACHED */
}
