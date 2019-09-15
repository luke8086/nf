/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * os64/nf_libc.c - os64 libc interface
 */

#include <kernel/kernel.h>

#include "../nf_cmmn.h"

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

/* exit interpreter with given status code */
void
nf_exit(char code)
{
    task_exit(code);
    /* NOTREACHED */
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
