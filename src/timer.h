#ifndef PW_TIMER_H
#define PW_TIMER_H

#include <stdint.h>

/// @file timer.h

// Deprecated
uint64_t pw_now_us();
void pw_timer_delay_ms(uint64_t ms);

// Waits and delays
void pw_time_delay_ms_blocking(uint32_t ms);
void pw_time_delay_us_blocking(uint32_t us);

#endif /* PW_TIMER_H */
