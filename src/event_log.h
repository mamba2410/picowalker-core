#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "types.h"

//#define EVENT_LOG_COUNT 24
#define EVENT_LOG_COUNT 23 // Original has 23, but theres space for 24

void pw_log_event(event_log_item_t *item, route_info_t *ri, event_log_type_t log_type, uint16_t extra, bool special_route, uint8_t their_pokemon_idx);

#endif /* EVENT_LOG_H */
