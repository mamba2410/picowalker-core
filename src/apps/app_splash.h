#ifndef PW_APP_SPLASH_H
#define PW_APP_SPLASH_H
#include "../states.h"
#include <stdint.h>

/// @file app_splash.h

void pw_splash_init(pw_state_t *s, screen_flags_t *sf);
void pw_splash_handle_input(pw_state_t *s, screen_flags_t *sf, uint8_t b);
void pw_splash_init_display(pw_state_t *s, screen_flags_t *sf);
void pw_splash_update_display(pw_state_t *s, screen_flags_t *sf);

#endif /* PW_APP_SPLASH_H */
