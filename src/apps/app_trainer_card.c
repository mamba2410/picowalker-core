#include <stdint.h>

#include "app_trainer_card.h"

#include "../states.h"
#include "../buttons.h"
#include "../screen.h"
#include "../utils.h"
#include "../eeprom_map.h"
#include "../eeprom.h"
#include "../timer.h"
#include "../types.h"
#include "../globals.h"

/// @file app_trainer_card.c

static uint32_t prev_step_counts[7] = {0,};

void pw_trainer_card_move_cursor(pw_state_t *s, int8_t m);

void pw_trainer_card_init(pw_state_t *s, const screen_flags_t *sf) {
    s->trainer_card.current_cursor = 0;
    s->trainer_card.previous_cursor = -1;
    s->trainer_card.current_substate = TC_NORMAL;

    pw_eeprom_read(
        PW_EEPROM_ADDR_HISTORIC_STEP_COUNT,
        (uint8_t*)prev_step_counts,
        PW_EEPROM_SIZE_HISTORIC_STEP_COUNT
    );

    for(size_t i = 0; i < 7; i++) {
        prev_step_counts[i] = swap_bytes_u32(prev_step_counts[i]);
    }

}

void pw_trainer_card_init_display(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_IMG_MENU_TITLE_TRAINER_CARD,
        PW_EEPROM_SIZE_IMG_MENU_TITLE_TRAINER_CARD
    );
    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN
    );
    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-8, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RIGHT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RIGHT
    );

    pw_screen_draw_from_eeprom(
        0, 16,
        16, 16,
        PW_EEPROM_ADDR_IMG_PERSON,
        PW_EEPROM_SIZE_IMG_PERSON
    );
    pw_screen_draw_from_eeprom(
        16, 16,
        80, 16,
        PW_EEPROM_ADDR_IMG_TRAINER_NAME,
        PW_EEPROM_SIZE_IMG_TRAINER_NAME
    );
    pw_screen_draw_from_eeprom(
        0, 32,
        16, 16,
        PW_EEPROM_ADDR_IMG_ROUTE_SMALL,
        PW_EEPROM_SIZE_IMG_ROUTE_SMALL
    );
    pw_screen_draw_from_eeprom(
        16, 32,
        80, 16,
        PW_EEPROM_ADDR_TEXT_ROUTE_NAME,
        PW_EEPROM_SIZE_TEXT_ROUTE_NAME
    );
    pw_screen_draw_from_eeprom(
        0, 48,
        32, 16,
        PW_EEPROM_ADDR_IMG_TIME_FRAME,
        PW_EEPROM_SIZE_IMG_TIME_FRAME
    );

    pw_dhms_t dhms = pw_time_get_dhms();
    pw_screen_draw_time(dhms.hours, dhms.minutes, dhms.seconds, 32, 48);
}

void pw_trainer_card_draw_dayview(uint8_t day, uint32_t day_steps,
                                  uint32_t total_steps, uint16_t total_days) {

    uint8_t x=0, y=0;
    pw_screen_draw_from_eeprom(
        x, y,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_LEFT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_LEFT
    );
    x+=8;

    pw_screen_clear_area(x, y, 12, 16);
    x+=12;

    pw_screen_draw_from_eeprom(
        x, y,
        8, 16,
        PW_EEPROM_ADDR_IMG_CHAR_DASH,
        PW_EEPROM_SIZE_IMG_CHAR
    );
    x+=8;

    x+=8;
    pw_screen_draw_integer(day, x, y);

    pw_screen_draw_from_eeprom(
        x, y,
        40, 16,
        PW_EEPROM_ADDR_IMG_DAYS_FRAME,
        PW_EEPROM_SIZE_IMG_DAYS_FRAME
    );
    x+=40;

    pw_screen_clear_area(x, y, 12, 16);
    x+=12;

    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-8, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RIGHT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RIGHT
    );
    x+=8;

    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-40, 16,
        40, 16,
        PW_EEPROM_ADDR_IMG_STEPS_FRAME,
        PW_EEPROM_SIZE_IMG_STEPS_FRAME
    );
    screen_pos_t until = pw_screen_draw_integer(day_steps, SCREEN_WIDTH-40, 16);
    pw_screen_clear_area(0, 16, until, 16);

    pw_screen_draw_from_eeprom(
        0, 32,
        64, 16,
        PW_EEPROM_ADDR_IMG_TOTAL_DAYS_FRAME,
        PW_EEPROM_SIZE_IMG_TOTAL_DAYS_FRAME
    );
    until = pw_screen_draw_integer(total_days, SCREEN_WIDTH, 32); // shift x by -1?
    pw_screen_clear_area(64, 32, until-64, 16);

    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-40, 48,
        40, 16,
        PW_EEPROM_ADDR_IMG_STEPS_FRAME,
        PW_EEPROM_SIZE_IMG_STEPS_FRAME
    );
    until = pw_screen_draw_integer(total_steps, SCREEN_WIDTH-40, 48);
    pw_screen_clear_area(0, 48, until, 16);

}

void pw_trainer_card_move_cursor(pw_state_t *s, int8_t m) {
    s->trainer_card.current_cursor += m;
    if(s->trainer_card.current_cursor < 0) s->trainer_card.current_cursor = 0;
    if(s->trainer_card.current_cursor > TRAINER_CARD_MAX_DAYS) s->trainer_card.current_cursor = TRAINER_CARD_MAX_DAYS;
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

void pw_trainer_card_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(b) {
    case BUTTON_L: {
        if(s->trainer_card.current_cursor <= 0) {
            s->trainer_card.current_substate = TC_GO_TO_MENU;
        } else {
            pw_trainer_card_move_cursor(s, -1);
        }
        break;
    }
    case BUTTON_M: {
        s->trainer_card.current_substate = TC_GO_TO_SPLASH;
        break;
    }
    case BUTTON_R: {
        pw_trainer_card_move_cursor(s, +1);
        break;
    }
    }
}

void pw_trainer_card_draw_update(pw_state_t *s, const screen_flags_t *sf) {
    if(s->trainer_card.previous_cursor != s->trainer_card.current_cursor) {
        if(s->trainer_card.current_cursor <= 0) {
            pw_trainer_card_init_display(s, sf);
        } else {
            uint32_t const total_steps = health_data_cache.total_steps;
            uint32_t const today_steps = health_data_cache.today_steps;
            uint16_t const total_days  = health_data_cache.total_days;
            pw_trainer_card_draw_dayview(
                s->trainer_card.current_cursor,
                prev_step_counts[s->trainer_card.current_cursor-1],
                total_steps,
                //total_steps+today_steps,
                total_days
            );
        }
        s->trainer_card.previous_cursor = s->trainer_card.current_cursor;
    }

    if(s->trainer_card.current_cursor <= 0) {
        // redraw time, possibly able to optimise
        pw_dhms_t dhms = pw_time_get_dhms();
        pw_screen_draw_time(dhms.hours, dhms.minutes, dhms.seconds, 32, 48);
    }

}


void pw_trainer_card_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    switch(s->trainer_card.current_substate) {
    case TC_NORMAL: {
        break;
    }
    case TC_GO_TO_MENU: {
        p->sid = STATE_MAIN_MENU;
        p->menu.cursor = 3;
        break;
    }
    case TC_GO_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        break;
    }
    }
}

