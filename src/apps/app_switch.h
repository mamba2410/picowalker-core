#ifndef PW_APP_SWITCH_H
#define PW_APP_SWITCH_H

#include "../states.h"

enum {
    SWITCH_TYPE_ITEM,
    SWITCH_TYPE_POKEMON,
};

enum {
    SWITCHES_CHOOSING,
    SWITCHES_WRITE_INV,
    SWITCHES_TO_SPLASH,
};

void pw_switch_init(pw_state_t *s, const screen_flags_t *sf);
void pw_switch_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_switch_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_switch_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b);
void pw_switch_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);

#endif /* PW_APP_SWITCH_H */

