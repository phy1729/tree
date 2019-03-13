/*
 * stdlib.h compatibility shim
 * Public domain
 */

#include_next <stdlib.h>

#ifndef LIBCOMPAT_STDLIB_H
#define LIBCOMPAT_STDLIB_H

void *reallocarray(void *, size_t, size_t);
long long strtonum(const char *, long long, long long, const char **);

#endif
