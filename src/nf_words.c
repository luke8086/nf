/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * x86/nf_words.c - x86 platform specific words
 */

#include "nf_cmmn.h"

/* 'exit' ( -- ) */
static int
nf_word_exit(struct nf_machine *m)
{
    (void)m;

    nf_exit(0);

    /* NOTREACHED */
}

#define NF_DECL_PRIM(name, data) { name, NF_WORD_PRIM, data, 0 }

/* define x86 system words */
void
nf_define_x86_words(struct nf_machine *m)
{
    int i, count;

    static struct nf_word words[] = {
        NF_DECL_PRIM("exit", (void*)nf_word_exit)
    };

    count = sizeof(words) / sizeof(words[0]);

    for (i = 0; i < count; ++i) {
        words[i].type = NF_WORD_PRIM;
        nf_define_word(m, &words[i]);
    }
}
