/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * os64/nf_main.c - os64 main routine
 */

#include <kernel/kernel.h>

#include "../nf_cmmn.h"

/* local functions */
static void nf_chattr(unsigned char attr);
static void nf_display_prompt(struct nf_machine *m);
static int nf_get_char(char *ch, int fd);
ssize_t nf_get_line(char *buf, size_t size, int *eof, int fd);
static int nf_interpret(struct nf_machine *m, char *path);
static int nf_run(int argc, char **argv);

/* set current char attribute */
static void
nf_chattr(unsigned char attr)
{
    nf_printf("\x1b\x04%c", attr);
}

/* get single character from keyboard */
/* display command line prompt */
static void
nf_display_prompt(struct nf_machine *m)
{
    nf_cell_t *c;
    int n = 5;
    char buf[80];
    int attr_border = 0x0a;
    int attr_text = 0x0f;
    int attr_hint = 0x0d;
    int attr_stack = 0x0e;

    /* tiny prompt for compilation mode */
    if (m->state == NF_STATE_COMPILE) {
        nf_chattr(0x0a);
        nf_printf("    ... ");
        nf_chattr(0x0f);
        return;
    }

    /* print the top row */
    nf_chattr(attr_border);
    nf_printf("\xda");
    memset(buf, 0xc4, 79);
    buf[79] = 0;
    nf_printf(buf);
    nf_printf("\xb3 ");

    /* handle empty stack */
    if (m->data_sp == m->data_stack) {

        nf_chattr(attr_hint);
        nf_printf("(empty)");

    /* display stack contents */
    } else {

        /* select at no more than n top items */
        c = m->data_sp - n;
        if (c < m->data_stack) {
            c = m->data_stack;
        }

        /* print ellipsis if there are more elements than displayed */
        if (c > m->data_stack) {
            nf_chattr(attr_hint);
            nf_printf("(...) ");
        }

        /* print stack elements */
        nf_chattr(attr_stack);
        while (c < m->data_sp) {
            nf_printf("%ld ", *c++);
        }

    }

    /* print the bottom row */
    nf_printf("\n");
    nf_chattr(attr_border);
    nf_printf("\xc0\xc4\xc4 >>> ");
    nf_chattr(attr_text);
}

/* read a single character from file or keyboard. return 0 on success */
static int
nf_get_char(char *ch, int fd)
{
    uint16_t key;

    *ch = 0;

    /* read from file */
    if (fd >= 0) {
        return file_read(fd, ch, 1) == 1 ? 0 : -1;

    /* read from keyboard */
    } else {

        while (kbd_read(&key))
            (void)task_switch();
        *ch = key & 0xFF;

        return 0;
    }
}

/* read line from file or keyboard. return length of the line or -1 on error */
ssize_t
nf_get_line(char *buf, size_t size, int *eof, int fd)
{
    int ret;
    char ch;
    size_t n;
    int intv;

    n = 0;
    intv = (fd < 0);
    *eof = 0;

    if (!size)
        return -1;

    /* fill buffer with 0 and leave space for at least one terminator */
    memset(buf, 0, size--);

    while (1) {
        ret = nf_get_char(&ch, fd);

        /* end of file */
        if (ret != 0) {
            *eof = 1;
            return n;
        }

        /* backspace in interactive mode with no previous characters */
        else if (intv && ch == '\b' && n == 0) {
            continue;
        }

        /* backspace in interactive mode with previous characters present */
        else if (intv && ch == '\b' && n > 0) {
            --n;
            nf_printf("\b");
            continue;
        }

        /* regular character with overflow */
        else if (n == size && ch != '\n') {
            continue;
        }

        /* regular character with no overflow */
        else if (n < size && ch != '\n') {
            buf[n++] = ch;
            if (intv)
                nf_printf("%c", ch);
            continue;
        }

        /* return key */
        else if (ch == '\n') {
            if (intv)
                nf_printf("\n");
            return n;
        }
    }

    /* NOTREACHED */
    return -1;
}

/* main interpreter loop */
static int
nf_interpret(struct nf_machine *m, char *path)
{
    char line[256];
    int eof, ret, intv;
    ssize_t size;
    int fd;

    fd = -1;
    intv = !path;
    ret = 0;
    eof = 0;

    /* open source file in file mode */
    if (path) {
        fd = vfs_open(path);
    }

    /* error when opening file */
    if (path && fd < 0) {
        nf_error(("cannot open file: %s", path));
        return -1;
    }

    /* interpret lines until end of file */
    while (!eof) {

        /* show prompt and get line */
        if (intv)
            nf_display_prompt(m);
        size = nf_get_line(line, sizeof(line), &eof, fd);

        /* error in file mode */
        if (size < 0 && !intv) {
            ret = -1;
            break;
        }

        /* error in interactive mode or empty line */
        if (size <= 0) {
            continue;
        }

        /* interpret line. in file mode, break on error */
        if (nf_intp_line(m, line) && !intv) {
            ret = -1;
            break;
        }
    }

    /* cleanup */
    if (fd >= 0)
        file_close(fd);

    return ret;
}

/* execute interpreter. return 0 on success. */
static int
nf_run(int argc, char **argv)
{
    struct nf_machine *m;

    /* initialize virtual machine */
    if (!(m = nf_init_machine(argc, argv))) {
        nf_error(("out of memory"));
        return -1;
    };

    /* setup os64-specific words */
    extern void nf_define_os64_words(struct nf_machine *m);
    nf_define_os64_words(m);

    (void)nf_interpret(m, "/data/init.nf");

    /* if no filename is given, run in interactive mode */
    char *path = argc ? argv[0] : 0;

    return nf_interpret(m, path);
}

/* main entry point */
void
nf_main(int argc, char **argv)
{
    int ret;

    ret = nf_run(argc, argv);
    nf_exit(ret);

    /* NOTREACHED */
}
