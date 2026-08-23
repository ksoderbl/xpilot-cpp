/*
 * Adapted from 'The UNIX Programming Environment' by Kernighan & Pike
 * and an example from the manualpage for vprintf by
 * Gaute Nessan, University of Tromsoe (gaute@staff.cs.uit.no).
 *
 * Modified by Bjoern Stabell.
 * Windows mods and memory leak detection by Dick Balaska.
 */

#include "xperror.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdarg>

#include "commonproto.h"

/*
 * This file defines several entry points:
 *
 * init_error()        - Initialize the error routine, accepts program name as input.
 * error()             - perror() with printf functionality.
 * warn(), ...
 */

/*
 * File local static data.
 */
static std::string progname;

static std::string prog_basename(std::string path)
{
    auto pos = path.find_last_of('/');
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

/*
 * Functions.
 */
void init_error(std::string prog)
{
    progname = prog_basename(prog); /* Beautify argv[0] */
}

void xpinfo(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    fprintf(stderr, "%s: INFO:  ", progname.c_str());

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
        fprintf(stderr, "\n");

    va_end(ap);
}

void warn(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    char *s = showtime();

    fprintf(stderr, "%s %s: WARN:  ", s, progname.c_str());

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
        fprintf(stderr, "\n");

    va_end(ap);
}

void error(const char *fmt, ...)
{
    size_t len;
    va_list ap;
    int e = errno;

    va_start(ap, fmt);

    fprintf(stderr, "%s: ERROR: ", progname.c_str());

    vfprintf(stderr, fmt, ap);

    if (e != 0)
        fprintf(stderr, ": (%s)", strerror(e));

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
        fprintf(stderr, "\n");

    va_end(ap);
}

void fatal(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    fprintf(stderr, "%s: FATAL: ", progname.c_str());

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
        fprintf(stderr, "\n");

    va_end(ap);

    exit(1);
}

void dumpcore(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    fprintf(stderr, "%s: DUMPCORE: ", progname.c_str());

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
        fprintf(stderr, "\n");

    va_end(ap);

    abort();
}

void debugprint(const char *fmt, ...)
{
#if 0
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    char *s = showtime();

    fprintf(stderr, "%s %s: DEBUG: ", s, progname.c_str());

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
        fprintf(stderr, "\n");

    va_end(ap);
#endif
}
