
#include <stdio.h>
#include <stdint.h>

#include "menu.h"
#include "states.h"
#include "buttons.h"
#include "screen.h"
#include "audio.h"
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

static uint8_t const MENU_COSTS[] = {
    10, 3, 0, 0, 0, 0
};

static size_t const CURSOR_Y_VALUES[] = {24, 26, 28, 30, 26, 24};

enum {
    MS_NORMAL,
    MS_CLICKED,
    MS_MESSAGE,
    MS_SPLASH,
};


// + = right
// - = left
// true = send to splash
void pw_menu_move_cursor(pw_state_t *s, int8_t move) {
    s->menu.cursor += move;

    if( s->menu.cursor < 0 || s->menu.cursor >= MENU_SIZE ) {
        s->menu.cursor = 0;
        s->menu.substate = MS_SPLASH;
        pw_audio_play_sound(SOUND_NAVIGATE_BACK);
        return;
    }

    pw_audio_play_sound(SOUND_CURSOR_MOVE);
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

void pw_menu_init(pw_state_t *s, const screen_flags_t *sf) {
    s->menu.message = MSG_NONE;
    s->menu.substate = MS_NORMAL;
}

void pw_menu_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    switch(s->menu.substate) {

    case MS_NORMAL: { break; }  // nothing to do
    case MS_MESSAGE: { break; } // nothing to do
    case MS_CLICKED: {

        if(MENU_ENTRIES[s->menu.cursor] == STATE_INVENTORY) {
            pw_brief_inventory_t inv;
            pw_detailed_inventory_t _detailed;
            pw_read_inventory(&inv, &_detailed);

            // no pokemon or items
            if(inv.caught_pokemon == 0 && inv.dowsed_items == 0) {
                s->menu.message = MSG_NOTHING_HELD;
                s->menu.substate = MS_MESSAGE;
                return;
            }
        }

        if(MENU_ENTRIES[s->menu.cursor] == STATE_DOWSING || MENU_ENTRIES[s->menu.cursor] == STATE_POKE_RADAR ) {
            route_info_t ri;
            pw_eeprom_read(
                PW_EEPROM_ADDR_ROUTE_INFO,
                (uint8_t*)&ri,
                PW_EEPROM_SIZE_ROUTE_INFO
            );

            if(ri.pokemon_summary.le_species == 0xffff || ri.pokemon_summary.le_species == 0x0000) {
                s->menu.message = MSG_NO_POKEMON_HELD;
                s->menu.substate = MS_MESSAGE;
                return;
            }
        }

        if(health_data_cache.current_watts < MENU_COSTS[s->menu.cursor]) {
            s->menu.message = MSG_NEED_WATTS;
            s->menu.substate = MS_MESSAGE;
            return;
        } else {
            health_data_cache.current_watts -= MENU_COSTS[s->menu.cursor];
        }

        p->sid = MENU_ENTRIES[s->menu.cursor];
        break;
    }
    case MS_SPLASH: {
        p->sid = STATE_SPLASH;
        break;
    }
    }
}


void pw_menu_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {

    if(s->menu.substate == MS_MESSAGE) {
        s->menu.substate = MS_NORMAL;
    } else {
        switch(b) {
            case BUTTON_L: {
                pw_menu_move_cursor(s, -1);
                break;
            };
            case BUTTON_M: {
                s->menu.substate = MS_CLICKED;
                pw_audio_play_sound(SOUND_NAVIGATE_MENU);
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

    // Cursor has moved, we want a redraw of cursor and watts
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

static void menu_draw_bottom_numbers(pw_state_t *s, const screen_flags_t *sf) {
    if(MENU_COSTS[s->menu.cursor] != 0) {
        size_t x = pw_screen_draw_integer(MENU_COSTS[s->menu.cursor], 16, SCREEN_HEIGHT-16);
        pw_screen_clear_area(0, SCREEN_HEIGHT-16, x, 16);
        pw_screen_draw_from_eeprom(
            24, SCREEN_HEIGHT-16,
            16, 16,
            PW_EEPROM_ADDR_IMG_WATTS,
            PW_EEPROM_SIZE_IMG_WATTS
        );
        pw_screen_draw_from_eeprom(
            40, SCREEN_HEIGHT-16,
            8, 16,
            PW_EEPROM_ADDR_IMG_CHAR_SLASH,
            PW_EEPROM_SIZE_IMG_CHAR
        );
    } else {
        pw_screen_clear_area(0, SCREEN_HEIGHT-16, SCREEN_WIDTH/2, 16);
    }

    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-16, SCREEN_HEIGHT-16,
        16, 16,
        PW_EEPROM_ADDR_IMG_WATTS,
        PW_EEPROM_SIZE_IMG_WATTS
    );
    size_t x = pw_screen_draw_integer(health_data_cache.current_watts, SCREEN_WIDTH-16, SCREEN_HEIGHT-16);
    pw_screen_clear_area(SCREEN_WIDTH/2, SCREEN_HEIGHT-16, x - SCREEN_WIDTH/2, 16);
    pw_screen_clear_area(16, SCREEN_HEIGHT-16, 8, 16);

}


static void menu_clear_draw_cursor(pw_state_t *s, const screen_flags_t *sf) {
    for(size_t i = 0; i < MENU_SIZE; i++) {
        if(s->menu.cursor == i) {
        eeprom_addr_t addr = (sf->frame&ANIM_FRAME_NORMAL_TIME)?PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL:PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;
            pw_screen_draw_from_eeprom(
                4+i*16, CURSOR_Y_VALUES[i]-8,
                8, 8,
                addr,
                PW_EEPROM_SIZE_IMG_ARROW
            );
        } else {
            pw_screen_clear_area(4+i*16, CURSOR_Y_VALUES[i]-8, 8, 8);
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

    for(size_t i = 0; i < MENU_SIZE; i++) {
        pw_screen_draw_from_eeprom(
            i*16, CURSOR_Y_VALUES[i],
            16, 16,
            MENU_ICONS[i],
            PW_EEPROM_SIZE_IMG_MENU_ICON_CONNECT
        );
    }

    menu_draw_bottom_numbers(s, sf);
    menu_clear_draw_cursor(s, sf);

}


void pw_menu_update_display(pw_state_t *s, const screen_flags_t *sf) {

    switch(s->menu.substate) {
        case MS_NORMAL: {
            // redraw whole bottom portion
            menu_draw_bottom_numbers(s, sf);

            // toggle cursor
            menu_clear_draw_cursor(s, sf);

            // redraw title
            pw_screen_draw_from_eeprom(
                8, 0,
                80, 16,
                MENU_TITLES[s->menu.cursor],
                PW_EEPROM_SIZE_IMG_MENU_TITLE_CONNECT
            );

            break;
        }
        case MS_MESSAGE: {
            // Draw message spanning the whole bottom
            //pw_screen_draw_from_eeprom(
            //    0, SCREEN_HEIGHT-16,
            //    SCREEN_WIDTH, 16,
            //    // TODO: change this to MENU_MESSAGES
            //    PW_EEPROM_ADDR_TEXT_NEED_WATTS + PW_EEPROM_SIZE_TEXT_NEED_WATTS*(s->menu.message-1),
            //    PW_EEPROM_SIZE_TEXT_NEED_WATTS
            //);
            //pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, SCREEN_BLACK);

            uint16_t addr = PW_EEPROM_ADDR_TEXT_NEED_WATTS + PW_EEPROM_SIZE_TEXT_NEED_WATTS*(s->menu.message-1);
            pw_img_t img = (pw_img_t){.width=SCREEN_WIDTH, .height=16, .data=eeprom_buf, .size=PW_EEPROM_SIZE_TEXT_NEED_WATTS};
            pw_eeprom_read(addr, eeprom_buf, PW_EEPROM_SIZE_TEXT_NEED_WATTS);
            pw_screen_overlay_text_box(&img, SCREEN_WIDTH, 16, SCREEN_BLACK);
            pw_screen_draw_img(&img, 0, SCREEN_HEIGHT-16);

            break;
        }
        case MS_CLICKED: { break; }
        case MS_SPLASH: { break; } // none, we should immediately change states
        default: break; // shouldn't get here
    }

}

