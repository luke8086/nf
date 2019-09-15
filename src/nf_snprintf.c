/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * minimal standalone implementation of {v,a,}snprintf
 */

#include <stdarg.h>

#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long long size_t;
#endif

#ifdef NF_PRINTF
#define PF_ASNPRINTF nf_asnprintf
#define PF_VSNPRINTF nf_vsnprintf
#define PF_SNPRINTF  nf_snprintf
#else
#define PF_ASNPRINTF pf_asnprintf
#define PF_VSNPRINTF pf_vsnprintf
#define PF_SNPRINTF  pf_snprintf
#endif

/* consecutive input states */
enum {
    PF_DEFAULT,
    PF_FLAGS,
    PF_WIDTH,
    PF_LENGTH,
    PF_CONV,
};

/* length specifiers */
enum {
    PF_NONE,
    PF_h,
    PF_l,
    PF_ll,
};

/* conversion specifiers */
enum {
    PF_c,
    PF_s,
    PF_d,
    PF_u,
    PF_x,
    PF_X,
};

/* char-emitter and arg-provider function typedefs */
typedef int (pf_emit_fn)(void *payload, char c);
typedef unsigned long long (pf_arg_fn)(void *payload);

/* configuration and state of a single printf command */
struct pf_config {
    pf_emit_fn *emit_fn;
    void *emit_payload;

    pf_arg_fn *arg_fn;
    void *arg_payload;
    va_list *arg_list;

    int emitted;
    int error;
    int state;

    int zpad;
    int rpad;
    int width;
    int length;
    int conv;
};

/* vasnprintf payload struct */
struct pf_vasnprintf_payload {
    char *buf;
    size_t i;
    size_t nbyte;
};

/* local functions */
static size_t pf_strlen(const char *s);

static unsigned long long pf_get_arg_va(va_list *va, int len, int conv);
static unsigned long long pf_get_arg(struct pf_config *c);

static void pf_emit_char(struct pf_config *c, char ch);
static void pf_emit_str(struct pf_config *c, char *s);
static void pf_emit_uint(struct pf_config *c, unsigned long long n, int neg);
static void pf_emit_int(struct pf_config *c, long long n);

static void pf_cprintf(const char *fmt, struct pf_config *c);
static int pf_vasnprintf_emit(void *payload, char ch);
static int pf_vasnprintf(char *buf, size_t nbyte, const char *fmt,
           va_list *arg_list, pf_arg_fn *arg_fn, void *arg_payload);

/* calculate the length of a string */
static size_t
pf_strlen(const char *s)
{
    size_t ret = 0;

    while (*s++) {
        ++ret;
    }

    return ret;
}

/* get an argument with given len and conv from a va_list */
static unsigned long long
pf_get_arg_va(va_list *va, int len, int conv)
{
    if (conv == PF_x || conv == PF_X)
        conv = PF_u;

    if (conv == PF_c && len == PF_NONE) {
        return va_arg(*va, int);
    }

    if (conv == PF_s && len == PF_NONE)
        return (unsigned long long)va_arg(*va, char *);

    if (conv == PF_d && len == PF_h)
        return va_arg(*va, int);

    if (conv == PF_d && len == PF_NONE)
        return va_arg(*va, int);

    if (conv == PF_d && len == PF_l)
        return va_arg(*va, long);

    if (conv == PF_d && len == PF_ll)
        return va_arg(*va, long long);

    if (conv == PF_u && len == PF_h)
        return va_arg(*va, unsigned);

    if (conv == PF_u && len == PF_NONE)
        return va_arg(*va, unsigned);

    if (conv == PF_u && len == PF_l)
        return va_arg(*va, unsigned long);

    if (conv == PF_u && len == PF_ll)
        return va_arg(*va, unsigned long long);

    return 0;
}

/* get a single argument */
static unsigned long long
pf_get_arg(struct pf_config *c)
{
    if (c->arg_list)
        return pf_get_arg_va(c->arg_list, c->length, c->conv);

    if (c->arg_fn)
        return c->arg_fn(c->arg_payload);

    return 0;
}

/* emit a single unformatted character */
static void
pf_emit(struct pf_config *c, char ch)
{
    if (c->error)
        return;

    if (c->emit_fn(c->emit_payload, ch)) {
        c->error = 1;
    } else {
        c->emitted++;
    }
}

/* emit a single formatted character */
static void
pf_emit_char(struct pf_config *c, char ch)
{
    int pad;

    /* emit left padding */
    for (pad = c->width - 1; !c->rpad && pad > 0; --pad) {
        pf_emit(c, ' ');
    }

    /* emit character */
    pf_emit(c, ch);


    /* emit right padding */
    for (pad = c->width - 1; c->rpad && pad > 0; --pad) {
        pf_emit(c, ' ');
    }
}

/* emit a string */
static void
pf_emit_str(struct pf_config *c, char *s)
{
    int pad, len;

    len = pf_strlen(s);

    /* emit left padding */
    for (pad = c->width - len; !c->rpad && pad > 0; --pad) {
        pf_emit(c, ' ');
    }

    /* emit string */
    while (*s) {
        pf_emit(c, *s++);
    }

    /* emit right padding */
    for (pad = c->width - len; c->rpad && pad > 0; --pad) {
        pf_emit(c, ' ');
    }

}

/* emit an unsigned integer */
static void
pf_emit_uint(struct pf_config *c, unsigned long long n, int neg)
{
    static const char *l_hex_digits = "0123456789abcdef";
    static const char *u_hex_digits = "0123456789ABCDEF";

    long long sn = (long long)sn;
    char buf[32];
    const char *digits;
    int i, d, base;
    int pad;
    int num_width, sign_width;

    sign_width = !!neg;

    /* "crop" the value to the requested size */
    switch (c->length) {
    case PF_h:      n = (unsigned short)n;      break;
    case PF_NONE:   n = (unsigned)n;            break;
    case PF_l:      n = (unsigned long)n;       break;
    case PF_ll:     n = (unsigned long long)n;  break;
    };

    /* select uppercase or lowercase character set */
    switch (c->conv) {
    case PF_d: base = 10; digits = l_hex_digits; break;
    case PF_u: base = 10; digits = l_hex_digits; break;
    case PF_x: base = 16; digits = l_hex_digits; break;
    case PF_X: base = 16; digits = u_hex_digits; break;
    default: c->error = 1; return;
    }

    /* save digits to the temporary buffer in a reverse order */
    i = 0;
    do {
        d = n % base;
        n = n / base;
        buf[i++] = digits[d];
    } while (n != 0 && (size_t)i < sizeof(buf));

    /* save the amount of digits */
    num_width = i;

    /* in case of zero padding of a negative number, emit '-' before padding */
    if (neg && c->zpad) {
        pf_emit(c, '-');
    }

    /* emit left padding (spaces or zeros) */
    for (pad = c->width - num_width - sign_width; !c->rpad && pad > 0; --pad) {
        pf_emit(c, c->zpad ? '0' : ' ');
    }

    /* in case of space padding of a negative number, emit '-' after padding */
    if (neg && !c->zpad) {
        pf_emit(c, '-');
    }

    /* emit digits in the correct order */
    for (i = num_width - 1; i >= 0; --i) {
        pf_emit(c, buf[i]);
    }

    /* emit right padding (only spaces) */
    for (pad = c->width - num_width - sign_width; c->rpad && pad > 0; --pad) {
        pf_emit(c, ' ');
    }
}

/* emit a signed integer */
static void
pf_emit_int(struct pf_config *c, long long n)
{
    int neg = 0;

    if (n < 0) {
        neg = 1;
        n *= -1;
    }
        
    pf_emit_uint(c, (unsigned long long)n, neg);
}

/* internal printf routine, operating on a pf_config struct */
static void
pf_cprintf(const char *fmt, struct pf_config *c)
{
    unsigned long long arg;
    char ch;

    c->state = PF_DEFAULT;
    c->error = 0;
    c->emitted = 0;

    while ((ch = *fmt++)) {

        /*
         * PF_DEFAULT
         */

        /* regular character */
        if (c->state == PF_DEFAULT && ch != '%') {
            pf_emit(c, ch);
            continue;
        }

        /* beginning of format specifier */
        if (c->state == PF_DEFAULT && ch == '%') {
            c->state = PF_FLAGS;
            c->length = PF_NONE;
            c->conv = PF_NONE;
            c->width = 0;
            c->zpad = 0;
            c->rpad = 0;
            continue;
        }

        /*
         * PF_FLAGS
        `*/

        /* handle flag '0' */
        if (c->state == PF_FLAGS && ch == '0') {
            c->zpad = 1;
            continue;
        }

        /* handle flag '-' */
        if (c->state == PF_FLAGS && ch == '-') {
            c->rpad = 1;
            continue;
        }

        /* handle character other than a flag */
        if (c->state == PF_FLAGS) {
            c->state = PF_WIDTH;
            /* FALLTHROUGH */
        }

        /*
         * PF_WIDTH
         */

        /* handle a single digit of the width */
        if (c->state == PF_WIDTH && ch >= '0' && ch <= '9') {
            c->width = c->width * 10 + (ch - '0');
            continue;
        }

        /* handle character other than a digit */
        if (c->state == PF_WIDTH) {
            c->state = PF_LENGTH;
            /* FALLTHROUGH */
        }

        /*
         * PF_LENGTH
         */

        /* handle %h */
        if (c->state == PF_LENGTH && c->length == PF_NONE && ch == 'h') {
            c->length = PF_h;
            continue;
        }

        /* handle %l */
        if (c->state == PF_LENGTH && c->length == PF_NONE && ch == 'l') {
            c->length = PF_l;
            continue;
        }

        /* handle %ll */
        if (c->state == PF_LENGTH && c->length == PF_l && ch == 'l') {
            c->length = PF_ll;
            continue;
        }

        /* handle character other than a length specifier */
        if (c->state == PF_LENGTH) {
            c->state = PF_CONV;
            /* FALLTHROUGH */
        }

        /*
         * PF_CONV
         */

        /* handle %d */
        if (c->state == PF_CONV && ch == 'd') {
            c->conv = PF_d;
            arg = pf_get_arg(c);
            pf_emit_int(c, (long long)arg);
            c->state = PF_DEFAULT;
            continue;
        }

        /* handle %u */
        if (c->state == PF_CONV && ch == 'u') {
            c->conv = PF_u;
            arg = pf_get_arg(c);
            pf_emit_uint(c, (unsigned long long)arg, 0);
            c->state = PF_DEFAULT;
            continue;
        }

        /* handle %x */
        if (c->state == PF_CONV && ch == 'x') {
            c->conv = PF_x;
            arg = pf_get_arg(c);
            pf_emit_uint(c, (unsigned long long)arg, 0);
            c->state = PF_DEFAULT;
            continue;
        }

        /* handle %X */
        if (c->state == PF_CONV && ch == 'X') {
            c->conv = PF_X;
            arg = pf_get_arg(c);
            pf_emit_uint(c, (unsigned long long)arg, 0);
            c->state = PF_DEFAULT;
            continue;
        }

        /* handle %c */
        if (c->state == PF_CONV && c->length == PF_NONE && ch == 'c') {
            c->conv = PF_c;
            arg = pf_get_arg(c);
            pf_emit_char(c, (char)arg);
            c->state = PF_DEFAULT;
            continue;
        }

        /* handle %s */
        if (c->state == PF_CONV && c->length == PF_NONE && ch == 's') {
            c->conv = PF_s;
            arg = pf_get_arg(c);
            pf_emit_str(c, (char *)arg);
            c->state = PF_DEFAULT;
            continue;
        }

        /* invalid format */
        c->error = 1;
    }
    
    pf_emit(c, 0);
}

/* vasnprintf's emit function: write to the character buffer */
static int
pf_vasnprintf_emit(void *payload, char ch)
{
    struct pf_vasnprintf_payload *p = (struct pf_vasnprintf_payload *)payload;

    if (p->i < p->nbyte)
        p->buf[p->i] = ch;

    p->i++;

    return 0;
}

/* snprintf interface supporting both va_list and arg_fn/arg_payload */
static int
pf_vasnprintf(char *buf, size_t nbyte, const char *fmt,
              va_list *arg_list, pf_arg_fn *arg_fn, void *arg_payload)
{
    struct pf_vasnprintf_payload payload, *p = &payload;
    struct pf_config config, *c = &config;

    /* setup payload */
    p->buf = buf;
    p->nbyte = nbyte;
    p->i = 0;

    /* setup pf_config */
    c->emit_fn = pf_vasnprintf_emit;
    c->emit_payload = p;
    c->arg_list = arg_list;
    c->arg_fn = arg_fn;
    c->arg_payload = arg_payload;

    /* process */
    pf_cprintf(fmt, c);

    /* in case of overflow, ensure that buffer is null-terminated */
    if (p->i > p->nbyte) {
        p->buf[p->nbyte - 1] = 0;
    }

    /* the return value is the amount of characters which would */
    /* be emitted, given enough space, or -1 on error */
    return c->error ? -1 : (p->i - 1);
}

/* snprintf interface accepting arg_fn/arg_payload arguments */
int
PF_ASNPRINTF(char *buf, size_t nbyte, const char *fmt,
             pf_arg_fn *arg_fn, void *arg_payload)
{
    return pf_vasnprintf(buf, nbyte, fmt, 0, arg_fn, arg_payload);
}

/* snprintf interface accepting a va_list argument */
int
PF_VSNPRINTF(char *buf, size_t nbyte, const char *fmt, va_list va)
{
    va_list va_copy;
    int ret;

    va_copy(va_copy, va);
    ret = pf_vasnprintf(buf, nbyte, fmt, &va_copy, 0, 0);
    va_end(va_copy);

    return ret;
}

/* snprintf interface accepting variable amount of arguments */
int
PF_SNPRINTF(char *buf, size_t nbyte, const char *fmt, ...)
{
    va_list va;
    int ret;

    va_start(va, fmt);
    ret = pf_vasnprintf(buf, nbyte, fmt, &va, 0, 0);
    va_end(va);

    return ret;
}
