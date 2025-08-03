/*
 * Adapted from 'The UNIX Programming Environment' by Kernighan & Pike
 * and an example from the manualpage for vprintf by
 * Gaute Nessan, University of Tromsoe (gaute@staff.cs.uit.no).
 *
 * Modified by Bjoern Stabell.
 * Windows mods and memory leak detection by Dick Balaska.
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdarg>

#include "strlcpy.h"

#include "xpconfig.h"
#include "const.h"
#include "xperror.h"
#include "portability.h"

/*
 * This file defines two entry points:
 *
 * init_error()                - Initialize the error routine, accepts program name
 *                          as input.
 * xperror()                - perror() with printf functionality.
 */

/*
 * File local static data.
 */
#define MAX_PROG_LENGTH 32
static char progname[MAX_PROG_LENGTH];

static const char *prog_basename(const char *prog)
{
    const char *p;

    p = strrchr(prog, '/');

    return (p != NULL) ? (p + 1) : prog;
}

/*
 * Functions.
 */
void init_error(const char *prog)
{
    const char *p = prog_basename(prog); /* Beautify arv[0] */

    strlcpy(progname, p, MAX_PROG_LENGTH);
}

void xpinfo(const char *fmt, ...)
{
    int len;
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
    {
        fprintf(stderr, "%s: ", progname);
    }

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
    {
        fprintf(stderr, "\n");
    }

    va_end(ap);
}

void xpwarn(const char *fmt, ...)
{
    int len;
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
    {
        fprintf(stderr, "%s: ", progname);
    }

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
    {
        fprintf(stderr, "\n");
    }

    va_end(ap);
}

void xperror(const char *fmt, ...)
{
    va_list ap;
    int e = errno;

    va_start(ap, fmt);

    if (progname[0] != '\0')
    {
        fprintf(stderr, "%s: ", progname);
    }

    vfprintf(stderr, fmt, ap);

    if (e != 0)
    {
        fprintf(stderr, ": (%s)", strerror(e));
    }
    fprintf(stderr, "\n");

    va_end(ap);
}

void xpfatal(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
    {
        fprintf(stderr, "%s: ", progname);
    }

    vfprintf(stderr, fmt, ap);

    fprintf(stderr, "\n");

    va_end(ap);

    exit(1);
}

void xpdumpcore(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
    {
        fprintf(stderr, "%s: ", progname);
    }

    vfprintf(stderr, fmt, ap);

    fprintf(stderr, "\n");

    va_end(ap);

    abort();
}
