#ifndef PW_MENU_H
#define PW_MENU_H

#include <stdbool.h>
#include <stdint.h>

#include "states.h"

/// @file menu.h

void pw_menu_init(pw_state_t *s, const screen_flags_t *sf);
void pw_menu_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_menu_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);
void pw_menu_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_menu_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);

extern const int8_t MENU_SIZE;

#endif /* PW_MENU_H */
