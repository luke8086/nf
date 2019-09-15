/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_word.c - word initialization, lookup and execution
 */

#include "nf_cmmn.h"

/* initialize a new word on the heap */
struct nf_word *
nf_init_word(struct nf_machine *m, char *name, enum nf_word_type type, void *data)
{
    size_t namelen = nf_strlen(name);
    struct nf_word *w;

    if (namelen > NF_WORD_MAX_WIDTH) {
        nf_printf("name too long\n");
        return 0;
    }

    w = nf_malloc(sizeof(struct nf_word));

    if (!w) {
        return 0;
    }

    w->type = type;
    w->data = data;

    nf_strncpy(w->name, name, namelen + 1);

    return w;
}

/* add a single word to the word dictionary */
void
nf_define_word(struct nf_machine *m, struct nf_word *w)
{
    w->next = m->words;
    m->words = w;
}

/* find the latest word with a given name */
struct nf_word *
nf_lookup_word(struct nf_machine *m, char *name)
{
    struct nf_word *w;

    for (w = m->words; w; w = w->next) {
        if (!nf_strcmp(w->name, name)) {
            return w;
        }
    }

    return 0;
}

/* execute a given word */
int
nf_call_word(struct nf_machine *m, struct nf_word *w)
{
    nf_word_handler_t handler;

    switch (w->type) {
    case NF_WORD_PRIM:
    case NF_WORD_STMT:
        handler = (nf_word_handler_t)w->data;
        return handler(m);
    case NF_WORD_COMP:
        return nf_exec(m, w->data);
    case NF_WORD_VAR:
        if (nf_data_check(m, 0, 1))
            return -1;
        nf_data_push(m, (nf_cell_t)w->data);
        return 0;
    default:
        return -1;
    }
}
