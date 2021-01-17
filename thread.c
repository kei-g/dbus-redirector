#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "assert.h"
#include "core.h"
#include "log.h"
#include "mem.h"
#include "thread.h"

static void *exec_entry(void *arg);

dbusrdr_ent_t *dbusrdr_execute_service(svc)
	dbusrdr_ent_t *svc;
{
	DR_ASSERT(svc != NULL);
	dbusrdr_ent_t *thr = dbusrdr_core_alloc_entry(svc->core);
	if (!thr)
		return NULL;
	thr->next = NULL;
	DR_ASSERT(svc->core != NULL);
	thr->core = svc->core;
	thr->thr.pid = 0;
	if (!pipe_init(&thr->thr.pipes)) {
		free(thr);
		return NULL;
	}
	thr->thr.svc = svc;
	int err = pthread_create(&thr->thr.thread, NULL, &exec_entry, thr);
	if (!err) {
		lprintf(LL_FATAL, "pthread_create: %s\n", strerror(err));
		pipe_close(&thr->thr.pipes);
		free(thr);
		return NULL;
	}
	return thr;
}

static void *exec_entry(arg)
	void *arg;
{
	DR_ASSERT(arg != NULL);
	dbusrdr_ent_t *thr = (dbusrdr_ent_t *)arg;
	DR_ASSERT(thr->core != NULL);
	dbusrdr_core_t *core = thr->core;
	do {
		DR_ASSERT(thr->thr.svc != NULL);
		dbusrdr_ent_t *svc = thr->thr.svc;
		thr->thr.pid = fork();
		if (thr->thr.pid < 0) {
			lprintf(LL_FATAL, "fork: %s\n", strerror(errno));
			break;
		}
		if (thr->thr.pid == 0) {
			if (!pipe_dup(&thr->thr.pipes))
				break;
			if (execve(svc->svc.exec.path,
				(char *const *)svc->svc.exec.argp,
				(char *const *)svc->svc.exec.envp) < 0) {
				lprintf(LL_FATAL, "execve: %s\n", strerror(errno));
				break;
			}
		}
		/* XXX: TODO - communicate with pipes */
		if (waitpid(thr->thr.pid, &thr->thr.status, 0) == -1)
			lprintf(LL_FATAL, "waitpid: %s\n", strerror(errno));
	} while (0);
	dbusrdr_core_remove_thread(core, &thr);
	return NULL;
}
