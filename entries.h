#ifndef __include_entries_h__
#define __include_entries_h__

#include "typedef.h"

void dbusrdr_entries_append(
	dbusrdr_entries_t *entries,
	dbusrdr_ent_t *ent
);

void dbusrdr_entries_deinit(
	dbusrdr_entries_t *entries
);

_Bool dbusrdr_entries_init(
	dbusrdr_entries_t *entries
);

void dbusrdr_entries_remove(
	dbusrdr_entries_t *entries,
	dbusrdr_ent_t *ent
);

_Bool dbusrdr_entries_traverse(
	dbusrdr_entries_t *entries,
	_Bool rdonly,
	dbusrdr_callback_f callback,
	...
);

#endif /* __include_entries_h__ */
