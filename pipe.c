#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "log.h"
#include "pipe.h"

static void close2fd(int fd[2]);
static bool pipe2fd(int fd[2]);
static bool setnonblock2fd(int fd[2]);

void pipe_close(pipes)
	pipe_t *pipes;
{
	DR_ASSERT(pipes != NULL);
	close2fd(pipes->err);
	close2fd(pipes->in);
	close2fd(pipes->out);
}

bool pipe_init(pipes)
	pipe_t *pipes;
{
	DR_ASSERT(pipes != NULL);
	if (!pipe2fd(pipes->in))
		return false;
	if (!pipe2fd(pipes->out)) {
		close2fd(pipes->in);
		return false;
	}
	if (!pipe2fd(pipes->err)) {
		close2fd(pipes->out);
		close2fd(pipes->in);
		return false;
	}
	return true;
}

bool pipe_dup(pipes)
	pipe_t *pipes;
{
	DR_ASSERT(pipes != NULL);
	if (dup2(pipes->in[1], 0) < 0) {
		lprintf(LL_FATAL, "dup2: %s\n", strerror(errno));
		return false;
	}
	if (dup2(pipes->out[1], 1) < 0) {
		lprintf(LL_FATAL, "dup2: %s\n", strerror(errno));
		return false;
	}
	if (dup2(pipes->err[1], 2) < 0) {
		lprintf(LL_FATAL, "dup2: %s\n", strerror(errno));
		return false;
	}
	return true;
}

static void close2fd(fd)
	int fd[2];
{
	for (int i = 0; i < 2; i++) {
		close(fd[i]);
		fd[i] = -1;
	}
}

static bool pipe2fd(fd)
	int fd[2];
{
	if (pipe(fd) < 0) {
		lprintf(LL_FATAL, "pipe: %s\n", strerror(errno));
		return false;
	}
	if (!setnonblock2fd(fd)) {
		close2fd(fd);
		return false;
	}
	return true;
}

static bool setnonblock2fd(fd)
	int fd[2];
{
	for (int i = 0; i < 2; i++) {
		if (fcntl(fd[i], F_SETFL, O_NONBLOCK) < 0) {
			lprintf(LL_FATAL, "fcntl: %s\n", strerror(errno));
			return false;
		}
	}
	return true;
}
