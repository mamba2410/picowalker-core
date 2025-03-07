#ifndef PW_TIMER_H
#define PW_TIMER_H

#include <stdint.h>

/// @file timer.h

/**
 * Structure containing days, hours, minutes and seconds
 */
typedef struct pw_dhms_s {
    uint16_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} pw_dhms_t;

// Deprecated
uint64_t pw_now_us();
void pw_timer_delay_ms(uint64_t ms);

// Waits and delays
void pw_time_delay_ms_blocking(uint32_t ms);
void pw_time_delay_us_blocking(uint32_t us);

void pw_time_seconds_to_dhms(pw_dhms_t *dhms, uint32_t rtc);

#endif /* PW_TIMER_H */
