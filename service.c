#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "core.h"
#include "entries.h"
#include "id.h"
#include "log.h"
#include "mem.h"

static bool verify_name(const char *name);

struct _dbusrdr_service {
	dbusrdr_ent_t *ent;
	dbusrdr_svc_t *next;
};

bool dbusrdr_add_exec_arg(svc, exec_arg)
	dbusrdr_svc_t *svc;
	char *exec_arg;
{
	DR_ASSERT(svc->ent != NULL);
	dbusrdr_ent_t *ent = svc->ent;
	char **new = (char **)realloc(ent->svc.exec.argp,
		sizeof(char *) * (ent->svc.exec.argc + 2));
	if (!new)
		return false;
	ent->svc.exec.argp = new;
	ent->svc.exec.argp[ent->svc.exec.argc++] = exec_arg;
	ent->svc.exec.argp[ent->svc.exec.argc] = NULL;
	return true;
}

bool dbusrdr_add_exec_env(svc, exec_env)
	dbusrdr_svc_t *svc;
	char *exec_env;
{
	DR_ASSERT(svc->ent != NULL);
	dbusrdr_ent_t *ent = svc->ent;
	char **new = (char **)realloc(ent->svc.exec.envp,
		sizeof(char *) * (ent->svc.exec.envc + 2));
	if (!new) {
		lprintf(LL_FATAL, "realloc: %s\n", strerror(errno));
		return false;
	}
	ent->svc.exec.envp = new;
	ent->svc.exec.envp[ent->svc.exec.envc++] = exec_env;
	ent->svc.exec.envp[ent->svc.exec.envc] = NULL;
	return true;
}

bool dbusrdr_attach_exec_path(svc, exec_path)
	dbusrdr_svc_t *svc;
	char *exec_path;
{
	DR_ASSERT(svc != NULL);
	DR_ASSERT(exec_path != NULL);
	DR_ASSERT(svc->ent != NULL);
	dbusrdr_ent_t *ent = svc->ent;
	struct stat status;
	if (stat(exec_path, &status) < 0) {
		lprintf(LL_WARN, "stat(%s): %s\n",
			exec_path, strerror(errno));
		return false;
	}
	DR_ASSERT(ent->core != NULL);
	if (!dbusrdr_is_executable(ent->id, &status)) {
		lprintf(LL_WARN, "%s is unexecutable\n", exec_path);
		return false;
	}
	ent->svc.exec.path = exec_path;
	return true;
}

bool dbusrdr_attach_interface(svc, name)
	dbusrdr_svc_t *svc;
	char *name;
{
	DR_ASSERT(svc != NULL);
	DR_ASSERT(name != NULL);
	DR_ASSERT(svc->ent != NULL);
	if (!verify_name(name)) {
		lprintf(LL_WARN, "inappropriate interface name, '%s'\n", name);
		return false;
	}
	svc->ent->svc.iface_name = name;
	return true;
}

bool dbusrdr_attach_signal(svc, name)
	dbusrdr_svc_t *svc;
	char *name;
{
	DR_ASSERT(svc != NULL);
	DR_ASSERT(name != NULL);
	DR_ASSERT(svc->ent != NULL);
	if (!verify_name(name)) {
		lprintf(LL_WARN, "inappropriate signal name, '%s'\n", name);
		return false;
	}
	svc->ent->svc.sig_name = name;
	return true;
}

dbusrdr_svc_t *dbusrdr_svc_alloc(core)
	dbusrdr_core_t *core;
{
	DR_ASSERT(core != NULL);
	dbusrdr_ent_t *ent = dbusrdr_core_alloc_entry(core);
	if (!ent)
		return NULL;
	ent->svc.exec.argc = 0;
	ent->svc.exec.argp = (char **)malloc(sizeof(char *) * 1);
	if (!ent->svc.exec.argp) {
		free(ent);
		return NULL;
	}
	ent->svc.exec.argp[0] = NULL;
	ent->svc.exec.envc = 0;
	ent->svc.exec.envp = (char **)malloc(sizeof(char *) * 1);
	if (!ent->svc.exec.envp) {
		free(ent->svc.exec.argp);
		free(ent);
		return NULL;
	}
	ent->svc.exec.envp[0] = NULL;
	ent->svc.exec.path = NULL;
	ent->svc.iface_name = NULL;
	ent->svc.sig_name = NULL;
	ent->next = NULL;
	ent->core = core;
	dbusrdr_svc_t *svc = (dbusrdr_svc_t *)malloc(sizeof(*svc));
	if (!svc) {
		free(ent->svc.exec.argp);
		free(ent->svc.exec.envp);
		free(ent);
		return NULL;
	}
	svc->ent = ent;
	svc->next = NULL;
	return svc;
}

void dbusrdr_svc_dealloc(svcp)
	dbusrdr_svc_t **svcp;
{
	DR_ASSERT(svcp != NULL);
	dbusrdr_svc_t *svc = *svcp;
	*svcp = NULL;
	DR_ASSERT(svc != NULL);
	dbusrdr_ent_t *ent = svc->ent;
	DR_ASSERT(ent != NULL);
	svc->ent = NULL;
	DR_ASSERT(ent->svc.exec.argp != NULL);
	free((void *)ent->svc.exec.argp);
	DR_ASSERT(ent->svc.exec.envp != NULL);
	free((void *)ent->svc.exec.envp);
	free(ent);
	free(svc);
}

static bool verify_name(name)
	const char *name;
{
	for (off_t i = 0;; i++) {
		char c = name[i];
		if (!c)
			return i != 0;
		if ('0' <= c && c <= '9')
			continue;
		if ('A' <= c && c <= 'Z')
			continue;
		if ('a' <= c && c <= 'z')
			continue;
		if (c == '.' || c == '-') {
			if (i == 0)
				return false;
			else if (name[i - 1] == '.' || name[i - 1] == '-')
				return false;
			else if (!name[i + 1])
				return false;
			else
				continue;
		}
		lprintf(LL_WARN, "invalid character '%c' appeared at %zd of '%s'\n",
			c, i, name);
		return false;
	}
}
