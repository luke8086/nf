/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * posix/nf_libc.c - posix libc interface
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../nf_cmmn.h"

/* allocate chunk of memory on the heap */
void *
nf_malloc(size_t size)
{
    return malloc(size);
}

/* deallocate given chunk of memory */
void
nf_free(void *ptr)
{
    free(ptr);
}

/* formatted print */
int nf_printf(const char *format, ...)
{
    va_list ap;
    char buf[1024];
    int ret;

    va_start(ap, format);
    ret = nf_vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (ret >= 0)
        fputs(buf, stdout);

    return ret;
}

/* exit interpreter with given status code */
void
nf_exit(char code)
{
    exit(code);
    /* NOTREACHED */
}
