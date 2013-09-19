#include "verbose.h"
#include <stdio.h>
#include <stdarg.h>

void verbose_out(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}
