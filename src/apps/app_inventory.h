#ifndef PW_APP_INVENTORY_H
#define PW_APP_INVENTORY_H

#include <stdint.h>

#include "../states.h"

/// @file app_inventory.h

void pw_inventory_init(pw_state_t *s, const screen_flags_t *sf);
void pw_inventory_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_inventory_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_inventory_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_inventory_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);


#endif /* PW_APP_INVENTORY_H */
