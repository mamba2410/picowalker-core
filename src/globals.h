#ifndef PW_BUFFERS_H
#define PW_BUFFERS_H

#include <stdint.h>

#include "types.h"

/// @file globals.h

#define EEPROM_BUF_SIZE         0x300   // largest image to read is 96x32=0x300 bytes
#define DECOMPRESSION_BUF_SIZE  (2*EEPROM_BUF_SIZE)
//#define DECOMPRESSION_BUF_SIZE  0x100
#define PACKET_BUF_SIZE         0x88


extern health_data_t health_data_cache;
extern walker_info_t walker_info_cache;
extern walker_info_t peer_info_cache;
extern pw_packet_t packet_buf;

extern uint8_t eeprom_buf[];
extern uint8_t decompression_buf[];




#endif /* PW_BUFFERS_H */
