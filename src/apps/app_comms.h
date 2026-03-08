#ifndef PW_APP_COMMS_H
#define PW_APP_COMMS_H

#include <stdint.h>
#include <stddef.h>

#include "../states.h"
#include "../ir/actions.h"

/// @file app_comms.h

extern const char* const PW_COMM_SUBSTATE_NAMES[N_COMM_SUBSTATE];

void pw_comms_init(pw_state_t *s, const screen_flags_t *sf);
void pw_comms_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);
void pw_comms_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_comms_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b);
void pw_comms_draw_update(pw_state_t *s, const screen_flags_t *sf);
void pw_comms_deinit(pw_state_t *s, const screen_flags_t *sf);


#endif /* PW_APP_COMMS_H */
