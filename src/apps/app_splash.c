#include <stdint.h>

#include "app_splash.h"
#include "../states.h"
#include "../buttons.h"
#include "../menu.h"
#include "../eeprom_map.h"
#include "../eeprom.h"
#include "../screen.h"
#include "../utils.h"
#include "../types.h"
#include "../globals.h"

/// @file app_splash.c

enum {
    SPLASH_NORMAL,
    SPLASH_GO_TO_MENU,
};

void pw_splash_init(pw_state_t *s, const screen_flags_t *sf) {
    pw_detailed_inventory_t di;
    pw_read_inventory(&(s->splash.inventory), &di);
    s->splash.current_substate = SPLASH_NORMAL;
}

void pw_splash_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(b) {
    case BUTTON_M: {
        s->splash.menu_cursor = (MENU_SIZE-1)/2;
        s->splash.current_substate = SPLASH_GO_TO_MENU;
        break;
    }
    case BUTTON_L: {
        s->splash.menu_cursor = MENU_SIZE-1;
        s->splash.current_substate = SPLASH_GO_TO_MENU;
        break;
    }
    case BUTTON_R: {
        s->splash.menu_cursor = 0;
        s->splash.current_substate = SPLASH_GO_TO_MENU;
        break;
    }
    }

}

void pw_splash_init_display(pw_state_t *s, const screen_flags_t *sf) {
    if(s->splash.inventory.caught_pokemon & INV_WALKING_POKEMON) {
        pw_screen_draw_from_eeprom(
            SCREEN_WIDTH-64, 0,
            64, 48,
            PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED_FRAME2,
            PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME
        );
    }

    pw_screen_draw_from_eeprom(
        0, SCREEN_HEIGHT-24-16,
        32, 24,
        PW_EEPROM_ADDR_IMG_ROUTE_LARGE,
        PW_EEPROM_SIZE_IMG_ROUTE_LARGE
    );

    for(uint8_t i = 0; i < 3; i++) {
        if(s->splash.inventory.caught_pokemon & (1<<(i+1))) {
            pw_screen_draw_from_eeprom(
                i*8, SCREEN_HEIGHT-8,
                8, 8,
                PW_EEPROM_ADDR_IMG_BALL,
                PW_EEPROM_SIZE_IMG_BALL
            );
        }
    }

    for(uint8_t i = 0; i < 3; i++) {
        if(s->splash.inventory.dowsed_items & (1<<(i+1))) {
            pw_screen_draw_from_eeprom(
                24+i*8, SCREEN_HEIGHT-8,
                8, 8,
                PW_EEPROM_ADDR_IMG_ITEM,
                PW_EEPROM_SIZE_IMG_ITEM
            );
        }
    }


    for(uint8_t i = 0; i < 4; i++) {
        if( (s->splash.inventory.received_bitfield&(1<<i)) ) {
            pw_screen_draw_from_eeprom(
                16+i*8, SCREEN_HEIGHT-16,
                8, 8,
                PW_EEPROM_ADDR_IMG_CARD_SUITS+i*PW_EEPROM_SIZE_IMG_CARD_SUIT_SYMBOL,
                PW_EEPROM_SIZE_IMG_CARD_SUIT_SYMBOL
            );
        }

    }

    if( s->splash.inventory.caught_pokemon & INV_EXTRA_POKEMON ) {
        pw_screen_draw_from_eeprom(
            0, SCREEN_HEIGHT-16,
            8, 8,
            PW_EEPROM_ADDR_IMG_BALL_LIGHT,
            PW_EEPROM_SIZE_IMG_BALL_LIGHT
        );
    }

    if( s->splash.inventory.dowsed_items & INV_EXTRA_ITEM ) {
        pw_screen_draw_from_eeprom(
            8, SCREEN_HEIGHT-16,
            8, 8,
            PW_EEPROM_ADDR_IMG_ITEM_LIGHT,
            PW_EEPROM_SIZE_IMG_ITEM_LIGHT
        );
    }

    pw_screen_draw_integer(health_data_cache.today_steps, SCREEN_WIDTH, SCREEN_HEIGHT-16);
    pw_screen_draw_horiz_line(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, SCREEN_BLACK);
}

void pw_splash_update_display(pw_state_t *s, const screen_flags_t *sf) {

    uint16_t frame_addr;
    if(sf->frame&ANIM_FRAME_NORMAL_TIME) {
        frame_addr = PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED_FRAME1;
    } else {
        frame_addr = PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED_FRAME2;
    }

    if(s->splash.inventory.caught_pokemon & INV_WALKING_POKEMON) {
        pw_screen_draw_from_eeprom(
            SCREEN_WIDTH-64, 0,
            64, 48,
            frame_addr,
            PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME
        );

    }

    pw_screen_draw_integer(health_data_cache.today_steps, SCREEN_WIDTH, SCREEN_HEIGHT-16);
    pw_screen_draw_horiz_line(48, SCREEN_HEIGHT-16, SCREEN_WIDTH-48, SCREEN_BLACK);

}

