
#include "app_dowsing.h"

#include "../buttons.h"
#include "../eeprom_map.h"
#include "../eeprom.h"
#include "../screen.h"
#include "../states.h"
#include "../rand.h"
#include "../utils.h"
#include "../types.h"
#include "../globals.h"

/** @file app_dowsing.c
 *
 */

#define BUSH_HEIGHT (SCREEN_HEIGHT-16-8-16)
#define ARROW_HEIGHT (SCREEN_HEIGHT-16-8)


static uint8_t img_buf[128];
static void check_guess_draw_init(pw_state_t *s, const screen_flags_t *sf);
static void replace_item_draw_update(pw_state_t *s, const screen_flags_t *sf);
static void replace_item_draw_init(pw_state_t *s, const screen_flags_t *sf);
static void selected_draw_update(pw_state_t *s, const screen_flags_t *sf);
static void choosing_draw_update(pw_state_t *s, const screen_flags_t *sf);
static void choosing_draw_init(pw_state_t *s, const screen_flags_t *sf);

static void match_substate_2(pw_state_t *s, const screen_flags_t *sf) {
    s->dowsing.previous_substate = s->dowsing.current_substate;
}

state_void_func_t const draw_init_funcs[N_DOWSING_STATES] = {
    [DOWSING_ENTRY]         = match_substate_2,
    [DOWSING_CHOOSING]      = choosing_draw_init,
    [DOWSING_SELECTED]      = match_substate_2,
    [DOWSING_INTERMEDIATE]  = match_substate_2,
    [DOWSING_CHECK_GUESS]   = check_guess_draw_init,
    [DOWSING_GIVE_ITEM]     = match_substate_2,
    [DOWSING_REPLACE_ITEM]  = replace_item_draw_init,
    [DOWSING_QUITTING]      = match_substate_2,
    [DOWSING_AWAIT_INPUT]   = pw_empty_event,
    [DOWSING_REVEAL_ITEM]   = match_substate_2,
};

state_void_func_t const draw_update_funcs[N_DOWSING_STATES] = {
    [DOWSING_ENTRY]         = pw_empty_event,
    [DOWSING_CHOOSING]      = choosing_draw_update,
    [DOWSING_SELECTED]      = selected_draw_update,
    [DOWSING_INTERMEDIATE]  = pw_empty_event,
    [DOWSING_CHECK_GUESS]   = pw_empty_event,
    [DOWSING_GIVE_ITEM]     = pw_empty_event,
    [DOWSING_REPLACE_ITEM]  = replace_item_draw_update,
    [DOWSING_QUITTING]      = pw_empty_event,
    [DOWSING_AWAIT_INPUT]   = pw_empty_event,
    [DOWSING_REVEAL_ITEM]   = pw_empty_event,

};

static void switch_substate(pw_state_t *s, uint8_t new) {
    s->dowsing.previous_substate = s->dowsing.current_substate;
    s->dowsing.current_substate = new;
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

static void move_cursor(pw_state_t *s, int8_t m) {
    s->dowsing.previous_cursor = s->dowsing.current_cursor;
    s->dowsing.current_cursor += m;

    if(s->dowsing.current_cursor > 5) s->dowsing.current_cursor = 5;
    else if(s->dowsing.current_cursor < 0) s->dowsing.current_cursor = 0;
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

static uint16_t get_item(app_dowsing_t *dowsing, route_info_t *ri, health_data_t *hd) {
    uint32_t today_steps = hd->today_steps;

    // TODO: checks for gift item
    struct {
        uint16_t le_item;
        uint16_t le_steps;
        uint8_t  percent;
    } event_item;
    pw_eeprom_read(PW_EEPROM_ADDR_SPECIAL_ITEM, (uint8_t*)(&event_item), sizeof(event_item));

    uint8_t rnd = pw_rand()%100;

    if(event_item.le_item != 0 || event_item.le_item != 0xffff) {
        if(today_steps >= event_item.le_steps) {
            if(rnd < event_item.percent) {
                return event_item.le_item;
            }
        }
    }

    for(uint8_t i = 0; i < 10; i++) {
        if(today_steps >= ri->le_route_item_steps[i]) {
            if(rnd < ri->route_item_percent[i])
                return ri->le_route_items[i];
        }
    }

    // should not get here, but just in case
    return ri->le_route_items[9];
}

static void replace_item_draw_init(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_clear();
    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN
    );

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_TEXT_SWITCH,
        PW_EEPROM_SIZE_TEXT_SWITCH
    );

    for(uint8_t i = 0; i < 3; i++) {
        pw_screen_draw_from_eeprom(
            20+i*(16+8), SCREEN_HEIGHT-32-8,
            8, 8,
            PW_EEPROM_ADDR_IMG_ITEM,
            PW_EEPROM_SIZE_IMG_ITEM
        );
    }
    s->dowsing.previous_substate = s->dowsing.current_substate;
}


void pw_dowsing_init(pw_state_t *s, const screen_flags_t *sf) {
    route_info_t ri;
    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t*)(&ri), sizeof(ri));

    s->dowsing.chosen_item = get_item(&(s->dowsing), &ri, &health_data_cache);
    printf("chosen item index: 0x%04x\n", s->dowsing.chosen_item);

    //s->dowsing.item_position = 0; // choose position
    s->dowsing.item_position = pw_rand()%6;

    s->dowsing.chosen_positions = 0; // set no guesses
    s->dowsing.choices_remaining = 2; // set tries left
    s->dowsing.user_input = false;
    s->dowsing.bush_shakes = 0;

    s->dowsing.current_cursor = s->dowsing.previous_cursor = 0;
    s->dowsing.current_substate = s->dowsing.previous_substate = DOWSING_ENTRY;

}

void pw_dowsing_init_display(pw_state_t *s, const screen_flags_t *sf) {
    pw_img_t grass = {.data=img_buf, .width=16, .height=16, .size=PW_EEPROM_SIZE_IMG_DOWSING_BUSH_DARK};
    pw_eeprom_read(
        PW_EEPROM_ADDR_IMG_DOWSING_BUSH_DARK,
        grass.data,
        PW_EEPROM_SIZE_IMG_DOWSING_BUSH_DARK
    );

    for(uint8_t i = 0; i < 6; i++) {
        pw_screen_draw_img(&grass, 16*i, BUSH_HEIGHT);
        if(i == s->dowsing.current_cursor) {
            if(sf->frame&ANIM_FRAME_NORMAL_TIME) {
                pw_screen_draw_from_eeprom(
                    16*i+2, SCREEN_HEIGHT-16-8,
                    8, 8,
                    PW_EEPROM_ADDR_IMG_ARROW_UP_NORMAL,
                    PW_EEPROM_SIZE_IMG_ARROW
                );
            } else {
                pw_screen_draw_from_eeprom(
                    16*i+2, SCREEN_HEIGHT-16-8,
                    8, 8,
                    PW_EEPROM_ADDR_IMG_ARROW_UP_OFFSET,
                    PW_EEPROM_SIZE_IMG_ARROW
                );
            }
        }
    }

    pw_screen_draw_from_eeprom(
        0, SCREEN_HEIGHT-16,
        96, 16,
        PW_EEPROM_ADDR_TEXT_DISCOVER_ITEM,
        PW_EEPROM_SIZE_TEXT_DISCOVER_ITEM
    );
    pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, 0x3);

    pw_screen_draw_from_eeprom(
        0, 0,
        32, 24,
        PW_EEPROM_ADDR_IMG_ROUTE_LARGE,
        PW_EEPROM_SIZE_IMG_ROUTE_LARGE
    );

    pw_screen_draw_from_eeprom(
        36, 0,
        32, 16,
        PW_EEPROM_ADDR_TEXT_LEFT,
        PW_EEPROM_SIZE_TEXT_LEFT
    );

    pw_screen_draw_from_eeprom(
        76, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_DIGITS + PW_EEPROM_SIZE_IMG_CHAR*s->dowsing.choices_remaining,
        PW_EEPROM_SIZE_IMG_CHAR
    );

}

static void choosing_draw_init(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_draw_from_eeprom(
        0, SCREEN_HEIGHT-16,
        96, 16,
        PW_EEPROM_ADDR_TEXT_DISCOVER_ITEM,
        PW_EEPROM_SIZE_TEXT_DISCOVER_ITEM
    );
    pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, 0x3);
    s->dowsing.previous_substate = s->dowsing.current_substate;

}

static void choosing_draw_update(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_clear_area(0, SCREEN_HEIGHT-16-8, SCREEN_WIDTH, 8);
    uint16_t addr = (sf->frame&ANIM_FRAME_NORMAL_TIME)?PW_EEPROM_ADDR_IMG_ARROW_UP_NORMAL:PW_EEPROM_ADDR_IMG_ARROW_UP_OFFSET;
    pw_screen_draw_from_eeprom(
        16*s->dowsing.current_cursor+4, SCREEN_HEIGHT-16-8,
        8, 8,
        addr,
        PW_EEPROM_SIZE_IMG_ARROW
    );

}

static void selected_draw_update(pw_state_t *s, const screen_flags_t *sf) {
    s->dowsing.bush_shakes++;
    uint8_t y = (sf->frame&ANIM_FRAME_NORMAL_TIME)?BUSH_HEIGHT+2:BUSH_HEIGHT-2;
    pw_screen_draw_from_eeprom(
        16*s->dowsing.current_cursor, y,
        16, 16,
        PW_EEPROM_ADDR_IMG_DOWSING_BUSH_DARK,
        PW_EEPROM_SIZE_IMG_DOWSING_BUSH_DARK
    );
    y = (sf->frame&ANIM_FRAME_NORMAL_TIME)?BUSH_HEIGHT:BUSH_HEIGHT+16-2;
    pw_screen_clear_area(16*s->dowsing.current_cursor, y, 16, 2);
}

static void replace_item_draw_update(pw_state_t *s, const screen_flags_t *sf) {
    for(uint8_t i = 0; i < 3; i++) {
        pw_screen_clear_area(20+i*(8+16), SCREEN_HEIGHT-32, 8, 8);
    }
    if((sf->frame&ANIM_FRAME_NORMAL_TIME)) {
        pw_screen_draw_from_eeprom(
            20+s->dowsing.current_cursor*(8+16), SCREEN_HEIGHT-32,
            8, 8,
            PW_EEPROM_ADDR_IMG_ARROW_UP_NORMAL,
            PW_EEPROM_SIZE_IMG_ARROW
        );
    } else {
        pw_screen_draw_from_eeprom(
            20+s->dowsing.current_cursor*(8+16), SCREEN_HEIGHT-32,
            8, 8,
            PW_EEPROM_ADDR_IMG_ARROW_UP_OFFSET,
            PW_EEPROM_SIZE_IMG_ARROW
        );
    }

    struct {
        uint16_t le_item;
        uint16_t pad;
    } inv[3];

    pw_eeprom_read(
        PW_EEPROM_ADDR_OBTAINED_ITEMS,
        (uint8_t*)inv,
        PW_EEPROM_SIZE_OBTAINED_ITEMS
    );

    uint16_t le_item = inv[s->dowsing.current_cursor].le_item;

    uint8_t cursor_item_index = pw_item_id_to_item_index(le_item);

    pw_screen_draw_from_eeprom(
        0, SCREEN_HEIGHT-16,
        96, 16,
        PW_EEPROM_ADDR_TEXT_ITEM_NAMES + cursor_item_index*PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE,
        PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE
    );
    pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, 0x3);

}

static void check_guess_draw_init(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_clear_area(16*s->dowsing.current_cursor, BUSH_HEIGHT, 16, 2);
    pw_screen_clear_area(16*s->dowsing.current_cursor, BUSH_HEIGHT+16-2, 16, 2);
    pw_screen_draw_from_eeprom(
        16*s->dowsing.current_cursor, BUSH_HEIGHT,
        16, 16,
        PW_EEPROM_ADDR_IMG_DOWSING_BUSH_LIGHT,
        PW_EEPROM_SIZE_IMG_DOWSING_BUSH_LIGHT
    );

    pw_screen_draw_from_eeprom(
        76, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_DIGITS + PW_EEPROM_SIZE_IMG_CHAR*s->dowsing.choices_remaining,
        PW_EEPROM_SIZE_IMG_CHAR
    );
    s->dowsing.current_substate = s->dowsing.current_substate;

}

void pw_dowsing_update_display(pw_state_t *s, const screen_flags_t *sf) {
    if(s->dowsing.previous_substate != s->dowsing.current_substate) {
        draw_init_funcs[s->dowsing.current_substate](s, sf);
    } else {
        draw_update_funcs[s->dowsing.current_substate](s, sf);
    }
}



void pw_dowsing_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(s->dowsing.current_substate) {
    case DOWSING_CHOOSING: {
        switch(b) {
        case BUTTON_L:
            move_cursor(s, -1);
            break;
        case BUTTON_R:
            move_cursor(s, +1);
            break;
        case BUTTON_M: {
            // If we haven't alreadt selected it
            if(!( (1<<s->dowsing.current_cursor) & s->dowsing.chosen_positions )) {
                switch_substate(s, DOWSING_SELECTED);
                break;

            }
        }
        }
        break;
    }
    case DOWSING_REPLACE_ITEM: {
        switch(b) {
        case BUTTON_R: {
            s->dowsing.current_cursor++;
            if(s->dowsing.current_cursor > 2)
                s->dowsing.current_cursor = 2;
            PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
            break;
        }
        case BUTTON_L: {
            s->dowsing.current_cursor--;
            if(s->dowsing.current_cursor < 0)
                switch_substate(s, DOWSING_QUITTING);
            PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
            break;
        }
        case BUTTON_M: {
            struct {
                uint16_t le_item;
                uint16_t pad;
            } inv[3];

            pw_eeprom_read(
                PW_EEPROM_ADDR_OBTAINED_ITEMS,
                (uint8_t*)inv,
                PW_EEPROM_SIZE_OBTAINED_ITEMS
            );

            printf("replacing index %d with 0x%04x\n", s->dowsing.current_cursor, s->dowsing.chosen_item);
            inv[s->dowsing.current_cursor].le_item = s->dowsing.chosen_item;
            pw_eeprom_write(
                PW_EEPROM_ADDR_OBTAINED_ITEMS,
                (uint8_t*)inv,
                PW_EEPROM_SIZE_OBTAINED_ITEMS
            );

            switch_substate(s, DOWSING_QUITTING);
            PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
            break;
        }
        }
        break;
    }

    case DOWSING_AWAIT_INPUT: {
        s->dowsing.user_input = true;
        break;
    }
    default:
        break;
    }
}

void pw_dowsing_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {

    switch(s->dowsing.current_substate) {
    case DOWSING_ENTRY:
        switch_substate(s, DOWSING_CHOOSING);
        break;
    case DOWSING_AWAIT_INPUT: {
        if(s->dowsing.user_input) {
            switch_substate(s, s->dowsing.next_substate);
        }
        break;
    }
    case DOWSING_SELECTED: {
        // after 4 frames, set substate check correct
        if(s->dowsing.bush_shakes >= 6) {
            s->dowsing.bush_shakes = 0;  // shakes done
            switch_substate(s, DOWSING_CHECK_GUESS);
        }
        break;
    }
    case DOWSING_CHECK_GUESS: {
        s->dowsing.chosen_positions |= 1<<(s->dowsing.current_cursor);   // add guess to guesses
        s->dowsing.choices_remaining--;

        check_guess_draw_init(s, sf);

        uint8_t item_pos = 1<<(s->dowsing.item_position);
        if(item_pos & s->dowsing.chosen_positions) {
            s->dowsing.choices_remaining = 1;  // we got it right
            switch_substate(s, DOWSING_REVEAL_ITEM);
        } else {

            pw_screen_draw_from_eeprom(
                0, SCREEN_HEIGHT-16,
                96, 16,
                PW_EEPROM_ADDR_TEXT_NOTHING_FOUND,
                PW_EEPROM_SIZE_TEXT_NOTHING_FOUND
            );
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, 0x3);

            // do we still have guesses remaining?
            if(s->dowsing.choices_remaining > 0) {
                s->dowsing.user_input = false;
                s->dowsing.current_substate = DOWSING_AWAIT_INPUT;
                s->dowsing.next_substate = DOWSING_INTERMEDIATE;
            } else {
                s->dowsing.choices_remaining = 0;  // we got it wrong
                switch_substate(s, DOWSING_REVEAL_ITEM);
            }

        }

        break;
    }
    case DOWSING_INTERMEDIATE: {
        if(s->dowsing.current_cursor == s->dowsing.item_position-1 || s->dowsing.current_cursor == s->dowsing.item_position+1) {
            pw_screen_draw_from_eeprom(
                0, SCREEN_HEIGHT-16,
                96, 16,
                PW_EEPROM_ADDR_TEXT_ITS_NEAR,
                PW_EEPROM_SIZE_TEXT_ITS_NEAR
            );
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, 0x3);
        } else {
            pw_screen_draw_from_eeprom(
                0, SCREEN_HEIGHT-16,
                96, 16,
                PW_EEPROM_ADDR_TEXT_FAR_AWAY,
                PW_EEPROM_SIZE_TEXT_FAR_AWAY
            );
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, 0x3);
        }

        s->dowsing.user_input = 0;
        s->dowsing.current_substate = DOWSING_AWAIT_INPUT;
        s->dowsing.next_substate = DOWSING_CHOOSING;
        break;
    }
    case DOWSING_GIVE_ITEM: {
        struct {
            uint16_t le_item;
            uint16_t pad;
        } inv[3];

        pw_eeprom_read(
            PW_EEPROM_ADDR_OBTAINED_ITEMS,
            (uint8_t*)inv,
            PW_EEPROM_SIZE_OBTAINED_ITEMS
        );

        uint8_t avail = 0;
        for(avail = 0; (avail<3) && (inv[avail].le_item != 0); avail++);

        pw_screen_draw_from_eeprom(
            0, SCREEN_HEIGHT-16,
            96, 16,
            PW_EEPROM_ADDR_TEXT_FOUND,
            PW_EEPROM_SIZE_TEXT_FOUND
        );

        uint8_t chosen_item_index = pw_item_id_to_item_index(s->dowsing.chosen_item);

        pw_screen_draw_from_eeprom(
            0, SCREEN_HEIGHT-32,
            96, 16,
            PW_EEPROM_ADDR_TEXT_ITEM_NAMES + PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE*chosen_item_index,
            PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE
        );
        pw_screen_draw_text_box(0, SCREEN_HEIGHT-32, SCREEN_WIDTH, 32, 0x3);

        if( avail >= 3 ) {
            s->dowsing.current_cursor = 0;

            s->dowsing.user_input = false;
            s->dowsing.current_substate = DOWSING_AWAIT_INPUT;
            s->dowsing.next_substate = DOWSING_REPLACE_ITEM;
        } else {

            inv[avail].le_item = s->dowsing.chosen_item;
            pw_eeprom_write(
                PW_EEPROM_ADDR_OBTAINED_ITEMS,
                (uint8_t*)inv,
                PW_EEPROM_SIZE_OBTAINED_ITEMS
            );

            s->dowsing.user_input = false;
            s->dowsing.current_substate = DOWSING_AWAIT_INPUT;
            s->dowsing.next_substate = DOWSING_QUITTING;
        }
        break;
    }
    case DOWSING_REVEAL_ITEM: {
        pw_screen_clear_area(16*s->dowsing.item_position, BUSH_HEIGHT-4, 16, 8);
        pw_screen_draw_from_eeprom(
            16*s->dowsing.item_position+4, BUSH_HEIGHT,
            8, 8,
            PW_EEPROM_ADDR_IMG_ITEM,
            PW_EEPROM_SIZE_IMG_ITEM
        );

        if(s->dowsing.choices_remaining > 0) {
            switch_substate(s, DOWSING_GIVE_ITEM);
        } else {
            s->dowsing.user_input = false;
            s->dowsing.current_substate = DOWSING_AWAIT_INPUT;
            s->dowsing.next_substate = DOWSING_QUITTING;
        }
        break;
    }
    case DOWSING_QUITTING: {
        p->sid = STATE_MAIN_MENU;
        p->menu.cursor = 1;
        break;
    }
    default:
        break;
    }

}

