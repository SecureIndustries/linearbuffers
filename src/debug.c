
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "debug.h"

enum linearbuffers_debug_level linearbuffers_debug_level	= linearbuffers_debug_level_error;
static pthread_mutex_t linearbuffers_debug_mutex		= PTHREAD_MUTEX_INITIALIZER;

static char *debug_buffer	= NULL;
static int debug_buffer_size	= 0;

static void linearbuffers_debug_lock (void)
{
	pthread_mutex_lock(&linearbuffers_debug_mutex);
}

static void linearbuffers_debug_unlock (void)
{
	pthread_mutex_unlock(&linearbuffers_debug_mutex);
}

const char * linearbuffers_debug_level_to_string (enum linearbuffers_debug_level level)
{
	switch (level) {
		case linearbuffers_debug_level_silent: return "silent";
		case linearbuffers_debug_level_error: return "error";
		case linearbuffers_debug_level_warning: return "warning";
		case linearbuffers_debug_level_notice: return "notice";
		case linearbuffers_debug_level_info: return "info";
		case linearbuffers_debug_level_debug: return "debug";
	}
	return "unknown";
}

enum linearbuffers_debug_level linearbuffers_debug_level_from_string (const char *string)
{
	if (string == NULL) {
		return linearbuffers_debug_level_error;
	}
	if (strcmp(string, "silent") == 0 || strcmp(string, "s") == 0) {
		return linearbuffers_debug_level_silent;
	}
	if (strcmp(string, "error") == 0 || strcmp(string, "e") == 0) {
		return linearbuffers_debug_level_error;
	}
	if (strcmp(string, "warning") == 0 || strcmp(string, "w") == 0) {
		return linearbuffers_debug_level_warning;
	}
	if (strcmp(string, "notice") == 0 || strcmp(string, "n") == 0) {
		return linearbuffers_debug_level_notice;
	}
	if (strcmp(string, "info") == 0 || strcmp(string, "i") == 0) {
		return linearbuffers_debug_level_info;
	}
	if (strcmp(string, "debug") == 0 || strcmp(string, "d") == 0) {
		return linearbuffers_debug_level_debug;
	}
	return linearbuffers_debug_level_error;
}

int linearbuffers_debug_printf (enum linearbuffers_debug_level level, const char *name, const char *function, const char *file, int line, const char *fmt, ...)
{
	int rc;
	va_list ap;

	struct timeval timeval;
	struct tm *tm;
	int milliseconds;
	char date[80];

	linearbuffers_debug_lock();

	va_start(ap, fmt);
	rc = vsnprintf(debug_buffer, debug_buffer_size, fmt, ap);
	va_end(ap);
	if (rc < 0) {
		linearbuffers_debug_unlock();
		goto bail;
	}
	if (debug_buffer_size == 0 ||
	    rc >= debug_buffer_size) {
		free(debug_buffer);
		debug_buffer = malloc(rc + 1);
		if (debug_buffer == NULL) {
			goto bail;
		}
		debug_buffer_size = rc + 1;
		va_start(ap, fmt);
		rc = vsnprintf(debug_buffer, debug_buffer_size, fmt, ap);
		va_end(ap);
		if (rc < 0) {
			linearbuffers_debug_unlock();
			goto bail;
		}
	}

	gettimeofday(&timeval, NULL);

	milliseconds = (int) ((timeval.tv_usec / 1000.0) + 0.5);
	if (milliseconds >= 1000) {
		milliseconds -= 1000;
		timeval.tv_sec++;
	}
	tm = localtime(&timeval.tv_sec);
	strftime(date, sizeof(date), "%x-%H:%M:%S", tm);

	fprintf(stderr, "linearbuffers:%s.%03d:%s:%s: %s (%s %s:%d)\n", date, milliseconds, name, linearbuffers_debug_level_to_string(level), debug_buffer, function, file, line);

	linearbuffers_debug_unlock();

	return 0;
bail:	va_end(ap);
	return -1;
}

__attribute__((destructor)) int linearbuffers_debug_fini (void)
{
	if (debug_buffer != NULL) {
		free(debug_buffer);
	}
	return 0;
}
