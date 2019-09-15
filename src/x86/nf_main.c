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

/* local functions */
static void nf_interpret_int(struct nf_machine *);

/* interactive interpreter loop */
static void
nf_interpret_int(struct nf_machine *m)
{
    char *line, *prompt;
    int v;

    for (;;) {
        prompt = m->state == NF_STATE_INTERPRET ? ">>> " : "... ";
        nf_printf("%s", prompt);

        line = nf_readline();
        (void)nf_intp_line(m, line);
    }

    /* NOTREACHED */
}

/* main entry point */
void
main(void)
{
    static char *argv[1] = { 0 };
    struct nf_machine *m;

    m = nf_init_machine(0, argv);

    if (!m) {
        nf_error(("error: out of memory\n"));
        nf_exit(1);
    }

    /* setup x86-specific words */
    nf_define_x86_words(m);

    nf_interpret_int(m);

    /* NOTREACHED */
}
