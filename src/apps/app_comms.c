#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "stdio.h"

#include "../states.h"
#include "../buttons.h"
#include "../screen.h"
#include "../eeprom_map.h"
#include "../flash.h"
#include "../ir/ir.h"
#include "../ir/actions.h"
#include "../globals.h"
#include "app_comms.h"

/** @file app_comms.c
 *
 */

const char* const PW_COMM_SUBSTATE_NAMES[N_COMM_SUBSTATE] = {
    [COMM_SUBSTATE_FIRST_IDLE] = "COMM_SUBSTATE_FIRST_IDLE",
    [COMM_SUBSTATE_FIRST_TIMEOUT] = "COMM_SUBSTATE_FIRST_TIMEOUT",
    [COMM_SUBSTATE_FIRST_SLAVE_PERFORM_REQUEST] = "COMM_SUBSTATE_FIRST_SLAVE_PERFORM_REQUEST",
    [COMM_SUBSTATE_FINDING_PEER] = "COMM_SUBSTATE_FINDING_PEER",
    [COMM_SUBSTATE_DETERMINE_ROLE] = "COMM_SUBSTATE_DETERMINE_ROLE",
    [COMM_SUBSTATE_AWAITING_SLAVE_ACK] = "COMM_SUBSTATE_AWAITING_SLAVE_ACK",
    [COMM_SUBSTATE_START_PEER_PLAY] = "COMM_SUBSTATE_START_PEER_PLAY",
    [COMM_SUBSTATE_PEER_PLAY_ACK] = "COMM_SUBSTATE_PEER_PLAY_ACK",
    [COMM_SUBSTATE_SEND_MASTER_SPRITES] = "COMM_SUBSTATE_SEND_MASTER_SPRITES",
    [COMM_SUBSTATE_SEND_MASTER_NAME_IMAGE] = "COMM_SUBSTATE_SEND_MASTER_NAME_IMAGE",
    [COMM_SUBSTATE_SEND_MASTER_TEAMDATA] = "COMM_SUBSTATE_SEND_MASTER_TEAMDATA",
    [COMM_SUBSTATE_READ_SLAVE_SPRITES] = "COMM_SUBSTATE_READ_SLAVE_SPRITES",
    [COMM_SUBSTATE_READ_SLAVE_NAME_IMAGE] = "COMM_SUBSTATE_READ_SLAVE_NAME_IMAGE",
    [COMM_SUBSTATE_READ_SLAVE_TEAMDATA] = "COMM_SUBSTATE_READ_SLAVE_TEAMDATA",
    [COMM_SUBSTATE_SEND_PEER_PLAY_DX] = "COMM_SUBSTATE_SEND_PEER_PLAY_DX",
    [COMM_SUBSTATE_RECV_PEER_PLAY_DX] = "COMM_SUBSTATE_RECV_PEER_PLAY_DX",
    [COMM_SUBSTATE_WRITE_PEER_PLAY_DATA] = "COMM_SUBSTATE_WRITE_PEER_PLAY_DATA",
    [COMM_SUBSTATE_SEND_PEER_PLAY_END] = "COMM_SUBSTATE_SEND_PEER_PLAY_END",
    [COMM_SUBSTATE_RECV_PEER_PLAY_END] = "COMM_SUBSTATE_RECV_PEER_PLAY_END",
    [COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION] = "COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION",
    [COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION] = "COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION",
    [COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION] = "COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION",
    [COMM_SUBSTATE_DISPLAY_ITEM_GIFT_ANIMATION] = "COMM_SUBSTATE_DISPLAY_ITEM_GIFT_ANIMATION",
    [COMM_SUBSTATE_DISPLAY_POKE_GIFT_ANIMATION] = "COMM_SUBSTATE_DISPLAY_POKE_GIFT_ANIMATION",
    [COMM_SUBSTATE_CALCULATE_PEER_PLAY_GIFT] = "COMM_SUBSTATE_CALCULATE_PEER_PLAY_GIFT",
    [COMM_SUBSTATE_SLAVE_PERFORM_REQUEST] = "COMM_SUBSTATE_SLAVE_PERFORM_REQUEST",
    [COMM_SUBSTATE_MASTER_DETERMINE_ACTION] = "COMM_SUBSTATE_MASTER_DETERMINE_ACTION",
    [COMM_SUBSTATE_SEND_TO_SPLASH] = "COMM_SUBSTATE_SEND_TO_SPLASH",
    [COMM_SUBSTATE_NO_PEER_FOUND] = "COMM_SUBSTATE_NO_PEER_FOUND",
    [COMM_SUBSTATE_CANNOT_CONNECT] = "COMM_SUBSTATE_CANNOT_CONNECT",
    [COMM_SUBSTATE_CANNOT_COMPLETE] = "COMM_SUBSTATE_CANNOT_COMPLETE",
    [COMM_SUBSTATE_TRAINER_UNAVAILABLE] = "COMM_SUBSTATE_TRAINER_UNAVAILABLE",
    [COMM_SUBSTATE_ALREADY_RECEIVED_EVENT] = "COMM_SUBSTATE_ALREADY_RECEIVED_EVENT",
    [COMM_SUBSTATE_CANNOT_CONNECT_AGAIN] = "COMM_SUBSTATE_CANNOT_CONNECT_AGAIN",
    [COMM_SUBSTATE_COULD_NOT_RECEIVE] = "COMM_SUBSTATE_COULD_NOT_RECEIVE",
    [COMM_SUBSTATE_COMPLETED] = "COMM_SUBSTATE_COMPLETED",
};

void pw_comms_init(pw_state_t *s, const screen_flags_t *sf) {
    //pw_eeprom_write_health_data(&health_data_cache);
    //pw_eeprom_write_walker_info(&walker_info_cache);

    if(s->sid == STATE_FIRST_COMMS) {
        s->comms.first_comms = true;
        s->comms.current_substate = COMM_SUBSTATE_FIRST_IDLE;
    } else {
        s->comms.first_comms = false;
        // TODO: stop for debugging
        s->comms.current_substate = COMM_SUBSTATE_FINDING_PEER;
        //s->comms.current_substate = COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION;
    }

    s->comms.advertising_attempts = 0;  // advertising attempts
    s->comms.loop_counter = 0;
    s->comms.timer = 0;
    s->comms.anim_frame = 0;
    s->comms.final_anim_frame = 0;
    //s->comms.final_anim_frame = WALK_END_ANIM_FRAMES;

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
        if(err == IR_ERR_ADVERTISING_MAX) {
            if(comms->first_comms) {
                comms->current_substate = COMM_SUBSTATE_FIRST_TIMEOUT;
                comms->timer = 5;
            } else {
                comms->current_substate = COMM_SUBSTATE_NO_PEER_FOUND;
            }
            err = IR_OK;
        }

        break;
    }
    case COMM_SUBSTATE_FIRST_SLAVE_PERFORM_REQUEST:
    case COMM_SUBSTATE_SLAVE_PERFORM_REQUEST: {
        err = pw_ir_recv_packet(&packet_buf, PACKET_BUF_SIZE, &n_rw);

        // TODO: switch on `err` and show "cannot complete" if its bad
        if(err == IR_OK || err == IR_ERR_SIZE_MISMATCH) {
            err = pw_action_slave_perform_request(comms, &packet_buf, n_rw);
            // TODO: Remove when all actions are implemented
            if(err != IR_OK) {
            if(comms->first_comms) {
                comms->current_substate = COMM_SUBSTATE_FIRST_TIMEOUT;
                comms->timer = 5;
                comms->anim_frame = 0;
            } else {
                comms->current_substate = COMM_SUBSTATE_CANNOT_COMPLETE;
            }
            }
        } else {
            printf("[Error] Slave can't perform request 0x%02x length %d\n", packet_buf.cmd, n_rw);
            if(comms->first_comms) {
                comms->current_substate = COMM_SUBSTATE_FIRST_TIMEOUT;
                comms->timer = 5;
                comms->anim_frame = 0;
            } else {
                comms->current_substate = COMM_SUBSTATE_CANNOT_COMPLETE;
            }
            err = IR_OK;
        }

        break;
    }
    case COMM_SUBSTATE_MASTER_DETERMINE_ACTION: {
        comms->current_substate = COMM_SUBSTATE_START_PEER_PLAY;
        // Fall through
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
        // If we timed out, the peer doesn't want to talk to us
        if(err == IR_ERR_TIMEOUT) {
            comms->current_substate = COMM_SUBSTATE_CANNOT_COMPLETE;
            err = IR_OK;
        }
        break;
    }
    case COMM_SUBSTATE_SEND_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        return;
    }
    case COMM_SUBSTATE_FIRST_IDLE:
                                       {
        // Spin while waiting for user input
        err = IR_OK;
        break;
    }
    case COMM_SUBSTATE_FIRST_TIMEOUT: {
        if(comms->timer == 0) {
            comms->current_substate = COMM_SUBSTATE_FIRST_IDLE;
            comms->advertising_attempts = 0;
            comms->anim_frame = 0;
        }
        err = IR_OK;
        break;
    }
    case COMM_SUBSTATE_CANNOT_COMPLETE:
    case COMM_SUBSTATE_TRAINER_UNAVAILABLE:
    case COMM_SUBSTATE_ALREADY_RECEIVED_EVENT:
    case COMM_SUBSTATE_CANNOT_CONNECT_AGAIN:
    case COMM_SUBSTATE_COULD_NOT_RECEIVE:
    case COMM_SUBSTATE_COMPLETED:
    case COMM_SUBSTATE_NO_PEER_FOUND: {
        // Spin while we wait for user input
        err = IR_OK;
        break;
    }
    case COMM_SUBSTATE_RETURN_TO_FIRST: {
        p->sid = STATE_FIRST_COMMS;
        err = IR_OK;
        break;
    }
    case COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION:
    case COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION: {
        if(comms->anim_frame >= comms->final_anim_frame) {
            comms->current_substate = COMM_SUBSTATE_SEND_TO_SPLASH;
        }
        err = IR_OK;
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


        if(!comms->first_comms) {
            comms->current_substate = COMM_SUBSTATE_SEND_TO_SPLASH;
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
    case COMM_SUBSTATE_FIRST_IDLE: {
        s->comms.current_substate = COMM_SUBSTATE_FINDING_PEER;
        break;
    }
    // TODO: ending animations
    default: break;
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
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            pw_screen_draw_message(SCREEN_HEIGHT-16, 1, 16); // no trainer found
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, SCREEN_BLACK);
            break;
        }
        case COMM_SUBSTATE_CANNOT_CONNECT: {
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            pw_screen_draw_message(SCREEN_HEIGHT-16, 4, 16); // cannot connect
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, SCREEN_BLACK);
            break;
        }
        case COMM_SUBSTATE_CANNOT_COMPLETE: {
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            pw_screen_draw_message(SCREEN_HEIGHT-32, 2, 32); // cannot complete
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-32, SCREEN_WIDTH, 32, SCREEN_BLACK);
            break;
        }
        case COMM_SUBSTATE_COMPLETED: {
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 16);
            pw_screen_draw_message(SCREEN_HEIGHT-16, 16, 16); // completed
            pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, SCREEN_BLACK);
            break;
        }
        // TODO: same as immediately above
        case COMM_SUBSTATE_TRAINER_UNAVAILABLE: { break; }
        case COMM_SUBSTATE_ALREADY_RECEIVED_EVENT: { break; }
        case COMM_SUBSTATE_CANNOT_CONNECT_AGAIN: { break; }
        case COMM_SUBSTATE_COULD_NOT_RECEIVE: { break; }
        case COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION: {
            // TODO: Draw bars, text box, remove arc
            break;
        }
        case COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION: {
            pw_screen_fill_area(0, 8, SCREEN_WIDTH, SCREEN_HEIGHT-2*8, SCREEN_WHITE);
            pw_screen_fill_area(0, 0, SCREEN_WIDTH, 8, SCREEN_BLACK);
            pw_screen_fill_area(0, SCREEN_HEIGHT-8, SCREEN_WIDTH, 8, SCREEN_BLACK);
            break;
        }
        case COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION: {
            pw_screen_fill_area(0, 8, (SCREEN_WIDTH-64)/2, SCREEN_HEIGHT-2*8, SCREEN_WHITE);
            pw_screen_fill_area((SCREEN_WIDTH-64)/2+64, 8, (SCREEN_WIDTH-64)/2, SCREEN_HEIGHT-2*8, SCREEN_WHITE);
            pw_screen_draw_from_eeprom(
                (SCREEN_WIDTH-64)/2, 8,
                64, 48,
                PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED + ((sf->frame & ANIM_FRAME_DOUBLE_TIME)*PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME),
                PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME
            );
            pw_screen_fill_area(0, 0, SCREEN_WIDTH, 8, SCREEN_BLACK);
            pw_screen_fill_area(0, SCREEN_HEIGHT-8, SCREEN_WIDTH, 8, SCREEN_BLACK);
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
        case COMM_SUBSTATE_FIRST_IDLE: {
            pw_img_t img = {.height=32, .width=32, .size=256, .data=eeprom_buf};
            pw_flash_read(FLASH_IMG_POKEWALKER, img.data);
            pw_screen_draw_img(&img, (SCREEN_WIDTH-32)/2, (SCREEN_HEIGHT-32)/2);

            img.width = 16;
            img.height = 8;
            img.size = 0x20;
            pw_flash_read(FLASH_IMG_FACE_NEUTRAL, img.data);
            pw_screen_draw_img(&img, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);
            break;
        }
        case COMM_SUBSTATE_FIRST_SLAVE_PERFORM_REQUEST: {
            pw_img_t face = {.width=16, .height=8, .size=32, .data=eeprom_buf};
            pw_flash_read(FLASH_IMG_FACE_HAPPY, face.data);
            pw_screen_draw_img(&face, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 48, 8, 8);
                                                            break;
                                                        }
        default: {
            break;
        }
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
        case COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION: {
            if(s->comms.anim_frame == 0) {
                pw_comms_init_display(s, sf);
                s->comms.anim_frame++;
                break;
            }
            // Frames 0-3 = black bars
            // 4 = large clous
            // 5 = large sprite first frame
            // 6-7 = large sprite second frame
            // 8-11 = small sprite animated + "has arrived"
            switch(s->comms.anim_frame) {
                case 3: {
                    pw_screen_draw_from_eeprom(
                        (SCREEN_WIDTH-32)/2, SCREEN_HEIGHT-32-16,
                        32, 24,
                        PW_EEPROM_ADDR_IMG_RADAR_APPEAR_CLOUD,
                        PW_EEPROM_SIZE_IMG_RADAR_APPEAR_CLOUD
                    );
                    break;
                }
                case 4: {
                    pw_screen_draw_from_eeprom(
                        (SCREEN_WIDTH-64)/2, 8,
                        64, 48,
                        PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED,
                        PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED
                    );
                    //pw_screen_fill_area(0, 0, SCREEN_WIDTH, 8, SCREEN_BLACK);
                    //pw_screen_fill_area(0, SCREEN_HEIGHT-8, SCREEN_WIDTH, 8, SCREEN_BLACK);
                    break;
                }
                case 5: {
                    pw_screen_draw_from_eeprom(
                        (SCREEN_WIDTH-64)/2, 8,
                        64, 48,
                        PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED+PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME,
                        PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED
                    );
                    //pw_screen_fill_area(0, 0, SCREEN_WIDTH, 8, SCREEN_BLACK);
                    //pw_screen_fill_area(0, SCREEN_HEIGHT-8, SCREEN_WIDTH, 8, SCREEN_BLACK);
                    break;
                }
                case 8: {
                    pw_screen_clear();
                    pw_screen_draw_from_eeprom(
                        0, SCREEN_HEIGHT-32,
                        80, 16,
                        PW_EEPROM_ADDR_TEXT_POKEMON_NAME,
                        PW_EEPROM_SIZE_TEXT_POKEMON_NAME
                    );
                    pw_screen_draw_message(SCREEN_HEIGHT-16, 13, 16);
                    pw_screen_draw_text_box(0, SCREEN_HEIGHT-32, SCREEN_WIDTH, 32, SCREEN_BLACK);
                }
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 16:
                    {
                    pw_screen_draw_from_eeprom(
                        (SCREEN_WIDTH-32)/2, 8,
                        32, 24,
                        PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED+((sf->frame & ANIM_FRAME_DOUBLE_TIME)*PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED_FRAME),
                        PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED
                    );
                    break;
                }

                default: break;
            }

            s->comms.anim_frame++;
            break;
        }
        case COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION: {
            if(s->comms.anim_frame == 0) {
                pw_comms_init_display(s, sf);
                s->comms.anim_frame++;
                break;
            }

            // Frames 0-3 Black bars + large animated
            // 4 cloud
            // 5-7 clear
            // 8-12 empty + "has left"
            switch(s->comms.anim_frame) {
                case 1:
                case 2:
                case 3: {
                    pw_screen_draw_from_eeprom(
                        (SCREEN_WIDTH-64)/2, 8,
                        64, 48,
                        PW_EEPROM_ADDR_IMG_POKEMON_LARGE_ANIMATED + ((sf->frame & ANIM_FRAME_DOUBLE_TIME)*PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME),
                        PW_EEPROM_SIZE_IMG_POKEMON_LARGE_ANIMATED_FRAME
                    );
                    break;
                }
                case 4: {
                    pw_screen_fill_area(
                        (SCREEN_WIDTH-64)/2, 8,
                        64, 48,
                        SCREEN_WHITE
                    );
                    pw_screen_draw_from_eeprom(
                        (SCREEN_WIDTH-32)/2, SCREEN_HEIGHT-32-16,
                        32, 24,
                        PW_EEPROM_ADDR_IMG_RADAR_APPEAR_CLOUD,
                        PW_EEPROM_SIZE_IMG_RADAR_APPEAR_CLOUD
                    );
                    break;
                }
                case 5: {
                    pw_screen_fill_area(
                        (SCREEN_WIDTH-64)/2, 8,
                        64, 48,
                        SCREEN_WHITE
                    );
                    break;
                }
                case 8: {
                    pw_screen_clear();
                    pw_screen_draw_from_eeprom(
                        0, SCREEN_HEIGHT-32,
                        80, 16,
                        PW_EEPROM_ADDR_TEXT_POKEMON_NAME,
                        PW_EEPROM_SIZE_TEXT_POKEMON_NAME
                    );
                    pw_screen_draw_message(SCREEN_HEIGHT-16, 14, 16); // "has left"
                    pw_screen_draw_text_box(0, SCREEN_HEIGHT-32, SCREEN_WIDTH, 32, SCREEN_BLACK);
                }
                default: break;
            }
            s->comms.anim_frame++;
            break;
        }
        // TODO: fill in
        case COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION:
        case COMM_SUBSTATE_DISPLAY_ITEM_GIFT_ANIMATION:
        case COMM_SUBSTATE_DISPLAY_POKE_GIFT_ANIMATION:
        case COMM_SUBSTATE_FIRST_IDLE: {
            if(s->comms.anim_frame == 0) {
                pw_comms_init_display(s, sf);
                s->comms.anim_frame++;
                break;
            }

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
        case COMM_SUBSTATE_FIRST_TIMEOUT: {
            pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 8);
            pw_img_t face = {.width=16, .height=8, .size=32, .data=eeprom_buf};
            pw_flash_read(FLASH_IMG_FACE_SAD, face.data);
            pw_screen_draw_img(&face, (SCREEN_WIDTH-16)/2, (SCREEN_HEIGHT-8)/2);
            s->comms.timer--;
            break;
        }
        case COMM_SUBSTATE_FIRST_SLAVE_PERFORM_REQUEST: {
            if(s->comms.anim_frame == 0) {
                pw_comms_init_display(s, sf);
                s->comms.anim_frame++;
            }

            if(sf->frame&ANIM_FRAME_NORMAL_TIME) {
                pw_img_t img = {.width=8, .height=8, .size=16, .data=eeprom_buf};
                pw_flash_read(FLASH_IMG_IR_ACTIVE, img.data);
                pw_screen_draw_img(&img, (SCREEN_WIDTH-8)/2, 0);
            } else {
                pw_screen_clear_area((SCREEN_WIDTH-8)/2, 0, 8, 8);
            }
            break;
        }
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
