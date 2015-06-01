/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_stmt.c - statement handling words
 */

#include "nf_common.h"

/* ':' ( -- ':' ) */
static int
nf_stmt_colon(struct nf_machine *m)
{
    if (m->state != NF_STATE_INTERPRET) {
        nf_error("syntax error");
        return -1;
    }

    nf_comp_start(m);

    if (nf_stmt_push(m, NF_STMT_COLON, m->comp_ip)) {
        nf_error("statement stack overflow");
        return -1;
    };

    return 0;
}

/* ';' ( ':' -- ) */
static int
nf_stmt_semicolon(struct nf_machine *m)
{
    struct nf_stmt *s;

    if (m->state != NF_STATE_COMPILE) {
        nf_error("syntax error");
        return -1;
    }

    s = nf_stmt_pop(m);

    if (!s || s->type != NF_STMT_COLON) {
        nf_error("syntax error");
        return -1;
    }

    nf_comp_finish(m);

    return 0;
}

/* 'if' ( -- 'if' ) */
static int
nf_stmt_if(struct nf_machine *m)
{
    struct nf_instr *i;

    // if interpreting, compile until 'then' and automatically exec
    if (m->state != NF_STATE_COMPILE) {
        nf_comp_start(m);
    }

    // insert a blank branch-unless
    i = nf_comp_instr(m, NF_OPCODE_BRANCH_UNLESS, 0);
    if (!i) {
        nf_error("compilation buffer overflow");
        return -1;
    }

    // and push it to be updated by 'else' or 'then'
    if (nf_stmt_push(m, NF_STMT_IF, i)) {
        nf_error("statement stack overflow");
        return -1;
    };

    return 0;
}

/* 'else' ( 'if' -- 'else' ) */
static int
nf_stmt_else(struct nf_machine *m)
{
    struct nf_instr *i_else;
    struct nf_stmt *s_if;

    if (m->state != NF_STATE_COMPILE) {
        nf_error("syntax error");
        return -1;
    }

    // update offset in the branch-if of 'if'. add 1 to skip 'else'
    s_if = nf_stmt_pop(m);
    if (!s_if || s_if->type != NF_STMT_IF) {
        nf_error("syntax error");
        return -1;
    }
    s_if->ip->value = (m->comp_ip - s_if->ip + 1);

    // insert a blank branch instruction
    i_else = nf_comp_instr(m, NF_OPCODE_BRANCH, 0);
    if (!i_else) {
        nf_error("compilation buffer overflow");
        return -1;
    }

    // and push to cs to be updated by 'then'
    if (nf_stmt_push(m, NF_STMT_ELSE, i_else)) {
        nf_error("statement stack overflow");
        return -1;
    };

    return 0;
}

/* 'then' ( 'if'|'else' -- ) */
static int
nf_stmt_then(struct nf_machine *m)
{
    struct nf_stmt *s;

    if (m->state != NF_STATE_COMPILE) {
        nf_error("syntax error");
        return -1;
    }

    // update offset of the last 'if' or 'else'
    s = nf_stmt_pop(m);
    if (!s || (s->type != NF_STMT_IF && s->type != NF_STMT_ELSE)) {
        nf_error("syntax error");
        return -1;
    }
    s->ip->value = m->comp_ip - s->ip;

    // if the statement stack is not empty, continue compilation
    if (nf_stmt_count(m)) {
        return 0;
    }

    // finish compilation and exec the bytecode
    nf_comp_finish(m);
    if (nf_exec(m, m->comp_buf)) {
        return -1;
    }

    return 0;
}

/* 'do' ( -- 'do' ) */
static int
nf_stmt_do(struct nf_machine *m)
{
    // if interpreting, compile up to 'until' or 'repeat' and exec
    if (m->state != NF_STATE_COMPILE) {
        nf_comp_start(m);
    }

    // push a 'do' statement pointing to the next instruction
    if (nf_stmt_push(m, NF_STMT_DO, m->comp_ip)) {
        nf_error("statement stack overflow");
        return -1;
    };

    return 0;
}

/* 'while' ( 'do' -- 'do' ) */
static int
nf_stmt_while(struct nf_machine *m)
{
    struct nf_instr *i;
    struct nf_stmt *s;

    if (m->state != NF_STATE_COMPILE) {
        nf_error("syntax error");
        return -1;
    }

    // make sure the last statement was 'do'
    s = nf_stmt_get(m, 0);
    if (!s || s->type != NF_STMT_DO) {
        nf_error("syntax error");
        return -1;
    }

    // insert a blank branch-unless instruction
    i = nf_comp_instr(m, NF_OPCODE_BRANCH_UNLESS, 0);
    if (!i) {
        nf_error("compilation buffer overflow");
        return -1;
    }

    // push a 'while' statement to be updated by 'repeat'
    if (nf_stmt_push(m, NF_STMT_WHILE, i)) {
        nf_error("statement stack overflow");
    };

    return 0;
}

/* 'repeat' word */
static int
nf_stmt_repeat(struct nf_machine *m)
{
    struct nf_stmt *s;

    if (m->state != NF_STATE_COMPILE) {
        nf_error("syntax error");
        return -1;
    }

    // update offset in 'while', add 1 to skip the next instruction
    s = nf_stmt_pop(m);
    if (!s || s->type != NF_STMT_WHILE) {
        nf_error("syntax error");
        return -1;
    }
    s->ip->value =  m->comp_ip - s->ip + 1;

    // fetch the last 'do' statement
    s = nf_stmt_pop(m);
    if (!s || s->type != NF_STMT_DO) {
        nf_error("syntax error");
        return -1;
    }

    // insert unconditional branch instruction
    // pointing to the address stored in 'do'
    if (!nf_comp_instr(m, NF_OPCODE_BRANCH, s->ip - m->comp_ip)) {
        nf_error("compilation buffer overflow");
        return -1;
    };

    // if the statement stack is not empty, continue compilation
    if (nf_stmt_count(m)) {
        return 0;
    }

    // finish compilation and exec the bytecode
    nf_comp_finish(m);
    if (nf_exec(m, m->comp_buf)) {
        return -1;
    }

    return 0;
}

/* 'until' word */
static int
nf_stmt_until(struct nf_machine *m)
{
    struct nf_stmt *s;

    if (m->state != NF_STATE_COMPILE) {
        nf_error("syntax error");
        return -1;
    }

    // fetch the last 'do' statement
    s = nf_stmt_pop(m);
    if (!s || s->type != NF_STMT_DO) {
        nf_error("syntax error");
        return -1;
    }

    // insert unconditional branch instruction
    // pointing to the address stored by 'do'
    if (!nf_comp_instr(m, NF_OPCODE_BRANCH_UNLESS, s->ip - m->comp_ip)) {
        nf_error("compilation buffer overflow");
        return -1;
    };

    // if the statement stack is not empty, continue compilation
    if (nf_stmt_count(m)) {
        return 0;
    }

    // finish compilation and exec the bytecode
    nf_comp_finish(m);
    if (nf_exec(m, m->comp_buf)) {
        return -1;
    }

    return 0;
}

/* append statement words to the dictionary */
void
nf_define_stmt_words(struct nf_machine *m)
{
    static struct nf_word words[] = {
        { .name = ":", .data = (void*)nf_stmt_colon },
        { .name = ";", .data = (void*)nf_stmt_semicolon },
        { .name = "if", .data = (void*)nf_stmt_if },
        { .name = "else", .data = (void*)nf_stmt_else },
        { .name = "then", .data = (void*)nf_stmt_then },
        { .name = "do", .data = (void*)nf_stmt_do },
        { .name = "repeat", .data = (void*)nf_stmt_repeat },
        { .name = "while", .data = (void*)nf_stmt_while },
        { .name = "until", .data = (void*)nf_stmt_until },
    };

    int i, count;

    count = sizeof(words) / sizeof(words[0]);

    for (i = 0; i < count; ++i) {
        words[i].type = NF_WORD_STMT;
        nf_define_word(m, &words[i]);
    }
}
