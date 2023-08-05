#ifndef PW_APP_SPLASH_H
#define PW_APP_SPLASH_H
#include "../states.h"
#include <stdint.h>

/// @file app_splash.h

void pw_splash_init(pw_state_t *s, const screen_flags_t *sf);
void pw_splash_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_splash_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_splash_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_splash_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);

#endif /* PW_APP_SPLASH_H */
