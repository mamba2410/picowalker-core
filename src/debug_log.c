#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include "debug_log.h"

void pw_log_write(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    pw_log_vprintf(fmt, args);

    va_end(args);
}

