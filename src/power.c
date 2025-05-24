#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "power.h"
#include "picowalker-defs.h"

volatile pw_power_context_t power_context = { };

uint8_t pw_power_process_battery() {
    pw_battery_status_t bs = pw_power_get_battery_status();
    if(bs.flags & PW_BATTERY_STATUS_FLAGS_FAULT) {
        printf("[Error] Battery faulted, shutting down\n");
        pw_battery_shutdown();
    }

    if( (bs.percent < PW_BATTERY_CRITICAL_THRESHOLD) && !(bs.flags & PW_BATTERY_STATUS_FLAGS_CHARGING) ) {
        printf("[Error] Battery is too low, shutting down\n");
        pw_battery_shutdown();
    }

    // TODO: simplify logic

    if(bs.percent < PW_BATTERY_LOW_THRESHOLD) {
        power_context.show_battery_low_icon = true;
    }

    if(bs.flags & PW_BATTERY_STATUS_FLAGS_CHARGING) {
        power_context.show_battery_low_icon = false;
        power_context.show_battery_charging_icon = true;
    } else {
        power_context.show_battery_charging_icon = false;
    }

    return bs.percent;
}

