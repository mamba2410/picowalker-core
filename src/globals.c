#include <stdint.h>

#include "globals.h"
#include "types.h"


health_data_t health_data_cache;
walker_info_t walker_info_cache;
walker_info_t peer_info_cache;
pw_packet_t packet_buf;

uint8_t eeprom_buf[EEPROM_BUF_SIZE];
uint8_t decompression_buf[DECOMPRESSION_BUF_SIZE];

