#ifndef PICOWALKER_CORE_H
#define PICOWALKER_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "picowalker_structures.h"

/**
 * @file picowalker_core.h
 *
 */


/*
 *  ==================================================================================
 *  SCREEN
 *  ==================================================================================
 */


/*
 *  ==================================================================================
 *  EEPROM
 *  ==================================================================================
 */


/*
 *  ==================================================================================
 *  FLASH
 *  ==================================================================================
 */


/*
 *  ==================================================================================
 *  BUTTONS
 *  ==================================================================================
 */

/**
 * Driver module should call this function when a button interrupt fires
 */
extern void pw_button_callback(pw_buttons_t b);

/*
 *  ==================================================================================
 *  IR
 *  ==================================================================================
 */


/*
 *  ==================================================================================
 *  ACCEL
 *  ==================================================================================
 */

/*
 * Types and defines
 */

/*
 *  Functions defined by driver
 */
void pw_accel_init();
uint32_t pw_accel_get_new_steps();
void pw_accel_sleep();
void pw_accel_wake();

/*
 *  ==================================================================================
 *  POWER
 *  ==================================================================================
 */

/*
 * Types and defines
 */
#define PW_BATTERY_STATUS_FLAGS_CHARGING    (1<<0)
#define PW_BATTERY_STATUS_FLAGS_FAULT       (1<<1)
#define PW_BATTERY_LOW_THRESHOLD            (20)
#define PW_BATTERY_CRITICAL_THRESHOLD       (10)

typedef struct pw_battery_status_s {
    uint8_t  percent;
    uint8_t flags;
} pw_battery_status_t;

#define PW_WAKE_REASON_RTC      (1<<0)
#define PW_WAKE_REASON_BATTERY  (1<<1)
#define PW_WAKE_REASON_BUTTON   (1<<2)
#define PW_WAKE_REASON_ACCEL    (1<<3)

typedef uint8_t pw_wake_reason_t;

/*
 *  Functions defined by driver
 */
void pw_power_init();
pw_battery_status_t pw_power_get_battery_status();
void pw_power_enter_sleep();
bool pw_power_should_sleep();
pw_wake_reason_t pw_power_get_wake_reason();
void pw_battery_shutdown();


/*
 *  ==================================================================================
 *  TIME
 *  ==================================================================================
 */

/*
 * Types and defines
 */
#define RTC_EVENT_EVERY_DAY     (1<<0)
#define RTC_EVENT_EVERY_HOUR    (1<<1)
#define RTC_EVENT_EVERY_MINUTE  (1<<2)
#define RTC_EVENT_EVERY_SECOND  (1<<3)

typedef uint8_t pw_rtc_events_t;

typedef struct pw_dhms_s {
    uint16_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} pw_dhms_t;

/*
 *  Functions defined by driver
 */
void pw_time_init_rtc(uint32_t last_sync);   // From RTC
void pw_time_set_rtc(uint32_t last_sync);    // From RTC
uint32_t pw_time_get_rtc();     // From RTC
pw_dhms_t pw_time_get_dhms();   // From RTC
pw_rtc_events_t pw_time_get_rtc_events();
uint64_t pw_time_get_us();  // Since boot
uint64_t pw_time_get_ms();  // Since boot
void pw_time_delay_ms(uint32_t ms);
void pw_time_delay_us(uint32_t us);


#endif /* PICOWALKER_DRIVERS_H */

