#ifndef PW_POWER_H
#define PW_POWER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "picowalker_structures.h"

// 10 seconds for debug testing
// #define PW_POWER_SLEEP_TIMEOUT_MS   (10000)
#define PW_POWER_SLEEP_TIMEOUT_MS (30000)  // 30 s

#define PW_POWER_BATTERY_CHECK_INTERVAL (15000)  // 15 s

/**
 *  Power context of the Pokewalker.
 */
typedef struct pw_power_context_s {
    uint8_t battery_percent;
    uint64_t last_user_action_time;
    uint64_t last_bat_check;
    bool show_battery_low_icon;
    bool show_battery_charging_icon;
} pw_power_context_t;

extern volatile pw_power_context_t power_context;

void pw_power_update();
uint8_t pw_power_get_battery();

/*
 * Functions defined in the driver module
 */

extern void pw_power_init();
extern void pw_power_start_measurement();
extern bool pw_power_result_available();
extern pw_power_status_t pw_power_get_status();
extern void pw_battery_shutdown();
extern bool pw_power_should_sleep();
extern pw_wake_reason_t pw_power_get_wake_reason();
extern void pw_power_clear_wake_reason(pw_wake_reason_t);
extern void pw_power_enter_sleep();
extern void pw_power_enter_light_sleep();
extern void pw_power_light_sleep_for(uint32_t ms);

#endif /* PW_POWER_H */
