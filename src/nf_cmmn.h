/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_cmmn.h - type definitions and declarations of global functions
 */

#ifndef _NF_CMMN_H_
#define _NF_CMMN_H_

#include <stdarg.h>

enum {
    NF_TOKEN_MAX_WIDTH = 4095,
    NF_WORD_MAX_WIDTH  = 31,
    NF_DATA_STACK_SIZE = 4096,
    NF_STMT_STACK_SIZE = 16,
    NF_COMP_BUF_SIZE   = 2048,
    NF_LINE_BUF_SIZE   = 1024
};

#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#elif defined(NF_SUPPORTS_LONG_LONG)
typedef unsigned long long size_t;
#elif defined(NF_SUPPORTS_LONG)
typedef unsigned long size_t;
#else
typedef unsigned size_t;
#endif

#if defined(__UINTMAX_TYPE__)
typedef __UINTMAX_TYPE__ uintmax_t;
#elif defined(NF_SUPPORTS_LONG_LONG)
typedef unsigned long long uintmax_t;
#elif defined(NF_SUPPORTS_LONG)
typedef unsigned long uintmax_t;
#else
typedef unsigned uintmax_t;
#endif


#if defined(__INTMAX_TYPE__)
typedef __INTMAX_TYPE__ intmax_t;
#elif defined(NF_SUPPORTS_LONG_LONG)
typedef long long intmax_t;
#elif defined(NF_SUPPORTS_LONG)
typedef long intmax_t;
#else
typedef int intmax_t;
#endif

typedef intmax_t nf_cell_t;

/* interpreter tokens */

enum nf_token_type {
    NF_TOKEN_EMPTY = 0,
    NF_TOKEN_WORD = 1,
    NF_TOKEN_NUMBER = 2,
    NF_TOKEN_STRING = 3,
    NF_TOKEN_INVALID = 4
};

struct nf_token {
    enum nf_token_type type;
    char str[NF_TOKEN_MAX_WIDTH + 1];
    nf_cell_t num;
};

/* bytecode instructions */

enum nf_opcode {
    NF_OPCODE_RETURN,
    NF_OPCODE_CALL,
    NF_OPCODE_LITERAL,
    NF_OPCODE_BRANCH,
    NF_OPCODE_BRANCH_IF,
    NF_OPCODE_BRANCH_UNLESS
};

struct nf_instr {
    enum nf_opcode opcode;
    nf_cell_t value;
};

/* statement */

enum nf_stmt_type {
    NF_STMT_COLON,
    NF_STMT_SEMICOLON,
    NF_STMT_IF,
    NF_STMT_ELSE,
    NF_STMT_THEN,
    NF_STMT_DO,
    NF_STMT_WHILE,
    NF_STMT_REPEAT,
    NF_STMT_UNTIL
};

struct nf_stmt {
    enum nf_stmt_type type;
    struct nf_instr *ip;
};

/* word */

struct nf_machine;
typedef int (*nf_word_handler_t)(struct nf_machine *m);

enum nf_word_type {
    NF_WORD_PRIM,
    NF_WORD_COMP,
    NF_WORD_STMT,
    NF_WORD_VAR
};

struct nf_word {
    char name[NF_WORD_MAX_WIDTH + 1];
    enum nf_word_type type;
    void *data;
    struct nf_word *next;
};

/* virtual machine */

enum nf_machine_state {
    NF_STATE_INTERPRET,
    NF_STATE_COMPILE,
    NF_STATE_EXECUTE
};

struct nf_machine {
    int state;
    struct nf_word *words;

    char line_buf[NF_LINE_BUF_SIZE];
    char *line_p;

    int argc;
    char **argv;

    nf_cell_t data_stack[NF_DATA_STACK_SIZE];
    nf_cell_t *data_sp;

    struct nf_instr comp_buf[NF_COMP_BUF_SIZE];
    struct nf_instr *comp_ip;

    struct nf_stmt stmt_stack[NF_STMT_STACK_SIZE];
    struct nf_stmt *stmt_sp;
};

/* macros */

#define nf_error(args) {    \
    nf_printf("error: ");   \
    nf_printf args;         \
    nf_printf("\n");        \
}

/* global functions */

/* nf_base.c */
void nf_define_base_words(struct nf_machine *m);

/* nf_intp.c */
int nf_intp_line(struct nf_machine *m, char *line);

/* nf_lex.c */
char *nf_parse_token(char *src, struct nf_token *tok);

/* nf_libc.c */
void *nf_malloc(size_t size);
void nf_free(void *ptr);
void nf_exit(int code);
int nf_printf(const char *format, ...);

/* nf_stmt.c */
void nf_define_stmt_words(struct nf_machine *m);

/* nf_mach.c */
int nf_data_check(struct nf_machine *m, size_t count_in, size_t count_out);
nf_cell_t nf_data_pop(struct nf_machine *m);
void nf_data_push(struct nf_machine *m, nf_cell_t v);
int nf_exec(struct nf_machine *m, struct nf_instr *i);

size_t nf_stmt_count(struct nf_machine *m);
int nf_stmt_push(struct nf_machine *m, enum nf_stmt_type type,
                 struct nf_instr *ip);
struct nf_stmt *nf_stmt_pop(struct nf_machine *m);
struct nf_stmt *nf_stmt_get(struct nf_machine *m, size_t n);

void nf_comp_start(struct nf_machine *m);
void nf_comp_finish(struct nf_machine *m);
struct nf_instr *nf_comp_instr(struct nf_machine *m, nf_cell_t opcode,
                               nf_cell_t value);

struct nf_machine *nf_init_machine(int argc, char **argv);

/* nf_prtf.c */
int nf_asnprintf(char *buf, size_t nbyte, const char *fmt,
                 uintmax_t (arg_fn)(void *), void *payload);
int nf_vsnprintf(char *buf, size_t nbyte, const char *fmt,
                 va_list va);

/* nf_str.c */
void *nf_memcpy(void *dest, const void *src, size_t n);
size_t nf_strlen(const char *s1);
int nf_strcmp(const char *s1, const char *s2);
char *nf_strncpy(char *dest, const char *src, size_t n);

/* nf_word.c */
struct nf_word *nf_init_word(struct nf_machine *m, char *name,
                             enum nf_word_type type, void *data);
void nf_define_word(struct nf_machine *m, struct nf_word *w);
struct nf_word *nf_lookup_word(struct nf_machine *m, char *name);
int nf_call_word(struct nf_machine *m, struct nf_word *w);

#endif /* _NF_CMMN_H_ */

