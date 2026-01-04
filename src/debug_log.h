#ifndef PW_DEBUG_LOG_H
#define PW_DEBUG_LOG_H

#include <stdarg.h>

#define pw_log_error(s, ...) pw_log_write("[Error] " s __VA_OPT__(,) __VA_ARGS__)
#define pw_log_warn(s, ...)  pw_log_write("[Warn ] " s __VA_OPT__(,) __VA_ARGS__)
#define pw_log_info(s, ...)  pw_log_write("[Info ] " s __VA_OPT__(,) __VA_ARGS__)
#define pw_log_debug(s, ...) pw_log_write("[Debug] " s __VA_OPT__(,) __VA_ARGS__)

void pw_log_write(const char* fmt, ...);

extern void pw_log_vprintf(const char* fmt, va_list list);

#endif /* PW_DEBUG_LOG_H */

