#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../states.h"

void pw_picowalker_settings_init(pw_state_t *s, const screen_flags_t *sf);
void pw_picowalker_settings_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);
void pw_picowalker_settings_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b);
void pw_picowalker_settings_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_picowalker_settings_update_display(pw_state_t *s, const screen_flags_t *sf);

