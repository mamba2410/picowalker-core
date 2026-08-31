#include "app_settings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../audio.h"
#include "../buttons.h"
#include "../eeprom.h"
#include "../eeprom_map.h"
#include "../globals.h"
#include "../pico_roms.h"
#include "../screen.h"

#define FALLTHROUGH __attribute__((fallthrough))

#define N_MAIN_OPTIONS  3
#define N_SOUND_OPTIONS 3
#define N_SHADE_OPTIONS 10

enum {
    SETTINGS_TOP_LEVEL,
    SETTINGS_SOUND,
    SETTINGS_SHADE,
    SETTINGS_GO_TO_MENU,
    SETTINGS_GO_TO_SPLASH,
    SETTINGS_GO_TO_PICOWALKER,
};

static pw_screen_pos_t cursor_positions[N_MAIN_OPTIONS][2] = {{0, 20}, {48, 20}, {0, 44}};

void pw_settings_init(pw_state_t *s, const screen_flags_t *sf) {
    (void)sf;
    s->settings.current_substate = SETTINGS_TOP_LEVEL;
    s->settings.main_cursor = 0;
    s->settings.sub_cursor = 0;
    s->settings.last_sub_cursor = 0;
}

void pw_settings_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    (void)sf;
    switch (s->settings.current_substate) {
        case SETTINGS_TOP_LEVEL: {
            // nothing to do
            break;
        }
        case SETTINGS_SOUND: {
            if (s->settings.last_sub_cursor != s->settings.sub_cursor) {
                s->settings.last_sub_cursor = s->settings.sub_cursor;
                pw_audio_set_volume(s->settings.sub_cursor);
                pw_audio_play_sound(SOUND_CURSOR_MOVE);
            }
            break;
        }
        case SETTINGS_SHADE: {
            if (s->settings.last_sub_cursor != s->settings.sub_cursor) {
                s->settings.last_sub_cursor = s->settings.sub_cursor;
                pw_screen_set_brightness(s->settings.sub_cursor);
                pw_audio_play_sound(SOUND_CURSOR_MOVE);
            }
            break;
        }
        case SETTINGS_GO_TO_SPLASH: {
            health_data_cache.settings &= ~SETTINGS_SHADE_MASK;
            health_data_cache.settings |= s->settings.sub_cursor << SETTINGS_SHADE_OFFSET;
            health_data_cache.settings &= ~SETTINGS_SOUND_MASK;
            health_data_cache.settings |= s->settings.sub_cursor << SETTINGS_SOUND_OFFSET;

            // Save settings byte by saving health data
            pw_eeprom_write_health_data(&health_data_cache);

            pw_audio_play_sound(SOUND_NAVIGATE_MENU);
            p->sid = STATE_SPLASH;
            break;
        }
        case SETTINGS_GO_TO_MENU: {
            p->sid = STATE_MAIN_MENU;
            p->menu.cursor = 5;
            break;
        }
        case SETTINGS_GO_TO_PICOWALKER: {
            p->sid = STATE_PICOWALKER;
            break;
        }
    }
}

void pw_settings_init_display(pw_state_t *s, const screen_flags_t *sf) {
    switch (s->settings.current_substate) {
        case SETTINGS_TOP_LEVEL: {
            pw_screen_draw_from_eeprom(
                8, 0, 80, 16, PW_EEPROM_ADDR_IMG_MENU_TITLE_SETTINGS, PW_EEPROM_SIZE_IMG_MENU_TITLE_SETTINGS, false);
            pw_screen_draw_from_eeprom(
                0, 0, 8, 16, PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN, PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN, true);
            pw_screen_draw_from_eeprom(
                8, 16, 40, 16, PW_EEPROM_ADDR_IMG_SOUND_FRAME, PW_EEPROM_SIZE_IMG_SOUND_FRAME, true);
            pw_screen_draw_from_eeprom(
                56, 16, 40, 16, PW_EEPROM_ADDR_IMG_SHADE_FRAME, PW_EEPROM_SIZE_IMG_SHADE_FRAME, true);

            pw_img_t img = (pw_img_t){.width = 88,
                .height = 16,
                .data = picowalker_fancy_text,
                .size = 88 * 16 / 4,
                .is_flipped = false,
                .lookup_table = {.addr = -1, .use_alt = false}};
            pw_screen_draw_img(&img, 8, 40);
            break;
        }
        case SETTINGS_SOUND: {
            pw_screen_clear_area(0, 32, PW_SCREEN_WIDTH, PW_SCREEN_HEIGHT / 2);
            pw_screen_draw_from_eeprom(s->settings.main_cursor * 48, 16 + 4, 8, 8,
                PW_EEPROM_ADDR_IMG_ARROW_RIGHT_INVERT, PW_EEPROM_SIZE_IMG_ARROW, true);
            pw_screen_draw_from_eeprom(
                8, 40, 24, 16, PW_EEPROM_ADDR_IMG_SPEAKER_OFF, PW_EEPROM_SIZE_IMG_SPEAKER_OFF, true);
            pw_screen_draw_from_eeprom(
                40, 40, 24, 16, PW_EEPROM_ADDR_IMG_SPEAKER_LOW, PW_EEPROM_SIZE_IMG_SPEAKER_LOW, true);
            pw_screen_draw_from_eeprom(
                72, 40, 24, 16, PW_EEPROM_ADDR_IMG_SPEAKER_HIGH, PW_EEPROM_SIZE_IMG_SPEAKER_HIGH, true);

            pw_eeprom_addr_t addr = sf->frame & ANIM_FRAME_NORMAL_TIME ? PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL
                                                                       : PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
            pw_screen_draw_from_eeprom(s->settings.sub_cursor * 32, 40 + 4, 8, 8, addr, PW_EEPROM_SIZE_IMG_ARROW, true);

            break;
        }
        case SETTINGS_SHADE: {
            pw_screen_draw_from_eeprom(s->settings.main_cursor * 48, 16 + 4, 8, 8,
                PW_EEPROM_ADDR_IMG_ARROW_RIGHT_INVERT, PW_EEPROM_SIZE_IMG_ARROW, true);
            pw_img_t shade_bars = {.width = 8,
                .height = 16,
                .data = eeprom_buf,
                .size = PW_EEPROM_ADDR_IMG_CONTRAST_DEMONSTRATOR,
                .is_flipped = false,
                .lookup_table = {.addr = PW_EEPROM_ADDR_IMG_CONTRAST_DEMONSTRATOR, .use_alt = true}};
            pw_eeprom_read(
                PW_EEPROM_ADDR_IMG_CONTRAST_DEMONSTRATOR, eeprom_buf, PW_EEPROM_SIZE_IMG_CONTRAST_DEMONSTRATOR);

            for (size_t i = 0; i < N_SHADE_OPTIONS; i++) {
                pw_screen_draw_img(&shade_bars, 8 + i * 8, 40);
            }

            pw_eeprom_addr_t addr = sf->frame & ANIM_FRAME_NORMAL_TIME ? PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL
                                                                       : PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;
            pw_screen_draw_from_eeprom(8 + s->settings.sub_cursor * 8, 32, 8, 8, addr, PW_EEPROM_SIZE_IMG_ARROW, true);

            pw_screen_pos_t x = 8 + N_SHADE_OPTIONS * 8;
            pw_screen_clear_area(x, 40, PW_SCREEN_WIDTH - x, 16);

            break;
        }
    }
}

void pw_settings_update_display(pw_state_t *s, const screen_flags_t *sf) {
    if (s->settings.current_substate != s->settings.previous_substate) {
        pw_settings_init_display(s, sf);
        s->settings.previous_substate = s->settings.current_substate;
        return;
    }

    switch (s->settings.current_substate) {
        case SETTINGS_TOP_LEVEL: {
            pw_eeprom_addr_t addr = sf->frame & ANIM_FRAME_NORMAL_TIME ? PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL
                                                                       : PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
            uint8_t c = s->settings.main_cursor;
            pw_screen_draw_from_eeprom(
                // s->settings.main_cursor*48, 16+4,
                cursor_positions[c][0], cursor_positions[c][1], 8, 8, addr, PW_EEPROM_SIZE_IMG_ARROW, true);
            for (int i = 0; i < N_MAIN_OPTIONS; i++) {
                if (i == s->settings.main_cursor) continue;
                pw_screen_clear_area(
                    // i*48, 16+4,
                    cursor_positions[i][0], cursor_positions[i][1], 8, 8);
            }
            break;
        }
        case SETTINGS_SOUND: {
            pw_eeprom_addr_t addr = sf->frame & ANIM_FRAME_NORMAL_TIME ? PW_EEPROM_ADDR_IMG_ARROW_RIGHT_NORMAL
                                                                       : PW_EEPROM_ADDR_IMG_ARROW_RIGHT_OFFSET;
            pw_screen_draw_from_eeprom(s->settings.sub_cursor * 32, 40 + 4, 8, 8, addr, PW_EEPROM_SIZE_IMG_ARROW, true);
            for (int i = 0; i < N_SOUND_OPTIONS; i++) {
                if (i == s->settings.sub_cursor) continue;
                pw_screen_clear_area(i * 32, 40 + 4, 8, 8);
            }

            break;
            case SETTINGS_SHADE: {
                pw_eeprom_addr_t addr = sf->frame & ANIM_FRAME_NORMAL_TIME ? PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL
                                                                           : PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;
                pw_screen_draw_from_eeprom(
                    8 + s->settings.sub_cursor * 8, 32, 8, 8, addr, PW_EEPROM_SIZE_IMG_ARROW, true);
                for (int i = 0; i < N_SHADE_OPTIONS; i++) {
                    if (i == s->settings.sub_cursor) continue;
                    pw_screen_clear_area(8 + i * 8, 32, 8, 8);
                }

                break;
            }
        }
    }
}

void pw_settings_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b) {
    (void)sf;
    switch (s->settings.current_substate) {
        case SETTINGS_TOP_LEVEL: {
            switch (b) {
                case PW_BUTTON_L: {
                    if (s->settings.main_cursor == 0) {
                        s->settings.current_substate = SETTINGS_GO_TO_MENU;
                        pw_audio_play_sound(SOUND_NAVIGATE_BACK);
                        break;
                    }
                    s->settings.main_cursor = (s->settings.main_cursor - 1) % N_MAIN_OPTIONS;
                    pw_audio_play_sound(SOUND_CURSOR_MOVE);
                    break;
                }
                case PW_BUTTON_R: {
                    s->settings.main_cursor = (s->settings.main_cursor + 1) % N_MAIN_OPTIONS;
                    pw_audio_play_sound(SOUND_CURSOR_MOVE);
                    break;
                }
                case PW_BUTTON_M: {
                    if (s->settings.main_cursor == 0) {
                        s->settings.current_substate = SETTINGS_SOUND;
                        s->settings.sub_cursor =
                            (health_data_cache.settings & SETTINGS_SOUND_MASK) >> SETTINGS_SOUND_OFFSET;
                        s->settings.last_sub_cursor =
                            (health_data_cache.settings & SETTINGS_SOUND_MASK) >> SETTINGS_SOUND_OFFSET;
                    } else if (s->settings.main_cursor == 1) {
                        s->settings.current_substate = SETTINGS_SHADE;
                        s->settings.sub_cursor =
                            (health_data_cache.settings & SETTINGS_SHADE_MASK) >> SETTINGS_SHADE_OFFSET;
                        s->settings.last_sub_cursor =
                            (health_data_cache.settings & SETTINGS_SHADE_MASK) >> SETTINGS_SHADE_OFFSET;
                    } else if (s->settings.main_cursor == 2) {
                        s->settings.current_substate = SETTINGS_GO_TO_PICOWALKER;
                        pw_audio_play_sound(SOUND_NAVIGATE_MENU);
                    }
                    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
                    break;
                }
            }
            break;
        }
        case SETTINGS_SOUND: {
            switch (b) {
                case PW_BUTTON_L: {
                    s->settings.sub_cursor = (s->settings.sub_cursor - 1 + N_SOUND_OPTIONS) % N_SOUND_OPTIONS;
                    break;
                }
                case PW_BUTTON_R: {
                    s->settings.sub_cursor = (s->settings.sub_cursor + 1) % N_SOUND_OPTIONS;
                    break;
                }
                case PW_BUTTON_M: {
                    s->settings.current_substate = SETTINGS_GO_TO_SPLASH;
                    break;
                }
            }
            break;
        }
        case SETTINGS_SHADE: {
            switch (b) {
                case PW_BUTTON_L: {
                    s->settings.sub_cursor = (s->settings.sub_cursor - 1 + N_SHADE_OPTIONS) % N_SHADE_OPTIONS;
                    break;
                }
                case PW_BUTTON_R: {
                    s->settings.sub_cursor = (s->settings.sub_cursor + 1) % N_SHADE_OPTIONS;
                    break;
                }
                case PW_BUTTON_M: {
                    s->settings.current_substate = SETTINGS_GO_TO_SPLASH;
                    break;
                }
            }
            break;
        }
    }
    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}
