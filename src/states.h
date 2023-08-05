#ifndef PW_STATES_H
#define PW_STATES_H

#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

/// @file states.h

#define PW_REQUEST_REDRAW       (1<<0)

#define PW_CLR_REQUEST(x, y) x &= ~(y)
#define PW_SET_REQUEST(x, y) x |=  (y)
#define PW_GET_REQUEST(x, y) (x&(y))

typedef enum {
    STATE_SCREENSAVER,
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_POKE_RADAR,
    STATE_DOWSING,
    STATE_CONNECT,
    STATE_TRAINER_CARD,
    STATE_INVENTORY,
    STATE_SETTINGS,
    STATE_ERROR,
    STATE_FIRST_CONNECT,
    STATE_BATTLE,
    N_STATES,
} pw_state_id_t;

typedef struct {
    uint8_t pad;
} app_screensaver_t;

typedef struct {
    pw_inventory_t inventory;
} app_splash_t;

/*
 *  Message index + 1
 *  Offset from eeprom 0x4330
 */
typedef enum {
    MSG_NONE,
    MSG_NEED_WATTS,
    MSG_NO_POKEMON_HELD,
    MSG_NOTHING_HELD,
} pw_menu_message_id_t;

typedef struct {
    int8_t cursor;
    pw_menu_message_id_t message;
    bool redraw_message;
    bool transition;
} app_menu_t;

typedef struct {
    int8_t user_cursor;
    int8_t active_bush;
    uint8_t current_substate;
    uint8_t previous_substate;
    uint8_t chosen_pokemon;
    uint8_t radar_level;
    uint8_t current_level;
    int8_t active_timer;
    int8_t invisible_timer;
    int8_t begin_timer;
    bool input_accepted;
} app_radar_t;

typedef struct {
    uint8_t item_position;
    uint8_t chosen_positions;
    uint8_t choices_remaining;
    uint16_t chosen_item;
    uint8_t chosen_item_index;
    uint8_t bush_shakes;
    uint8_t user_input;
} app_dowsing_t;

typedef struct {
    uint8_t current_substate;
    uint8_t advertising_attempts;
    uint8_t screen_state;
} app_connect_t;

typedef struct {
    uint8_t current_substate;
    uint8_t previous_substate;
} app_trainer_card_t;

typedef struct {
    int8_t cursor;
    uint8_t current_substate;
    uint8_t previous_substate;
} app_inventory_t;

typedef struct {} pw_settings_t;

typedef struct {
    uint8_t current_substate;
    uint8_t previous_substate;
    uint8_t current_hp; // lo= , hi=
    uint8_t chosen_pokemon; // 0..3
    int8_t  anim_frame;
    uint8_t actions;
    uint8_t substate_queue_index;
    uint8_t substate_queue_len;
    int8_t  switch_cursor;
    int8_t  prev_switch_cursor;
} app_battle_t;

typedef struct {
    uint8_t sid;
    uint8_t requests;   // [0]=redraw
    union {
        app_screensaver_t screensaver;
        app_splash_t splash;
        app_menu_t menu;
        app_connect_t connect;
        app_radar_t radar;
        app_dowsing_t dowsing;
        app_trainer_card_t trainer_card;
        app_inventory_t inventory;
        app_battle_t battle;
    };
} pw_state_t;

typedef struct {
    uint8_t frame;
} screen_flags_t;

#define ANIM_FRAME_NORMAL_TIME_OFFSET       1
#define ANIM_FRAME_NORMAL_TIME              (1<<ANIM_FRAME_NORMAL_TIME_OFFSET)  // every half second
#define ANIM_FRAME_DOUBLE_TIME_OFFSET       0
#define ANIM_FRAME_DOUBLE_TIME              (1<<ANIM_FRAME_DOUBLE_TIME_OFFSET)  // every quarter second

typedef void (*state_loop_func_t)(pw_state_t* s, pw_state_t *p, const screen_flags_t *sf);
typedef void (*state_void_func_t)(pw_state_t* s, const screen_flags_t *sf);
typedef void (*state_input_func_t)(pw_state_t* s, const screen_flags_t *sf, uint8_t b);

typedef struct {
    state_loop_func_t loop;
    state_void_func_t draw_init;
    state_void_func_t draw_update;
    state_void_func_t init;
    state_void_func_t deinit;
    state_input_func_t input;
} state_funcs_t;

/*
 *  Use an array of structures to represent each state.
 *  Each state should have:
 *  - name/string
 *  - initialiser function
 *  - event loop function
 *  - input handler function
 *  - draw initialiser function
 *  - draw update function
 */
extern const char* const state_strings[];
extern const state_funcs_t STATE_FUNCS[];

pw_state_t pw_get_state();

void pw_state_init();
void pw_state_run_event_loop();
void pw_state_handle_input(uint8_t b);
void pw_state_draw_init();
void pw_state_draw_update();

/*
 *  State functions
 */
void pw_empty_event(pw_state_t *s, screen_flags_t *sf);
void pw_empty_input(pw_state_t *s, screen_flags_t *sf, uint8_t b);

// STATE_ERROR
void pw_error_init_display(pw_state_t *s, screen_flags_t *sf);

#endif /* PW_STATES_H */

