#include <stdio.h>

#include "states.h"
#include "menu.h"
#include "buttons.h"
#include "screen.h"
#include "pico_roms.h"
#include "ir/ir.h"

#include "apps/app_splash.h"
#include "apps/app_trainer_card.h"
#include "apps/app_comms.h"
#include "apps/app_inventory.h"
#include "apps/app_dowsing.h"
#include "apps/app_poke_radar.h"
#include "apps/app_battle.h"
#include "apps/app_first_comms.h"

const char* const state_strings[N_STATES] = {
    [STATE_SCREENSAVER]     = "Screensaver",
    [STATE_SPLASH]          = "Splash screen",
    [STATE_MAIN_MENU]       = "Main menu",
    [STATE_POKE_RADAR]      = "Poke radar",
    [STATE_DOWSING]         = "Dowsing",
    [STATE_COMMS]           = "Connect",
    [STATE_TRAINER_CARD]    = "Trainer card",
    [STATE_INVENTORY]       = "Pokemon and items",
    [STATE_SETTINGS]        = "Settings",
    [STATE_ERROR]           = "Error",
    [STATE_FIRST_COMMS]     = "First connect",
};

// TODO: change function sigs
state_funcs_t const STATE_FUNCS[N_STATES] = {
    [STATE_SCREENSAVER]     = {
        .init=pw_empty_event,
        .loop=pw_send_to_splash,
        .input=pw_empty_input,
        .draw_init=pw_screen_clear,
        .draw_update=pw_screen_clear,
        .deinit=pw_empty_event,
    },
    [STATE_SPLASH]          = {
        .init=pw_splash_init,
        .loop=pw_splash_event_loop,
        .input=pw_splash_handle_input,
        .draw_init=pw_splash_init_display,
        .draw_update=pw_splash_update_display,
        .deinit=pw_empty_event,
    },
    [STATE_MAIN_MENU]       = {
        .init=pw_menu_init,
        .loop=pw_menu_event_loop,
        .input=pw_menu_handle_input,
        .draw_init=pw_menu_init_display,
        .draw_update=pw_menu_update_display,
        .deinit=pw_empty_event,
    },
    [STATE_POKE_RADAR]      = {
        .init=pw_poke_radar_init,
        .loop=pw_poke_radar_event_loop,
        .input=pw_poke_radar_handle_input,
        .draw_init=pw_poke_radar_init_display,
        .draw_update=pw_poke_radar_update_display,
        .deinit=pw_empty_event,
    },
    [STATE_BATTLE]          = {
        .init=pw_battle_init,
        .loop=pw_battle_event_loop,
        .input=pw_battle_handle_input,
        .draw_init=pw_battle_init_display,
        .draw_update=pw_battle_update_display,
        .deinit=pw_empty_event,
    },
    [STATE_DOWSING]         = {
        .init=pw_dowsing_init,
        .loop=pw_dowsing_event_loop,
        .input=pw_dowsing_handle_input,
        .draw_init=pw_dowsing_init_display,
        .draw_update=pw_dowsing_update_display,
        .deinit=pw_empty_event,
    },
    [STATE_COMMS]         = {
        .init=pw_comms_init,
        .loop=pw_comms_event_loop,
        .input=pw_comms_handle_input,
        .draw_init=pw_comms_init_display,
        .draw_update=pw_comms_draw_update,
        .deinit=pw_empty_event,
    },
    [STATE_TRAINER_CARD]    = {
        .init=pw_trainer_card_init,
        .loop=pw_trainer_card_event_loop,
        .input=pw_trainer_card_handle_input,
        .draw_init=pw_trainer_card_init_display,
        .draw_update=pw_trainer_card_draw_update,
        .deinit=pw_empty_event,
    },
    [STATE_INVENTORY]       = {
        .init=pw_inventory_init,
        .loop=pw_inventory_event_loop,
        .input=pw_inventory_handle_input,
        .draw_init=pw_inventory_init_display,
        .draw_update=pw_inventory_update_display,
        .deinit=pw_empty_event,
    },
    [STATE_SETTINGS]        = {
        .init=pw_empty_event,
        .loop=pw_send_to_splash,
        .input=pw_empty_input,
        .draw_init=pw_screen_clear,
        .draw_update=pw_empty_event,
        .deinit=pw_empty_event,
    },
    [STATE_ERROR]           = {
        .init=pw_empty_event,
        .loop=pw_send_to_splash,
        .input=pw_empty_input,
        .draw_init=pw_error_init_display,
        .draw_update=pw_empty_event,
        .deinit=pw_empty_event,
    },
    [STATE_FIRST_COMMS]   = {
        .init=pw_first_comms_init,
        .loop=pw_first_comms_event_loop,
        .input=pw_first_comms_handle_input,
        .draw_init=pw_first_comms_init_display,
        .draw_update=pw_first_comms_draw_update,
        .deinit=pw_empty_event,
    },
};

/*
 *  ========================================
 *  State functions
 *  ========================================
 */
void pw_empty_event(pw_state_t *s, const screen_flags_t *sf) {}
void pw_empty_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {}
void pw_send_to_splash(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    p->sid = STATE_SPLASH;
}

void pw_error_init_display(pw_state_t *s, const screen_flags_t *sf) {
    pw_img_t sad_pokewalker_img   = {.height=48, .width=48, .data=sad_pokewalker, .size=576};
    pw_screen_draw_img(&sad_pokewalker_img, 0, 0);

}

