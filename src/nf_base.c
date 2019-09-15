/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_base.c - base words
 */

#include "nf_cmmn.h"

#define NF_OPERATOR_BINARY(name, expr)      \
    static int                              \
    nf_base_ ## name(struct nf_machine *m)  \
    {                                       \
        nf_cell_t n1, n2;                   \
                                            \
        if (nf_data_check(m, 2, 1))         \
            return -1;                      \
                                            \
        n2 = nf_data_pop(m);                \
        n1 = nf_data_pop(m);                \
        nf_data_push(m, expr);              \
                                            \
        return 0;                           \
    }

#define NF_OPERATOR_UNARY(name, expr)       \
    static int                              \
    nf_base_ ## name(struct nf_machine *m)  \
    {                                       \
        nf_cell_t n;                        \
                                            \
        if (nf_data_check(m, 1, 1))         \
            return -1;                      \
                                            \
        n = nf_data_pop(m);                 \
        nf_data_push(m, expr);              \
                                            \
        return 0;                           \
    }

NF_OPERATOR_BINARY(add, n1 + n2)
NF_OPERATOR_BINARY(sub, n1 - n2)
NF_OPERATOR_BINARY(mul, n1 * n2)
NF_OPERATOR_BINARY(div, n1 / n2)
NF_OPERATOR_BINARY(mod, n1 % n2)

NF_OPERATOR_BINARY(bool_and, n1 && n2)
NF_OPERATOR_BINARY(bool_or,  n1 || n2)
NF_OPERATOR_UNARY (bool_not,  !n)

NF_OPERATOR_BINARY(bit_and, n1 & n2)
NF_OPERATOR_BINARY(bit_or,  n1 | n2)
NF_OPERATOR_BINARY(bit_xor, n1 ^ n2)
NF_OPERATOR_UNARY (bit_not, ~n)

NF_OPERATOR_BINARY(eq, n1 == n2)
NF_OPERATOR_BINARY(ne, n1 != n2)
NF_OPERATOR_BINARY(lt, n1 <  n2)
NF_OPERATOR_BINARY(le, n1 <= n2)
NF_OPERATOR_BINARY(gt, n1 >  n2)
NF_OPERATOR_BINARY(ge, n1 >= n2) 


/* 'dup' ( x -- x x ) */
static int
nf_base_dup(struct nf_machine *m)
{
    nf_cell_t x;

    if (nf_data_check(m, 1, 2))
        return -1;

    x = nf_data_pop(m);
    nf_data_push(m, x);
    nf_data_push(m, x);

    return 0;
}

/* 'drop' ( x -- ) */
static int
nf_base_drop(struct nf_machine *m)
{
    if (nf_data_check(m, 1, 0))
        return -1;

    (void)nf_data_pop(m);

    return 0;
}

/* 'swap' ( x1 x2 - x2 x1 ) */
static int
nf_base_swap(struct nf_machine *m)
{
    nf_cell_t x1, x2;

    if (nf_data_check(m, 2, 2))
        return -1;

    x2 = nf_data_pop(m);
    x1 = nf_data_pop(m);

    nf_data_push(m, x2);
    nf_data_push(m, x1);

    return 0;
}

/* 'over' ( x1 x2 -- x1 x2 x1 ) */
static int
nf_base_over(struct nf_machine *m)
{
    nf_cell_t x1, x2;

    if (nf_data_check(m, 2, 3))
        return -1;

    x2 = nf_data_pop(m);
    x1 = nf_data_pop(m);
    nf_data_push(m, x1);
    nf_data_push(m, x2);
    nf_data_push(m, x1);

    return 0;
}

/* 'rot' ( x1 x2 x3 -- x2 x3 x1 ) */
static int
nf_base_rot(struct nf_machine *m)
{
    nf_cell_t x1, x2, x3;

    if (nf_data_check(m, 3, 3))
        return -1;

    x3 = nf_data_pop(m);
    x2 = nf_data_pop(m);
    x1 = nf_data_pop(m);

    nf_data_push(m, x2);
    nf_data_push(m, x3);
    nf_data_push(m, x1);

    return 0;
}

/* 'exec' ( -- ) */
static int
nf_base_exec(struct nf_machine *m)
{
    struct nf_instr *i = m->comp_buf;
    return nf_exec(m, i);
}

/* 'def' ( s -- ) */
static int
nf_base_def(struct nf_machine *m)
{
    char *name;
    size_t i_count;
    struct nf_word *w;
    struct nf_instr *i;

    if (nf_data_check(m, 1, 0))
        return -1;

    name = (char *)nf_data_pop(m);

    /* copy instructions to a new buffer */
    i_count = m->comp_ip - m->comp_buf;
    i = nf_malloc(i_count * sizeof(struct nf_instr));
    if (!i) {
        nf_error("out of memory");
        return -1;
    }
    nf_memcpy(i, m->comp_buf, i_count * sizeof(struct nf_instr));

    /* initialize a word and add to the dictionary */
    w = nf_init_word(m, name, NF_WORD_COMP, i);
    if (!w) {
        nf_error("out of memory");
        return -1;
    }
    nf_define_word(m, w);

    return 0;
}

/* 'var' ( n s -- ) */
static int
nf_base_var(struct nf_machine *m)
{
    char *name;
    void *val;
    struct nf_word *w;

    if (nf_data_check(m, 2, 0))
        return -1;
    
    name = (char *)nf_data_pop(m);
    val = (void *)nf_data_pop(m);

    w = nf_init_word(m, name, NF_WORD_VAR, val);
    if (!w) {
        nf_error("out of memory");
        return -1;
    }
    nf_define_word(m, w);

    return 0;
}

/* ':=' ( n s -- ) */
static int
nf_base_assign(struct nf_machine *m)
{
    char *name;
    void *val;
    struct nf_word *w;

    if (nf_data_check(m, 2, 0))
        return -1;
    
    name = (char *)nf_data_pop(m);
    val = (void *)nf_data_pop(m);

    w = nf_lookup_word(m, name);
    if (!w || w->type != NF_WORD_VAR) {
        nf_error("unknown variable");
        return -1;
    }
    w->data = val;

    return 0;
}

/* 'argc' ( -- n ) */
static int
nf_base_argc(struct nf_machine *m)
{
    if (nf_data_check(m, 0, 1))
        return -1;

    nf_data_push(m, m->argc);

    return 0;
}

/* 'argv' ( n -- s ) */
static int
nf_base_argv(struct nf_machine *m)
{
    int n;

    if (nf_data_check(m, 1, 1))
        return -1;

    n = (int)nf_data_pop(m);
    nf_data_push(m, (nf_cell_t)m->argv[n]);

    return 0;
}

/* '.s' ( -- ) */
static int
nf_base_dot_s(struct nf_machine *m)
{
    nf_cell_t *c;

    for (c = m->data_stack; c < m->data_sp; ++c) {
        nf_printf("%ld", *c);
        if (c + 1 < m->data_sp)
            nf_printf(" ");
    }

    nf_printf("\n");

    return 0;
}

/* '.' ( x -- ) */
static int
nf_base_dot(struct nf_machine *m)
{
    nf_cell_t x;

    if (nf_data_check(m, 1, 0))
        return -1;

    x = nf_data_pop(m);
    nf_printf("%ld ", x);

    return 0;
}

/* 'cr' ( -- ) */
static int
nf_base_cr(struct nf_machine *m)
{
    nf_printf("\n");
    return 0;
}

/* arg-provider for asnprintf */
static unsigned long long
nf_printf_arg_fn(void *payload)
{
    struct nf_machine *m = (struct nf_machine *)payload;

    if (nf_data_check(m, 1, 0))
        return 0;

    return nf_data_pop(m);
}

/* 'printf' ( ... s -- n ) */
static int
nf_base_printf(struct nf_machine *m)
{
    char buf[1024];
    char *fmt;
    int ret;

    if (nf_data_check(m, 1, 1))
        return -1;

    fmt = (char *)nf_data_pop(m);

    ret = nf_asnprintf(buf, sizeof(buf), fmt, nf_printf_arg_fn, m);
    nf_printf("%s", buf);

    nf_data_push(m, ret);

    return 0;
}

#define NF_DECL_PRIM(name, data) { name, NF_WORD_PRIM, data, 0 }

/* append base words to the dictionary */
void
nf_define_base_words(struct nf_machine *m)
{
    static struct nf_word words[] = {
        NF_DECL_PRIM("dup",    (void*)nf_base_dup),
        NF_DECL_PRIM("drop",   (void*)nf_base_drop),
        NF_DECL_PRIM("swap",   (void*)nf_base_swap),
        NF_DECL_PRIM("over",   (void*)nf_base_over),
        NF_DECL_PRIM("rot",    (void*)nf_base_rot),

        NF_DECL_PRIM("+",      (void*)nf_base_add),
        NF_DECL_PRIM("-",      (void*)nf_base_sub),
        NF_DECL_PRIM("*",      (void*)nf_base_mul),
        NF_DECL_PRIM("/",      (void*)nf_base_div),
        NF_DECL_PRIM("%",      (void*)nf_base_mod),
        NF_DECL_PRIM("&&",     (void*)nf_base_bool_and),
        NF_DECL_PRIM("||",     (void*)nf_base_bool_or),
        NF_DECL_PRIM("!",      (void*)nf_base_bool_not),
        NF_DECL_PRIM("&",      (void*)nf_base_bit_and),
        NF_DECL_PRIM("|",      (void*)nf_base_bit_or),
        NF_DECL_PRIM("^",      (void*)nf_base_bit_xor),
        NF_DECL_PRIM("~",      (void*)nf_base_bit_not),
        NF_DECL_PRIM("==",     (void*)nf_base_eq),
        NF_DECL_PRIM("!=",     (void*)nf_base_ne),
        NF_DECL_PRIM("<",      (void*)nf_base_lt),
        NF_DECL_PRIM(">",      (void*)nf_base_gt),
        NF_DECL_PRIM("<=",     (void*)nf_base_le),
        NF_DECL_PRIM(">=",     (void*)nf_base_ge),

        NF_DECL_PRIM("exec",   (void*)nf_base_exec),
        NF_DECL_PRIM("def",    (void*)nf_base_def),
        NF_DECL_PRIM("var",    (void*)nf_base_var),
        NF_DECL_PRIM(":=",     (void*)nf_base_assign),

        NF_DECL_PRIM("argc",   (void*)nf_base_argc),
        NF_DECL_PRIM("argv",   (void*)nf_base_argv),

        NF_DECL_PRIM("printf", (void*)nf_base_printf),
        NF_DECL_PRIM(".s",     (void*)nf_base_dot_s),
        NF_DECL_PRIM(".",      (void*)nf_base_dot),
        NF_DECL_PRIM("cr",     (void*)nf_base_cr),
    };

    int i, count;

    count = sizeof(words) / sizeof(words[0]);

    for (i = 0; i < count; ++i) {
        nf_define_word(m, &words[i]);
    }
}
