#ifndef PICOWALKER_CORE_H
#define PICOWALKER_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "picowalker_structures.h"

/**
 * @file picowalker_core.h
 *
 * Collection of functions that the drivers can call to feed the core.
 * Mostly just callbacks, but some are utility functions as well
 *
 */


/**
 * Driver module should call this function when a button interrupt fires
 */
extern void pw_button_callback(pw_buttons_t b);

#endif /* PICOWALKER_DRIVERS_H */

