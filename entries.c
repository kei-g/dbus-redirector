#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "entries.h"
#include "log.h"

void dbusrdr_entries_append(entries, ent)
	dbusrdr_entries_t *entries;
	dbusrdr_ent_t *ent;
{
	DR_ASSERT(entries != NULL);
	DR_ASSERT(entries->tail != NULL);
	DR_ASSERT(ent->next == NULL);
	pthread_rwlock_wrlock(&entries->rwlock);
	*entries->tail = ent;
	entries->tail = &ent->next;
	pthread_rwlock_unlock(&entries->rwlock);
}

void dbusrdr_entries_deinit(entries)
	dbusrdr_entries_t *entries;
{
	DR_ASSERT(entries != NULL);
	DR_ASSERT(entries->head == NULL);
	DR_ASSERT(entries->tail == &entries->head);
	pthread_rwlock_destroy(&entries->rwlock);
}

bool dbusrdr_entries_init(entries)
	dbusrdr_entries_t *entries;
{
	DR_ASSERT(entries != NULL);
	entries->head = NULL;
	entries->tail = &entries->head;
	int err = pthread_rwlock_init(&entries->rwlock, NULL);
	if (err) {
		lprintf(LL_FATAL, "pthread_rwlock_init: %s\n", strerror(err));
		return false;
	}
	return true;
}

void dbusrdr_entries_remove(entries, ent)
	dbusrdr_entries_t *entries;
	dbusrdr_ent_t *ent;
{
	DR_ASSERT(entries != NULL);
	DR_ASSERT(ent != NULL);
	pthread_rwlock_wrlock(&entries->rwlock);
	for (dbusrdr_ent_t *cur = entries->head; cur; cur = cur->next)
		if (cur == ent) {
			entries->head = ent->next;
			if (entries->tail == &ent->next)
				entries->tail = &entries->head;
			ent->next = NULL;
			pthread_rwlock_unlock(&entries->rwlock);
			return;
		}
		else if (cur->next == ent) {
			cur->next = ent->next;
			if (entries->tail == &ent->next)
				entries->tail = &cur->next;
			ent->next = NULL;
			pthread_rwlock_unlock(&entries->rwlock);
			return;
		}
	pthread_rwlock_unlock(&entries->rwlock);
	lprintf(LL_WARN, "%s: %p is not found in entries\n",
		__FUNCTION__, ent);
}

bool dbusrdr_entries_traverse(
	dbusrdr_entries_t *entries,
	bool rdonly,
	dbusrdr_callback_f callback,
	...
)
{
	DR_ASSERT(entries != NULL);
	DR_ASSERT(callback != NULL);
	if (rdonly)
		pthread_rwlock_rdlock(&entries->rwlock);
	else
		pthread_rwlock_wrlock(&entries->rwlock);
	bool res = 0;
	for (dbusrdr_ent_t *cur = entries->head; cur;) {
		dbusrdr_ent_t *next = cur->next;
		va_list args;
		va_start(args, callback);
		res = (*callback)(cur, &args);
		va_end(args);
		if (res)
			break;
		cur = next;
	}
	pthread_rwlock_unlock(&entries->rwlock);
	return res;
}
