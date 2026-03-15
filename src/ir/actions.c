#include <stdbool.h>

#include <string.h> // memcpy()

#include "../debug_log.h"
#include "../eeprom_map.h"
#include "../eeprom.h"
#include "../types.h"
#include "../timer.h"
#include "../states.h"
#include "../rand.h"
#include "ir.h"
#include "actions.h"
#include "compression.h"
#include "../globals.h"
#include "../states.h"
#include "../apps/app_comms.h"
#include "../event_log.h"

#define FALLTHROUGH __attribute__((fallthrough))

#define ACTION_DELAY_MS 1

ir_err_t pw_ir_eeprom_do_write(pw_packet_t *packet, size_t len);
ir_err_t pw_ir_identity_ack(pw_packet_t *packet);
static void create_peer_play_data(peer_play_data_t *ppd);

/*
 *  Listen for a packet.
 *  If we don't hear anything, send advertising byte
 *  then listen for reply
 */
ir_err_t pw_action_listen_and_advertise(app_comms_t *comms, pw_packet_t *packet, size_t *pn_read) {

    ir_err_t err = IR_ERR_TIMEOUT;

    err = pw_ir_recv_packet(packet, 8, pn_read);

    if(*pn_read > 0) {
        return IR_OK;
    }

    (void)pw_ir_send_advertising_packet();

    comms->advertising_attempts++;
    if(comms->advertising_attempts > MAX_ADVERTISING_PACKETS) {
        return IR_ERR_ADVERTISING_MAX;
    }

    err = pw_ir_recv_packet(packet, 8, pn_read);

    return err;
}


/*
 *  Do one action per call, called in the main event loop
 */
ir_err_t pw_action_try_find_peer(app_comms_t *comms, pw_packet_t *packet, size_t packet_max) {
    (void)packet_max;

    ir_err_t err = IR_ERR_UNHANDLED_ERROR;
    size_t n_read = 0;

    switch(comms->current_substate) {
    case COMM_SUBSTATE_FINDING_PEER: {

        err = pw_action_listen_and_advertise(comms, packet, &n_read);

        switch(err) {
        case IR_ERR_SIZE_MISMATCH:  // also ok since we might recv 0xfc
        case IR_OK:
            // We received at least one byte, so fall through and check what it was.
            comms->current_substate = COMM_SUBSTATE_DETERMINE_ROLE;
            break;
        case IR_ERR_TIMEOUT:
            return IR_OK; // ignore timeout
        case IR_ERR_ADVERTISING_MAX:
            return IR_ERR_ADVERTISING_MAX;
        default:
            return err; // TODO: change this
        }

        FALLTHROUGH;
    }
    case COMM_SUBSTATE_DETERMINE_ROLE: {

        // TODO: If `n_read` > 1 then test if its 0xFC followed by master assert
        // If it is a master assert, then we follow that packet instead

        // We should already have a response in the packet buffer
        switch(packet->cmd) {
        case CMD_ADVERTISING: // we found peer, we request master

            packet->cmd = CMD_ASSERT_MASTER;
            packet->extra = EXTRA_BYTE_FROM_WALKER;
            err = pw_ir_send_packet(packet, 8, &n_read);

            comms->current_substate = COMM_SUBSTATE_AWAITING_SLAVE_ACK;
            break;
        case CMD_ASSERT_MASTER: // peer found us, peer requests master
            packet->cmd = CMD_SLAVE_ACK;
            packet->extra = 2;

            // record master key
            uint8_t session_id_master[SESSION_ID_SIZE];
            for(int i = 0; i < SESSION_ID_SIZE; i++)
                session_id_master[i] = packet->session_id_bytes[i];

            err = pw_ir_send_packet(packet, 8, &n_read);

            // combine keys
            pw_ir_mix_session_id(session_id_master);
            pw_time_delay_ms(ACTION_DELAY_MS);

            if(comms->first_comms) {
                comms->current_substate = COMM_SUBSTATE_FIRST_SLAVE_PERFORM_REQUEST;
            } else {
                comms->current_substate = COMM_SUBSTATE_SLAVE_PERFORM_REQUEST;
            }
            break;
        default:
            return IR_ERR_UNEXPECTED_PACKET;
        }
        break;
    }
    case COMM_SUBSTATE_AWAITING_SLAVE_ACK: {   // we have sent master request

        // wait for answer
        err = pw_ir_recv_packet(packet, 8, &n_read);
        if(err != IR_OK) return err;

        // TODO: Test for advertising byte, probably just log and ignore
        if(packet->cmd != CMD_SLAVE_ACK) return IR_ERR_UNEXPECTED_PACKET;

        // combine keys
        pw_ir_mix_session_id(packet->session_id_bytes);

        // key exchange done, we are now master
        // TODO STATE: Move us into some master state to determine what to do next
        comms->current_substate = COMM_SUBSTATE_MASTER_DETERMINE_ACTION;
        break;
    }
    default:
        return IR_ERR_UNKNOWN_SUBSTATE;
    }
    return err;
}


/*
 *  We are slave, given already recv'd packet, respond appropriately
 */
ir_err_t pw_action_slave_perform_request(app_comms_t *comms, pw_packet_t *packet, size_t len) {

    ir_err_t err = IR_ERR_UNHANDLED_ERROR;
    size_t n_rw;

    switch(packet->cmd) {
    case CMD_IDENTITY_REQ: {
        packet->cmd = CMD_IDENTITY_RSP;
        packet->extra = EXTRA_BYTE_FROM_WALKER;

        // Put our info into the reply packet and update total step count
        // Update our ram cache to reflect
        int r = pw_eeprom_read_walker_info((walker_info_t*)packet->payload);
        ((walker_info_t*)packet->payload)->be_step_count = swap_bytes_u32(health_data_cache.total_steps);
        walker_info_cache = *(walker_info_t*)packet->payload;

        // Write health data to eeprom
        pw_eeprom_write_health_data(&health_data_cache);

        // Write current watts to a special area so master can read it later
        uint16_t current_watts = swap_bytes_u16(health_data_cache.current_watts);
        pw_eeprom_write(0xce8a, (uint8_t*)&current_watts, 2);

        if(r < 0) {
            return IR_ERR_BAD_DATA;
        }

        pw_time_delay_ms(ACTION_DELAY_MS);

        err = pw_ir_send_packet(packet, 8+sizeof(walker_info_t), &n_rw);

        break;
    }
    case CMD_IDENTITY_SEND:
    case CMD_IDENTITY_SEND_ALIAS1:
    case CMD_IDENTITY_SEND_ALIAS2:
    case CMD_IDENTITY_SEND_ALIAS3: {
        err = pw_ir_identity_ack(packet);
        break;
    }
    case CMD_EEPROM_WRITE_CMP_00:
    case CMD_EEPROM_WRITE_RAW_00:
    case CMD_EEPROM_WRITE_CMP_80:
    case CMD_EEPROM_WRITE_RAW_80: {
        err = pw_ir_eeprom_do_write(packet, len);

        pw_time_delay_ms(ACTION_DELAY_MS);
        packet->cmd = CMD_EEPROM_WRITE_ACK;
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        pw_ir_send_packet(packet, 8, &n_rw);
        break;
    }
    case CMD_EEPROM_READ_REQ: {
        uint16_t addr = packet->payload[0]<<8 | packet->payload[1];
        size_t len = packet->payload[2];

        packet->cmd = CMD_EEPROM_READ_RSP;
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        pw_eeprom_read(addr, packet->payload, len);

        pw_time_delay_ms(ACTION_DELAY_MS);

        err = pw_ir_send_packet(packet, 8+len, &n_rw);
        break;
    }
    case CMD_PING: {
        packet->cmd = CMD_PONG;
        packet->extra = EXTRA_BYTE_FROM_WALKER;

        pw_time_delay_ms(ACTION_DELAY_MS);

        err = pw_ir_send_packet(packet, 8, &n_rw);
        break;
    }
    case CMD_CONNECT_COMPLETE: {
        packet->cmd = CMD_CONNECT_COMPLETE_ACK;
        packet->cmd = EXTRA_BYTE_FROM_WALKER;
        pw_time_delay_ms(ACTION_DELAY_MS);

        err = pw_ir_send_packet(packet, 8, &n_rw);
        comms->current_substate = COMM_SUBSTATE_COMPLETED;
        break;
    }
    case CMD_WALK_END_REQ: {
        packet->cmd = CMD_WALK_END_ACK;
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        pw_time_delay_ms(ACTION_DELAY_MS);
        err = pw_ir_send_packet(packet, 8, &n_rw);

        pw_ir_end_walk();

        comms->current_substate = COMM_SUBSTATE_DISPLAY_WALK_END_ANIMATION;
        comms->final_anim_frame = WALK_END_ANIM_FRAMES;
        comms->anim_frame = 0;
        break;
    }
    case CMD_WALK_START_INIT:
        health_data_cache.today_steps = 0;
        FALLTHROUGH;
    case CMD_WALK_START: {
        // keep cmd
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        pw_time_delay_ms(ACTION_DELAY_MS);
        err = pw_ir_send_packet(packet, 8, &n_rw);
        pw_ir_start_walk();

        // Start animation
        comms->current_substate = COMM_SUBSTATE_DISPLAY_WALK_START_ANIMATION;
        comms->final_anim_frame = WALK_START_ANIM_FRAMES;
        comms->anim_frame = 0;
        err = IR_OK;
        
        break;
    }
    case CMD_DISCONNECT: {
        err = IR_OK;
        comms->current_substate = COMM_SUBSTATE_COMPLETED;
        break;
    }
    case CMD_NOCOMPLETE_ALIAS1: {
        err = IR_OK;
        comms->current_substate = COMM_SUBSTATE_CANNOT_COMPLETE;
        break;
    }
    case CMD_WALKER_RESET_1: {
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        pw_time_delay_ms(ACTION_DELAY_MS);
        pw_eeprom_reliable_read(
            PW_EEPROM_ADDR_UNIQUE_IDENTITY_DATA_1,
            PW_EEPROM_ADDR_UNIQUE_IDENTITY_DATA_2,
            packet->payload,
            sizeof(unique_identity_data_t)
        );
        err = pw_ir_send_packet(packet, 8+sizeof(unique_identity_data_t), &n_rw);
        pw_eeprom_reset(true, false);
        comms->current_substate = COMM_SUBSTATE_RETURN_TO_FIRST;
        break;
    }
    case CMD_PEER_PLAY_START: {
        // TODO: Check if we can play, if not send CMD_PEER_PLAY_SEEN
        packet->cmd = CMD_PEER_PLAY_RSP;
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        pw_time_delay_ms(ACTION_DELAY_MS);

        // Grab their protocol version so we can repeat it back to them.
        // Dmitry's writeup says its 0x02 but my two OG walkers are 0x00
        // Picowalker gets assigned 0x02 from the same ROM so I'm not sure why they're different.
        uint8_t proto_ver = packet->payload[0x5c];

        { // Limit scope of pointer recast
            walker_info_t *wi = (walker_info_t*)packet->payload;
            pw_log_debug("Peer play peer has version %d.%d\n", wi->protocol_ver, wi->protocol_subver);
            pw_log_debug("We have version %d.%d\n", walker_info_cache.protocol_ver, walker_info_cache.protocol_subver);
        }

        pw_eeprom_reliable_read(
            PW_EEPROM_ADDR_IDENTITY_DATA_1,
            PW_EEPROM_ADDR_IDENTITY_DATA_2,
            packet->payload,
            sizeof(walker_info_t)
        );
        // TODO: remove
        packet->payload[0x10] = pw_rand(); // Randomise UID
        packet->payload[0x0c] = pw_rand(); // Randomise TID
        packet->payload[0x5c] = proto_ver;

        err = pw_ir_send_packet(packet, 8+sizeof(walker_info_t), &n_rw);
        break;
    }
    case CMD_PEER_PLAY_RSP: {
        // Shouldn't happen if we're in slave mode
        err = IR_ERR_UNEXPECTED_PACKET;
        break;
    }
    case CMD_PEER_PLAY_DX: {
        pw_eeprom_write(PW_EEPROM_ADDR_CURRENT_PEER_DATA,
            packet->payload,
            sizeof(peer_play_data_t)
        );
        packet->cmd = CMD_PEER_PLAY_DX;
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        create_peer_play_data((peer_play_data_t*)packet->payload);

        /*
        peer_play_data_t *ppd = (peer_play_data_t*)packet->payload;

        // Read `walker_info_t`
        pw_eeprom_reliable_read(
            PW_EEPROM_ADDR_IDENTITY_DATA_1,
            PW_EEPROM_ADDR_IDENTITY_DATA_2,
            eeprom_buf,
            sizeof(walker_info_t)
        );
        walker_info_t *wi = (walker_info_t*)(eeprom_buf);
        // TODO: read from global health data
        ppd->be_current_watts = 9999;
        ppd->be_current_steps = 99999;
        ppd->le_unk0 = wi->le_unk0;
        ppd->le_unk2 = wi->le_unk2;
        for(size_t i = 0; i < 8; i++)
            ppd->trainer_name[i] = wi->le_trainer_name[i];
        wi = NULL;

        // Read `route_info_t`
        pw_eeprom_read(
            PW_EEPROM_ADDR_ROUTE_INFO,
            eeprom_buf,
            sizeof(route_info_t)
        );
        route_info_t *ri = (route_info_t*)(eeprom_buf);
        ppd->le_species = ri->pokemon_summary.le_species;
        ppd->pokemon_flags_1 = ri->pokemon_summary.pokemon_flags_1;
        ppd->pokemon_flags_2 = ri->pokemon_summary.pokemon_flags_2;
        for(size_t i = 0; i < 11; i++) {
            ppd->pokemon_name[i] = ri->pokemon_nickname[i];
        }
        */

        err = pw_ir_send_packet(packet, 8+sizeof(peer_play_data_t), &n_rw);
        break;
    }
    case CMD_PEER_PLAY_END: {
        packet->cmd = CMD_PEER_PLAY_END;
        packet->extra = EXTRA_BYTE_FROM_WALKER;
        err = pw_ir_send_packet(packet, 8, &n_rw);
        comms->current_substate = COMM_SUBSTATE_CALCULATE_PEER_PLAY_GIFT;
        break;
    }
    case CMD_PEER_PLAY_SEEN: {
        comms->current_substate = COMM_SUBSTATE_CANNOT_CONNECT_AGAIN;
        break;
    }
    default: {
        pw_log_error("Slave recv unhandled packet: %02x\n", packet->cmd);
        err = IR_ERR_UNEXPECTED_PACKET;
        break;
    }

    }

    return err;
}

/*
 *  Sequence:
 *  send CMD_PEER_PLAY_START
 *  recv CMD_PEER_PLAY_RSP
 *  send master EEPROM:0x91BE to slave EEPROM:0xF400
 *  send master EEPROM:0xCC00 to slave EEPROM:0xDC00
 *  read slave EEPROM:0x91BE to master EEPROM:0xF400
 *  read slave EEPROM:0xCC00 to master EEPROM:0xDC00
 *  send CMD_PEER_PLAY_DX
 *  recv CMD_PEER_PLAY_DX ?
 *  write data to master EEPROM:0xF6C0
 *  send CMD_PEER_PLAY_END
 *  recv CMD_PEER_PLAY_END
 *  display animation
 *  calculate gift
 */
ir_err_t pw_action_peer_play(app_comms_t *comms, pw_packet_t *packet, size_t max_len) {
    ir_err_t err = IR_ERR_UNHANDLED_ERROR;
    size_t n_read;

    switch(comms->current_substate) {
    case COMM_SUBSTATE_START_PEER_PLAY: {

        packet->cmd = CMD_PEER_PLAY_START;
        packet->extra = EXTRA_BYTE_FROM_WALKER;

        pw_eeprom_read_walker_info((walker_info_t*)packet->payload);

        packet->payload[0x10] = (uint8_t)(pw_rand()&0xff);  // Hack to change UID each time to prevent "already connected" error
        packet->payload[0x5c] = 0x00; // For some reason my pokewalker uses 0x00, but picowalker is assigned 0x02

        // TODO: remove this in proper code
        err = pw_ir_send_packet(packet, 8+sizeof(walker_info_t), &n_read);
        if(err != IR_OK) return err;

        comms->current_substate = COMM_SUBSTATE_PEER_PLAY_ACK;
        break;
    }
    case COMM_SUBSTATE_PEER_PLAY_ACK: {

        err = pw_ir_recv_packet(packet, 8+sizeof(walker_info_t), &n_read);
        { // Limit scope of pointer recast
            walker_info_t *wi = (walker_info_t*)packet->payload;
            pw_log_debug("Peer play peer has version %d.%d\n", wi->protocol_ver, wi->protocol_subver);
            pw_log_debug("We have version %d.%d\n", walker_info_cache.protocol_ver, walker_info_cache.protocol_subver);
        }

        switch(packet->cmd) {
        case CMD_PEER_PLAY_RSP:
            break;
        case CMD_PEER_PLAY_SEEN:
            return IR_ERR_PEER_ALREADY_SEEN;
        default:
            return IR_ERR_UNEXPECTED_PACKET;
        }
        if(err != IR_OK) return err;

        comms->current_substate = COMM_SUBSTATE_SEND_MASTER_SPRITES;
        comms->advertising_attempts = 0; // reset loop counter
        break;
    }
    case COMM_SUBSTATE_SEND_MASTER_SPRITES: {

        size_t write_size = 128;    // should always be 128-bytes
        size_t cur_write_size   = (size_t)(comms->advertising_attempts) * write_size;

        err = pw_action_send_large_raw_data_from_eeprom(
                  PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED,              // src
                  PW_EEPROM_ADDR_IMG_CURRENT_PEER_POKEMON_ANIMATED_SMALL, // dst
                  PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED,              // size
                  write_size, &(comms->advertising_attempts), packet, max_len
              );
        if(err != IR_OK) return err;

        if(cur_write_size >= PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED) {
            comms->advertising_attempts = 0;  // reset loop counter
            comms->current_substate = COMM_SUBSTATE_SEND_MASTER_NAME_IMAGE;
        }
        break;
    }
    case COMM_SUBSTATE_SEND_MASTER_NAME_IMAGE: {

        size_t write_size = 128;
        size_t cur_write_size   = (size_t)(comms->advertising_attempts) * write_size;

        err = pw_action_send_large_raw_data_from_eeprom(
                  PW_EEPROM_ADDR_TEXT_POKEMON_NAME,               // src
                  PW_EEPROM_ADDR_TEXT_CURRENT_PEER_POKEMON_NAME,  // dst
                  PW_EEPROM_SIZE_TEXT_POKEMON_NAME,               // size
                  write_size, &(comms->advertising_attempts), packet, max_len
              );

        if(cur_write_size >= PW_EEPROM_SIZE_TEXT_POKEMON_NAME) {
            comms->advertising_attempts = 0; // reset loop counter
            comms->current_substate = COMM_SUBSTATE_SEND_MASTER_TEAMDATA;
        }
        break;
    }
    case COMM_SUBSTATE_SEND_MASTER_TEAMDATA: {

        size_t write_size = 128;
        size_t cur_write_size   = (size_t)(comms->advertising_attempts) * write_size;

        err = pw_action_send_large_raw_data_from_eeprom(
                  PW_EEPROM_ADDR_TEAM_DATA_STRUCT,            // src
                  PW_EEPROM_ADDR_CURRENT_PEER_TEAM_DATA,      // dst
                  PW_EEPROM_SIZE_TEAM_DATA_STRUCT,            // size
                  write_size, &(comms->advertising_attempts), packet, max_len
              );

        if(cur_write_size >= PW_EEPROM_SIZE_TEAM_DATA_STRUCT) {
            comms->advertising_attempts = 0; // reset loop counter
            comms->current_substate = COMM_SUBSTATE_READ_SLAVE_SPRITES;
        }
        break;
    }
    case COMM_SUBSTATE_READ_SLAVE_SPRITES: {

        size_t read_size = 128; // NOTE: this can be anything so long as it fits in your buffer
        // TODO: move this buffer dependancy inti pw_ir_read()
        err = pw_action_read_large_raw_data_from_eeprom(
                  PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED,              // src
                  PW_EEPROM_ADDR_IMG_CURRENT_PEER_POKEMON_ANIMATED_SMALL, // dst
                  PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED,              // size
                  read_size, &(comms->advertising_attempts), packet, max_len
              );

        size_t cur_read_size   = (size_t)(comms->advertising_attempts) * read_size;

        if(cur_read_size >= PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED) {
            comms->advertising_attempts = 0;  // reset loop counter
            comms->current_substate = COMM_SUBSTATE_READ_SLAVE_NAME_IMAGE;
        }
        break;
    }
    case COMM_SUBSTATE_READ_SLAVE_NAME_IMAGE: {

        size_t read_size = 128;  // TODO: See above
        err = pw_action_read_large_raw_data_from_eeprom(
                  PW_EEPROM_ADDR_TEXT_POKEMON_NAME,               // src
                  PW_EEPROM_ADDR_TEXT_CURRENT_PEER_POKEMON_NAME,  // dst
                  PW_EEPROM_SIZE_TEXT_POKEMON_NAME,               // size
                  read_size, &(comms->advertising_attempts), packet, max_len
              );

        size_t cur_read_size   = (size_t)(comms->advertising_attempts) * read_size;

        if(cur_read_size >= PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED) {
            comms->advertising_attempts = 0;  // reset loop counter
            comms->current_substate = COMM_SUBSTATE_READ_SLAVE_TEAMDATA;
        }
        break;
    }
    case COMM_SUBSTATE_READ_SLAVE_TEAMDATA: {

        size_t read_size = 128;  // TODO: See above
        err = pw_action_read_large_raw_data_from_eeprom(
                  PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED,              // src
                  PW_EEPROM_ADDR_IMG_CURRENT_PEER_POKEMON_ANIMATED_SMALL, // dst
                  PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED,              // size
                  read_size, &(comms->advertising_attempts), packet, max_len
              );

        size_t cur_read_size   = (size_t)(comms->advertising_attempts) * read_size;

        if(cur_read_size >= PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED) {
            comms->advertising_attempts = 0;  // reset loop counter
            comms->current_substate = COMM_SUBSTATE_SEND_PEER_PLAY_DX;
        }
        break;
    }
    case COMM_SUBSTATE_SEND_PEER_PLAY_DX: {
        packet->cmd = CMD_PEER_PLAY_DX;
        packet->extra = 1;

        create_peer_play_data((peer_play_data_t*)packet->payload);


        // TODO: move sizze to #define
        err = pw_ir_send_packet(packet, 0x40, &n_read);;
        if(err != IR_OK) return err;

        comms->current_substate = COMM_SUBSTATE_RECV_PEER_PLAY_DX;
        break;
    }
    case COMM_SUBSTATE_RECV_PEER_PLAY_DX: {
        err = pw_ir_recv_packet(packet, 0x40, &n_read);
        if(err != IR_OK) return err;

        pw_eeprom_write(PW_EEPROM_ADDR_CURRENT_PEER_DATA, packet->payload, PW_EEPROM_SIZE_CURRENT_PEER_DATA);

        comms->current_substate = COMM_SUBSTATE_SEND_PEER_PLAY_END;
        break;
    }
    case COMM_SUBSTATE_SEND_PEER_PLAY_END: {
        packet->cmd = CMD_PEER_PLAY_END;
        packet->extra = EXTRA_BYTE_TO_WALKER;
        err = pw_ir_send_packet(packet, 8, &n_read);

        comms->current_substate = COMM_SUBSTATE_RECV_PEER_PLAY_END;
        break;
    }
    case COMM_SUBSTATE_RECV_PEER_PLAY_END: {
        err = pw_ir_recv_packet(packet, 8, &n_read);
        if(err != IR_OK) return err;
        if(packet->cmd != CMD_PEER_PLAY_END) return IR_ERR_UNEXPECTED_PACKET;
        comms->current_substate = COMM_SUBSTATE_CALCULATE_PEER_PLAY_GIFT;
        break;
    }
    case COMM_SUBSTATE_DISPLAY_PEER_PLAY_ANIMATION: {
        err = IR_ERR_NOT_IMPLEMENTED;
        break;
    }
    case COMM_SUBSTATE_CALCULATE_PEER_PLAY_GIFT: {
        err = IR_ERR_NOT_IMPLEMENTED;
        break;
    }
    default:
        err = IR_ERR_UNKNOWN_SUBSTATE;
        break;
    }

    return err;
}


/*
 *  Send an eeprom section from `src` on host to `dst` on peer.
 *  Throws error if `dst` or `final_write_size` isn't 128-byte aligned
 *
 *  Designed to be run in a loop, hence only one read and one write.
 */
ir_err_t pw_action_send_large_raw_data_from_eeprom(uint16_t src, uint16_t dst, size_t final_write_size,
        size_t write_size, uint8_t *pcounter, pw_packet_t *packet, size_t max_len) {
    (void)max_len;
    ir_err_t err = IR_ERR_UNHANDLED_ERROR;

    size_t cur_write_size   = (size_t)(*pcounter) * write_size;
    uint16_t cur_write_addr = dst + cur_write_size;
    uint16_t cur_read_addr  = src + cur_write_size;
    size_t n_read = 0;

    // If we have written something, we expect an acknowledgment
    if(cur_write_size > 0) {
        err = pw_ir_recv_packet(packet, 8, &n_read);
        if(err != IR_OK) return err;
        if(packet->cmd != CMD_EEPROM_WRITE_ACK) return IR_ERR_UNEXPECTED_PACKET;
    }

    if( (cur_write_addr&0x07) > 0) return IR_ERR_UNALIGNED_WRITE;
    //if( (final_write_size&0x07) > 0) return IR_ERR_UNALIGNED_WRITE;   // walker can handle this

    pw_time_delay_ms(ACTION_DELAY_MS);

    if( cur_write_size < final_write_size) {
        packet->cmd = (uint8_t)(cur_write_addr&0xff) + 2; // Need +2 to make it raw write command
        packet->extra = (uint8_t)(cur_write_addr>>8);
        pw_eeprom_read(cur_read_addr, packet->payload, write_size);

        err = pw_ir_send_packet(packet, 8+write_size, &n_read);
        if(err != IR_OK) return err;
        (*pcounter)++;
    }

    return err;
}


/*
 *  Send an eeprom section from `src` on peer to `dst` on host.
 *
 *  Designed to be run in a loop, hence only one read and one write.
 */
ir_err_t pw_action_read_large_raw_data_from_eeprom(uint16_t src, uint16_t dst, size_t final_read_size,
        size_t read_size, uint8_t *pcounter, pw_packet_t *packet, size_t max_len) {
    (void)max_len;

    ir_err_t err;
    size_t cur_read_size   = (size_t)(*pcounter) * read_size;
    uint16_t cur_write_addr = dst + cur_read_size;
    uint16_t cur_read_addr  = src + cur_read_size;
    size_t n_read = 0;

    size_t remaining_read = final_read_size - cur_read_size;
    if(remaining_read <= 0) return IR_OK;

    read_size = (remaining_read<read_size)?remaining_read:read_size;

    packet->cmd = CMD_EEPROM_READ_REQ;;
    packet->extra = EXTRA_BYTE_TO_WALKER;
    packet->payload[0] = (uint8_t)(cur_read_addr>>8);
    packet->payload[1] = (uint8_t)(cur_read_addr&0xff);
    packet->payload[2] = read_size;

    err = pw_ir_send_packet(packet, 8+3, &n_read);
    if(err != IR_OK) return err;

    pw_time_delay_ms(ACTION_DELAY_MS);

    err = pw_ir_recv_packet(packet, read_size+8, &n_read);
    if(err != IR_OK) return err;
    if(packet->cmd != CMD_EEPROM_READ_RSP) return IR_ERR_UNEXPECTED_PACKET;

    pw_eeprom_write(cur_write_addr, packet->payload, read_size);

    (*pcounter)++;

    // TODO: create a new error for awaiting read/write?
    return err;
}

/*
 * Send a contiguous section of data from `src` ptr to `dst` eeprom address
 *
 *  Designed to be run in a loop, hence only one read and one write.
 */
ir_err_t pw_action_send_large_raw_data_from_pointer(uint8_t *src, uint16_t dst, size_t final_write_size,
        size_t write_size, uint8_t *pcounter, pw_packet_t *packet, size_t max_len) {
    (void)max_len;
    ir_err_t err = IR_ERR_UNHANDLED_ERROR;

    size_t cur_write_size   = (size_t)(*pcounter) * write_size;
    uint16_t cur_write_addr = dst + cur_write_size;
    uint8_t *cur_read_addr  = src + cur_write_size;
    size_t n_read = 0;

    // If we have written something, we expect an acknowledgment
    if(cur_write_size > 0) {
        err = pw_ir_recv_packet(packet, 8, &n_read);
        if(err != IR_OK) return err;
        if(packet->cmd != CMD_EEPROM_WRITE_ACK) return IR_ERR_UNEXPECTED_PACKET;
    }

    if( (cur_write_addr&0x07) > 0) return IR_ERR_UNALIGNED_WRITE;
    //if( (final_write_size&0x07) > 0) return IR_ERR_UNALIGNED_WRITE;   // walker can handle this

    pw_time_delay_ms(ACTION_DELAY_MS);

    if( cur_write_size < final_write_size) {
        packet->cmd = (uint8_t)(cur_write_addr&0xff) + 2; // Need +2 to make it raw write command
        packet->extra = (uint8_t)(cur_write_addr>>8);
        //pw_eeprom_read(cur_read_addr, packet+8, write_size);
        memcpy(packet->payload, cur_read_addr, write_size);

        err = pw_ir_send_packet(packet, 8+write_size, &n_read);
        if(err != IR_OK) return err;
        (*pcounter)++;
    }

    return err;
}

ir_err_t pw_ir_eeprom_do_write(pw_packet_t *packet, size_t len) {
    ir_err_t err = IR_OK;
    uint8_t *data;
    uint8_t wlen = 128;

    uint8_t cmd = packet->cmd;
    uint16_t addr = (packet->extra<<8) | (cmd&0x80);
    // compressed if 0x00 or 0x02 and length < 136
    bool cmp = ( (cmd&0x02) == 0 ) && (len<0x88);

    if(cmp) {
        // decompress
        int e = pw_decompress_data(packet->payload, decompression_buf, len-8);
        if(e != 0) return IR_ERR_BAD_DATA;
        data = decompression_buf;
    } else {
        data = packet->payload;
    }

    if(addr == 0xd700) {
        pw_log_debug("decomp species: %02x%02x\n", data[1], data[0]);
    }

    pw_eeprom_write(addr, data, wlen);

    return err;
}


void pw_ir_end_walk() {

    walker_info_t info;

    int res = pw_eeprom_read_walker_info(&info);
    if(res < 0) {
        pw_log_warn("Can't read walker info\n");
    }

    info.le_unk1 = 0;
    info.le_unk3 = 0;
    info.flags &= ~WALKER_INFO_FLAG_HAS_POKEMON;

    pw_eeprom_write_walker_info(&info);

    walker_info_cache = info;

    health_data_cache.current_watts = 0;
    health_data_cache.event_log_index = 0;
    pw_eeprom_write_health_data(&health_data_cache);

    pw_eeprom_set_area(PW_EEPROM_ADDR_CAUGHT_POKEMON_SUMMARY, 0, 0x64);
    pw_eeprom_set_area(PW_EEPROM_ADDR_EVENT_LOG, 0, PW_EEPROM_SIZE_EVENT_LOG);
    pw_eeprom_set_area(PW_EEPROM_ADDR_RECEIVED_BITFIELD, 0, 0x6c8);
    pw_eeprom_set_area(PW_EEPROM_ADDR_MET_PEER_DATA, 0, 0x1568);
    pw_eeprom_set_area(PW_EEPROM_ADDR_ROUTE_INFO, 0, 0x10);

}


void pw_ir_start_walk() {

    uint8_t *buf = eeprom_buf;

    buf[0] = 0xa5;
    int res = pw_eeprom_reliable_write(
            PW_EEPROM_ADDR_COPY_MARKER_1,
            PW_EEPROM_ADDR_COPY_MARKER_2,
            buf,
            1
        );
    if(res < 0) {
        pw_log_error("Couldn't write copy marker\n");
    }

    // buf_size must wholly divide into copy size
    const size_t sz = 128;
    for(size_t i = 0; i < 0x2900; i+=sz) {
        // Sometimes reads zero here for x amount of bytes
        // but later on line 746, it reads ok
        int a = pw_eeprom_read(PW_EEPROM_ADDR_SCENARIO_STAGING_AREA+i, buf, sz);
        uint16_t species = buf[0] | (uint16_t)(buf[1])<<8;
        if(i == 0 && species == 0) {
            pw_log_debug("n read: %d\n", a);
        }
        pw_eeprom_write(PW_EEPROM_ADDR_ROUTE_INFO+i, buf, sz);
    }

    for(size_t i = 0; i < 0x280; i+=sz) {
        pw_eeprom_read(PW_EEPROM_ADDR_TEAM_DATA_STAGING+i, buf, sz);
        pw_eeprom_read(PW_EEPROM_ADDR_TEAM_DATA_STRUCT+i,  buf, sz);
    }

    buf[0] = 0x00;
    res = pw_eeprom_reliable_write(
            PW_EEPROM_ADDR_COPY_MARKER_1,
            PW_EEPROM_ADDR_COPY_MARKER_2,
            buf,
            1
        );
    if(res < 0) {
        pw_log_error("Couldn't clear copy marker\n");
    }

    health_data_cache.walk_minute_counter = 0;
    health_data_cache.event_log_index = 0;
    health_data_cache.current_watts = 0;

    pw_eeprom_write_health_data(&health_data_cache);

    // this always reads ok, so the write must have been fine
    route_info_t *route_info = (route_info_t*)buf;
    pw_eeprom_read(PW_EEPROM_ADDR_SCENARIO_STAGING_AREA, (uint8_t*)route_info, PW_EEPROM_SIZE_ROUTE_INFO);
    pw_log_debug("d700 species: %04x\n", route_info->pokemon_summary.le_species);


    pw_eeprom_set_area(PW_EEPROM_ADDR_EVENT_LOG, 0, PW_EEPROM_SIZE_EVENT_LOG);
    pw_eeprom_set_area(PW_EEPROM_ADDR_MET_PEER_DATA, 0, 0x1568);
    pw_eeprom_set_area(PW_EEPROM_ADDR_CAUGHT_POKEMON_SUMMARY, 0, 0x64);

    //walker_info_t *info = (walker_info_t*)buf;
    walker_info_t *info = &walker_info_cache;

    res = pw_eeprom_read_walker_info(info);
    if(res != 0) {
        pw_log_error("Reading walker info failed in walk start (%d)\n", res);
    }

    info->le_unk0 = peer_info_cache.le_unk0;
    info->le_unk1 = info->le_unk0;
    info->le_unk2 = peer_info_cache.le_unk2;
    info->le_unk3 = info->le_unk2;

    info->flags &= ~WALKER_INFO_FLAG_POKEMON_JOINED;
    info->flags |= WALKER_INFO_FLAG_INIT | WALKER_INFO_FLAG_HAS_POKEMON;

    info->le_tid = peer_info_cache.le_tid;
    info->le_sid = peer_info_cache.le_sid;

    for(size_t i = 0; i < 8; i++) {
        info->le_trainer_name[i] = peer_info_cache.le_trainer_name[i];
    }

    info->identity_data = peer_info_cache.identity_data;

    info->protocol_ver = peer_info_cache.protocol_ver;
    //info->protocol_ver = 0x02;
    info->protocol_subver = peer_info_cache.protocol_subver;
    info->unk5 = peer_info_cache.unk5;
    //info->unk8 = 0x02;
    info->unk8 = peer_info_cache.unk8;

    pw_eeprom_write_walker_info(info);
    info = 0;

    // Write current time
    // Should be redundant since we set it with command 0x32 and co.
    uint32_t last_sync = swap_bytes_u32(peer_info_cache.be_last_sync);
    pw_time_set_rtc(last_sync);
    health_data_cache.last_sync = last_sync;

    // make walk start event

    event_log_item_t *event_item = malloc(sizeof(*event_item));

    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t*)route_info, PW_EEPROM_SIZE_ROUTE_INFO);
    //pw_log_debug("8f00 species: %04x\n", route_info->pokemon_summary.le_species);
    ////pw_eeprom_read(0xd700, (uint8_t*)route_info, PW_EEPROM_SIZE_ROUTE_INFO);
    ////pw_log_debug("d700 species: %04x\n", route_info->pokemon_summary.le_species);


    //event_item->le_our_species = route_info->pokemon_summary.le_species;
    //event_item->our_pokemon_flags = route_info->pokemon_summary.pokemon_flags_1;
    //for(uint8_t i = 0; i < 11; i++)
    //    event_item->our_pokemon_name[i] = route_info->pokemon_nickname[i];
    //pw_log_debug("event species: %04x\n", event_item->le_our_species);

    //for(size_t i = 0; i < 11; i++)
    //    event_item->our_pokemon_name[i] = route_info->pokemon_nickname[i];

    //event_item->route_image_index = route_info->route_image_index;
    //event_item->event_type = 0x19;

    //pw_log_event(event_item);

    uint8_t zero = 0;
    for(size_t i = 0; i < 24; i++) {
        pw_eeprom_write(PW_EEPROM_ADDR_EVENT_LOG + i*sizeof(event_log_item_t) + 0x84, &zero, 1);
    }

    pw_log_event(event_item, route_info, EVENT_TYPE_WALK_STARTED, 0, false, 0);

    // TODO: Remove this
    // testing more log items

    /*
    pw_log_event(event_item, route_info, EVENT_TYPE_POKEMON_CAUGHT, 0, false, 1);
    pw_log_event(event_item, route_info, EVENT_TYPE_MOOD_HAPPY, 0, false, 0);
    pw_log_event(event_item, route_info, EVENT_TYPE_ITEM_DOWSED, 51, false, 0); // 51 = PP up
    pw_log_event(event_item, route_info, EVENT_TYPE_POKEMON_RAN, route_info->route_pokemon[0].le_species, false, 2);
    */


    free(event_item);
}

//void pw_log_event(event_log_item_t *event_item) {
//    pw_eeprom_write(PW_EEPROM_ADDR_EVENT_LOG, (uint8_t*)event_item, sizeof(*event_item));
//}


ir_err_t pw_ir_identity_ack(pw_packet_t *packet) {
    size_t n_rw;
    switch(packet->cmd) {
    case CMD_IDENTITY_SEND:
        packet->cmd = CMD_IDENTITY_ACK;
        break;
    case CMD_IDENTITY_SEND_ALIAS1:
        packet->cmd = CMD_IDENTITY_ACK_ALIAS1;
        break;
    case CMD_IDENTITY_SEND_ALIAS2:
        packet->cmd = CMD_IDENTITY_ACK_ALIAS2;
        break;
    case CMD_IDENTITY_SEND_ALIAS3:
        packet->cmd = CMD_IDENTITY_ACK_ALIAS3;
        break;
    default:
        return IR_ERR_UNEXPECTED_PACKET;
    }

    packet->extra = EXTRA_BYTE_TO_WALKER;

    for(size_t i = 0; i < sizeof(walker_info_t); i++) {
        ((uint8_t*)(&peer_info_cache))[i] = packet->payload[i];
    }

    pw_log_debug("Peer version %d.%d\n", peer_info_cache.protocol_ver, peer_info_cache.protocol_subver);
    pw_log_debug("We have version %d.%d\n", walker_info_cache.protocol_ver, walker_info_cache.protocol_subver);

    // Set the rtc, that's it
    if(peer_info_cache.be_last_sync != 0) {
        uint32_t last_sync = swap_bytes_u32(peer_info_cache.be_last_sync);
        walker_info_cache.be_last_sync = last_sync;
        health_data_cache.last_sync = last_sync;
        pw_time_set_rtc(health_data_cache.last_sync);
    } else {
        pw_log_debug("peer info last sync was zero\n");
    }

    pw_time_delay_ms(ACTION_DELAY_MS);

    ir_err_t err = pw_ir_send_packet(packet, 8, &n_rw);
    return err;
}


void pw_ir_calculate_peer_play_item(uint32_t seed, bool our_steps_more, app_comms_t *comms) {
    uint8_t reward_index = 0;
    if(seed < 2500) {
        reward_index = our_steps_more ? 8 : 9;
        comms->display_message = 48;
    } else if (seed < 5000) {
        reward_index = our_steps_more ? 6 : 7;
        comms->display_message = 47;
    } else if(seed < 10000) {
        reward_index = our_steps_more ? 4 : 5;
        comms->display_message = 46;
    } else if(seed < 20000) {
        reward_index = our_steps_more ? 2 : 3;
        comms->display_message = 45;
    } else {
        reward_index = our_steps_more ? 0 : 1;
        comms->display_message = 44;
    }

    route_info_t *route_info = (route_info_t*)(eeprom_buf);
    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t*)route_info, PW_EEPROM_SIZE_ROUTE_INFO);
    comms->reward_item = route_info->le_route_items[reward_index];
}


void pw_ir_add_peer_play_item(uint16_t item, uint8_t index) {
    struct item {
        uint16_t le_item;
        uint16_t unused;
    } items[10];
    pw_eeprom_read(PW_EEPROM_ADDR_PEER_PLAY_ITEMS, (uint8_t*)items, PW_EEPROM_SIZE_PEER_PLAY_ITEMS);

    items[index].le_item = item;

    pw_eeprom_write(PW_EEPROM_ADDR_PEER_PLAY_ITEMS, (uint8_t*)items, PW_EEPROM_SIZE_PEER_PLAY_ITEMS);
}


ir_err_t pw_ir_end_peer_play(app_comms_t *comms) {
    peer_play_data_t peer_data;
    pw_eeprom_read(PW_EEPROM_ADDR_CURRENT_PEER_DATA, (uint8_t*)&peer_data, sizeof(peer_play_data_t));
    peer_data.be_current_watts = swap_bytes_u16(peer_data.be_current_watts);
    peer_data.be_current_steps = swap_bytes_u16(peer_data.be_current_steps);

    uint32_t seed = 10*(peer_data.be_current_watts + health_data_cache.current_watts) + peer_data.be_current_steps + health_data_cache.today_steps;
    seed = (seed > 20000) ? 20000 : seed;
    pw_log_debug("Watts: %u, %u; Steps: %u, %u\n", peer_data.be_current_watts, health_data_cache.current_watts, peer_data.be_current_steps, health_data_cache.today_steps);

    pw_log_debug("Peer play seed: %u\n", seed);

    uint8_t free_index = pw_item_get_free_peer_play_index();
    pw_log_debug("Free index: %u\n", free_index);

    if(free_index != 0xff) {
        pw_ir_calculate_peer_play_item(seed, peer_data.be_current_steps < health_data_cache.today_steps, comms);
        pw_ir_add_peer_play_item(comms->reward_item, free_index);
        pw_log_debug("Adding item 0x%04x\n", comms->reward_item);
    } else {
        uint16_t watts_to_add = seed/20;
        watts_to_add = (watts_to_add > 99) ? 99 : watts_to_add;
        pw_log_debug("Adding %u watts\n", watts_to_add);
        health_data_cache.current_watts += watts_to_add;
        if(health_data_cache.current_watts > 9999) {
            health_data_cache.current_watts = 9999;
        }

        // After 10 peers, we don't record any more
        // TODO: Not sure if this is correct behaviour
        return IR_OK;
    }

    { // Limit pointer scope
        event_log_item_t *item = (event_log_item_t*)eeprom_buf;
        memset(item, 0, sizeof(*item));
        route_info_t *route_info = (route_info_t*)(eeprom_buf + sizeof(event_log_item_t));
        pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t*)route_info, PW_EEPROM_SIZE_ROUTE_INFO);

        pw_log_setup_peer_play_event(item, &peer_data);
        event_log_type_t event_type = 1+free_index;

        // TODO: special route
        pw_log_event(item, route_info, event_type, 0, false, 0);
        pw_log_debug("Logged peer play event\n");
    }

    pw_peer_shuffle_team_data();

    return IR_OK;
}


static void create_peer_play_data(peer_play_data_t *ppd) {
    ppd->be_current_steps = swap_bytes_u32(health_data_cache.today_steps);
    ppd->be_current_watts = swap_bytes_u16(health_data_cache.current_watts);
    //ppd->be_current_steps = swap_bytes_u32(9999);
    //ppd->be_current_watts = swap_bytes_u16(9999);

    ppd->le_unk0 = walker_info_cache.le_unk0;
    ppd->le_unk2 = walker_info_cache.le_unk2;

    pokemon_summary_t *ps = (pokemon_summary_t*)eeprom_buf;
    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO+0, (uint8_t*)ps, sizeof(pokemon_summary_t));

    ppd->le_species = ps->le_species;
    ppd->pokemon_flags_1 = ps->pokemon_flags_1;
    ppd->pokemon_flags_2 = ps->pokemon_flags_2;

    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO+10, (uint8_t*)ppd->pokemon_name, 22);
    pw_eeprom_read(PW_EEPROM_ADDR_IDENTITY_DATA_1+72, (uint8_t*)ppd->trainer_name, 16);

}


