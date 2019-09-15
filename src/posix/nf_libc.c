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

/* copy memory area */
void *
nf_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

/* calculate the length of a string */
size_t
nf_strlen(const char *s1)
{
    return strlen(s1);
}

/* compare two strings */
int
nf_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

/* copy a string */
char *
nf_strncpy(char *dest, const char *src, size_t n)
{
    return strncpy(dest, src, n);
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
nf_exit(int code)
{
    exit(code);
    /* NOTREACHED */
}
