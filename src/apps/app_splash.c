#include <stdint.h>

#include "app_splash.h"
#include "../states.h"
#include "../buttons.h"
#include "../menu.h"
#include "../eeprom_map.h"
#include "../eeprom.h"
#include "../screen.h"
#include "../audio.h"
#include "../utils.h"
#include "../types.h"
#include "../accel.h"
#include "../globals.h"

#include "../picowalker_core.h"

/// @file app_splash.c

enum {
    SPLASH_NORMAL,
    SPLASH_GO_TO_MENU,
};

enum {
    IDLE,
    WALKING,
    STROLLING,
};

const pw_screen_pos_t origin = 32;

void pw_splash_init(pw_state_t *s, const screen_flags_t *sf) {
    (void)sf;
    pw_detailed_inventory_t di;
    pw_read_inventory(&(s->splash.inventory), &di);
    s->splash.current_substate = SPLASH_NORMAL;
}

void pw_splash_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b) {
    (void)sf;
    switch(b) {
    case PW_BUTTON_M: {
        s->splash.menu_cursor = (MENU_SIZE-1)/2;
        s->splash.current_substate = SPLASH_GO_TO_MENU;
	pw_audio_play_sound(SOUND_NAVIGATE_MENU);
        break;
    }
    case PW_BUTTON_L: {
        s->splash.menu_cursor = MENU_SIZE-1;
        s->splash.current_substate = SPLASH_GO_TO_MENU;
	pw_audio_play_sound(SOUND_NAVIGATE_MENU);
        break;
    }
    case PW_BUTTON_R: {
        s->splash.menu_cursor = 0;
        s->splash.current_substate = SPLASH_GO_TO_MENU;
	pw_audio_play_sound(SOUND_NAVIGATE_MENU);
        break;
    }
    }

}

void pw_splash_init_display(pw_state_t *s, const screen_flags_t *sf) {

    s->splash.walking = IDLE;
    s->splash.offset = 0;
    s->splash.anim_frame = 0;
    s->splash.is_flipped = false;
    s->splash.last_check_frame = sf->frame;

    (void)sf;
    if(s->splash.inventory.caught_pokemon & INV_WALKING_POKEMON) {
        pw_screen_draw_from_eeprom(
            PW_SCREEN_WIDTH-64, 0,
            64, 48,
            PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED_FRAME2,
            PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME,
            true
        );
    }

    pw_screen_draw_from_eeprom(
        0, PW_SCREEN_HEIGHT-24-16,
        32, 24,
        PW_EEPROM_ADDR_IMG_ROUTE_LARGE,
        PW_EEPROM_SIZE_IMG_ROUTE_LARGE,
        true
    );

    for(uint8_t i = 0; i < 3; i++) {
        if(s->splash.inventory.caught_pokemon & (1<<(i+1))) {
            pw_screen_draw_from_eeprom(
                i*8, PW_SCREEN_HEIGHT-8,
                8, 8,
                PW_EEPROM_ADDR_IMG_BALL,
                PW_EEPROM_SIZE_IMG_BALL,
                true
            );
        }
    }

    for(uint8_t i = 0; i < 3; i++) {
        if(s->splash.inventory.dowsed_items & (1<<(i+1))) {
            pw_screen_draw_from_eeprom(
                24+i*8, PW_SCREEN_HEIGHT-8,
                8, 8,
                PW_EEPROM_ADDR_IMG_ITEM,
                PW_EEPROM_SIZE_IMG_ITEM,
                true
            );
        }
    }


    for(uint8_t i = 0; i < 4; i++) {
        if( (s->splash.inventory.received_bitfield&(1<<i)) ) {
            pw_screen_draw_from_eeprom(
                16+i*8, PW_SCREEN_HEIGHT-16,
                8, 8,
                PW_EEPROM_ADDR_IMG_CARD_SUITS+i*PW_EEPROM_SIZE_IMG_CARD_SUIT_SYMBOL,
                PW_EEPROM_SIZE_IMG_CARD_SUIT_SYMBOL,
                true
            );
        }

    }

    if( s->splash.inventory.caught_pokemon & INV_EXTRA_POKEMON ) {
        pw_screen_draw_from_eeprom(
            0, PW_SCREEN_HEIGHT-16,
            8, 8,
            PW_EEPROM_ADDR_IMG_BALL_LIGHT,
            PW_EEPROM_SIZE_IMG_BALL_LIGHT,
            true
        );
    }

    if( s->splash.inventory.dowsed_items & INV_EXTRA_ITEM ) {
        pw_screen_draw_from_eeprom(
            8, PW_SCREEN_HEIGHT-16,
            8, 8,
            PW_EEPROM_ADDR_IMG_ITEM_LIGHT,
            PW_EEPROM_SIZE_IMG_ITEM_LIGHT,
            true
        );
    }

    pw_screen_pos_t left_x = pw_screen_draw_integer_with_overline(health_data_cache.today_steps, PW_SCREEN_WIDTH, PW_SCREEN_HEIGHT-16, PW_SCREEN_BLACK);
    pw_screen_draw_horiz_line(0, PW_SCREEN_HEIGHT-16, left_x, PW_SCREEN_BLACK);
}

void pw_splash_update_display(pw_state_t *s, const screen_flags_t *sf) {

    uint16_t frame_addr;
    if(sf->frame&ANIM_FRAME_NORMAL_TIME) {
        frame_addr = PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED_FRAME1;
    } else {
        frame_addr = PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED_FRAME2;
    }

    if(s->splash.inventory.caught_pokemon & INV_WALKING_POKEMON) {
        switch(s->splash.walking) {
            case IDLE: {
                pw_screen_draw_from_eeprom(
                    PW_SCREEN_WIDTH-64, 0,
                    64, 48,
                    frame_addr,
                    PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME,
                    true
                );
                if (pw_accel_get_activity() != 0) s->splash.walking = WALKING;
                break;
            }
            case WALKING: {
                pw_screen_clear_area(origin, 0, s->splash.offset, 48);
                pw_screen_draw_from_eeprom(
                    origin + s->splash.offset, 0,
                    64, 48,
                    frame_addr,
                    PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME,
                    true
                );

                // Move right if still walking, move left if done walking...
                if (pw_accel_get_activity() != 0) {
                    if (s->splash.offset >= PW_SCREEN_WIDTH - origin) {
                        s->splash.walking = STROLLING;
                        s->splash.offset = (PW_SCREEN_WIDTH - origin);
                    }
                    else s->splash.offset += 4;
                }
                else {
                    if (s->splash.offset <= 0) {
                        s->splash.walking = IDLE;
                        s->splash.offset = 0;
                    }
                    else s->splash.offset -= 4;
                }
                break;
            }
            case STROLLING: {
                if (pw_accel_get_activity() != 0) {
                    if (s->splash.offset <= 0 && !s->splash.is_flipped) {
                        s->splash.is_flipped = true;
                        s->splash.offset = 0;
                    }

                    if (s->splash.offset >= 32 && s->splash.is_flipped) {
                        s->splash.is_flipped = false;
                        s->splash.offset = 32;
                    }

                    if(sf->frame&ANIM_FRAME_DOUBLE_TIME) {
                        frame_addr = PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED_FRAME1;
                    } else {
                        frame_addr = PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED_FRAME2;
                    }

                    pw_img_t img = {
                        .width=32, 
                        .height=24,
                        .data=decompression_buf,
                        .size=PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED_FRAME, 
                        .is_flipped=s->splash.is_flipped,
                        .lookup_table = {
                            .addr=frame_addr,
                            .use_alt=true
                        }
                    };
                    pw_eeprom_read(frame_addr, img.data, PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED_FRAME);

                    pw_screen_clear_area(origin, 0, s->splash.offset, 48);
                    pw_screen_clear_area(origin+s->splash.offset+32, 0, PW_SCREEN_WIDTH - (origin+s->splash.offset+32), 48);
                    pw_screen_draw_img(&img, origin + s->splash.offset, 24);
                    
                    if (s->splash.anim_frame >= 3 && s->splash.anim_frame % 2 == 0) {
                        if (img.is_flipped) s->splash.offset += 4;
                        else s->splash.offset -= 4;
                        s->splash.anim_frame = 0;
                    }
                    s->splash.anim_frame++;
                }
                else {
                    s->splash.walking = WALKING;
                    s->splash.offset = (PW_SCREEN_WIDTH - origin);
                    s->splash.anim_frame = 0;
                }
                break;
            }
        }
    }

    pw_screen_pos_t left_x = pw_screen_draw_integer_with_overline(health_data_cache.today_steps, PW_SCREEN_WIDTH, PW_SCREEN_HEIGHT-16, PW_SCREEN_BLACK);
    pw_screen_draw_horiz_line(0, PW_SCREEN_HEIGHT-16, left_x, PW_SCREEN_BLACK);

}

void pw_splash_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    (void)sf;
    switch(s->splash.current_substate) {
        case SPLASH_NORMAL: {
            if(s->splash.last_check_frame != sf->frame) {
                s->splash.last_check_frame = sf->frame;
                if(pw_accel_get_activity() != 0) {
                    pw_accel_process_steps();
                }
            }
            break;
        }
        case SPLASH_GO_TO_MENU: {
            p->sid = STATE_MAIN_MENU;
            p->menu.cursor = s->splash.menu_cursor;
            break;
        }
        default: {
            // nothing to do
            break;
        }
    }
}


