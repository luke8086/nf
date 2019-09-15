/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_mach.c - nf virtual machine functions
 */

#include "nf_cmmn.h"

/*
 * ensure the stack contains enough elements and enough space to support
 * given instruction requirements.  on success, return 0. on failure,
 * display an error and return -1;
 */
int
nf_data_check(struct nf_machine *m, size_t count_in, size_t count_out)
{
    size_t used = m->data_sp - m->data_stack;
    size_t free = NF_DATA_STACK_SIZE - used;

    if (used < count_in) {
        nf_error(("data stack underflow"));
        return -1;
    }

    if (free + count_in < count_out) {
        nf_error(("data stack overflow"));
        return -1;
    }

    return 0;
}

/* pop value from the data stack. don't check for errors. */
nf_cell_t
nf_data_pop(struct nf_machine *m)
{
    return *(--m->data_sp);
}

/* push value to the data stack. don't check for errors. */
void
nf_data_push(struct nf_machine *m, nf_cell_t v)
{
    *(m->data_sp++) = v;
}

/* return amount of elements on the statement stack */
size_t
nf_stmt_count(struct nf_machine *m)
{
    return m->stmt_sp - m->stmt_stack;
}

/* push to the statement stack. return 0 on success, -1 on overflow */
int
nf_stmt_push(struct nf_machine *m, enum nf_stmt_type type, struct nf_instr *ip)
{
    if (nf_stmt_count(m) >= NF_STMT_STACK_SIZE) {
        return -1;
    }

    m->stmt_sp->type = type;
    m->stmt_sp->ip = ip;

    ++(m->stmt_sp);

    return 0;
}

/* pop from the statement stack. return pointer or 0 on underflow */
struct nf_stmt *
nf_stmt_pop(struct nf_machine *m)
{
    if (!nf_stmt_count(m)) {
        return 0;
    }

    return --m->stmt_sp;
}

/* return pointer to the nth element from the statement stack, or 0 on underflow */
struct nf_stmt *
nf_stmt_get(struct nf_machine *m, size_t n)
{
    n++;

    if (nf_stmt_count(m) < n) {
        return 0;
    }

    return m->stmt_sp - n;
}

/* begin compilation */
void
nf_comp_start(struct nf_machine *m)
{
    m->comp_ip = m->comp_buf;
    m->stmt_sp = m->stmt_stack;
    m->state = NF_STATE_COMPILE;
}

/* finish compilation */
void
nf_comp_finish(struct nf_machine *m)
{
    nf_comp_instr(m, NF_OPCODE_RETURN, 0);
    m->state = NF_STATE_INTERPRET;
}

/*
 * append instruction to the compilation buffer
 * return pointer to the instruction or 0 on overflow
 */
struct nf_instr *
nf_comp_instr(struct nf_machine *m, nf_cell_t opcode, nf_cell_t value)
{
    if (m->comp_ip - m->comp_buf >= NF_COMP_BUF_SIZE) {
        return 0;
    }

    m->comp_ip->opcode = opcode;
    m->comp_ip->value = value;

    return (m->comp_ip)++;
}

/* execute nf bytecode */
int
nf_exec(struct nf_machine *m, struct nf_instr *i)
{
    enum nf_machine_state state = m->state;
    int ret = 0;

    m->state = NF_STATE_EXECUTE;

    while (1) {
        if (i->opcode == NF_OPCODE_CALL) {
            struct nf_word *w = (struct nf_word *)i->value;
            if (nf_call_word(m, w)) {
                ret = -1;
                break;
            }
            i++;
        }

        else if (i->opcode == NF_OPCODE_LITERAL) {
            if (nf_data_check(m, 0, 1)) {
                ret = -1;
                break;
            }
            nf_data_push(m, i->value);
            i++;
        }

        else if (i->opcode == NF_OPCODE_BRANCH) {
            i += i->value;
        }

        else if (i->opcode == NF_OPCODE_BRANCH_IF) {
            if (nf_data_check(m, 1, 0)) {
                ret = -1;
                break;
            }
            nf_cell_t v = nf_data_pop(m);
            i += (v ? i->value : 1);
        }

        else if (i->opcode == NF_OPCODE_BRANCH_UNLESS) {
            if (nf_data_check(m, 1, 0)) {
                ret = -1;
                break;
            }
            nf_cell_t v = nf_data_pop(m);
            i += (!v ? i->value : 1);
        }

        else if (i->opcode == NF_OPCODE_RETURN) {
            ret = 0;
            break;
        }

        else {
            nf_error(("invalid opcode: %d/%ld", i->opcode, i->value));
            ret = -1;
            break;
        }
    }

    m->state = state;

    return ret;
}

/* create new nf_machine on the heap. return 0 if there's not enough memory */
struct nf_machine *
nf_init_machine(int argc, char **argv)
{
    struct nf_machine *m;

    m = nf_malloc(sizeof(struct nf_machine));
    if (!m)
        return 0;

    m->data_sp = m->data_stack;
    m->line_p = m->line_buf;

    m->state = NF_STATE_INTERPRET;
    m->words = 0;

    m->argc = argc;
    m->argv = argv;

    nf_define_base_words(m);
    nf_define_stmt_words(m);

    return m;
}
