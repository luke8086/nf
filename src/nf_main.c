/*
 * Copyright (c) 2019 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * x86/nf_main.c - main interpreter loop
 */

#include "nf_cmmn.h"

/* platform functions */
char *nf_readline(void);
void nf_define_x86_words(struct nf_machine *);

/* embedded init code (from nf_strt.asm) */
extern void *nf_init_code;
extern void *nf_init_code_end;

/* local functions */
static void nf_print_prompt(struct nf_machine *m);
static void nf_interpret_init(struct nf_machine *);

/* print fancy prompt */
static void
nf_print_prompt(struct nf_machine *m)
{
    size_t n;
    nf_cell_t *c;

    if (nf_getx() != 0) {
        nf_printf("\n");
    }

    if (m->state != NF_STATE_INTERPRET) {
        nf_printf(" ... ");
        return;
    }

    nf_printf("\xda\xc4\xc4[");

    n = m->data_sp - m->data_stack;

    if (n > 5) {
        nf_printf("... ");
    }

    c = (n > 5) ? m->data_sp - 5 : m->data_stack;

    while (c < m->data_sp) {
        nf_printf("%ld", *c);

        if (c < m->data_sp - 1) {
            nf_printf(" ");
        }

        ++c;
    }

    nf_printf("]");

    while (nf_getx() < 79) {
        nf_printf("\xc4");
    }

    nf_printf("\n\xc0>>> ");
}

/* interpret embedded init code line by line */
static void
nf_interpret_init(struct nf_machine *m)
{
    const char *p = (const char *)&nf_init_code;
    const char *end = (const char *)&nf_init_code_end;
    char buf[NF_LINE_BUF_SIZE];
    int i;

    while (p < end) {
        /* copy one line into buf */
        i = 0;
        while (p < end && *p != '\n' && *p != '\r' && i < NF_LINE_BUF_SIZE - 1) {
            buf[i++] = *p++;
        }
        buf[i] = '\0';

        /* skip line endings */
        while (p < end && (*p == '\r' || *p == '\n')) {
            ++p;
        }

        /* interpret the line */
        if (i > 0) {
            nf_intp_line(m, buf);
        }
    }
}

/* main entry point */
void
main(void)
{
    static char *argv[1] = { 0 };
    struct nf_machine *m;
    char *line;

    nf_printf("\n");

    m = nf_init_machine(0, argv);

    if (!m) {
        nf_error(("error: out of memory\n"));
        nf_exit(1);
    }

    /* setup x86-specific words */
    nf_define_x86_words(m);

    /* interpret embedded init code */
    nf_interpret_init(m);

    /* interactive interpreter loop */
    for (;;) {
        nf_print_prompt(m);
        line = nf_readline();
        (void)nf_intp_line(m, line);
    }

    /* NOTREACHED */
}
