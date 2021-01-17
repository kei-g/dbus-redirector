#ifndef __include_service_h__
#define __include_service_h__

#include "typedef.h"

_Bool dbusrdr_add_exec_arg(
	dbusrdr_svc_t *svc,
	char *exec_arg
);

_Bool dbusrdr_add_exec_env(
	dbusrdr_svc_t *svc,
	char *exec_env
);

_Bool dbusrdr_attach_exec_path(
	dbusrdr_svc_t *svc,
	char *exec_path
);

_Bool dbusrdr_attach_interface(
	dbusrdr_svc_t *svc,
	char *name
);

_Bool dbusrdr_attach_signal(
	dbusrdr_svc_t *svc,
	char *name
);

dbusrdr_svc_t *dbusrdr_svc_alloc(
	dbusrdr_core_t *core
);

void dbusrdr_svc_dealloc(
	dbusrdr_svc_t **svcp
);

#endif /* __include_service_h__ */
