/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * os64/nf_libc.c - os64 libc interface
 */

#include <kernel/kernel.h>

#include "../nf_common.h"

/* allocate a chunk of memory */
void *
nf_malloc(size_t size)
{
    return kheap_alloc(size);
}

/* deallocate a chunk of memory */
void
nf_free(void *ptr)
{
    kheap_free(ptr);
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

/* exit interpreter with given status code */
void
nf_exit(int code)
{
    task_exit(code);
    // NOTREACHED
}

/* write a string to the terminal */
int
nf_puts(const char *s)
{
    int fd;
    ssize_t len;

    len = strlen(s);

    fd = vfs_open("/dev/vt");

    if (fd < 0)
        return -1;

    if (file_write(fd, (void*)s, len) != len)
        return -1;

    if (file_close(fd))
        return -1;

    return len;
}

/* formatted print */
int
nf_printf(const char *fmt, ...)
{
    int ret;
    va_list args;
    char buf[1024];

    va_start(args, fmt);
    ret = nf_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (ret < 0)
        return -1;

    if (nf_puts(buf) != ret)
        return -1;

    return ret;
}
