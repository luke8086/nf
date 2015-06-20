/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * posix/nf_main.c - posix main routine
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/readline.h>

#include "../nf_common.h"

/* local functions */
static int nf_interpret_int(struct nf_machine *m);
static int nf_interpret_file(struct nf_machine *m);

/* interactive interpreter loop */
static int
nf_interpret_int(struct nf_machine *m)
{
    char *line;
    char *prompt;

    while (1) {
        prompt = m->state == NF_STATE_INTERPRET ? ">>> " : "... ";
        line = readline(prompt);

        if (!line) {
            printf("\n");
            return 0;
        }

        (void)nf_intp_line(m, line);
    }

    // NOTREACHED
    return -1;
}

/* file interpreter loop */
static int
nf_interpret_file(struct nf_machine *m)
{
    size_t n;
    char *line = 0;

    while (1) {
        if (getline(&line, &n, stdin) <= 0) {
            if (line)
                nf_free(line);
            return 0;
        }

        if (nf_intp_line(m, line))
            return -1;
    }

    // NOTREACHED
    return -1;
}

/* main entry point */
int
main(int argc, char **argv)
{
    struct nf_machine *m;
    int ret;

    m = nf_init_machine(argc, argv);
    if (!m) {
        nf_error("error: out of memory\n");
        nf_exit(1);
    }

    ret = isatty(fileno(stdin))
        ? nf_interpret_int(m)
        : nf_interpret_file(m);

    return ret;
}
