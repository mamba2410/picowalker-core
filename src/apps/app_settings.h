#ifndef PW_APP_SETTINGS_H
#define PW_APP_SETTINGS_H

#include <stdint.h>

#include "../states.h"


/// @file app_splash.h

void pw_settings_init(pw_state_t *s, const screen_flags_t *sf);
void pw_settings_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_settings_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_settings_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_settings_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);

#endif /* PW_APP_SETTINGS_H */
