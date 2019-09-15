/*
 * Copyright (c) 2019 luke8086.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * nf_str.c - string manipulation
 */

#include "nf_cmmn.h"

/* copy memory area */
void *
nf_memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *srcb = (unsigned char *)src;
    unsigned char *destb = (unsigned char *)dest;

    while (n--) {
        *(destb++) = *(srcb++);
    }

    return dest;
}

/* calculate the length of a string */
size_t
nf_strlen(const char *s1)
{
    size_t ret = 0;

    while (*s1++) {
        ++ret;
    }

    return ret;
}

/* compare two strings */
int
nf_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }
    return (*s1 - *s2);
}

/* copy a string */
char *
nf_strncpy(char *dest, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }

    while (i < n) {
        dest[i++] = '\0';
    }

    return dest;
}
