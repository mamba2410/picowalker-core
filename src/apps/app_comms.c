#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "stdio.h"

#include "../states.h"
#include "../buttons.h"
#include "../screen.h"
#include "../eeprom_map.h"
#include "../ir/ir.h"
#include "../ir/actions.h"
#include "../globals.h"
#include "app_comms.h"

/** @file app_comms.c
 *
 */

const char* const PW_COMM_SUBSTATE_NAMES[N_COMM_SUBSTATE] = {
};

void pw_comms_init(pw_state_t *s, const screen_flags_t *sf) {
    //pw_eeprom_write_health_data(&health_data_cache);
    //pw_eeprom_write_walker_info(&walker_info_cache);

    if(s->sid == STATE_FIRST_COMMS) {
        s->comms.first_comms = true;
        s->comms.current_substate = COMM_SUBSTATE_IDLE;
    } else {
        s->comms.first_comms = false;
        s->comms.current_substate = COMM_SUBSTATE_FINDING_PEER;
    }

    s->comms.advertising_attempts = 0;  // advertising attempts
    s->comms.loop_counter = 0;
    s->comms.timer = 0;
    s->comms.anim_frame = 0;

    // TODO: Turn on IR hardware if in normal comms state
    // delegate to "finding peer" if in first comms state
    // Go through an "init hardware" state before finding peer?
}

void pw_comms_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {

    app_comms_t *comms = &s->comms;
    ir_err_t err = IR_ERR_UNHANDLED_ERROR;
    size_t n_rw;

    switch(comms->current_substate) {
    case COMM_SUBSTATE_FINDING_PEER:
    case COMM_SUBSTATE_DETERMINE_ROLE:
    case COMM_SUBSTATE_AWAITING_SLAVE_ACK: {
        // TODO: do we need all of this error prop? Either no prop
        // and change state in function, or full prop and change state here
        err = pw_action_try_find_peer(comms, &packet_buf, PACKET_BUF_SIZE);
        break;
    }
    case COMM_SUBSTATE_SLAVE_PERFORM_REQUEST: {
        err = pw_ir_recv_packet(&packet_buf, PACKET_BUF_SIZE, &n_rw);

        // TODO: switch on `err` and show "cannot complete" if its bad
        if(err == IR_OK || err == IR_ERR_SIZE_MISMATCH) {
            err = pw_action_slave_perform_request(comms, &packet_buf, n_rw);
        }

        break;
    }
    // Fallthrough for all peer play packet exchanges
    case COMM_SUBSTATE_START_PEER_PLAY:
    case COMM_SUBSTATE_PEER_PLAY_ACK:
    case COMM_SUBSTATE_SEND_MASTER_SPRITES:
    case COMM_SUBSTATE_SEND_MASTER_NAME_IMAGE:
    case COMM_SUBSTATE_SEND_MASTER_TEAMDATA:
    case COMM_SUBSTATE_READ_SLAVE_SPRITES:
    case COMM_SUBSTATE_READ_SLAVE_NAME_IMAGE:
    case COMM_SUBSTATE_READ_SLAVE_TEAMDATA:
    case COMM_SUBSTATE_SEND_PEER_PLAY_DX:
    case COMM_SUBSTATE_RECV_PEER_PLAY_DX:
    case COMM_SUBSTATE_WRITE_PEER_PLAY_DATA:
    case COMM_SUBSTATE_SEND_PEER_PLAY_END:
    case COMM_SUBSTATE_RECV_PEER_PLAY_END: {
        err = pw_action_peer_play(comms, &packet_buf, PACKET_BUF_SIZE);
        break;
    }
    case COMM_SUBSTATE_SEND_TO_SPLASH: {
        s->sid = STATE_SPLASH;
        break;
    }
    case COMM_SUBSTATE_IDLE: {
        // First comms just spin for 
        if(!comms->first_comms) {
            comms->current_substate = COMM_SUBSTATE_SEND_TO_SPLASH;
        }
        break;
    }
    default: {
        printf("[Error] Unknown comm state %d\n", comms->current_substate);
        break;
    }
    } // switch(cs)

    // TODO: remove this and display proper messages on screen
    if(err != IR_OK) {
        printf("[Info] IR error \"%s\"\n\tSubstate \"%s\"\n",
               PW_IR_ERR_NAMES[err],
               PW_COMM_SUBSTATE_NAMES[s->comms.current_substate]
              );

        comms->current_substate = COMM_SUBSTATE_SEND_TO_SPLASH;
    }

}

void pw_comms_init_display(pw_state_t *s, const screen_flags_t *sf) {

    switch(s->comms.current_substate) {
        case COMM_SUBSTATE_FINDING_PEER: {
            // Draw pokewalker image, "connecting" and arcs
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
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, SCREEN_BLACK);
            break;
        }
        case COMM_SUBSTATE_NO_PEER_FOUND: {
            // TODO: Draw message, text box, remove arc
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            break;
        }
        case COMM_SUBSTATE_CANNOT_CONNECT: {
            // TODO: Draw message, text box, remove arc
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            break;
        }
        // TODO: same as immediately above
        case COMM_SUBSTATE_CANNOT_COMPLETE: { break; }
        case COMM_SUBSTATE_TRAINER_UNAVAILABLE: { break; }
        case COMM_SUBSTATE_ALREADY_RECEIVED_EVENT: { break; }
        case COMM_SUBSTATE_CANNOT_CONNECT_AGAIN: { break; }
        case COMM_SUBSTATE_COULD_NOT_RECEIVE: { break; }
        case COMM_SUBSTATE_COMPLETED: { break; }
        case COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION: {
            // TODO: Draw bars, text box, remove arc
            break;
        }
        case COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION: {
            // TODO: Draw bars, text box, remove arc
            break;
        }
        case COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION: {
            // TODO: Draw bars, text box, remove arc
            break;
        }
        case COMM_SUBSTATE_DISPLAY_ITEM_GIFT_ANIMATION: {
            // TODO: Draw bars, text box, remove arc
            break;
        }
        case COMM_SUBSTATE_DISPLAY_POKE_GIFT_ANIMATION: {
            // TODO: Draw bars, text box, remove arc
            break;
        }
        default: {
            break;
        }
    }

}

void pw_comms_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {

    switch(s->comms.current_substate) {
    case COMM_SUBSTATE_NO_PEER_FOUND:
    case COMM_SUBSTATE_CANNOT_CONNECT:
    case COMM_SUBSTATE_CANNOT_COMPLETE:
    case COMM_SUBSTATE_TRAINER_UNAVAILABLE:
    case COMM_SUBSTATE_ALREADY_RECEIVED_EVENT:
    case COMM_SUBSTATE_CANNOT_CONNECT_AGAIN:
    case COMM_SUBSTATE_COULD_NOT_RECEIVE:
    case COMM_SUBSTATE_COMPLETED: {
        s->comms.current_substate = COMM_SUBSTATE_SEND_TO_SPLASH;
        break;
    }
    case COMM_SUBSTATE_IDLE: {
        s->comms.current_substate = COMM_SUBSTATE_FINDING_PEER;
        break;
    }
    // TODO: ending animations
    default: break;
    }

}

// TODO: rename to upate_display
void pw_comms_draw_update(pw_state_t *s, const screen_flags_t *sf) {

    // TODO: Animations
    switch(s->comms.current_substate) {
        // Regular states with blinking cursor
        case COMM_SUBSTATE_FINDING_PEER:
        case COMM_SUBSTATE_DETERMINE_ROLE:
        case COMM_SUBSTATE_AWAITING_SLAVE_ACK:
        case COMM_SUBSTATE_START_PEER_PLAY:
        case COMM_SUBSTATE_PEER_PLAY_ACK:
        case COMM_SUBSTATE_SEND_MASTER_SPRITES:
        case COMM_SUBSTATE_SEND_MASTER_NAME_IMAGE:
        case COMM_SUBSTATE_SEND_MASTER_TEAMDATA:
        case COMM_SUBSTATE_READ_SLAVE_SPRITES:
        case COMM_SUBSTATE_READ_SLAVE_NAME_IMAGE:
        case COMM_SUBSTATE_READ_SLAVE_TEAMDATA:
        case COMM_SUBSTATE_SEND_PEER_PLAY_DX:
        case COMM_SUBSTATE_RECV_PEER_PLAY_DX:
        case COMM_SUBSTATE_WRITE_PEER_PLAY_DATA:
        case COMM_SUBSTATE_SEND_PEER_PLAY_END:
        case COMM_SUBSTATE_RECV_PEER_PLAY_END: {
            if(sf->frame & ANIM_FRAME_NORMAL_TIME) {
                pw_screen_draw_from_eeprom(
                    (SCREEN_WIDTH-8)/2, 0,
                    8, 16,
                    PW_EEPROM_ADDR_IMG_IR_ARCS,
                    PW_EEPROM_SIZE_IMG_IR_ARCS
                );
            } else {
                pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            }
            break;
        }
        // Error states to display message
        case COMM_SUBSTATE_NO_PEER_FOUND:
        case COMM_SUBSTATE_CANNOT_CONNECT:
        case COMM_SUBSTATE_CANNOT_COMPLETE:
        case COMM_SUBSTATE_TRAINER_UNAVAILABLE:
        case COMM_SUBSTATE_ALREADY_RECEIVED_EVENT:
        case COMM_SUBSTATE_CANNOT_CONNECT_AGAIN:
        case COMM_SUBSTATE_COULD_NOT_RECEIVE:
        case COMM_SUBSTATE_COMPLETED: {

            if(s->comms.anim_frame == 0) {
                pw_comms_init_display(s, sf);
                s->comms.anim_frame++;
            }
            break;
        }
        // TODO: fill in
        case COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION:
        case COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION:
        case COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION:
        case COMM_SUBSTATE_DISPLAY_ITEM_GIFT_ANIMATION:
        case COMM_SUBSTATE_DISPLAY_POKE_GIFT_ANIMATION:
        default: {
            break;
        }
    }
}


void pw_comms_deinit(pw_state_t *s, const screen_flags_t *sf) {
    //int res;
    //res = pw_eeprom_read_walker_info(&walker_info_cache);
    //res = pw_eeprom_read_health_data(&health_data_cache);
}
