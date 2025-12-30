#ifndef PW_EEPROM_H
#define PW_EEPROM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "picowalker_structures.h"
#include "types.h"

/// @file eeprom.h

/*
 *  Derivative functions, driver agnostic
 */
int pw_eeprom_reliable_read(pw_eeprom_addr_t addr1, pw_eeprom_addr_t addr2, uint8_t *buf, size_t len);
int pw_eeprom_reliable_write(pw_eeprom_addr_t addr1, pw_eeprom_addr_t addr2, uint8_t *buf, size_t len);
uint8_t pw_eeprom_checksum(uint8_t *buf, size_t len);
bool pw_eeprom_check_for_nintendo();
void pw_eeprom_reset(bool clear_events, bool clear_steps);
void pw_eeprom_initialise_health_data(bool clear_time);


void pw_eeprom_write_health_data(health_data_t *hd_orig);
int pw_eeprom_read_health_data(health_data_t *hd);
void pw_eeprom_write_walker_info(walker_info_t *wi_orig);
int pw_eeprom_read_walker_info(walker_info_t *wi);


/*
 *  Functions defined by the driver
 */
extern void pw_eeprom_init();
extern int pw_eeprom_read(pw_eeprom_addr_t addr, uint8_t *buf, size_t len);
extern int pw_eeprom_write(pw_eeprom_addr_t addr, const uint8_t *buf, size_t len);
extern void pw_eeprom_set_area(pw_eeprom_addr_t addr, uint8_t v, size_t len);
extern void pw_eeprom_sleep();
extern void pw_eeprom_wake();

#endif /* PW_EEPROM_H */
