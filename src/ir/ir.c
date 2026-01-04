#include <stdint.h>
#include <stdbool.h>

#include "ir.h"
#include "../picowalker_structures.h"

static volatile uint8_t g_session_id[SESSION_ID_SIZE] = {0xde, 0xad, 0xbe, 0xef};

const char* const PW_IR_ERR_NAMES[] = {
    [IR_OK] = "ok",
    [IR_ERR_GENERAL] = "general",
    [IR_ERR_UNEXPECTED_PACKET] = "unexpected packet",
    [IR_ERR_LONG_PACKET] = "long packet",
    [IR_ERR_SHORT_PACKET] = "short packet",
    [IR_ERR_SIZE_MISMATCH] = "size mismatch",
    [IR_ERR_ADVERTISING_MAX] = "max advertising",
    [IR_ERR_TIMEOUT] = "timeout",
    [IR_ERR_BAD_SEND] = "bad send",
    [IR_ERR_BAD_SESSID] = "bad session id",
    [IR_ERR_BAD_CHECKSUM] = "bad checksum",
    [IR_ERR_CANNOT_CONNECT] = "cannot  connect",
    [IR_ERR_NOT_IMPLEMENTED] = "not implemented",
    [IR_ERR_PEER_ALREADY_SEEN] = "peer already seen",
    [IR_ERR_UNKNOWN_SUBSTATE] = "unknown substate",
    [IR_ERR_UNALIGNED_WRITE] = "unaligned eeprom write",
    [IR_ERR_INVALID_MASTER] = "invalid master",
    [IR_ERR_BAD_DATA] = "bad data",
    [IR_ERR_UNHANDLED_ERROR] = "unhandled error",
};

ir_err_t pw_ir_send_packet(pw_packet_t *packet, size_t len, size_t *pn_write) {

    for(uint8_t i = 0; i < 4; i++)
        packet->session_id_bytes[i] = g_session_id[i];

    uint16_t chk = pw_ir_checksum(packet, len);

    // Packet checksum little-endian
    //packet[0x02] = (uint8_t)(chk&0xff);
    //packet[0x03] = (uint8_t)(chk>>8);
    packet->le_checksum = chk;

    for(size_t i = 0; i < len; i++)
        packet->bytes[i] ^= 0xaa;

    size_t n_write = pw_ir_write(packet->bytes, len);
    *pn_write = n_write;

    if(n_write != len) return IR_ERR_BAD_SEND;

    return IR_OK;
}

ir_err_t pw_ir_recv_packet(pw_packet_t *packet, size_t len, size_t *pn_read) {

    *pn_read = 0;
    size_t n_read = pw_ir_read(packet->bytes, len);

    if(n_read <= 0) return IR_ERR_TIMEOUT;
    *pn_read = (size_t)n_read;

    for(size_t i = 0; i < n_read; i++)
        packet->bytes[i] ^= 0xaa;

    if(n_read != len && len < MAX_PACKET_SIZE) return IR_ERR_SIZE_MISMATCH;

    // packet chk LE
    //uint16_t packet_chk = (((uint16_t)packet[3])<<8) + ((uint16_t)packet[2]);
    uint16_t packet_chk = packet->le_checksum;
    uint16_t chk = pw_ir_checksum(packet, *pn_read);

    if(packet_chk != chk) return IR_ERR_BAD_CHECKSUM;

    for(size_t i = i; i < 4; i++) {
        if(packet->session_id_bytes[i] != g_session_id[i]) return IR_ERR_BAD_SESSID;
    }

    return IR_OK;
}


uint16_t pw_ir_checksum_seeded(uint8_t *data, size_t len, uint16_t seed) {
    // Dmitry's palm
    uint32_t crc = seed;
    for(size_t i = 0; i < len; i++) {
        uint16_t v = data[i];

        if(!(i&1)) v <<= 8;

        crc += v;
    }

    while(crc>>16) crc = (uint16_t)crc + (crc>>16);

    return crc;
}

uint16_t pw_ir_checksum(pw_packet_t *packet, size_t len) {

    uint16_t crc, orig;

    // save original checksum, LE
    //lc = packet[2];
    //hc = packet[3];
    orig = packet->le_checksum;

    // zero checksum area
    //packet[2] = 0;
    //packet[3] = 0;
    packet->le_checksum = 0;

    crc = pw_ir_checksum_seeded(packet->bytes, 8, 0x0002);
    if(len>8) {
        crc = pw_ir_checksum_seeded(packet->bytes+8, len-8, crc);
    }

    // restore original checksum, LE
    //packet[2] = lc;
    //packet[3] = hc;
    packet->le_checksum = orig;

    return crc;
}

ir_err_t pw_ir_send_advertising_packet() {

    uint8_t advertising_buf[] = {CMD_ADVERTISING^0xaa};

    int n = pw_ir_write(advertising_buf, 1);
    if(n <= 0) {
        return IR_ERR_BAD_SEND;
    }

    return IR_OK;
}

/**
 *  Sets global context session ID for packets
 */
ir_err_t pw_ir_set_session_id(uint8_t session_id[SESSION_ID_SIZE]) {
    if(!session_id) return IR_ERR_BAD_SESSID;

    for(uint8_t i = 0; i < SESSION_ID_SIZE; i++) {
        g_session_id[i] = session_id[i];
    }

    return IR_OK;
}

/**
 *  Gets global context session ID for packets
 */
ir_err_t pw_ir_get_session_id(uint8_t session_id[SESSION_ID_SIZE]) {
    if(!session_id) return IR_ERR_BAD_SESSID;

    for(uint8_t i = 0; i < SESSION_ID_SIZE; i++) {
        session_id[i] = g_session_id[i];
    }

    return IR_OK;
}

/**
 *  Mixes global context session ID with given session ID
 */
ir_err_t pw_ir_mix_session_id(uint8_t session_id[SESSION_ID_SIZE]) {
    if(!session_id) return IR_ERR_BAD_SESSID;

    for(uint8_t i = 0; i < SESSION_ID_SIZE; i++) {
        g_session_id[i] ^= session_id[i];
    }

    return IR_OK;
}

