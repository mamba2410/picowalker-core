#ifndef PW_TIMER_H
#define PW_TIMER_H

#include <stdint.h>

#include "picowalker-defs.h"

/// @file timer.h

// Waits and delays
void pw_time_delay_ms_blocking(uint32_t ms);
void pw_time_delay_us_blocking(uint32_t us);

void pw_rtc_regular_processing();

#endif /* PW_TIMER_H */
