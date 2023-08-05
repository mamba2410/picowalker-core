#ifndef PW_APP_TRAINER_CARD_H
#define PW_APP_TRAINER_CARD_H

#include <stdint.h>

#include "../states.h"

/// @file app_states.h

#define TRAINER_CARD_MAX_DAYS   7

enum {
    TC_NORMAL,
    TC_GO_TO_SPLASH,
    TC_GO_TO_MENU,
};

void pw_trainer_card_init(pw_state_t *s, const screen_flags_t *sf);
void pw_trainer_card_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_trainer_card_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_trainer_card_draw_update(pw_state_t *s, const screen_flags_t *sf);
void pw_trainer_card_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);


#endif /* PW_APP_TRAINER_CARD_H */
