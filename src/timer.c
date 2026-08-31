#include "timer.h"

#include <stdint.h>

#include "accel.h"
#include "debug_log.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "globals.h"
#include "picowalker_structures.h"
#include "utils.h"

pw_dhms_t last_check = {
    0,
};

/**
 * Called regularly from main loop, checks if any RTC events have occurred,
 * and if they have, perform the corresponding action.
 *
 * Polls the driver for which RTC events have happened.
 */
void pw_rtc_regular_processing() {
    pw_rtc_events_t events = pw_time_get_rtc_events();

    // pw_log_debug("events: 0x%04x\n", events);
    if (events & RTC_EVENT_EVERY_HOUR) {
        pw_log_debug("every hour\n");
        // idk why we do this
        // if(health_data_cache.total_steps < 9999999) {
        //    if(health_data_cache.today_steps < 9999999) {
        //        health_data_cache.today_steps += 1;
        //    }
        //}

        health_data_cache.last_sync = pw_time_get_rtc();
        pw_eeprom_write_health_data(&health_data_cache);

        route_info_t ri;
        pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t *)&ri, PW_EEPROM_SIZE_ROUTE_INFO);

        // TODO: Log event "fell asleep" if we have a pokemon walking with us
        // pw_log_event();
    }

    if (events & RTC_EVENT_EVERY_DAY) {
        pw_log_debug("every day\n");
        health_data_cache.total_days += 1;

        pw_accel_process_steps();

        pw_eeprom_write_health_data(&health_data_cache);

        uint32_t historic_steps[7];
        pw_eeprom_read(
            PW_EEPROM_ADDR_HISTORIC_STEP_COUNT, (uint8_t *)historic_steps, PW_EEPROM_SIZE_HISTORIC_STEP_COUNT);

        for (size_t i = 6; i > 0; i--) {
            historic_steps[i] = historic_steps[i - 1];
            pw_log_debug("Today -%lu: 0x%08x\n", i + 1, historic_steps[i]);
        }
        historic_steps[0] = swap_bytes_u32(health_data_cache.today_steps);
        pw_log_debug("Today -1: 0x%08x\n", historic_steps[0]);
        health_data_cache.today_steps = 0;

        pw_eeprom_write(
            PW_EEPROM_ADDR_HISTORIC_STEP_COUNT, (uint8_t *)historic_steps, PW_EEPROM_SIZE_HISTORIC_STEP_COUNT);

        // supposed to erase peer team area 0xDC00/CURRENT_PEER_TEAM_DATA
    }
}

/**
 * Returns flags of what periodic events need to be handled
 */
pw_rtc_events_t pw_time_get_rtc_events() {
    pw_rtc_events_t events = 0;

    pw_dhms_t now = pw_time_get_dhms();
    if (last_check.days == 0) {
        last_check = now;
        return events;
    }

    if (now.seconds != last_check.seconds) {
        events |= RTC_EVENT_EVERY_SECOND;
    }

    if (now.minutes != last_check.minutes) {
        events |= RTC_EVENT_EVERY_MINUTE;
    }

    if (now.hours != last_check.hours) {
        events |= RTC_EVENT_EVERY_HOUR;
    }

    uint8_t end_of_day_hour = walker_info_cache.flags >> 3;
    if (now.days != last_check.days && now.hours == end_of_day_hour) {
        events |= RTC_EVENT_EVERY_DAY;
    }

    last_check = now;

    return events;
}
