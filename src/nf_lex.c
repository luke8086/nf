/*
 * Copyright (c) 2015 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_lex.c - lexical parser
 */

#include "nf_cmmn.h"

/* local functions */
static int nf_is_delim(int c);
static int nf_dec_to_int(char c);
static int nf_oct_to_int(int c);
static int nf_hex_to_int(int c);
static int nf_is_dec(int c);
static int nf_is_oct(int c);
static int nf_is_hex(int c);
static char *nf_parse_escape_seq(char *src, char *dst);
static int nf_match_string(char *src);
static char *nf_parse_string(char *src, struct nf_token *tok);
static int nf_match_number(char *src);
static char *nf_parse_number(char *src, struct nf_token *tok);
static char *nf_parse_word(char *src, struct nf_token *tok);
static int nf_match_comment(char *src);
static char *nf_skip_delim(char *src);

/* check for a delimiter */
static int
nf_is_delim(int c)
{
    return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}

/* return integer value of a decimal digit or -1 on error */
static int
nf_dec_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    return -1;
}

/* return integer value of an octal digit or -1 on error */
static int
nf_oct_to_int(int c)
{
    if (c >= '0' && c <= '7')
        return c - '0';
    return -1;
}

/* return integer value of a hexadecimal digit or -1 on error */
static int
nf_hex_to_int(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* check for a decimal digit */
static int
nf_is_dec(int c)
{
    return nf_dec_to_int(c) >= 0;
}

/* check for an octal digit */
static int
nf_is_oct(int c)
{
    return nf_oct_to_int(c) >= 0;
}

/* check for a hexadecimal digit */
static int
nf_is_hex(int c)
{
    return nf_hex_to_int(c) >= 0;
}

/*
 * parse escape sequence from src and store character to dst[0]
 * return address of the first non-consumed character or 0 on error
 */
static char *
nf_parse_escape_seq(char *src, char *dst)
{
    char s0 = src[0];
    char s1 = s0 ? src[1] : 0;
    char s2 = s1 ? src[2] : 0;
    char ch;
    int n;

    /* \xnn - hexadecimal number, 3 characters */
    if (s0 == 'x' && nf_is_hex(s1) && nf_is_hex(s2)) {
        ch = nf_hex_to_int(s1) * 16 + nf_hex_to_int(s2);
        n = 3;
    }

    /* \nnn - octal number, 3 characters  */
    else if (nf_is_oct(s0) && nf_is_oct(s1) && nf_is_oct(s2)) {
        ch = nf_oct_to_int(s0) * 64 + nf_oct_to_int(s1) * 8 + nf_oct_to_int(s2);
        n = 3;
    }

    /* \s - single character */
    else switch (s0) {
    case 'a':  ch = '\a'; n = 1; break;
    case 'b':  ch = '\b'; n = 1; break;
    case 'f':  ch = '\f'; n = 1; break;
    case 'n':  ch = '\n'; n = 1; break;
    case 'r':  ch = '\r'; n = 1; break;
    case 't':  ch = '\t'; n = 1; break;
    case 'v':  ch = '\v'; n = 1; break;
    case '\\': ch = '\\'; n = 1; break;
    case '\'': ch = '\''; n = 1; break;
    case '\"': ch = '\"'; n = 1; break;
    case '\?': ch = '\?'; n = 1; break;
    default:   ch = 0;    n = 0; break;
    }

    /* success */
    if (n) {
        *dst = ch;
        return src + n;
    }

    /* error */
    return 0;
}

/* check if src matches a string */
static int
nf_match_string(char *src)
{
    return (src && src[0] == '\"');
}

/*
 * parse string from src and store in tok
 * return address of the first non-consumed character or 0 on error
 */
static char *
nf_parse_string(char *src, struct nf_token *tok)
{
    char s0, s1;
    int n;

    /* make sure src matches a string */
    if (!nf_match_string(src)) {
        tok->type = NF_TOKEN_INVALID;
        return 0;
    }

    /* initialize token */
    tok->type = NF_TOKEN_STRING;
    n = 0;

    /* skip initial quote */
    src++;

    while (1) {

        s0 = src[0];
        s1 = s0 ? src[1] : 0;

        /* the only valid termination of string: " followed by 0 or delimiter */
        if (s0 == '"' && (!s1 || nf_is_delim(s1))) {
            src += 1;
            tok->str[n] = 0;
            return src;
        }

        /* premature null terminator, " terminator or buffer overflow */
        else if (!s0 || s0 == '\"' || n >= NF_TOKEN_MAX_WIDTH) {
            tok->type = NF_TOKEN_INVALID;
            return 0;
        }

        /* escape sequence */
        else if (s0 == '\\') {
            src = nf_parse_escape_seq(src + 1, &tok->str[n++]);
            if (!src) {
                tok->type = NF_TOKEN_INVALID;
                return 0;
            }
        }

        /* regular character */
        else {
            tok->str[n++] = s0;
            src += 1;
            continue;
        }

    }

    /* NOTREACHED */
}

/* check if src matches a number */
static int
nf_match_number(char *src)
{
    /* null pointer */
    if (!src) {
        return 0;
    }

    /* starts with decimal digit */
    else if (nf_is_dec(src[0])) {
        return 1;
    }

    /* starts with +/- and decimal digit */
    else if ((src[0] == '-' || src[0] == '+') && nf_is_dec(src[1])) {
        return 1;
    }

    /* not a number */
    else {
        return 0;
    }
}

/*
 * parse number from src and store in tok
 * return address of the first non-consumed character or 0 on error
 */
static char *
nf_parse_number(char *src, struct nf_token *tok)
{
    int mul;
    int base;
    char *s = src;

    tok->type = NF_TOKEN_NUMBER;
    tok->num = 0;

    /* - sign */
    if (s[0] == '-') {
        mul = -1;
        s += 1;
    }
    /* + sign */
    else if (s[0] == '+') {
        mul = 1;
        s += 1;
    }
    /* no explicit sign */
    else {
        mul = 1;
    }

    /* hexadecimal prefix */
    if (s[0] == '0' && s[1] == 'x' && nf_is_hex(s[2])) {
        base = 16;
        s += 2;
    }
    /* octal prefix with octal character */
    else if (s[0] == '0' && nf_is_oct(s[1])) {
        base = 8;
        s += 1;
    }
    /* octal prefix with non octal character */
    else if (s[0] == '0' && s[1] && !nf_is_delim(s[1])) {
        tok->type = NF_TOKEN_INVALID;
        return 0;
    }
    /* no prefix */
    else {
        base = 10;
    }

    while (1) {

        /* null-terminator or delimiter */
        if (s[0] == 0 || nf_is_delim(s[0])) {
            tok->num *= mul;
            return s;
        }

        /* base 10 and dec digit */
        else if (base == 10 && nf_is_dec(s[0])) {
            tok->num = tok->num * base + nf_dec_to_int(s[0]);
            s += 1;
            continue;
        }

        /* base 8 and oct digit */
        else if (base == 8 && nf_is_oct(s[0])) {
            tok->num = tok->num * base + nf_oct_to_int(s[0]);
            s += 1;
            continue;
        }

        /* base 16 and hex digit */
        else if (base == 16 && nf_is_hex(s[0])) {
            tok->num = tok->num * base + nf_hex_to_int(s[0]);
            s += 1;
            continue;
        }

        /* invalid character */
        else {
            tok->type = NF_TOKEN_INVALID;
            return 0;
        }

    }

    /* NOTREACHED */
}

/*
 * parse word from src and store in tok
 * return address of the first non-consumed character or 0 on error
 */
static char *
nf_parse_word(char *src, struct nf_token *tok)
{
    int n;
    char s0;

    /* assume first character is already valid */
    tok->type = NF_TOKEN_WORD;
    n = 0;

    while (1) {

        s0 = src[0];

        /* null terminator or delimiter */
        if (!s0 || nf_is_delim(s0)) {
            tok->str[n] = 0;
            return src;
        }

        /* buffer overflow */
        else if (n >= NF_TOKEN_MAX_WIDTH) {
            tok->type = NF_TOKEN_INVALID;
            return 0;
        }

        /* regular character */
        else {
            tok->str[n++] = s0;
            src += 1;
        }

    }

    /* NOTREACHED */
}

/* check if src matches a comment */
static int
nf_match_comment(char *src)
{
    return (src && src[0] == '\\' && src[1] == ' ');
}

/*
 * skip initial delimiters from src
 * return address of the first non-delimiter character or 0 on end of string
 */
static char *
nf_skip_delim(char *src)
{
    char s0;

    /* handle null pointer */
    if (!src) {
        return 0;
    }

    while (1) {

        s0 = src[0];

        /* end of string */
        if (!s0) {
            return 0;
        }

        /* delimiter */
        else if (nf_is_delim(s0)) {
            src++;
            continue;
        }

        /* regular character */
        else {
            return src;
        }

    }

    /* NOTREACHED */
}

/*
 * parse first token from src and save in tok
 * return address of the first non-consumed character or 0 on error or end of string
 */
char *
nf_parse_token(char *src, struct nf_token *tok)
{
    /* skip initial delimiters */
    if (!(src = nf_skip_delim(src))) {
        tok->type = NF_TOKEN_EMPTY;
        return 0;
    }

    /* handle line comment */
    if (nf_match_comment(src)) {
        tok->type = NF_TOKEN_EMPTY;
        return 0;
    }

    /* parse string */
    if (nf_match_string(src)) {
        return nf_parse_string(src, tok);
    }

    /* parse number */
    if (nf_match_number(src)) {
        return nf_parse_number(src, tok);
    }

    /* parse word */
    else {
        return nf_parse_word(src, tok);
    }

    /* NOTREACHED */
}
