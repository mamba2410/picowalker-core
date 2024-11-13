/// @file picowalker.c

#include <stdint.h>
#include <stdbool.h>

#include "picowalker.h"
#include "picowalker-defs.h"
#include "buttons.h"
#include "screen.h"
#include "states.h"
#include "rand.h"
#include "states.h"
#include "timer.h"
#include "globals.h"
#include "utils.h"
#include "ir/ir.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "accel.h"
#include "power.h"

struct {
    uint64_t now;
    uint64_t prev_screen_redraw;
    uint64_t prev_accel_check;
} walker_timings;

pw_state_t a1, a2;
pw_state_t *current_state = &a1, *pending_state = &a2;
screen_flags_t screen_flags;

void walker_setup() {
    // Setup IR uart and rx interrupts
    pw_battery_init();
    pw_eeprom_init();
    pw_accel_init();
    pw_ir_init();
    pw_button_init();
    pw_screen_init();
    pw_srand(0x12345678);

    if(!pw_eeprom_check_for_nintendo()) {
        printf("No nintendo found!\n");
        pw_eeprom_reset(true, true);
    }

    int read_res;
    read_res = pw_eeprom_read_walker_info(&walker_info_cache);
    read_res = pw_eeprom_read_health_data(&health_data_cache);

    if(walker_info_cache.flags & WALKER_INFO_FLAG_INIT) {
        current_state->sid = STATE_SPLASH;
        pending_state->sid = STATE_SPLASH;
    } else {
        current_state->sid = STATE_FIRST_COMMS;
        pending_state->sid = STATE_FIRST_COMMS;
    }

    walker_timings.now = pw_now_us();
    walker_timings.prev_accel_check = 0;

    // Initialise the first states
    pw_screen_clear();
    STATE_FUNCS[current_state->sid].init(current_state, &screen_flags);
    STATE_FUNCS[current_state->sid].draw_init(current_state, &screen_flags);
}


void walker_loop() {
    uint64_t td;

    // TODO: Things to do regardless of state (eg check steps, battery etc.)
    walker_timings.now = pw_now_us();
    td = (walker_timings.prev_accel_check>walker_timings.now)?(walker_timings.prev_accel_check-walker_timings.now):(walker_timings.now-walker_timings.prev_accel_check);
    if(td > ACCEL_NORMAL_SAMPLE_TIME_US) {
        walker_timings.prev_accel_check = walker_timings.now;
        pw_accel_process_steps();
    }

    // Run current state's event loop
    STATE_FUNCS[current_state->sid].loop(current_state, pending_state, &screen_flags);

    // TODO: invalid sid checking
    if(pending_state->sid != current_state->sid) {
        STATE_FUNCS[current_state->sid].deinit(current_state, &screen_flags);

        pw_state_t *tmp = current_state;
        current_state = pending_state;
        pending_state = tmp;

        *pending_state = (pw_state_t) {
            0
        }; // clang-format why
        pending_state->sid = current_state->sid;

        pw_screen_clear();
        STATE_FUNCS[current_state->sid].init(current_state, &screen_flags);
        STATE_FUNCS[current_state->sid].draw_init(current_state, &screen_flags);
    }

    // Update screen since (presumably) we aren't doing anything time-critical
    walker_timings.now = pw_now_us();
    td = (walker_timings.prev_screen_redraw>walker_timings.now)?(walker_timings.prev_screen_redraw-walker_timings.now):(walker_timings.now-walker_timings.prev_screen_redraw);

    if(td > SCREEN_REDRAW_DELAY_US || PW_GET_REQUEST(current_state->requests, PW_REQUEST_REDRAW)) {
        walker_timings.prev_screen_redraw = walker_timings.now;
        STATE_FUNCS[current_state->sid].draw_update(current_state, &screen_flags);
        screen_flags.frame = (screen_flags.frame+1)%4;
        PW_CLR_REQUEST(current_state->requests, PW_REQUEST_REDRAW);
    }

    // Check if we should sleep
    walker_timings.now = pw_now_us();
    td = (power_context.last_user_action_time>walker_timings.now)?(power_context.last_user_action_time-walker_timings.now):(walker_timings.now-power_context.last_user_action_time);
    if(td > PW_POWER_SLEEP_TIMEOUT_MS) {
        printf("Sleep timeout hit, entering sleep\n");

        // Pass control to "driver" and enter sleep
        // Driver should bring all clocks, hardware etc back to how it was left
        pw_power_enter_sleep();

        // Update last action time so we don't immediately fall asleep again
        power_context.last_user_action_time = pw_now_us();

        // Re-draw the screen
        STATE_FUNCS[current_state->sid].draw_init(current_state, &screen_flags);
    }
}

void pw_state_handle_input(uint8_t b) {
    power_context.last_user_action_time = pw_now_us();
    STATE_FUNCS[current_state->sid].input(current_state, &screen_flags, b);
}


/**
 *  Entry for the picowalker
 */
void walker_entry() {

    walker_setup();

    // Event loop
    // BEWARE: Could (WILL) receive interrupts during this time
    while(true) {
        walker_loop();
    }

}

