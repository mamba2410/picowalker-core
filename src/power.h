#ifndef PW_POWER_H
#define PW_POWER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 10 seconds for debug testing
#define PW_POWER_SLEEP_TIMEOUT_MS   (10000)
#define PW_POWER_LOW_BATTERY_PERCENT (10)

/**
 *  Power context of the Pokewalker.
 */
typedef struct pw_power_context_s {
    uint8_t  battery_percent;
    uint64_t last_user_action_time;
} pw_power_context_t;

extern volatile pw_power_context_t power_context;

#endif /* PW_POWER_H */

