/*
 * Public domain
 * unistd.h compatibility shim
 */

#include_next <unistd.h>

#ifndef LIBCOMPAT_UNISTD_H
#define LIBCOMPAT_UNISTD_H

int pledge(const char *, const char *);

#endif
