/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_intp.c - interpreter
 */

#include "nf_cmmn.h"

/* local functions */
static int nf_intp_string(struct nf_machine *m, struct nf_token *t);
static int nf_intp_number(struct nf_machine *m, struct nf_token *t);
static int nf_intp_word(struct nf_machine *m, struct nf_token *t);
static int nf_intp_token(struct nf_machine *m, struct nf_token *t);

/* interpret string token */
static int
nf_intp_string(struct nf_machine *m, struct nf_token *t)
{
    size_t len;
    char *p;

    // duplicate string on the heap
    len = nf_strlen(t->str) + 1;
    p = nf_malloc(len);
    if (!p) {
        nf_error("out of memory");
        return -1;
    }
    nf_memcpy(p, t->str, len);

    // in interpret mode, push address to the stack
    if (m->state == NF_STATE_INTERPRET) {

        if (nf_data_check(m, 0, 1))
            return -1;
        nf_data_push(m, (nf_cell_t)p);

    // in compilation mode, compile heap address as a literal
    } else {

        if (!nf_comp_instr(m, NF_OPCODE_LITERAL, (nf_cell_t)p)) {
            nf_error("compilation buffer overflow");
            return -1;
        }

    }

    return 0;
}

/* interpret number token */
static int
nf_intp_number(struct nf_machine *m, struct nf_token *t)
{
    // in interpretation mode, push to the stack
    if (m->state == NF_STATE_INTERPRET) {

        if (nf_data_check(m, 0, 1))
            return -1;
        nf_data_push(m, t->num);

    // in compilation mode mode, compile as a literal
    } else {

        if (!nf_comp_instr(m, NF_OPCODE_LITERAL, t->num)) {
            nf_error("compilation buffer overflow");
            return -1;
        }

    }
    return 0;
}

/* interpret word token */
static int
nf_intp_word(struct nf_machine *m, struct nf_token *t)
{
    struct nf_word *p = nf_lookup_word(m, t->str);

    if (!p) {
        nf_error("unknown word");
        return -1;
    }

    // in interpretation mode or if word is a statement, execute it
    if (m->state == NF_STATE_INTERPRET || p->type == NF_WORD_STMT) {

        if (nf_call_word(m, p)) {
            return -1;
        }

    // in compilation mode, compile CALL instruction
    } else {

        if (!nf_comp_instr(m, NF_OPCODE_CALL, (nf_cell_t)p)) {
            nf_error("compilation buffer overflow");
            return -1;
        }

    }

    return 0;
}

/* interpret a single token. return 0 on success. */
static int
nf_intp_token(struct nf_machine *m, struct nf_token *t)
{
    switch(t->type) {

    case NF_TOKEN_EMPTY:
        return 0;

    case NF_TOKEN_INVALID:
        nf_error("invalid token\n");
        return -1;

    case NF_TOKEN_STRING:
        return nf_intp_string(m, t);

    case NF_TOKEN_NUMBER:
        return nf_intp_number(m, t);

    case NF_TOKEN_WORD: 
        return nf_intp_word(m, t);

    default:
        return -1;

    }
}

/* interpret a single line of code. return 0 on success. */
int
nf_intp_line(struct nf_machine *m, char *line)
{
    struct nf_token token, *t;

    t = &token;

    while (line) {
        line = nf_parse_token(line, t);
        if (nf_intp_token(m, t))
            return -1;
    }

    return 0;
}

