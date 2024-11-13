#ifndef PW_BUTTONS_H
#define PW_BUTTONS_H

#include <stdint.h>

/// @file buttons.h

extern void pw_button_init();

void pw_button_callback(uint8_t b);

#endif /* PW_BUTTONS_H */
