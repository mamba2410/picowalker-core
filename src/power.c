#include "power.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "debug_log.h"
#include "picowalker_core.h"
#include "picowalker_structures.h"
#include "timer.h"

volatile pw_power_context_t power_context = {0};

void pw_power_update() {
    if (pw_time_get_ms() - power_context.last_bat_check > PW_POWER_BATTERY_CHECK_INTERVAL) {
        pw_power_start_measurement();
        power_context.last_bat_check = pw_time_get_ms();
    }

    // Read measurement/flags
    pw_power_status_t bs = pw_power_get_status();

    if (bs.flags & PW_POWER_STATUS_FLAGS_FAULT) {
        pw_log_error("Battery faulted, shutting down\n");
        pw_battery_shutdown();
        return;  // Shouldn't get here
    }

    if (bs.flags & PW_POWER_STATUS_FLAGS_CHARGING) {
        // pw_log_info("Charging\n");
    }

    if (bs.flags & PW_POWER_STATUS_FLAGS_CHARGE_ENDED) {
        // pw_log_info("Discharging\n");
    }

    if (bs.flags & PW_POWER_STATUS_FLAGS_PLUGGED) {
        pw_log_info("Plugged in\n");
    }

    if (bs.flags & PW_POWER_STATUS_FLAGS_UNPLUGGED) {
        pw_log_info("Unplugged\n");
    }

    if (bs.flags & PW_POWER_STATUS_FLAGS_CHARGING) {
        power_context.show_battery_low_icon = false;
        power_context.show_battery_charging_icon = true;
    } else if (bs.flags & PW_POWER_STATUS_FLAGS_CHARGE_ENDED) {
        power_context.show_battery_charging_icon = false;
    }

    if (bs.flags & PW_POWER_STATUS_FLAGS_TIMEOUT) {
        pw_log_warn("Battery measurement timed out\n");
        // Battery percent invalid, so we skip it
    }

    if (!(bs.flags & PW_POWER_STATUS_FLAGS_MEASUREMENT)) {
        return;
    }

    // If we got here, we have power measurements

    power_context.battery_percent = bs.percent;

    if ((bs.percent < PW_POWER_CRITICAL_THRESHOLD) && !(bs.flags & PW_POWER_STATUS_FLAGS_CHARGING)) {
        pw_log_error("Battery is too low, shutting down\n");
        pw_battery_shutdown();
    }

    if (bs.percent < PW_POWER_LOW_THRESHOLD) {
        power_context.show_battery_low_icon = true;
    }
}

int pw_power_get_mode() {
    return (pw_current_loop == pw_normal_loop) ? 0 : 1;
}

uint8_t pw_power_get_battery() {
    return power_context.battery_percent;
}
