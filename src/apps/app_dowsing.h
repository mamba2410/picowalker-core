#ifndef PW_APP_DOWSING_H
#define PW_APP_DOWSING_H

#include <stdint.h>

#include "../states.h"

/// @file app_dowsing.h

enum dowsing_state {
    DOWSING_ENTRY,
    DOWSING_CHOOSING,
    DOWSING_SELECTED,
    DOWSING_INTERMEDIATE,
    DOWSING_CHECK_GUESS,
    DOWSING_GIVE_ITEM,
    DOWSING_REPLACE_ITEM,
    DOWSING_QUITTING,
    DOWSING_AWAIT_INPUT,
    DOWSING_REVEAL_ITEM,
    N_DOWSING_STATES
};

void pw_dowsing_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_dowsing_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_dowsing_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);
void pw_dowsing_init(pw_state_t *s, const screen_flags_t *sf);
void pw_dowsing_init_display(pw_state_t *s, const screen_flags_t *sf);

#endif /* PW_APP_DOWSING_H */
