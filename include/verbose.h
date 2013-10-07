#ifndef _VERBOSE_H
#define _VERBOSE_H

extern int verbose;

void verbose_out(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));;

#define VERBOSE(...) if (verbose > 0) verbose_out("V: " __VA_ARGS__)
#define VVERBOSE(...) if (verbose > 1) verbose_out("V: " __VA_ARGS__)
#define VVVERBOSE(...) if (verbose > 2) verbose_out("V: " __VA_ARGS__)
#define INFO(...) verbose_out("I: " __VA_ARGS__)
#define ERROR(...) verbose_out("E: " __VA_ARGS__)

#endif
