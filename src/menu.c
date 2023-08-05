
#include <stdio.h>
#include <stdint.h>

#include "menu.h"
#include "states.h"
#include "buttons.h"
#include "screen.h"
#include "utils.h"
#include "types.h"
#include "eeprom_map.h"
#include "eeprom.h"
#include "globals.h"

static pw_state_id_t const MENU_ENTRIES[] = {
    STATE_POKE_RADAR,
    STATE_DOWSING,
    STATE_COMMS,
    STATE_TRAINER_CARD,
    STATE_INVENTORY,
    STATE_SETTINGS,
};
const int8_t MENU_SIZE = sizeof(MENU_ENTRIES)/sizeof(pw_state_id_t);

static uint16_t const MENU_TITLES[] = {
    PW_EEPROM_ADDR_IMG_MENU_TITLE_POKERADAR,
    PW_EEPROM_ADDR_IMG_MENU_TITLE_DOWSING,
    PW_EEPROM_ADDR_IMG_MENU_TITLE_CONNECT,
    PW_EEPROM_ADDR_IMG_MENU_TITLE_TRAINER_CARD,
    PW_EEPROM_ADDR_IMG_MENU_TITLE_INVENTORY,
    PW_EEPROM_ADDR_IMG_MENU_TITLE_SETTINGS,
};

static uint16_t const MENU_ICONS[] = {
    PW_EEPROM_ADDR_IMG_MENU_ICON_POKERADAR,
    PW_EEPROM_ADDR_IMG_MENU_ICON_DOWSING,
    PW_EEPROM_ADDR_IMG_MENU_ICON_CONNECT,
    PW_EEPROM_ADDR_IMG_MENU_ICON_TRAINER_CARD,
    PW_EEPROM_ADDR_IMG_MENU_ICON_INVENTORY,
    PW_EEPROM_ADDR_IMG_MENU_ICON_SETTINGS,
};

enum {
    MS_NORMAL,
    MS_CLICKED,
    MS_SPLASH,
};


// + = right
// - = left
// true = send to splash
bool pw_menu_move_cursor(pw_state_t *s, int8_t move) {
    s->menu.cursor += move;

    if( s->menu.cursor < 0 || s->menu.cursor >= MENU_SIZE ) {
        s->menu.cursor = 0;
        s->menu.substate = MS_SPLASH;
        return true;
    }

    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
    return false;
}

void pw_menu_init(pw_state_t *s, const screen_flags_t *sf) {
    s->menu.message = MSG_NONE;
}

void pw_menu_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    switch(s->menu.substate) {

    case MS_NORMAL: {
        break;    // nothing to do
    }
    case MS_CLICKED: {

        if(MENU_ENTRIES[s->menu.cursor] == STATE_INVENTORY) {
            pw_brief_inventory_t inv;
            pw_detailed_inventory_t _detailed;
            pw_read_inventory(&inv, &_detailed);

            // no pokemon or items
            if(inv.caught_pokemon == 0 && inv.dowsed_items == 0) {
                s->menu.message = MSG_NOTHING_HELD;
                return;
            }
        }

        p->sid = MENU_ENTRIES[s->menu.cursor];
        break;
    }
    case MS_SPLASH: {
        p->sid = STATE_SPLASH;
    }
    }
}

void pw_menu_init_display(pw_state_t *s, const screen_flags_t *sf) {

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        MENU_TITLES[s->menu.cursor],
        PW_EEPROM_SIZE_IMG_MENU_TITLE_CONNECT
    );
    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_LEFT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_LEFT
    );
    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-8, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RIGHT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RIGHT
    );

    size_t y_values[] = {24, 26, 28, 30, 26, 24};
    for(size_t i = 0; i < MENU_SIZE; i++) {
        pw_screen_draw_from_eeprom(
            i*16, y_values[i],
            16, 16,
            MENU_ICONS[i],
            PW_EEPROM_SIZE_IMG_MENU_ICON_CONNECT
        );

        if(s->menu.cursor == i) {
            pw_screen_draw_from_eeprom(
                4+i*16, y_values[i]-8,
                8, 8,
                PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL,
                PW_EEPROM_SIZE_IMG_ARROW
            );
        } else {
            pw_screen_clear_area(4+i*16, y_values[i]-8, 8, 8);
        }
    }

    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-16, SCREEN_HEIGHT-16,
        16, 16,
        PW_EEPROM_ADDR_IMG_WATTS,
        PW_EEPROM_SIZE_IMG_WATTS
    );
    pw_screen_draw_integer(health_data_cache.current_watts, SCREEN_WIDTH-16, SCREEN_HEIGHT-16);

}

void pw_menu_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {

    // if there's a message, draw it
    if(s->menu.message != MSG_NONE) {
        pw_screen_clear_area(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16);
        s->menu.message = MSG_NONE;
        PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
        return;
    }

    switch(b) {
    case BUTTON_L: {
        pw_menu_move_cursor(s, -1);
        break;
    };
    case BUTTON_M: {
        s->menu.substate = MS_CLICKED;
        break;
    };
    case BUTTON_R: {
        pw_menu_move_cursor(s, +1);
        break;
    };
    default:
        break;
    }

}

void pw_menu_update_display(pw_state_t *s, const screen_flags_t *sf) {

    // quick way to redraw if we were just displaying a message
    if(s->menu.redraw_message) {
        pw_menu_init_display(s, sf);
        s->menu.redraw_message = false;
    }


    /*
     *  Redraw title, arrows
     */
    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        MENU_TITLES[s->menu.cursor],
        PW_EEPROM_SIZE_IMG_MENU_TITLE_CONNECT
    );

    size_t y_values[] = {24, 26, 28, 30, 26, 24};
    for(size_t i = 0; i < MENU_SIZE; i++) {
        if(s->menu.cursor == i) {
            eeprom_addr_t addr = (sf->frame&ANIM_FRAME_NORMAL_TIME)?PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL:PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;
            pw_screen_draw_from_eeprom(
                4+i*16, y_values[i]-8,
                8, 8,
                addr,
                PW_EEPROM_SIZE_IMG_ARROW
            );

        } else {
            pw_screen_clear_area(4+i*16, y_values[i]-8, 8, 8);
        }
    }

    // TODO: Move out of here and only draw once
    // draw menu message if we have one
    if(s->menu.message != MSG_NONE) {
        pw_screen_draw_from_eeprom(
            0, SCREEN_HEIGHT-16,
            SCREEN_WIDTH, 16,
            // TODO: change this to MENU_MESSAGES
            PW_EEPROM_ADDR_TEXT_NEED_WATTS + PW_EEPROM_SIZE_TEXT_NEED_WATTS*(s->menu.message-1),
            PW_EEPROM_SIZE_TEXT_NEED_WATTS
        );
        pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, SCREEN_BLACK);
    }

}



