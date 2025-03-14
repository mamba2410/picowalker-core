#ifndef PW_ACCEL_H
#define PW_ACCEL_H

#include <stdint.h>

#define ACCEL_NORMAL_SAMPLE_TIME_US     15000000 // 15s
#define TOTAL_STEPS_MAX                 9999999
#define TODAY_STEPS_MAX                 99999
#define CURRENT_WATTS_MAX               9999

void pw_accel_process_steps();

#endif /* PW_ACCEL_H */
