#ifndef PW_TIMER_H
#define PW_TIMER_H

#include <stdint.h>

#include "picowalker_structures.h"

/// @file timer.h

void pw_rtc_regular_processing();
pw_rtc_events_t pw_time_get_rtc_events();

/*
 * Functions defined by the driver module
 */
extern void pw_time_init_rtc(uint32_t last_sync);   // From RTC
extern void pw_time_set_rtc(uint32_t last_sync);    // From RTC
extern uint32_t pw_time_get_rtc();     // From RTC
extern pw_dhms_t pw_time_get_dhms();   // From RTC
extern pw_rtc_events_t pw_time_get_rtc_events();
extern uint64_t pw_time_get_us();  // Since boot
extern uint64_t pw_time_get_ms();  // Since boot
extern void pw_time_delay_ms(uint32_t ms);
extern void pw_time_delay_us(uint32_t us);

#endif /* PW_TIMER_H */

