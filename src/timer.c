#include <stdint.h>

#include "picowalker-defs.h"
#include "timer.h"


/**
 * Deprecated
 */
uint64_t pw_now_us() {
    return pw_time_get_us();
}


/**
 * Deprecated
 */
void pw_timer_delay_ms(uint64_t ms) {
    pw_time_delay_ms_blocking(ms);
}


/**
 * Delays for approx `ms` milliseconds by polling `pw_time_get_ms()`.
 *
 * Maximum delay is 2^32 ms ~= 4 million seconds
 *
 * @param ms milliseconds to delay
 */
void pw_time_delay_ms_blocking(uint32_t ms) {
    uint64_t start = pw_time_get_ms();
    while( (pw_time_get_ms() - start) < ms) { }
}


/**
 * Delays for approx `us` microseconds by polling `pw_time_get_us()`.
 *
 * Maximum delay is 2^32 us ~= 4 thousand seconds, just over an hour
 *
 * @param us microseconds to delay
 */
void pw_time_delay_us_blocking(uint32_t us) {
    uint64_t start = pw_time_get_us();
    while( (pw_time_get_us() - start) < us) { }
}


/**
 * Converts an RTC seconds timestamp to hours, minutes and seconds
 */
void pw_time_seconds_to_dhms(pw_dhms_t *dhms, uint32_t rtc) {
    dhms->seconds = rtc%60;
    rtc = rtc/60;
    dhms->minutes = rtc%60;
    rtc = rtc/60;
    dhms->hours = rtc%24;
    rtc = rtc/24;
    dhms->days = rtc;
}

