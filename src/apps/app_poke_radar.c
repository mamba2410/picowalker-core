#include <stdint.h>
#include <stddef.h>

#include "app_poke_radar.h"

#include "../states.h"
#include "../types.h"
#include "../globals.h"
#include "../screen.h"
#include "../utils.h"
#include "../eeprom.h"
#include "../eeprom_map.h"
#include "../rand.h"
#include "../buttons.h"

/** @file app_poke_radar.c
 * Radar find pokemon game
 *
 */

static uint8_t radar_level_to_index[4] = {0, 0, 1, 2};
static screen_pos_t bush_xs[4] = {8, 16, 56, 64};
static screen_pos_t bush_ys[4] = {0, 24, 0, 24};
static uint8_t invisible_timer_divisors[4] = {1, 1, 2, 3}; // idk
static uint8_t active_timers[4] = {20, 15, 10, 7};


/**
 * Redraw cursor and clear other cursor areas
 *
 * @param radar Pointer to current radar struct.
 * @param sf Pointer to current screen flags.
 *
 */
static void draw_cursor_update(app_radar_t *radar, const screen_flags_t *sf) {
    pw_screen_draw_from_eeprom(
        bush_xs[radar->user_cursor]-8, bush_ys[radar->user_cursor]+8,
        8, 8,
        (sf->frame&ANIM_FRAME_NORMAL_TIME)? PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL:PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET,
        PW_EEPROM_SIZE_IMG_ARROW
    );

    for(uint8_t i = 0; i < 4; i++) {
        if(i == radar->user_cursor) continue;
        pw_screen_clear_area(bush_xs[i]-8, bush_ys[i]+8, 8, 8);
    }
}

/**
 * Initialise the radar struct.
 *
 * @param s Pointer to current state, always interpreted as `app_radar_t`.
 * @param sf Pointer to current screen flags.
 *
 */
void pw_poke_radar_init(pw_state_t *s, const screen_flags_t *sf) {
    route_info_t ri;
    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t*)&ri, sizeof(ri));

    pw_poke_radar_choose_pokemon(&(s->radar), &ri, &health_data_cache);

    s->radar.user_cursor = 0;
    s->radar.active_bush = pw_rand()%4;
    s->radar.current_substate = RADAR_CHOOSING;
    s->radar.previous_substate = RADAR_CHOOSING;
    s->radar.current_level = 0;
    s->radar.invisible_timer = 2+(pw_rand()%10)/invisible_timer_divisors[s->radar.current_level];
    s->radar.active_timer = active_timers[s->radar.current_level];
    s->radar.input_accepted = false;
}

/**
 * Initialise the screen for radar app.
 *
 * @param s Pointer to current state, always interpreted as `app_radar_t`.
 * @param sf Pointer to current screen flags.
 *
 */
void pw_poke_radar_init_display(pw_state_t *s, const screen_flags_t *sf) {
    switch(s->radar.current_substate) {
    case RADAR_CHOOSING: {
        pw_img_t bush = {.width=32, .height=24, .data=eeprom_buf, .size=192};
        pw_eeprom_read(PW_EEPROM_ADDR_IMG_RADAR_BUSH, eeprom_buf, PW_EEPROM_SIZE_IMG_RADAR_BUSH);

        for(uint8_t i = 0; i < 4; i++)
            pw_screen_draw_img(&bush, bush_xs[i], bush_ys[i]);

        pw_screen_draw_message(SCREEN_HEIGHT-16, 28, 16); // "find a pokemon!"

        break;
    }
    case RADAR_BUSH_OK: {
        pw_screen_draw_from_eeprom(
            bush_xs[s->radar.active_bush]+16, bush_ys[s->radar.active_bush],
            16, 16,
            PW_EEPROM_ADDR_IMG_RADAR_CLICK,
            PW_EEPROM_SIZE_IMG_RADAR_CLICK
        );
        break;
    }
    case RADAR_FAILED: {
        pw_screen_draw_message(SCREEN_HEIGHT-16, 30, 16); // "it got away"
        break;
    }
    case RADAR_START_BATTLE: {
        break;
    }
    }
}

/**
 * Update the screen for radar app.
 *
 * @param s Pointer to current state, always interpreted as `app_radar_t`.
 * @param sf Pointer to current screen flags.
 *
 */
void pw_poke_radar_update_display(pw_state_t *s, const screen_flags_t *sf) {
    if(s->radar.current_substate != s->radar.previous_substate) {
        s->radar.previous_substate = s->radar.current_substate;
        pw_poke_radar_init_display(s, sf);
        return;
    }


    switch(s->radar.current_substate) {
    case RADAR_CHOOSING: {
        draw_cursor_update(s, sf);

        if(s->radar.invisible_timer > 0) {
            s->radar.invisible_timer--;
            break;
        }

        pw_screen_draw_from_eeprom(
            bush_xs[s->radar.active_bush]+16, bush_ys[s->radar.active_bush],
            16, 16,
            PW_EEPROM_ADDR_IMG_RADAR_BUBBLE_ONE + radar_level_to_index[s->radar.current_level]*PW_EEPROM_SIZE_IMG_RADAR_BUBBLE_ONE,
            PW_EEPROM_SIZE_IMG_RADAR_BUBBLE_ONE
        );

        if(s->radar.active_timer > 0) {
            s->radar.active_timer--;
            break;
        }

        break;
    }
    case RADAR_BUSH_OK: {
        draw_cursor_update(s, sf);
        if(s->radar.invisible_timer > 0)
            s->radar.invisible_timer--;
        break;
    }
    case RADAR_FAILED: {
        draw_cursor_update(s, sf);
        break;
    }
    case RADAR_START_BATTLE: {
        pw_screen_fill_area(0, s->radar.begin_timer*8, SCREEN_WIDTH, 8, SCREEN_BLACK);
        pw_screen_fill_area(0, SCREEN_HEIGHT-(s->radar.begin_timer+1)*8, SCREEN_WIDTH, 8, SCREEN_BLACK);
        s->radar.begin_timer++;
        break;
    }
    }
}

/**
 * Handle input for the poke radar app.
 *
 * @param s Pointer to current state, always interpreted as `app_radar_t`.
 * @param sf Pointer to current screen flags.
 * @param b Button input.
 *
 */
void pw_poke_radar_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(s->radar.current_substate) {
    case RADAR_CHOOSING: {
        switch(b) {
        case BUTTON_L: {
            s->radar.user_cursor = (s->radar.user_cursor-1+4)%4; // in mod 4, +3 == -1
            break;
        }
        case BUTTON_R: {
            s->radar.user_cursor = (s->radar.user_cursor+1)%4;
            break;
        }
        case BUTTON_M: {
            if(s->radar.user_cursor == s->radar.active_bush) {
                s->radar.current_substate = RADAR_BUSH_OK;
                s->radar.invisible_timer = s->radar.active_timer = 3;
            } else {
                s->radar.current_substate = RADAR_FAILED;
            }
            break;
        }
        }
        break;
    }
    case RADAR_BUSH_OK: {
        break;
    }
    case RADAR_FAILED: {
        s->radar.input_accepted = true;
        break;
    }
    }
}

/**
 * Main event loop for poke radar app.
 *
 * @param s Pointer to current state, always interpreted as `app_radar_t`.
 * @param p Pointer to pending state. If `p->sid` is modified, then program switches to this state on next loop.
 * @param sf Pointer to current screen flags.
 *
 */
void pw_poke_radar_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {

    switch(s->radar.current_substate) {
    case RADAR_CHOOSING: {
        if(s->radar.invisible_timer <= 0 && s->radar.active_timer <= 0) {
            s->radar.current_substate = RADAR_FAILED;
            PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
        }
        break;
    }
    case RADAR_BUSH_OK: {
        if(s->radar.invisible_timer <= 0) {

            if(s->radar.current_level >= s->radar.radar_level) {
                //  move to battle state
                s->radar.invisible_timer = 0;
                s->radar.current_substate = RADAR_START_BATTLE;
                break;
            }

            s->radar.active_bush = pw_rand()%4;
            s->radar.current_level++;                // inc exclamation level
            s->radar.current_substate = RADAR_CHOOSING;
            s->radar.invisible_timer = 2+(pw_rand()%10)/invisible_timer_divisors[s->radar.current_level];
            s->radar.active_timer = active_timers[s->radar.current_level];
            PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
        }
        break;
    }
    case RADAR_FAILED: {
        if(s->radar.input_accepted) {
            p->sid = STATE_SPLASH;
        }
        break;
    }
    case RADAR_START_BATTLE: {
        if(s->radar.begin_timer >= 4) {
            p->sid = STATE_BATTLE;
            p->battle.chosen_pokemon = s->radar.chosen_pokemon;
        }
        break;
    }
    }

}


/**
 * Choose the pokemon to be encountered based on steps.
 *
 * @param radar Pointer to current `app_radar_t` struct.
 * @param ri Pointer to initialised `route_info_t` struct.
 * @param hd Pointer to current `health_data_t` struct.
 *
 */
void pw_poke_radar_choose_pokemon(app_radar_t *radar, route_info_t *ri, health_data_t *hd) {
    uint32_t today_steps =hd->today_steps;

    special_inventory_t inv;
    pw_eeprom_read(PW_EEPROM_ADDR_RECEIVED_BITFIELD, (uint8_t*)&inv, 1);

    int8_t event_index;
    pw_eeprom_read(PW_EEPROM_ADDR_SPECIAL_POKEMON_EVENT_INDEX, (uint8_t*)(&event_index), 1);

    //pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_POKEMON, (uint8_t*)(ri->route_pokemon), PW_EEPROM_SIZE_ROUTE_POKEMON);

    bool valid_event = event_index > 0;
    if( valid_event && !inv.event_pokemon ) {
        // event
        pokemon_summary_t event_pokemon;
        pw_eeprom_read(
            PW_EEPROM_ADDR_SPECIAL_POKEMON_BASIC_DATA,
            (uint8_t*)&event_pokemon,
            sizeof(event_pokemon)
        );

        struct {
            uint16_t le_steps;
            uint8_t chance;
        } special;
        pw_eeprom_read(PW_EEPROM_ADDR_SPECIAL_POKEMON_STEPS_REQUIRED, (uint8_t*)&special, sizeof(special));

        if(today_steps > special.le_steps) {
            if( pw_rand()%100 < special.chance ) {
                uint32_t rnd = pw_rand();
                radar->chosen_pokemon = OPTION_EVENT;   // found event pokemon
                radar->radar_level = 2+rnd%2;        // radar level = 2 or 3
                return;
            }
        }
    } else {
        // not event

        uint8_t rnd = pw_rand()%100;

        for(uint8_t i = 0; i < 3; i++) {
            if(today_steps < ri->le_route_pokemon_steps[i]) continue;
            if(rnd > ri->route_pokemon_percent[i]) continue;

            radar->chosen_pokemon = i;                  // found pokemon index
            radar->radar_level = 2-i+pw_rand()%2;    // set radar level based on rarity
            return;
        }

    }

    // unreachable
    radar->chosen_pokemon = OPTION_C;
    radar->radar_level = pw_rand()%2;

}

