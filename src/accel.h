#ifndef PW_ACCEL_H
#define PW_ACCEL_H

#include <stdint.h>

#define ACCEL_NORMAL_SAMPLE_TIME_US     30000000 // 30s
#define TOTAL_STEPS_MAX                 9999999
#define TODAY_STEPS_MAX                 99999
#define CURRENT_WATTS_MAX               9999

void pw_accel_process_steps();

/*
 * Functions defined in driver module
 */

extern void pw_accel_init();
extern uint32_t pw_accel_get_new_steps();
extern void pw_accel_sleep();
extern void pw_accel_wake();

#endif /* PW_ACCEL_H */

