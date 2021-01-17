#ifndef __include_log_h__
#define __include_log_h__

typedef enum {
	LL_DEBUG,
	LL_FATAL,
	LL_INFO,
	LL_NORMAL,
	LL_SUCCESS,
	LL_WARN,
} loglevel_t;

void lprintf(
	loglevel_t level,
	const char *const fmt,
	...
);

#endif /* __include_log_h__ */
