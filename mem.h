#ifndef __include_mem_h__
#define __include_mem_h__

#include <sys/types.h>

#define free(ptr) dbusrdr_free(__FUNCTION__, ptr)
#define malloc(size) dbusrdr_malloc(__FUNCTION__, size)
#define realloc(ptr, size) dbusrdr_realloc(__FUNCTION__, ptr, size)

void dbusrdr_free(
	const char *const caller,
	void *ptr
);

void *dbusrdr_malloc(
	const char *const caller,
	size_t size
);

void *dbusrdr_realloc(
	const char *const caller,
	void *ptr,
	size_t size
);

#endif /* __include_mem_h__ */
