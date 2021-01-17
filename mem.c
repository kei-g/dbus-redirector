#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "log.h"

void dbusrdr_free(caller, ptr)
	const char *const caller;
	void *ptr;
{
	DR_ASSERT(caller != NULL);
	if (ptr)
		free(ptr);
}

void *dbusrdr_malloc(caller, size)
	const char *const caller;
	size_t size;
{
	DR_ASSERT(caller != NULL);
	void *ptr = malloc(size);
	if (!ptr)
		lprintf(LL_FATAL, "%s: unable to malloc %zu bytes: %s\n",
			caller, size, strerror(errno));
	return ptr;
}

void *dbusrdr_realloc(caller, ptr, size)
	const char *const caller;
	void *ptr;
	size_t size;
{
	DR_ASSERT(caller != NULL);
	DR_ASSERT(ptr != NULL);
	DR_ASSERT(0 < size);
	void *new = realloc(ptr, size);
	if (!new)
		lprintf(LL_FATAL, "%s: unable to realloc %zu bytes: %s\n",
			caller, size, strerror(errno));
	return new;
}
