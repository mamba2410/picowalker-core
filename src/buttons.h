#ifndef PW_BUTTONS_H
#define PW_BUTTONS_H

#include <stdint.h>

#include "picowalker_structures.h"

/**
 * @file buttons.h
 */

void pw_button_callback(pw_buttons_t b);

/*
 * Functions defined by the driver module
 */

extern void pw_button_init();

#endif /* PW_BUTTONS_H */
