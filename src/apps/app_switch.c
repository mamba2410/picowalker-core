#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../utils.h"
#include "../states.h"
#include "../eeprom_map.h"
#include "../screen.h"
#include "../buttons.h"
#include "../types.h"

#include "app_switch.h"

/**
 * Switch screen, multipurpose for both items and pokemon
 */

void pw_switch_init(pw_state_t *s, const screen_flags_t *sf) {
    (void)sf;
    s->switches.cursor = 1;
    s->switches.current_substate = SWITCHES_CHOOSING;

    pw_detailed_inventory_t di;
    pw_brief_inventory_t bi;
    pw_read_inventory(&bi, &di);

    switch(s->switches.switch_type) {
    case SWITCH_TYPE_ITEM: {
        for(size_t i = 0; i < 3; i++) {
            s->switches.inv_ids[i] = di.dowsed_items[i];
            s->switches.inv_indices[i] = pw_item_id_to_item_index(di.dowsed_items[i]);
        }
        s->switches.switch_index = pw_item_id_to_item_index(s->switches.switch_id);
        //s->switches.switch_index = s->switches.switch_id; // already an index
        break;
    }
    case SWITCH_TYPE_POKEMON: {
        pokemon_summary_t pokemon;
        for(size_t i = 0; i < 3; i++) {
            s->switches.inv_ids[i] = di.caught_pokemon[i];
            s->switches.inv_indices[i] = pw_pokemon_id_to_pokemon_index(di.caught_pokemon[i], &pokemon)-1;
        }
        s->switches.switch_index = s->switches.switch_id; // already an index
        break;
    }
    default: {
        // unreachable
        break;
    }
    }
}

void pw_switch_init_display(pw_state_t *s, const screen_flags_t *sf) {
    (void)sf;
    pw_eeprom_addr_t addr = 0;
    size_t size = 0;

    switch(s->switches.switch_type) {
    case SWITCH_TYPE_ITEM: {
        addr = PW_EEPROM_ADDR_IMG_ITEM;
        size = PW_EEPROM_SIZE_IMG_ITEM;
        break;
    }
    case SWITCH_TYPE_POKEMON: {
        addr = PW_EEPROM_ADDR_IMG_BALL;
        size = PW_EEPROM_SIZE_IMG_BALL;
        break;
    }
    }

    pw_screen_clear();
    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN,
        false
    );

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_TEXT_SWITCH,
        PW_EEPROM_SIZE_TEXT_SWITCH,
        false
    );

    for(uint8_t i = 0; i < 3; i++) {
        pw_screen_draw_from_eeprom(
            20+i*(16+8), PW_SCREEN_HEIGHT-32-8,
            8, 8,
            addr,
            size,
            true
        );
    }

    pw_screen_draw_from_eeprom(
        20+s->switches.cursor*(16+8), PW_SCREEN_HEIGHT-32,
        8, 8,
        PW_EEPROM_ADDR_IMG_ARROW_UP_NORMAL,
        PW_EEPROM_SIZE_IMG_ARROW,
        false
    );
}

void pw_switch_update_display(pw_state_t *s, const screen_flags_t *sf) {
    pw_eeprom_addr_t addr = 0;
    size_t size = 0;
    pw_screen_pos_t width = 0;

    switch(s->switches.switch_type) {
    case SWITCH_TYPE_ITEM: {
        uint8_t idx = s->switches.inv_indices[s->switches.cursor];
        size = PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE;
        addr = PW_EEPROM_ADDR_TEXT_ITEM_NAMES + size*idx;
        width = 96;
        break;
    }
    case SWITCH_TYPE_POKEMON: {
        uint8_t idx = s->switches.inv_indices[s->switches.cursor];
        size = PW_EEPROM_SIZE_TEXT_POKEMON_NAME;
        addr = PW_EEPROM_ADDR_TEXT_POKEMON_NAMES + size*idx;
        width = 80;
        break;
    }
    }

    for(uint8_t i = 0; i < 3; i++) {
        pw_screen_clear_area(20+i*(8+16), PW_SCREEN_HEIGHT-32, 8, 8);
    }
    if(sf->frame&ANIM_FRAME_DOUBLE_TIME) {
        pw_screen_draw_from_eeprom(
            20+s->switches.cursor*(8+16), PW_SCREEN_HEIGHT-32,
            8, 8,
            PW_EEPROM_ADDR_IMG_ARROW_UP_NORMAL,
            PW_EEPROM_SIZE_IMG_ARROW,
            false
        );
    } else {
        pw_screen_draw_from_eeprom(
            20+s->switches.cursor*(8+16), PW_SCREEN_HEIGHT-32,
            8, 8,
            PW_EEPROM_ADDR_IMG_ARROW_UP_OFFSET,
            PW_EEPROM_SIZE_IMG_ARROW,
            false
        );
    }

    if(s->switches.cursor != s->switches.prev_cursor) {
        pw_screen_draw_from_eeprom(
            0, PW_SCREEN_HEIGHT-16,
            width, 16,
            addr,
            size,
            false
        );
        pw_screen_draw_text_box(0, PW_SCREEN_HEIGHT-16, PW_SCREEN_WIDTH, 16, PW_SCREEN_BLACK);
        s->switches.prev_cursor = s->switches.cursor;
    }

}

void pw_switch_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b) {
    (void)sf;
    switch(b) {
    case PW_BUTTON_L: {
        if(s->switches.cursor == 0) {
            s->switches.current_substate = SWITCHES_TO_SPLASH;
            break;
        }
        s->switches.cursor = (s->switches.cursor-1+3)%3;
        break;
    }
    case PW_BUTTON_R: {
        if(s->switches.cursor >= 2) break;
        s->switches.cursor = (s->switches.cursor+1)%3;
        break;
    }
    case PW_BUTTON_M: {
        s->switches.current_substate = SWITCHES_WRITE_INV;
        break;
    }
    }

    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

void pw_switch_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    (void)sf;
    switch(s->switches.current_substate) {
    case SWITCHES_CHOOSING: {
        break;
    }
    case SWITCHES_WRITE_INV: {
        route_info_t ri;
        pw_eeprom_read(
            PW_EEPROM_ADDR_ROUTE_INFO,
            (uint8_t*)(&ri),
            sizeof(ri)
        );

        switch(s->switches.switch_type) {
        case SWITCH_TYPE_ITEM: {
            struct {
                uint16_t le_item;
                uint16_t pad;
            } items[3];

            pw_eeprom_read(
                PW_EEPROM_ADDR_OBTAINED_ITEMS,
                (uint8_t*)(items),
                PW_EEPROM_SIZE_OBTAINED_ITEMS
            );

            items[s->switches.cursor].le_item = ri.le_route_items[s->switches.switch_index];

            pw_eeprom_write(
                PW_EEPROM_ADDR_OBTAINED_ITEMS,
                (uint8_t*)(items),
                PW_EEPROM_SIZE_OBTAINED_ITEMS
            );

            break;
        }
        case SWITCH_TYPE_POKEMON: {

            pokemon_summary_t caught_pokemon[3];
            pw_eeprom_read(
                PW_EEPROM_ADDR_CAUGHT_POKEMON_SUMMARY,
                (uint8_t*)caught_pokemon,
                PW_EEPROM_SIZE_CAUGHT_POKEMON_SUMMARY
            );

            caught_pokemon[s->switches.cursor] = ri.route_pokemon[s->switches.switch_index];

            pw_eeprom_write(
                PW_EEPROM_ADDR_CAUGHT_POKEMON_SUMMARY,
                (uint8_t*)caught_pokemon,
                PW_EEPROM_SIZE_CAUGHT_POKEMON_SUMMARY
            );
            break;
        }
        }
        s->switches.current_substate = SWITCHES_TO_SPLASH;
        break;
    }
    case SWITCHES_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        break;
    }
    }

}

