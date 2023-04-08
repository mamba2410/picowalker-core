#ifndef PW_APP_BATTLE_H
#define PW_APP_BATTLE_H

#include <stdint.h>
#include <stddef.h>

#include "../states.h"

enum battle_state {
    N_BATTLE_STATES,
};

void pw_battle_init(state_vars_t *sv);
void pw_battle_init_display(state_vars_t *sv);
void pw_battle_update_display(state_vars_t *sv);
void pw_battle_handle_input(state_vars_t *sv, uint8_t b);
void pw_battle_event_loop(state_vars_t *sv);

#endif /* PW_APP_BATTLE_H */