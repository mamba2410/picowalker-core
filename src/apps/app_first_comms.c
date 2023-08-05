#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../flash.h"
#include "../states.h"
#include "../eeprom.h"
#include "../ir/ir.h"
#include "../ir/actions.h"
#include "../screen.h"
#include "../buttons.h"
#include "../globals.h"
#include "app_first_comms.h"


/** @file app_first_comms.c
 * ```
 *  current_substate = comm_substate
 *  reg_a = happy/sad/neutral
 *  reg_b = prev happy/sad/neutral
 *  reg_c = advertising_attempts
 *  reg_x = timer
 * ```
 *
 */

void pw_first_comms_init(pw_state_t *s, const screen_flags_t *sf) {
    s->comms.current_substate = COMM_SUBSTATE_NONE;
    s->comms.screen_state = FC_SUBSTATE_WAITING;
    s->comms.advertising_attempts = 0;
    s->comms.timer = 0;
    pw_ir_set_comm_state(COMM_STATE_DISCONNECTED);
}

void pw_first_comms_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    comm_state_t cs = pw_ir_get_comm_state();
    ir_err_t err = IR_ERR_UNHANDLED_ERROR;
    size_t n_rw;

    switch(cs) {
    case COMM_STATE_AWAITING: {
        err = pw_action_try_find_peer(&s->comms, &packet_buf, PACKET_BUF_SIZE);
        break;
    }
    case COMM_STATE_SLAVE: {
        printf("Slave waiting\n");
        err = pw_ir_recv_packet(&packet_buf, PACKET_BUF_SIZE, &n_rw);
        if(err == IR_OK || err == IR_ERR_SIZE_MISMATCH) {
            err = pw_action_slave_perform_request(&packet_buf, n_rw);
        }

        if(pw_ir_get_comm_state() == COMM_STATE_DISCONNECTED && err == IR_OK) {
            s->comms.screen_state = FC_SUBSTATE_SUCCESS;
        }
        break;
    }
    case COMM_STATE_MASTER: {
        err = IR_ERR_INVALID_MASTER; // we are not allowed to be master in this state
        break;
    }
    case COMM_STATE_DISCONNECTED: {
        err = IR_OK;
        s->comms.advertising_attempts = 0;
        if(s->comms.screen_state == FC_SUBSTATE_TIMEOUT && s->comms.timer == 0) {
            s->comms.screen_state = FC_SUBSTATE_WAITING;
        }
        if(s->comms.screen_state == FC_SUBSTATE_SUCCESS) {
            p->sid = STATE_SPLASH;
        }
        break;
    }
    default: {
        printf("Error: Unexpected comm state 0x%02x\n", cs);
        err = IR_ERR_UNKNOWN_SUBSTATE;
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
        s->comms.advertising_attempts = 0;
        s->comms.screen_state = FC_SUBSTATE_TIMEOUT;
        s->comms.timer = 5;
    }
}

void pw_first_comms_init_display(pw_state_t *s, const screen_flags_t *sf) {

    pw_img_t img = {.height=32, .width=32, .size=256, .data=eeprom_buf};
    pw_flash_read(FLASH_IMG_POKEWALKER, img.data);
    pw_screen_draw_img(&img, (SCREEN_WIDTH-32)/2, (SCREEN_HEIGHT-32)/2);

    img.width = 16;
    img.height = 8;
    img.size = 0x20;
    pw_flash_read(FLASH_IMG_FACE_NEUTRAL, img.data);
    pw_screen_draw_img(&img, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);

}

void pw_first_comms_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {

    if( b == BUTTON_M && pw_ir_get_comm_state() == COMM_STATE_DISCONNECTED ) {
        // if we are actually initialised
        if(pw_eeprom_check_for_nintendo() && walker_info_cache.flags&0x01) { // TODO: walker inited flag
            s->comms.screen_state = FC_SUBSTATE_SUCCESS;
            return;
        }
    }

    s->comms.previous_screen_state = 0;
    s->comms.advertising_attempts = 0;
    s->comms.current_substate = COMM_SUBSTATE_FINDING_PEER;
    pw_ir_set_comm_state(COMM_STATE_AWAITING);
}

void pw_first_comms_draw_update(pw_state_t *s, const screen_flags_t *sf) {
    // TODO: update smiley faces from FLASH
    comm_state_t cs = pw_ir_get_comm_state();

    switch(cs) {
    case COMM_STATE_DISCONNECTED: {
        switch(s->comms.screen_state) {
        case FC_SUBSTATE_WAITING: {
            pw_img_t img = {.width=8, .height=8, .size=16, .data=eeprom_buf};
            if(sf->frame&ANIM_FRAME_NORMAL_TIME) {
                pw_flash_read(FLASH_IMG_UP_ARROW, img.data);
                pw_screen_draw_img(&img, (SCREEN_WIDTH-8)/2, 48);
            } else {
                pw_screen_clear_area((SCREEN_WIDTH-8)/2, 48, 8, 8);
            }

            img.width = 16;
            img.size=32;
            pw_flash_read(FLASH_IMG_FACE_NEUTRAL, img.data);
            pw_screen_draw_img(&img, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);

            break;
        }
        case FC_SUBSTATE_TIMEOUT: {
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 8);
            pw_img_t face = {.width=16, .height=8, .size=32, .data=eeprom_buf};
            pw_flash_read(FLASH_IMG_FACE_SAD, face.data);
            pw_screen_draw_img(&face, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);
            s->comms.timer--;
            break;
        }
        }
        break;
    }
    case COMM_STATE_AWAITING:
    case COMM_STATE_SLAVE: {
        pw_img_t face = {.width=16, .height=8, .size=32, .data=eeprom_buf};
        pw_flash_read(FLASH_IMG_FACE_HAPPY, face.data);
        pw_screen_draw_img(&face, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);
        pw_screen_clear_area((SCREEN_WIDTH-8)/2, 48, 8, 8);

        if(sf->frame&ANIM_FRAME_NORMAL_TIME) {
            pw_img_t img = {.width=8, .height=8, .size=16, .data=eeprom_buf};
            pw_flash_read(FLASH_IMG_IR_ACTIVE, img.data);
            pw_screen_draw_img(&img, (SCREEN_WIDTH-8)/2, 0);
        } else {
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 8);
        }
        break;
    }
    case COMM_STATE_MASTER:
    default: {
        // unreachable, hopefully
        break;
    }
    }
}

