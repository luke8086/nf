/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * os64/nf_words.c - os64 platform specific words
 */

#include <kernel/kernel.h>
#include <gui/gui.h>

#include "../nf_cmmn.h"

/* spawn a new nf process ( ... argc name -- pid ) */
static int
nf_sys_spawn(struct nf_machine *m)
{
    char *name, **argv;
    int i, argc;

    if (nf_data_check(m, 2, 1))
        return -1;

    name = (char*)nf_data_pop(m);
    argc = (int)nf_data_pop(m);

    argv = nf_malloc(argc * sizeof(char*));

    if (!argv) {
        nf_error("out of memory");
        return -1;
    }

    if (nf_data_check(m, argc, 1))
        return -1;

    for (i = 0; i < argc; ++i) {
        argv[i] = (char*)nf_data_pop(m);
    }

    task_pid_t pid = task_spawn_name(name, argc, argv);
    nf_data_push(m, pid);

    return 0;
}

/* wait a new nf process ( pid -- code ) */
static int
nf_sys_wait(struct nf_machine *m)
{
    task_pid_t pid;
    int code;

    if (nf_data_check(m, 1, 1))
        return -1;

    pid = (task_pid_t)nf_data_pop(m);
    code = task_waitpid(pid);
    nf_data_push(m, code);

    return 0;
}

/* open a file ( name -- fd ) */
static int
nf_sys_open(struct nf_machine *m)
{
    char *path;
    int fd;

    if (nf_data_check(m, 1, 1))
        return -1;

    path = (char*)nf_data_pop(m);
    fd = vfs_open(path);
    nf_data_push(m, fd);

    return 0;
}

/* close a file ( fd -- ret ) */
static int
nf_sys_close(struct nf_machine *m)
{
    int fd;
    int ret;

    if (nf_data_check(m, 1, 1))
        return -1;

    fd = (int)nf_data_pop(m);
    ret = file_close(fd);
    nf_data_push(m, ret);

    return 0;
}

/* read from file ( nbyte buf fd -- ret ) */
static int
nf_sys_read(struct nf_machine *m)
{
    size_t nbyte;
    void *buf;
    int fd;
    ssize_t ret;

    if (nf_data_check(m, 3, 1))
        return -1;

    fd = (int)nf_data_pop(m);
    buf = (void*)nf_data_pop(m);
    nbyte = (size_t)nf_data_pop(m);
    ret = file_read(fd, buf, nbyte);
    nf_data_push(m, ret);

    return 0;
}

/* write to file ( nbyte buf fd -- ret ) */
static int
nf_sys_write(struct nf_machine *m)
{
    size_t nbyte;
    void *buf;
    int fd;
    ssize_t ret;

    if (nf_data_check(m, 3, 1))
        return -1;

    fd = (int)nf_data_pop(m);
    buf = (void*)nf_data_pop(m);
    nbyte = (size_t)nf_data_pop(m);
    ret = file_write(fd, buf, nbyte);
    nf_data_push(m, ret);

    return 0;
}

/* terminate process ( code -- ) */
static int
nf_sys_exit(struct nf_machine *m)
{
    int code;

    if (nf_data_check(m, 1, 0))
        return -1;

    code = (int)nf_data_pop(m);
    task_exit(code);

    /* NOTREACHED */
    return 0;
}

/* allocate a chunk of memory ( size -- ptr ) */
static int
nf_sys_malloc(struct nf_machine *m)
{
    size_t size;
    void *addr;

    if (nf_data_check(m, 1, 1))
        return -1;

    size = (size_t)nf_data_pop(m);
    addr = nf_malloc(size);
    nf_data_push(m, (nf_cell_t)size);

    return 0;
}

/* deallocate a chunk of memory ( ptr -- ) */
static int
nf_sys_free(struct nf_machine *m)
{
    void *addr;

    if (nf_data_check(m, 1, 0))
        return -1;

    addr = (void *)nf_data_pop(m);
    nf_free(addr);

    return 0;
}

/* change desktop background ( path -- ) */
static int
nf_sys_set_bg(struct nf_machine *m)
{
    char *path;

    if (nf_data_check(m, 1, 0))
        return -1;

    path = (char *)nf_data_pop(m);
    gui_set_bg(path);

    return 0;
}

/* define os64 system words */
void
nf_define_os64_words(struct nf_machine *m)
{
    int i, count;

    static struct nf_word words[] = {
        { .name = "sys-spawn",    .data = (void*)nf_sys_spawn },
        { .name = "sys-wait",     .data = (void*)nf_sys_wait },
        { .name = "sys-open",     .data = (void*)nf_sys_open },
        { .name = "sys-close",    .data = (void*)nf_sys_close },
        { .name = "sys-read",     .data = (void*)nf_sys_read },
        { .name = "sys-write",    .data = (void*)nf_sys_write },
        { .name = "sys-exit",     .data = (void*)nf_sys_exit },
        { .name = "sys-malloc",   .data = (void*)nf_sys_malloc },
        { .name = "sys-free",     .data = (void*)nf_sys_free },
        { .name = "sys-set-bg",   .data = (void*)nf_sys_set_bg },
    };
    count = sizeof(words) / sizeof(words[0]);
    for (i = 0; i < count; ++i) {
        words[i].type = NF_WORD_PRIM;
        nf_define_word(m, &words[i]);
    }
}
