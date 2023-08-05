#ifndef PW_APP_POKE_RADAR_H
#define PW_APP_POKE_RADAR_H

#include <stdint.h>

#include "../states.h"
#include "../types.h"

/// @file app_poke_radar.h

/*
 *  Copying what the walker does.#
 *  ABC is 1+index in eeprom
 *  event is just 4
 *  idk why they did this, since all they ever do is subtract 1 again
 *  but hey, i'll be consistent
 */
#define OPTION_A        0
#define OPTION_B        1
#define OPTION_C        2
#define OPTION_EVENT    3

typedef enum {
    RADAR_CHOOSING,
    RADAR_BUSH_OK,
    RADAR_FAILED,
    RADAR_START_BATTLE,
    N_RADAR_STATES,
} radar_state_id_t;

void pw_poke_radar_init(pw_state_t *s, const screen_flags_t *sf);
void pw_poke_radar_init_display(pw_state_t *s, const screen_flags_t *sf);
void pw_poke_radar_update_display(pw_state_t *s, const screen_flags_t *sf);
void pw_poke_radar_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b);
void pw_poke_radar_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf);

void pw_poke_radar_choose_pokemon(app_radar_t *radar, route_info_t *ri, health_data_t *hd);

#endif /* PW_APP_POKE_RADAR_H */
