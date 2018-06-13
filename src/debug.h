
#if !defined(LINEARBUFFERS_DEBUG_NAME)
#define LINEARBUFFERS_DEBUG_NAME	"unknown"
#endif

enum linearbuffers_debug_level {
	linearbuffers_debug_level_silent,
	linearbuffers_debug_level_error,
	linearbuffers_debug_level_warning,
	linearbuffers_debug_level_notice,
	linearbuffers_debug_level_info,
	linearbuffers_debug_level_debug,
};

extern enum linearbuffers_debug_level linearbuffers_debug_level;

#define linearbuffers_debugf(a...) { \
	if (linearbuffers_debug_level >= linearbuffers_debug_level_debug) { \
		linearbuffers_debug_printf(linearbuffers_debug_level_debug, LINEARBUFFERS_DEBUG_NAME, __FUNCTION__, __FILE__, __LINE__, a); \
	} \
}

#define linearbuffers_warningf(a...) { \
	if (linearbuffers_debug_level >= linearbuffers_debug_level_warning) { \
		linearbuffers_debug_printf(linearbuffers_debug_level_warning, LINEARBUFFERS_DEBUG_NAME, __FUNCTION__, __FILE__, __LINE__, a); \
	} \
}

#define linearbuffers_noticef(a...) { \
	if (linearbuffers_debug_level >= linearbuffers_debug_level_notice) { \
		linearbuffers_debug_printf(linearbuffers_debug_level_notice, LINEARBUFFERS_DEBUG_NAME, __FUNCTION__, __FILE__, __LINE__, a); \
	} \
}

#define linearbuffers_infof(a...) { \
	if (linearbuffers_debug_level >= linearbuffers_debug_level_info) { \
		linearbuffers_debug_printf(linearbuffers_debug_level_info, LINEARBUFFERS_DEBUG_NAME, __FUNCTION__, __FILE__, __LINE__, a); \
	} \
}

#define linearbuffers_errorf(a...) { \
	if (linearbuffers_debug_level >= linearbuffers_debug_level_error) { \
		linearbuffers_debug_printf(linearbuffers_debug_level_error, LINEARBUFFERS_DEBUG_NAME, __FUNCTION__, __FILE__, __LINE__, a); \
	} \
}

const char * linearbuffers_debug_level_to_string (enum linearbuffers_debug_level level);
enum linearbuffers_debug_level linearbuffers_debug_level_from_string (const char *string);
int linearbuffers_debug_printf (enum linearbuffers_debug_level level, const char *name, const char *function, const char *file, int line, const char *fmt, ...) __attribute__((format(printf, 6, 7)));
