#ifndef PW_ACCEL_H
#define PW_ACCEL_H

#include <stdint.h>

#define ACCEL_NORMAL_SAMPLE_TIME_US     5000000 // 5s
#define TODAY_STEPS_MAX                 99999
#define CURRENT_WATTS_MAX               9999

void pw_accel_process_steps();

// implemented by drivers
extern int8_t pw_accel_init();
extern uint32_t pw_accel_get_new_steps();

#endif /* PW_ACCEL_H */
