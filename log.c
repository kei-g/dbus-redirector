#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "log.h"

static const char *const level2str(loglevel_t level);

FILE *__restrict__ FDLOG = NULL;
const char *PROGNAME = NULL;

void lprintf(loglevel_t level, const char *const fmt, ...)
{
	DR_ASSERT(FDLOG != NULL);
	DR_ASSERT(PROGNAME != NULL);
	fprintf(FDLOG, "[%s] (%s) ", PROGNAME, level2str(level));
	DR_ASSERT(fmt != NULL);
	va_list ap;
	va_start(ap, fmt);
	vfprintf(FDLOG, fmt, ap);
	va_end(ap);
}

static const char *const level2str(loglevel_t level)
{
	switch (level) {
	case LL_DEBUG:
		return "DEBUG";
	case LL_FATAL:
		return "\033[31mFATAL\033[m";
	case LL_INFO:
		return "\033[36mINFO\033[m";
	case LL_NORMAL:
		return "NORMAL";
	case LL_SUCCESS:
		return "\033[32mSUCCESS\033[m";
	case LL_WARN:
		return "\033[33mWARN\033[m";
	default:
		__builtin_assume(0);
	}
}
