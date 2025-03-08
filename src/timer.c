#include <stdint.h>

#include "picowalker-defs.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "globals.h"
#include "timer.h"
#include "utils.h"


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
 * Called regularly from main loop, checks if any RTC events have occurred,
 * and if they have, perform the corresponding action.
 *
 * Polls the driver for which RTC events have happened.
 */
void pw_rtc_regular_processing() {
    pw_rtc_events_t events = pw_rtc_get_events();

    //printf("[Debug] events: 0x%04x\n", events);
    if(events & RTC_EVENT_EVERY_HOUR) {
        printf("[Debug] every hour\n");
        // idk why we do this
        if(health_data_cache.total_steps < 9999999) {
            if(health_data_cache.today_steps < 9999999) {
                health_data_cache.today_steps += 1;
            }
        }

        pw_eeprom_write_health_data(&health_data_cache);

        route_info_t ri;
        pw_eeprom_read(
            PW_EEPROM_ADDR_ROUTE_INFO,
            (uint8_t*)&ri,
            PW_EEPROM_SIZE_ROUTE_INFO
        );

        // TODO: Log event "fell asleep" if we have a pokemon walking with us
        //pw_log_event();
    }

    if(events & RTC_EVENT_EVERY_DAY) {
        printf("[Debug] every day\n");
        health_data_cache.total_days += 1;

        pw_eeprom_write_health_data(&health_data_cache);

        uint32_t historic_steps[7];
        pw_eeprom_read(
            PW_EEPROM_ADDR_HISTORIC_STEP_COUNT,
            (uint8_t*)historic_steps,
            PW_EEPROM_SIZE_HISTORIC_STEP_COUNT
        );

        for(size_t i = 6; i > 0; i--) {
            historic_steps[i] = historic_steps[i-1];
        }
        historic_steps[0] = swap_bytes_u32(health_data_cache.today_steps);
        health_data_cache.today_steps = 0;

        pw_eeprom_write(
            PW_EEPROM_ADDR_HISTORIC_STEP_COUNT,
            (uint8_t*)historic_steps,
            PW_EEPROM_SIZE_HISTORIC_STEP_COUNT
        );

        // supposed to erase peer team area 0xDC00/CURRENT_PEER_TEAM_DATA

    }

}

