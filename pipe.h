#ifndef __include_pipe_h__
#define __include_pipe_h__

#include <stdbool.h>

typedef struct _pipe {
	int err[2];
	int in[2];
	int out[2];
} pipe_t;

void pipe_close(
	pipe_t *p
);

_Bool pipe_dup(
	pipe_t *p
);

_Bool pipe_init(
	pipe_t *p
);

#endif /* __include_pipe_h__ */
