#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>

#include "power.h"
#include "picowalker-defs.h"

volatile pw_power_context_t power_context = { };

void walker_loop();
extern void (*current_loop)(void);

void pw_power_update() {

    if(pw_time_get_ms() - power_context.last_bat_check > PW_POWER_BATTERY_CHECK_INTERVAL) {
        pw_power_start_measurement();
        power_context.last_bat_check = pw_time_get_ms();
    }

    if(!pw_power_result_available()) {
        // Nothing to do, leave early
        return;
    }

    // Read measurement/flags
    pw_power_status_t bs = pw_power_get_status();

    if(bs.flags & PW_POWER_STATUS_FLAGS_FAULT) {
        printf("[Error] Battery faulted, shutting down\n");
        pw_battery_shutdown();
        return; // Shouldn't get here
    }

    if(bs.flags & PW_POWER_STATUS_FLAGS_CHARGING) {
        power_context.show_battery_low_icon = false;
        power_context.show_battery_charging_icon = true;
    } else {
        power_context.show_battery_charging_icon = false;
    }

    if(bs.flags & PW_POWER_STATUS_FLAGS_TIMEOUT) {
        printf("[Warn ] Battery measurement timed out\n");
        // Battery percent invalid, so we skip it
        return;
    }

    if( (bs.percent < PW_POWER_CRITICAL_THRESHOLD) && !(bs.flags & PW_POWER_STATUS_FLAGS_CHARGING) ) {
        printf("[Error] Battery is too low, shutting down\n");
        pw_battery_shutdown();
    }

    if(bs.percent < PW_POWER_LOW_THRESHOLD) {
        power_context.show_battery_low_icon = true;
    }

}


int pw_power_get_mode() {
    return (current_loop == walker_loop)? 0: 1;
}

