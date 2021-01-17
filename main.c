#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "core.h"
#include "log.h"
#include "service.h"

extern FILE *__restrict__ FDLOG;
extern const char *PROGNAME;

static dbusrdr_core_t *parse_args(
	int argc,
	char *argv[]
);

static bool parse_service_options(
	dbusrdr_core_t *core,
	char *svcopts
);

int main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	FDLOG = stderr;
	PROGNAME = basename(argv[0]);
	dbusrdr_core_t *core = parse_args(argc, argv);
	if (!core)
		return 1;
	dbusrdr_core_run(core);
	return 0;
}

static dbusrdr_core_t *parse_args(argc, argv)
	int argc;
	char *argv[];
{
	DR_ASSERT(0 < argc);
	DR_ASSERT(argv != NULL);
	dbusrdr_core_t *core = dbusrdr_core_alloc();
	if (!core)
		return NULL;
	int daemonize = 0, is_session = 0, is_system = 0, trunc = 0;
	struct option lopts[] = {
		{ "daemonize", no_argument, &daemonize, 'd', },
		{ "log", required_argument, NULL, 'l', },
		{ "service", optional_argument, NULL, 's', },
		{ "session", no_argument, &is_session, 0, },
		{ "system", no_argument, &is_system, 0, },
		{ "truncate-logfile", no_argument, &trunc, 't', },
		{ NULL, no_argument, NULL, 0, },
	};
	char *logpath = NULL;
	for (;;) {
		int idx = 0;
		int c = getopt_long(argc, argv, "dl:s:t", lopts, &idx);
		if (c < 0)
			break;
		switch (c) {
		case 'l':
			DR_ASSERT(optarg != NULL);
			logpath = optarg;
			break;
		case 's':
			DR_ASSERT(0 < optind && optind < argc);
			DR_ASSERT(argv[optind] != NULL);
			DR_ASSERT(*argv[optind] != '\0');
			if (!parse_service_options(core, argv[optind])) {
				dbusrdr_core_dealloc(&core);
				return NULL;
			}
			break;
		}
	}
	if (logpath) {
		FILE *f = fopen(logpath, trunc ? "w" : "a");
		if (!f) {
			lprintf(LL_FATAL, "fopen(%s): %s\n",
				logpath, strerror(errno));
			dbusrdr_core_dealloc(&core);
			return NULL;
		}
		FDLOG = f;
	}
	if (is_session && is_system) {
		lprintf(LL_WARN, "bustype must be either session or system\n");
		dbusrdr_core_dealloc(&core);
		return NULL;
	}
	if (is_session)
		is_system = 0;
	if (!dbusrdr_core_connect(core, is_system)) {
		dbusrdr_core_dealloc(&core);
		return NULL;
	}
	if (daemonize) {
		pid_t pid = fork();
		if (pid < 0) {
			lprintf(LL_FATAL, "fork: %s\n", strerror(errno));
			dbusrdr_core_dealloc(&core);
			return NULL;
		}
		if (pid) {
			lprintf(LL_SUCCESS, "fork: pid=%d\n", pid);
			exit(0);
		}
	}
	return core;
}

typedef enum {
	svc_arg = 0,
	svc_env,
	svc_exec,
	svc_iface,
	svc_sig,
} servicearg_t;

static bool parse_service_options(core, svcopts)
	dbusrdr_core_t *core;
	char *svcopts;
{
	DR_ASSERT(core != NULL);
	DR_ASSERT(svcopts != NULL);
	dbusrdr_svc_t *svc = dbusrdr_svc_alloc(core);
	if (!svc)
		return false;
	for (char *subopts = svcopts; *subopts;) {
		char *const token[] = {
			[svc_arg] = "arg",
			[svc_env] = "env",
			[svc_exec] = "exec",
			[svc_iface] = "interface",
			[svc_sig] = "signal",
		}, *value = NULL;
		switch (getsubopt(&subopts, token, &value)) {
		case svc_arg:
			if (!dbusrdr_add_exec_arg(svc, value)) {
				dbusrdr_svc_dealloc(&svc);
				return false;
			}
			break;
		case svc_env:
			if (!dbusrdr_add_exec_env(svc, value)) {
				dbusrdr_svc_dealloc(&svc);
				return false;
			}
			break;
		case svc_exec:
			if (!dbusrdr_attach_exec_path(svc, value)) {
				dbusrdr_svc_dealloc(&svc);
				return false;
			}
			break;
		case svc_iface:
			if (!dbusrdr_attach_interface(svc, value)) {
				dbusrdr_svc_dealloc(&svc);
				return false;
			}
			break;
		case svc_sig:
			if (!dbusrdr_attach_signal(svc, value)) {
				dbusrdr_svc_dealloc(&svc);
				return false;
			}
			break;
		default:
			lprintf(LL_WARN, "unrecognized suboption, %s\n", value);
			break;
		}
	}
	if (!dbusrdr_core_enqueue_service(core, svc)) {
		dbusrdr_svc_dealloc(&svc);
		return false;
	}
	return true;
}
