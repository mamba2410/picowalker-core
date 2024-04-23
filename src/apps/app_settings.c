#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "app_settings.h"
#include "../eeprom_map.h"
#include "../screen.h"
#include "../audio.h"
#include "../buttons.h"
#include "../globals.h"
#include "../eeprom.h"

#define N_MAIN_OPTIONS 2
#define N_SOUND_OPTIONS 3
#define N_SHADE_OPTIONS 10

enum {
    SETTINGS_TOP_LEVEL,
    SETTINGS_SOUND,
    SETTINGS_SHADE,
    SETTINGS_GO_TO_MENU,
    SETTINGS_GO_TO_SPLASH,
};

void pw_settings_init(pw_state_t *s, const screen_flags_t *sf) {
    s->settings.current_substate = SETTINGS_TOP_LEVEL;
    s->settings.main_cursor = 0;
    s->settings.sub_cursor = 0;
}

void pw_settings_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    switch(s->settings.current_substate) {
    case SETTINGS_TOP_LEVEL: {
        // nothing to do
        break;
    }
    case SETTINGS_GO_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        break;
    }
    case SETTINGS_GO_TO_MENU: {
        p->sid = STATE_MAIN_MENU;
        p->menu.cursor = 5;
        break;
    }
    }
}

void pw_settings_init_display(pw_state_t *s, const screen_flags_t *sf) {
    switch(s->settings.current_substate) {
    case SETTINGS_TOP_LEVEL: {
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
        pw_screen_draw_from_eeprom(
            8, 16,
            40, 16,
            PW_EEPROM_ADDR_IMG_SOUND_FRAME,
            PW_EEPROM_SIZE_IMG_SOUND_FRAME
        );
        pw_screen_draw_from_eeprom(
            56, 16,
            40, 16,
            PW_EEPROM_ADDR_IMG_SHADE_FRAME,
            PW_EEPROM_SIZE_IMG_SHADE_FRAME
        );
        break;
    }
    case SETTINGS_SOUND: {
        pw_screen_draw_from_eeprom(
            s->settings.main_cursor*48, 16+4,
            8, 8,
            PW_EEPROM_ADDR_IMG_ARROW_RIGHT_INVERT,
            PW_EEPROM_SIZE_IMG_ARROW
        );
        pw_screen_draw_from_eeprom(
            8, 40,
            24, 16,
            PW_EEPROM_ADDR_IMG_SPEAKER_OFF,
            PW_EEPROM_SIZE_IMG_SPEAKER_OFF
        );
        pw_screen_draw_from_eeprom(
            40, 40,
            24, 16,
            PW_EEPROM_ADDR_IMG_SPEAKER_LOW,
            PW_EEPROM_SIZE_IMG_SPEAKER_LOW
        );
        pw_screen_draw_from_eeprom(
            72, 40,
            24, 16,
            PW_EEPROM_ADDR_IMG_SPEAKER_HIGH,
            PW_EEPROM_SIZE_IMG_SPEAKER_HIGH
        );

        eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL:
                             PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
        pw_screen_draw_from_eeprom(
            s->settings.sub_cursor*32, 40+4,
            8, 8,
            addr,
            PW_EEPROM_SIZE_IMG_ARROW
        );

        break;
    }
    case SETTINGS_SHADE: {
        pw_screen_draw_from_eeprom(
            s->settings.main_cursor*48, 16+4,
            8, 8,
            PW_EEPROM_ADDR_IMG_ARROW_RIGHT_INVERT,
            PW_EEPROM_SIZE_IMG_ARROW
        );
        pw_img_t shade_bars = {
            .width=8, .height=16,
            .data=eeprom_buf, .size=PW_EEPROM_ADDR_IMG_CONTRAST_DEMONSTRATOR
        };
        pw_eeprom_read(
            PW_EEPROM_ADDR_IMG_CONTRAST_DEMONSTRATOR,
            eeprom_buf,
            PW_EEPROM_SIZE_IMG_CONTRAST_DEMONSTRATOR
        );

        for(size_t i = 0; i < N_SHADE_OPTIONS; i++) {
            pw_screen_draw_img(&shade_bars, 8+i*8, 40);
        }

        eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL:
                             PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;
        pw_screen_draw_from_eeprom(
            8+s->settings.sub_cursor*8, 32,
            8, 8,
            addr,
            PW_EEPROM_SIZE_IMG_ARROW
        );

        break;
    }
    }
}

void pw_settings_update_display(pw_state_t *s, const screen_flags_t *sf) {
    if(s->settings.current_substate != s->settings.previous_substate) {
        pw_settings_init_display(s, sf);
        s->settings.previous_substate = s->settings.current_substate;
        return;
    }

    switch(s->settings.current_substate) {
    case SETTINGS_TOP_LEVEL: {
        eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL:
                             PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
        pw_screen_draw_from_eeprom(
            s->settings.main_cursor*48, 16+4,
            8, 8,
            addr,
            PW_EEPROM_SIZE_IMG_ARROW
        );
        for(size_t i = 0; i < 2; i++) {
            if(i == s->settings.main_cursor) continue;
            pw_screen_clear_area(
                i*48, 16+4,
                8, 8
            );
        }
        break;
    }
    case SETTINGS_SOUND: {

        eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL:
                             PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
        pw_screen_draw_from_eeprom(
            s->settings.sub_cursor*32, 40+4,
            8, 8,
            addr,
            PW_EEPROM_SIZE_IMG_ARROW
        );
        for(size_t i = 0; i < N_SOUND_OPTIONS; i++) {
            if(i == s->settings.sub_cursor) continue;
            pw_screen_clear_area(
                i*32, 40+4,
                8, 8
            );
        }

        break;
        case SETTINGS_SHADE: {
            eeprom_addr_t addr = sf->frame&ANIM_FRAME_NORMAL_TIME?PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL:
                                 PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;
            pw_screen_draw_from_eeprom(
                8+s->settings.sub_cursor*8, 32,
                8, 8,
                addr,
                PW_EEPROM_SIZE_IMG_ARROW
            );
            for(size_t i = 0; i < N_SHADE_OPTIONS; i++) {
                if(i == s->settings.sub_cursor) continue;
                pw_screen_clear_area(
                    8+i*8, 32,
                    8, 8
                );
            }

            break;
        }
    }
    }
}

void pw_settings_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(s->settings.current_substate) {
    case SETTINGS_TOP_LEVEL: {
        switch(b) {
        case BUTTON_L: {
            if(s->settings.main_cursor == 0) {
                s->settings.current_substate = SETTINGS_GO_TO_MENU;
		pw_audio_play_sound(SOUND_NAVIGATE_BACK);
                break;
            }
	    pw_audio_play_sound(SOUND_CURSOR_MOVE);
            // fall through otherwise
        }
        case BUTTON_R: {
            s->settings.main_cursor = (s->settings.main_cursor+1)%2;
	    pw_audio_play_sound(SOUND_CURSOR_MOVE);
            break;
        }
        case BUTTON_M: {
            if(s->settings.main_cursor == 0) {
                s->settings.current_substate = SETTINGS_SOUND;
                s->settings.sub_cursor = (health_data_cache.settings&SETTINGS_SOUND_MASK)>>SETTINGS_SOUND_OFFSET;
            } else {
                s->settings.current_substate = SETTINGS_SHADE;
                s->settings.sub_cursor = (health_data_cache.settings&SETTINGS_SHADE_MASK)>>SETTINGS_SHADE_OFFSET;
            }
            PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
            break;
        }
        }
        break;
    }
    case SETTINGS_SOUND: {
        switch(b) {
        case BUTTON_L: {
            // TODO: update settings
            s->settings.sub_cursor = (s->settings.sub_cursor-1+N_SOUND_OPTIONS)%N_SOUND_OPTIONS;
            health_data_cache.settings &= ~SETTINGS_SOUND_MASK;
            health_data_cache.settings |= s->settings.sub_cursor<<SETTINGS_SOUND_OFFSET;
	    pw_audio_volume = s->settings.sub_cursor;
	    pw_audio_play_sound(SOUND_CURSOR_MOVE);
            break;
        }
        case BUTTON_R: {
            // TODO: update settings
            s->settings.sub_cursor = (s->settings.sub_cursor+1)%N_SOUND_OPTIONS;
            health_data_cache.settings &= ~SETTINGS_SOUND_MASK;
            health_data_cache.settings |= s->settings.sub_cursor<<SETTINGS_SOUND_OFFSET;
	    pw_audio_volume = s->settings.sub_cursor;
	    pw_audio_play_sound(SOUND_CURSOR_MOVE);
            break;
        }
        case BUTTON_M: {
            s->settings.current_substate = SETTINGS_GO_TO_SPLASH;
	    pw_audio_play_sound(SOUND_NAVIGATE_MENU);
            break;
        }
        }
        break;
    }
    case SETTINGS_SHADE: {
        switch(b) {
        case BUTTON_L: {
            s->settings.sub_cursor = (s->settings.sub_cursor-1+N_SHADE_OPTIONS)%N_SHADE_OPTIONS;
            health_data_cache.settings &= ~SETTINGS_SHADE_MASK;
            health_data_cache.settings |= s->settings.sub_cursor<<SETTINGS_SHADE_OFFSET;
	    pw_audio_play_sound(SOUND_CURSOR_MOVE);
            break;
        }
        case BUTTON_R: {
            s->settings.sub_cursor = (s->settings.sub_cursor+1)%N_SHADE_OPTIONS;
            health_data_cache.settings &= ~SETTINGS_SHADE_MASK;
            health_data_cache.settings |= s->settings.sub_cursor<<SETTINGS_SHADE_OFFSET;
	    pw_audio_play_sound(SOUND_CURSOR_MOVE);
            break;
        }
        case BUTTON_M: {
            s->settings.current_substate = SETTINGS_GO_TO_SPLASH;
	    pw_audio_play_sound(SOUND_NAVIGATE_MENU);
            break;
        }
        }
        break;
    }
    }
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

