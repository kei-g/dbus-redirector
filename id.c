#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "id.h"
#include "log.h"
#include "mem.h"

struct _dbusrdr_id {
	int groupnum;
	gid_t *groups;
	uid_t uid;
};

void dbusrdr_id_dealloc(idp)
	dbusrdr_id_t **idp;
{
	DR_ASSERT(idp != NULL);
	dbusrdr_id_t *id = *idp;
	*idp = NULL;
	DR_ASSERT(id != NULL);
	free(id->groups);
	free(id);
}

dbusrdr_id_t *dbusrdr_id_init(void)
{
	dbusrdr_id_t *id = (dbusrdr_id_t *)malloc(sizeof(*id));
	if (!id)
		return NULL;
	int n = getgroups(0, NULL);
	if (n < 0) {
		lprintf(LL_FATAL, "getgroups: %s\n", strerror(errno));
		free(id);
		return NULL;
	}
	id->groupnum = n + 1;
	id->groups = (gid_t *)malloc(sizeof(gid_t) * id->groupnum);
	if (!id->groups) {
		free(id);
		return NULL;
	}
	id->groups[0] = getegid();
	n = getgroups(id->groupnum - 1, id->groups + 1);
	if (n < 0) {
		lprintf(LL_FATAL, "getgroups: %s\n", strerror(errno));
		free(id->groups);
		free(id);
		return NULL;
	}
	id->uid = geteuid();
	return id;
}

bool dbusrdr_is_executable(id, status)
	const dbusrdr_id_t *id;
	const struct stat *status;
{
	DR_ASSERT(id != NULL);
	DR_ASSERT(status != NULL);
	if (status->st_mode & S_IXOTH)
		return true;
	if ((status->st_mode & S_IXUSR) && status->st_uid == id->uid)
		return true;
	if (status->st_mode & S_IXGRP)
		for (int i = 0; i < id->groupnum; i++)
			if (status->st_gid == id->groups[i])
				return true;
	return false;
}
