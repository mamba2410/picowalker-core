/// @file picowalker.c

#include <stdint.h>
#include <stdbool.h>

#include "picowalker.h"
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

struct {
    uint64_t now;
    uint64_t prev_screen_redraw;
    uint64_t prev_accel_check;
} walker_timings;


void walker_setup() {
    // Setup IR uart and rx interrupts
    pw_ir_init();
    pw_button_init();
    pw_screen_init();
    pw_eeprom_init();
    pw_accel_init();
    pw_srand(0x12345678);

    if(!pw_eeprom_check_for_nintendo()) {
        pw_eeprom_reset(true, true);
    }

    pw_eeprom_reliable_read(
        PW_EEPROM_ADDR_IDENTITY_DATA_1,
        PW_EEPROM_ADDR_IDENTITY_DATA_2,
        (uint8_t*)&walker_info_cache,
        sizeof(walker_info_cache)
    );

    pw_eeprom_reliable_read(
        PW_EEPROM_ADDR_HEALTH_DATA_1,
        PW_EEPROM_ADDR_HEALTH_DATA_2,
        (uint8_t*)&health_data_cache,
        sizeof(health_data_cache)
    );

    // swap BE in eeprom to LE in host
    health_data_cache.today_steps   = swap_bytes_u32(health_data_cache.today_steps);
    health_data_cache.total_steps   = swap_bytes_u32(health_data_cache.total_steps);
    health_data_cache.last_sync     = swap_bytes_u32(health_data_cache.last_sync);
    health_data_cache.total_days    = swap_bytes_u16(health_data_cache.total_days);
    health_data_cache.current_watts = swap_bytes_u16(health_data_cache.current_watts);

    if(walker_info_cache.flags & WALKER_INFO_FLAG_INIT) {
        pw_set_state(STATE_SPLASH);
    } else {
        pw_set_state(STATE_FIRST_CONNECT);
    }

    walker_timings.now = pw_now_us();
    walker_timings.prev_accel_check = 0;

}

pw_state_t a1, a2;
pw_state_t *current_state = &a1, *pending_state = &a2;
screen_flags_t screen_flags;

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
    }
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

