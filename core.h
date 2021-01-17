#ifndef __include_core_h__
#define __include_core_h__

#include "service.h"

dbusrdr_core_t *dbusrdr_core_alloc(void);

dbusrdr_ent_t *dbusrdr_core_alloc_entry(
	dbusrdr_core_t *core
);

_Bool dbusrdr_core_connect(
	dbusrdr_core_t *core,
	_Bool systembus
);

void dbusrdr_core_dealloc(
	dbusrdr_core_t **corep
);

_Bool dbusrdr_core_enqueue_service(
	dbusrdr_core_t *core,
	dbusrdr_svc_t *svc
);

void dbusrdr_core_remove_thread(
	dbusrdr_core_t *core,
	dbusrdr_ent_t **thrp /* NOTE: must be a thread entry */
);

void dbusrdr_core_run(
	dbusrdr_core_t *core
);

#endif /* __include_core_h__ */
