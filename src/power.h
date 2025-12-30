#ifndef PW_POWER_H
#define PW_POWER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "picowalker_structures.h"

// 10 seconds for debug testing
//#define PW_POWER_SLEEP_TIMEOUT_MS   (10000)
#define PW_POWER_SLEEP_TIMEOUT_MS   (30000) // 30 s

#define PW_POWER_BATTERY_CHECK_INTERVAL (15000) // 15 s

/**
 *  Power context of the Pokewalker.
 */
typedef struct pw_power_context_s {
    uint8_t  battery_percent;
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

#endif /* PW_POWER_H */

