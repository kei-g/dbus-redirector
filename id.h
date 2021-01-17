#ifndef __include_id_h__
#define __include_id_h__

#include "typedef.h"

void dbusrdr_id_dealloc(
	dbusrdr_id_t **idp
);

dbusrdr_id_t *dbusrdr_id_init(void);

#include <sys/stat.h>

_Bool dbusrdr_is_executable(
	const dbusrdr_id_t *id,
	const struct stat *status
);

#endif /* __include_id_h__ */
