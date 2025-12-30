#ifndef PICOWALKER_STRUCTURES_H
#define PICOWALKER_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file picowalker_structures.h
 *
 * Common data structures to share between the core and drivers.
 * Drivers treat this as an API and build driver modules to this spec.
 *
 */

/*
 *  ==================================================================================
 *  SCREEN
 *  ==================================================================================
 */

#define PW_SCREEN_WIDTH    96
#define PW_SCREEN_HEIGHT   64

/**
 * Position of something on screen
 * Can be negative, because images can be off screen on the left.
 *
 * Valid range: -PW_SCREEN_WIDTH to PW_SCREEN_WIDTH-1
 */
typedef int8_t pw_screen_pos_t;


/**
 * An image data structure for pokewalker images
 * Will change when colour images are implemented
 */
typedef struct pw_img_s {
    uint8_t *data;
    size_t size;
    screen_pos_t height, width;
} pw_img_t;


/**
 * Relates to the original pokewalker image.
 * Give names to the pixel values.
 * Since the original screen is an LCD, a value of 0 is white.
 */
typedef enum pw_screen_color_e {
    PW_SCREEN_WHITE = 0,
    PW_SCREEN_LGREY = 1,
    PW_SCREEN_DGREY = 2,
    PW_SCREEN_BLACK = 3,
} pw_screen_color_t;


/**
 * There are ten brightness pips
 * Value ranges from 0 to 9
 */
#define PW_SCREEN_MAX_BRIGHTNESS 9


/*
 *  ==================================================================================
 *  EEPROM
 *  ==================================================================================
 */

/*
 * Types and defines
 */
typedef uint16_t eeprom_addr_t;

/*
 *  Functions defined by the driver
 */
void pw_eeprom_init();
int pw_eeprom_read(eeprom_addr_t addr, uint8_t *buf, size_t len);
int pw_eeprom_write(eeprom_addr_t addr, uint8_t *buf, size_t len);
void pw_eeprom_set_area(eeprom_addr_t addr, uint8_t v, size_t len);
void pw_eeprom_sleep();
void pw_eeprom_wake();


/*
 *  ==================================================================================
 *  FLASH
 *  ==================================================================================
 */

/*
 * Types and defines
 */
typedef enum {
    FLASH_IMG_POKEWALKER,
    FLASH_IMG_FACE_NEUTRAL,
    FLASH_IMG_FACE_HAPPY,
    FLASH_IMG_FACE_SAD,
    FLASH_IMG_UP_ARROW,
    FLASH_IMG_IR_ACTIVE,
    FLASH_IMG_TINY_CHARS,
} pw_flash_img_t;

/*
 *  Functions defined by the driver
 */
void pw_flash_sleep();
void pw_flash_wake();

/*
 *  ==================================================================================
 *  BUTTONS
 *  ==================================================================================
 */

/*
 * Types and defines
 */
enum {
    BUTTON_L = 0x01,
    BUTTON_M = 0x02,
    BUTTON_R = 0x04,
};

#define DEBOUNCE_TIME_US    100000   // 100ms

/*
 *  Functions defined by the driver
 */
void pw_button_init();

/*
 *  ==================================================================================
 *  IR
 *  ==================================================================================
 */

/*
 * Types and defines
 */
#define MAX_PACKET_SIZE (128+8)

#define PW_IR_READ_TIMEOUT_MS   200u
#define PW_IR_READ_TIMEOUT_US   (PW_IR_READ_TIMEOUT_MS*1000)
#define PW_IR_READ_TIMEOUT_DS   (PW_IR_READ_TIMEOUT_MS/100)

/*
 *  Functions defined by the driver
 */
void pw_ir_init();
int pw_ir_read(uint8_t *buf, size_t len);
int pw_ir_write(uint8_t *buf, size_t len);
void pw_ir_sleep();
void pw_ir_wake();

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
#define PW_POWER_STATUS_FLAGS_CHARGING    (1<<0)
#define PW_POWER_STATUS_FLAGS_FAULT       (1<<1)
#define PW_POWER_STATUS_FLAGS_TIMEOUT     (1<<2)
#define PW_POWER_STATUS_FLAGS_MEASUREMENT (1<<3)
#define PW_POWER_STATUS_FLAGS_CHARGE_ENDED    (1<<4)
#define PW_POWER_STATUS_FLAGS_PLUGGED     (1<<5)
#define PW_POWER_STATUS_FLAGS_UNPLUGGED   (1<<6)
#define PW_POWER_LOW_THRESHOLD            (20)
#define PW_POWER_CRITICAL_THRESHOLD       (10)

typedef struct pw_power_status_s {
    uint8_t  percent;
    uint8_t flags;
} pw_power_status_t;

#define PW_WAKE_REASON_RTC      (1<<0)
#define PW_WAKE_REASON_BATTERY  (1<<1)
#define PW_WAKE_REASON_BUTTON   (1<<2)
#define PW_WAKE_REASON_ACCEL    (1<<3)

typedef uint8_t pw_wake_reason_t;

/*
 *  Functions defined by driver
 */
void pw_power_init();
pw_power_status_t pw_power_get_status();
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

#endif /* PW_PICOWALKER_INCLUDE_H */

