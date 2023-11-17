#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../states.h"
#include "../buttons.h"
#include "../screen.h"
#include "../eeprom_map.h"
#include "../ir/ir.h"
#include "../ir/actions.h"
#include "../globals.h"
#include "app_comms.h"

/** @file app_comms.c
 * ```
 *  current_substate = comm_substate
 *  reg_b = screen_state
 *  reg_c = advertising_counter
 * ```
 */

enum {
    CSS_NORMAL,
    CSS_GO_TO_SPLASH,
};

void pw_comms_init(pw_state_t *s, const screen_flags_t *sf) {
    //pw_eeprom_write_health_data(&health_data_cache);
    //pw_eeprom_write_walker_info(&walker_info_cache);

    s->comms.current_substate = COMM_SUBSTATE_FINDING_PEER;
    s->comms.screen_state = CSS_NORMAL;
    s->comms.advertising_attempts = 0;  // advertising attempts
    pw_ir_set_comm_state(COMM_STATE_AWAITING);
}

void pw_comms_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {

    switch(s->comms.screen_state) {
    case CSS_NORMAL: {
        comm_state_t cs = pw_ir_get_comm_state();
        ir_err_t err = IR_ERR_UNHANDLED_ERROR;
        size_t n_rw;

        switch(cs) {
        case COMM_STATE_AWAITING: {
            err = pw_action_try_find_peer(&s->comms, &packet_buf, PACKET_BUF_SIZE);
            break;
        }
        case COMM_STATE_SLAVE: {
            err = pw_ir_recv_packet(&packet_buf, PACKET_BUF_SIZE, &n_rw);
            if(err == IR_OK || err == IR_ERR_SIZE_MISMATCH) {
                err = pw_action_slave_perform_request(&packet_buf, n_rw);
            }
            break;
        }
        case COMM_STATE_MASTER: {
            if(s->comms.current_substate == COMM_SUBSTATE_AWAITING_SLAVE_ACK)
                s->comms.current_substate = COMM_SUBSTATE_START_PEER_PLAY;
            err = pw_action_peer_play(&s->comms, &packet_buf, PACKET_BUF_SIZE);
            break;
        }
        case COMM_STATE_DISCONNECTED: {
            err = IR_OK;
            break;
        }
        } // switch(cs)

        if(err != IR_OK) {
            printf("\tError code: %02x: %s\n\tState: %d\n\tSubstate %d\n",
                   err, PW_IR_ERR_NAMES[err],
                   pw_ir_get_comm_state(),
                   s->comms.current_substate
                  );

            pw_ir_set_comm_state(COMM_STATE_DISCONNECTED);
        }

        break;
    }
    case CSS_GO_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        break;
    }
    } // screen_state

}

void pw_comms_init_display(pw_state_t *s, const screen_flags_t *sf) {

    pw_screen_draw_from_eeprom(
        (SCREEN_WIDTH-32)/2, SCREEN_HEIGHT-32-16,
        32, 32,
        PW_EEPROM_ADDR_IMG_POKEWALKER_BIG,
        PW_EEPROM_SIZE_IMG_POKEWALKER_BIG
    );
    pw_screen_draw_from_eeprom(
        (SCREEN_WIDTH-8)/2, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_IR_ARCS,
        PW_EEPROM_SIZE_IMG_IR_ARCS
    );
    pw_screen_draw_from_eeprom(
        0, SCREEN_HEIGHT-16,
        96, 16,
        PW_EEPROM_ADDR_TEXT_CONNECTING,
        PW_EEPROM_SIZE_TEXT_CONNECTING
    );

}

void pw_comms_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {

    switch(b) {
    case BUTTON_M:
        if(pw_ir_get_comm_state() == COMM_STATE_DISCONNECTED) {
            s->comms.screen_state = CSS_GO_TO_SPLASH;
        }
        break;
    case BUTTON_L:
    case BUTTON_R:
    default:
        break;
    }

}

void pw_comms_draw_update(pw_state_t *s, const screen_flags_t *sf) {

    if(sf->frame & ANIM_FRAME_NORMAL_TIME) {
        pw_screen_draw_from_eeprom(
            (SCREEN_WIDTH-8)/2, 0,
            8, 16,
            PW_EEPROM_ADDR_IMG_IR_ARCS,
            PW_EEPROM_SIZE_IMG_IR_ARCS
        );
    } else
        pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);

}


void pw_comms_deinit(pw_state_t *s, const screen_flags_t *sf) {
    //int res;
    //res = pw_eeprom_read_walker_info(&walker_info_cache);
    //res = pw_eeprom_read_health_data(&health_data_cache);
}
