#ifndef PW_APP_BATTLE_H
#define PW_APP_BATTLE_H

#include <stdint.h>
#include <stddef.h>

#include "../states.h"

/// @file app_battle.h

enum battle_state {
    BATTLE_OPENING,
    BATTLE_APPEARED,
    BATTLE_CHOOSING,
    BATTLE_OUR_ACTION,
    BATTLE_THEIR_ACTION,
    BATTLE_WE_LOST,
    BATTLE_THEY_FLED,
    BATTLE_STAREDOWN,
    BATTLE_CATCH_SETUP,
    BATTLE_THREW_BALL,
    BATTLE_CLOUD_ANIM,
    BATTLE_BALL_WOBBLE,
    BATTLE_ALMOST_HAD_IT,
    BATTLE_CATCH_STARS,
    BATTLE_POKEMON_CAUGHT,
    BATTLE_SWITCH,
    BATTLE_PROCESS_CAUGHT_POKEMON,
    BATTLE_GO_TO_SPLASH,
    N_BATTLE_STATES,
};

void pw_battle_init(pw_state_t *s, const screen_flags_t *sf);
void pw_battle_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_battle_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_battle_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_battle_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);

#endif /* PW_APP_BATTLE_H */
