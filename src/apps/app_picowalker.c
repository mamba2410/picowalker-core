#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>

#include "app_picowalker.h"
#include "../eeprom_map.h"
#include "../picowalker-defs.h"
#include "../power.h"
#include "../screen.h"
#include "../states.h"

#define N_ENTRIES 1

enum {
    SUBSTATE_NORMAL,
    SUBSTATE_GO_TO_SPLASH,
    SUBSTATE_GO_TO_SETTINGS,
};

void pw_picowalker_settings_init(pw_state_t *s, const screen_flags_t *sf) {
    s->picowalker.current_substate = SUBSTATE_NORMAL;
    s->picowalker.cursor = 0;
}


void pw_picowalker_settings_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    switch(s->picowalker.current_substate) {
        case SUBSTATE_NORMAL: {
            break;
        }
        case SUBSTATE_GO_TO_SPLASH: {
            p->sid = STATE_SPLASH;
            break;
        }
        case SUBSTATE_GO_TO_SETTINGS: {
            p->sid = STATE_SETTINGS;
            break;
        }
        default: {
            printf("[Error] Unknown substate %d in %s",
                    s->picowalker.current_substate,
                    state_strings[s->sid]);
            break;
        }
    }
}


void pw_picowalker_settings_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(b) {
        case BUTTON_L: {
            s->picowalker.cursor--;
            if(s->picowalker.cursor < 0) {
                s->picowalker.current_substate = SUBSTATE_GO_TO_SETTINGS;
            }
            break;
        }
        case BUTTON_M: {
            s->picowalker.current_substate = SUBSTATE_GO_TO_SPLASH;
            break;
        }
        case BUTTON_R: {
            if(s->picowalker.cursor < N_ENTRIES-1) {
                s->picowalker.cursor++;
            }
            break;
        }
        default: {
            printf("[Error] Unknown input %d in substate %s\n",
                    b, state_strings[s->sid]);
            break;
        }
    }
}


void pw_picowalker_settings_init_display(pw_state_t *s, const screen_flags_t *sf) {
    screen_pos_t x, y;
    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_IMG_MENU_TITLE_SETTINGS,
        PW_EEPROM_SIZE_IMG_MENU_TITLE_SETTINGS
    );
    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN
    );

    // Draw battery percentage
    y = 16;
    x = 16;
    pw_screen_draw_from_eeprom(
        x, y+4,
        8, 8,
        PW_EEPROM_ADDR_IMG_LOW_BATTERY,
        PW_EEPROM_SIZE_IMG_LOW_BATTERY
    );
    uint8_t percent = pw_power_get_battery();
    x = SCREEN_WIDTH-8;
    x = pw_screen_draw_integer(percent, x, y);
    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-8, y,
        8, 16,
        PW_EEPROM_ADDR_IMG_CHAR_SLASH,
        PW_EEPROM_SIZE_IMG_CHAR
    );
}


void pw_picowalker_settings_update_display(pw_state_t *s, const screen_flags_t *sf) {
    eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL:
                         PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
    pw_screen_draw_from_eeprom(
        0, 16+4+s->picowalker.cursor*16,
        8, 8,
        addr,
        PW_EEPROM_SIZE_IMG_ARROW
    );
    for(size_t i = 0; i < N_ENTRIES; i++) {
        if(i == s->picowalker.cursor) continue;
        pw_screen_clear_area(
            0, 16+i*16,
            8, 8
        );
    }
}

