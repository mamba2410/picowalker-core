#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <stdio.h>

#include "app_picowalker.h"
#include "../debug_log.h"
#include "../eeprom_map.h"
#include "../pico_roms.h"
#include "../picowalker_core.h"
#include "../picowalker_structures.h"
#include "../power.h"
#include "../globals.h"
#include "../eeprom.h"
#include "../screen.h"
#include "../audio.h"
#include "../states.h"

#define N_ENTRIES 1

// TODO: Move me
#define N_COLOR_MODES 4


enum {
    SUBSTATE_NORMAL,
    SUBSTATE_GO_TO_SPLASH,
    SUBSTATE_GO_TO_SETTINGS,
};

void pw_picowalker_settings_init(pw_state_t *s, const screen_flags_t *sf) {
    (void)sf;
    s->picowalker.current_substate = SUBSTATE_NORMAL;
    s->picowalker.cursor = 0;
}


void pw_picowalker_settings_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    (void)sf;
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
            pw_log_error("Unknown substate %d in %s",
                    s->picowalker.current_substate,
                    state_strings[s->sid]);
            break;
        }
    }
}


void pw_picowalker_settings_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b) {
    (void)sf;
    switch(b) {
        case PW_BUTTON_L: {
            s->picowalker.cursor--;
            if(s->picowalker.cursor < 0) {
                s->picowalker.current_substate = SUBSTATE_GO_TO_SETTINGS;
            }
            pw_audio_play_sound(SOUND_NAVIGATE_BACK);
            break;
        }
        case PW_BUTTON_M: {
            if(s->picowalker.cursor == 0) {
                pw_color_mode = (pw_color_mode + 1) % N_COLOR_MODES;
                health_data_cache.color_mode = pw_color_mode;
                pw_eeprom_write_health_data(&health_data_cache);
            } else {
                s->picowalker.current_substate = SUBSTATE_GO_TO_SPLASH;
            }
            pw_audio_play_sound(SOUND_NAVIGATE_MENU);
            break;
        }
        case PW_BUTTON_R: {
            if(s->picowalker.cursor < N_ENTRIES-1) {
                s->picowalker.cursor++;
            }
            break;
        }
        default: {
            pw_log_error("Unknown input %d in substate %s\n",
                    b, state_strings[s->sid]);
            pw_audio_play_sound(SOUND_CURSOR_MOVE);
            break;
        }
    }
}


static void draw_color_option(pw_screen_pos_t x, pw_screen_pos_t y) {
    pw_img_t img = (pw_img_t) {
        .width = 48,
        .height = 16,
        .size = 48*16/4,
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=false
        }
    };

    if(pw_color_mode == (N_COLOR_MODES-1)) {
        img.data = color_fancy_text;
    } else {
        img.data = grey_fancy_text;
    }

    pw_screen_draw_img(&img, x, y);
    x = PW_SCREEN_WIDTH;
    x = pw_screen_draw_integer(pw_color_mode, x, y);
}

void pw_picowalker_settings_init_display(pw_state_t *s, const screen_flags_t *sf) {
    (void)s;
    (void)sf;
    pw_screen_pos_t x, y;

    s->picowalker.previous_color_mode = pw_color_mode;
    pw_screen_clear();
    
    // Title bar
    pw_img_t img = (pw_img_t){
        .width=80,
        .height=16,
        .data=picowalker_border_text,
        .size=80*16/4,
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=false
        }
    };
    pw_screen_draw_img(&img, 8, 0);
    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN,
        true
    );

    draw_color_option(8, 16);

    // Draw battery percentage
    y = 32;
    x = 8;
    img = (pw_img_t) {
        .width=48,
        .height=16,
        .data=battery_fancy_text,
        .size=32*16/4, // 48*16/4
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=false
        }
    };
    pw_screen_draw_img(&img, x, y);
    uint8_t percent = pw_power_get_battery();
    x = PW_SCREEN_WIDTH-8;
    x = pw_screen_draw_integer(percent, x, y);

    img = (pw_img_t) {
        .width=8,
        .height=16,
        .data=percent_char,
        .size=8*16/4,
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=false
        }
    };
    pw_screen_draw_img(&img, PW_SCREEN_WIDTH-8, y);
}


void pw_picowalker_settings_update_display(pw_state_t *s, const screen_flags_t *sf) {
    if(pw_color_mode != s->picowalker.previous_color_mode) {
        pw_picowalker_settings_init_display(s, sf);
        return;
    }

    pw_eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL:
                         PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
    pw_screen_draw_from_eeprom(
        0, 16+4+s->picowalker.cursor*16,
        8, 8,
        addr,
        PW_EEPROM_SIZE_IMG_ARROW,
        true
    );
    for(int8_t i = 0; i < N_ENTRIES; i++) {
        if(i == s->picowalker.cursor) continue;
        pw_screen_clear_area(
            0, 16+i*16,
            8, 8
        );
    }

    draw_color_option(8, 16);

    // Draw battery percentage
    pw_screen_pos_t y = 32;
    pw_screen_pos_t x = 8;
    pw_img_t img = (pw_img_t) {
        .width=48,
        .height=16,
        .data=battery_fancy_text,
        .size=32*16/4, // 48*16/4
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=false
        }
    };
    pw_screen_draw_img(&img, x, y);
    uint8_t percent = pw_power_get_battery();
    x = PW_SCREEN_WIDTH-8;
    x = pw_screen_draw_integer(percent, x, y);

    img = (pw_img_t) {
        .width=8,
        .height=16,
        .data=percent_char,
        .size=8*16/4,
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=false
        }
    };
    pw_screen_draw_img(&img, PW_SCREEN_WIDTH-8, y);
}

