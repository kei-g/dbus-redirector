#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "core.h"
#include "entries.h"
#include "id.h"
#include "log.h"
#include "mem.h"
#include "thread.h"

typedef struct _dbusrdr_interface dbusrdr_ifc_t;

static bool add_matches(dbusrdr_core_t *core);
static bool dealloc_service(dbusrdr_ent_t *svc, va_list *argp);
static bool dealloc_thread(dbusrdr_ent_t *thr, va_list *argp);
static bool exec_entry_if_matched(dbusrdr_ent_t *svc, va_list *argp);
static bool request_names(dbusrdr_core_t *core);

#include <dbus/dbus.h>

struct _dbusrdr_core {
	DBusBusType bustype;
	DBusConnection *connection;
	DBusError error;
	dbusrdr_id_t *id;
	struct {
		dbusrdr_ifc_t *head;
		dbusrdr_ifc_t **tail;
	} interfaces;
	struct {
		dbusrdr_svc_t *head;
		dbusrdr_svc_t **tail;
	} queue;
	dbusrdr_entries_t services;
	dbusrdr_entries_t threads;
};

struct _dbusrdr_interface {
	const char *name;
	dbusrdr_ifc_t *next;
};

struct _dbusrdr_service {
	dbusrdr_ent_t *ent;
	dbusrdr_svc_t *next;
};

dbusrdr_core_t *dbusrdr_core_alloc(void)
{
	dbusrdr_core_t *core = (dbusrdr_core_t *)malloc(sizeof(*core));
	if (!core)
		return NULL;
	core->bustype = DBUS_BUS_SESSION;
	core->connection = NULL;
	dbus_error_init(&core->error);
	do {
		core->id = dbusrdr_id_init();
		if (!core->id)
			break;
		do {
			core->interfaces.head = NULL;
			core->interfaces.tail = &core->interfaces.head;
			core->queue.head = NULL;
			core->queue.tail = &core->queue.head;
			if (!dbusrdr_entries_init(&core->services))
				break;
			if (!dbusrdr_entries_init(&core->threads)) {
				dbusrdr_entries_deinit(&core->services);
				break;
			}
			return core;
		} while (0);
		dbusrdr_id_dealloc(&core->id);
	} while (0);
	dbus_error_free(&core->error);
	free(core);
	return NULL;
}

dbusrdr_ent_t *dbusrdr_core_alloc_entry(core)
	dbusrdr_core_t *core;
{
	dbusrdr_ent_t *ent = (dbusrdr_ent_t *)malloc(sizeof(*ent));
	if (!ent)
		return NULL;
	ent->core = core;
	ent->id = core->id;
	ent->next = NULL;
	return ent;
}

bool dbusrdr_core_connect(dbusrdr_core_t *core, bool systembus)
{
	DR_ASSERT(core != NULL);
	core->bustype = systembus ? DBUS_BUS_SYSTEM : DBUS_BUS_SESSION;
	core->connection = dbus_bus_get(core->bustype, &core->error);
	if (dbus_error_is_set(&core->error)) {
		lprintf(LL_FATAL, "dbus_bus_get: %s\n", core->error.message);
		return false;
	}
	lprintf(LL_INFO, "dbus_bus_get: connected to %s bus, %p\n",
		systembus ? "system" : "session", core->connection);
	if (request_names(core))
		return false;
	return add_matches(core);
}

void dbusrdr_core_dealloc(corep)
	dbusrdr_core_t **corep;
{
	DR_ASSERT(corep != NULL);
	dbusrdr_core_t *core = *corep;
	*corep = NULL;
	DR_ASSERT(core != NULL);
	for (dbusrdr_ifc_t *ifc = core->interfaces.head; ifc;) {
		dbusrdr_ifc_t *next = ifc->next;
		free(ifc);
		ifc = next;
	}
	for (dbusrdr_svc_t *svc = core->queue.head; svc;) {
		dbusrdr_svc_t *next = svc->next;
		free(svc);
		svc = next;
	}
	dbusrdr_entries_traverse(&core->services, false, &dealloc_service);
	dbusrdr_entries_traverse(&core->threads, false, &dealloc_thread);
	dbusrdr_entries_deinit(&core->services);
	dbusrdr_entries_deinit(&core->threads);
	dbusrdr_id_dealloc(&core->id);
	if (core->connection)
		dbus_connection_close(core->connection);
	dbus_error_free(&core->error);
	free(core);
}

bool dbusrdr_core_enqueue_service(core, svc)
	dbusrdr_core_t *core;
	dbusrdr_svc_t *svc;
{
	dbusrdr_ifc_t *ifc;
	for (ifc = core->interfaces.head; ifc; ifc = ifc->next)
		if (strcmp(ifc->name, svc->ent->svc.iface_name) == 0)
			break;
	if (!ifc) {
		ifc = (dbusrdr_ifc_t *)malloc(sizeof(*ifc));
		if (!ifc)
			return false;
		ifc->name = svc->ent->svc.iface_name;
		ifc->next = NULL;
		*core->interfaces.tail = ifc;
		core->interfaces.tail = &ifc->next;
	}
	*core->queue.tail = svc;
	core->queue.tail = &svc->next;
	return true;
}

void dbusrdr_core_remove_thread(core, thrp)
	dbusrdr_core_t *core;
	dbusrdr_ent_t **thrp;
{
	DR_ASSERT(core != NULL);
	DR_ASSERT(thrp != NULL);
	dbusrdr_ent_t *thr = *thrp;
	DR_ASSERT(thr != NULL);
	*thrp = NULL;
	dbusrdr_entries_remove(&core->threads, thr);
	dealloc_thread(thr, NULL);
}

void dbusrdr_core_run(core)
	dbusrdr_core_t *core;
{
	DR_ASSERT(core != NULL);
	for (;;) {
		dbus_connection_read_write(core->connection, 0);
		DBusMessage *msg = dbus_connection_pop_message(core->connection);
		if (!msg) {
			usleep(100000);
			continue;
		}
		int code = dbusrdr_entries_traverse(&core->services,
			true, &exec_entry_if_matched, msg);
		dbus_message_unref(msg);
		if (code < 0)
			break;
	}
}

static bool add_matches(core)
	dbusrdr_core_t *core;
{
	DR_ASSERT(core != NULL);
	dbusrdr_svc_t *svc = core->queue.head;
	while (svc) {
		dbusrdr_svc_t *next = svc->next;
		DR_ASSERT(svc->ent);
		dbusrdr_ent_t *ent = svc->ent;
		const char *const fmt = "type='signal',interface='%s'";
		DR_ASSERT(ent->svc.iface_name != NULL);
		DR_ASSERT(ent->svc.iface_name[0] != '\0');
		size_t len = strlen(fmt) + strlen(ent->svc.iface_name);
		char *match = (char *)malloc(len);
		if (!match)
			break;
		sprintf(match, fmt, ent->svc.iface_name);
		dbus_bus_add_match(core->connection, match, &core->error);
		free(match);
		dbus_connection_flush(core->connection);
		if (dbus_error_is_set(&core->error)) {
			lprintf(LL_FATAL, "failed to add an interface '%s', %s\n",
				ent->svc.iface_name, core->error.message);
			break;
		}
		lprintf(LL_INFO, "dbus_bus_add_match(%s) success\n",
			ent->svc.iface_name);
		dbusrdr_entries_append(&core->services, ent);
		free(svc);
		svc = next;
	}
	bool success = svc == NULL;
	while (svc) {
		dbusrdr_svc_t *next = svc->next;
		free(svc);
		svc = next;
	}
	core->queue.head = NULL;
	core->queue.tail = &core->queue.head;
	return success;
}

static bool dealloc_service(svc, argp)
	dbusrdr_ent_t *svc;
	va_list *argp;
{
	DR_ASSERT(svc != NULL);
	DR_ASSERT(svc->svc.exec.argp != NULL);
	free(svc->svc.exec.argp);
	DR_ASSERT(svc->svc.exec.envp != NULL);
	free(svc->svc.exec.envp);
	free(svc);
	return false;
}

static bool dealloc_thread(thr, argp)
	dbusrdr_ent_t *thr;
	va_list *argp;
{
	DR_ASSERT(thr != NULL);
	pipe_close(&thr->thr.pipes);
	pthread_detach(thr->thr.thread);
	free(thr);
	return false;
}

static bool exec_entry_if_matched(svc, argp)
	dbusrdr_ent_t *svc;
	va_list *argp;
{
	DR_ASSERT(svc != NULL);
	DR_ASSERT(argp != NULL);
	DBusMessage *msg = va_arg(*argp, DBusMessage*);
	DR_ASSERT(msg != NULL);
	DR_ASSERT(svc->svc.iface_name != NULL);
	DR_ASSERT(svc->svc.sig_name != NULL);
	if (!dbus_message_is_signal(msg, svc->svc.iface_name, svc->svc.sig_name))
		return false;
	dbusrdr_ent_t *thr = dbusrdr_execute_service(svc);
	if (thr)
		dbusrdr_entries_append(&svc->core->threads, thr);
	return true;
}

static bool request_names(core)
	dbusrdr_core_t *core;
{
	DR_ASSERT(core != NULL);
	dbusrdr_ifc_t *ifc = core->interfaces.head;
	while (ifc) {
		dbusrdr_ifc_t *next = ifc->next;
		DR_ASSERT(ifc->name != NULL);
		int ret = dbus_bus_request_name(core->connection, ifc->name,
			DBUS_NAME_FLAG_REPLACE_EXISTING, &core->error);
		if (dbus_error_is_set(&core->error)) {
			lprintf(LL_FATAL, "dbus_bus_request_name(%s): %s\n",
				ifc->name, core->error.message);
			break;
		}
		lprintf(LL_INFO, "dbus_bus_request_name(%s): success(%d)\n",
			ifc->name, ret);
		free(ifc);
		ifc = next;
	}
	bool success = ifc == NULL;
	while (ifc) {
		dbusrdr_ifc_t *next = ifc->next;
		free(ifc);
		ifc = next;
	}
	core->interfaces.head = NULL;
	core->interfaces.tail = &core->interfaces.head;
	return success;
}
