#ifndef __include_typedef_h__
#define __include_typedef_h__

#include "pipe.h"

typedef struct _dbusrdr_core dbusrdr_core_t;
typedef struct _dbusrdr_entries dbusrdr_entries_t;
typedef struct _dbusrdr_entry dbusrdr_ent_t;
typedef struct _dbusrdr_id dbusrdr_id_t;
typedef struct _dbusrdr_service dbusrdr_svc_t;

#include <stdarg.h>

typedef _Bool (*dbusrdr_callback_f)(dbusrdr_ent_t*, va_list*);

#define __USE_UNIX98

#include <pthread.h>

struct _dbusrdr_entry {
	dbusrdr_core_t *core;
	dbusrdr_id_t *id;
	dbusrdr_ent_t *next;
	union {
		struct {
			struct {
				int argc;
				char **argp;
				int envc;
				char **envp;
				const char *path;
			} exec;
			const char *iface_name;
			const char *sig_name;
		} svc;
		struct {
			pid_t pid;
			pipe_t pipes;
			int status;
			dbusrdr_ent_t *svc;
			pthread_t thread;
		} thr;
	};
};

struct _dbusrdr_entries {
	dbusrdr_ent_t *head;
	pthread_rwlock_t rwlock;
	dbusrdr_ent_t **tail;
};

#endif /* __include_typedef_h__ */
