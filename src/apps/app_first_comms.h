#ifndef PW_APP_FIRST_COMMS_H
#define PW_APP_FIRST_COMMS_H

#include <stdint.h>

#include "../states.h"

/// @file app_first_comms.h

enum {
    FC_SUBSTATE_WAITING,
    FC_SUBSTATE_CONNECTING,
    FC_SUBSTATE_TIMEOUT,
    FC_SUBSTATE_SUCCESS,
    N_FC_SUBSTATES,
};

void pw_first_comms_init(pw_state_t *s, const screen_flags_t *sf);
void pw_first_comms_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);
void pw_first_comms_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_first_comms_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_first_comms_draw_update(pw_state_t *s, const screen_flags_t *sf);

#endif /* PW_APP_FIRST_COMMS_H */
