#ifndef PW_TIMER_H
#define PW_TIMER_H

#include <stdint.h>

#include "picowalker-defs.h"

/// @file timer.h

void pw_rtc_regular_processing();
pw_rtc_events_t pw_time_get_rtc_events();

#endif /* PW_TIMER_H */
