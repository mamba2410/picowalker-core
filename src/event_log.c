#include "event_log.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>  // for memset

#include "debug_log.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "globals.h"
#include "picowalker_structures.h"
#include "types.h"
#include "utils.h"

#define FLAGS1_FORM(x)     ((x) & 0x1f)
#define FLAGS1_GENDER(x)   (((x) & 0x20) >> 5)
#define FLAGS2_HAS_FORM(x) ((x) & 0x01)

static uint8_t setup_flags(uint8_t flags1, uint8_t flags2) {
    // [0..4] = form
    // [5..6] = gender (M, F, None)
    // [7]    = special/has form
    uint8_t ret = 0;

    ret |= FLAGS1_FORM(flags1);
    ret |= FLAGS1_GENDER(flags1) << 5;
    ret |= FLAGS2_HAS_FORM(flags2) << 7;

    return ret;
}

void pw_log_event(event_log_item_t *item, route_info_t *ri, event_log_type_t event_type, uint16_t extra,
    bool special_route, uint8_t pokemon_idx) {
    uint8_t next_idx = health_data_cache.event_log_index;

    pw_log_debug("Writing event log for event 0x%02x at index %d...", event_type, next_idx);

    event_log_type_t stored_event_type = EVENT_TYPE_EMPTY_ENTRY;
    pw_eeprom_read(PW_EEPROM_ADDR_EVENT_LOG + next_idx * sizeof(event_log_item_t), (uint8_t *)(&stored_event_type),
        sizeof(event_log_type_t));

    // Only write "fell asleep" if there's no other event there
    if ((event_type == EVENT_TYPE_FELL_ASLEEP) && (stored_event_type != EVENT_TYPE_EMPTY_ENTRY)) {
        return;
    }

    // Don't overwrite the "walk start" event
    if (stored_event_type == EVENT_TYPE_WALK_STARTED) {
        next_idx = (next_idx + 1) % EVENT_LOG_COUNT;
    }

    // Zero log item if its not a peer play one
    if (event_type > EVENT_TYPE_PEER_PLAY_10) {
        memset(item, 0, sizeof(event_log_item_t));
    }

    // Set up event log struct
    item->event_type = event_type;
    item->le_extra = extra;
    item->be_time = swap_bytes_u32(health_data_cache.last_sync + 60 * next_idx);
    item->be_our_watts = swap_bytes_u16(health_data_cache.current_watts);
    item->be_steps = swap_bytes_u32(health_data_cache.today_steps);
    item->le_our_species = ri->pokemon_summary.le_species;

    for (size_t i = 0; i < 11; i++) {
        item->our_pokemon_name[i] = ri->pokemon_nickname[i];
    }

    item->pokemon_friendship = ri->pokemon_happiness;
    item->our_pokemon_flags = setup_flags(ri->pokemon_summary.pokemon_flags_1, ri->pokemon_summary.pokemon_flags_2);

    if (special_route) {
        pw_eeprom_read(0xbf06, (uint8_t *)(&item->route_image_index), 1);
        pw_eeprom_read(PW_EEPROM_ADDR_SPECIAL_ROUTE_NAME_NINTENDOENC, (uint8_t *)(item->route_name),
            PW_EEPROM_SIZE_SPECIAL_ROUTE_NAME_NINTENDOENC);
    } else {
        item->route_image_index = ri->route_image_index;
        for (size_t i = 0; i < 21; i++) {
            item->route_name[i] = ri->route_name[i];
        }
    }

    switch (pokemon_idx) {
        case 1:
        case 2:
        case 3: {
            pokemon_summary_t summary = ri->route_pokemon[pokemon_idx - 1];
            item->le_other_species = summary.le_species;
            item->other_pokemon_flags = setup_flags(summary.pokemon_flags_1, summary.pokemon_flags_2);
            break;
        }
        case 4: {
            if ((event_type == EVENT_TYPE_POKEMON_RAN) || (event_type == EVENT_TYPE_POKEMON_LOST)) {
                pw_eeprom_read(0xbf08, (uint8_t *)&item->le_other_species, 2);  // TODO: magnic number

            } else {
                pw_eeprom_read(0xba44, (uint8_t *)&item->le_other_species, 2);  // TODO: magnic number
            }
            uint8_t other_flags_1;
            uint8_t other_flags_2;
            pw_eeprom_read(0xbf0d, (uint8_t *)&other_flags_1, 1);
            pw_eeprom_read(0xbf0e, (uint8_t *)&other_flags_2, 1);
            item->other_pokemon_flags = setup_flags(other_flags_1, other_flags_2);
            break;
        }
        default:
            break;
    }

    pw_eeprom_write(
        PW_EEPROM_ADDR_EVENT_LOG + next_idx * sizeof(event_log_item_t), (uint8_t *)item, sizeof(event_log_item_t));
    health_data_cache.event_log_index = (next_idx + 1) % EVENT_LOG_COUNT;
    pw_eeprom_write_health_data(&health_data_cache);
}

/*
 * Add peer play specific data to the event log item which the normal function doesn't touch.
 * After this, call `pw_log_event()` with the same `item` ad it should work fine
 */
void pw_log_setup_peer_play_event(event_log_item_t *item, peer_play_data_t *peer_data) {
    item->le_other_species = peer_data->le_species;
    for (uint8_t i = 0; i < 8; i++) {
        item->other_trainer_name[i] = peer_data->trainer_name[i];
    }

    for (uint8_t i = 0; i < 11; i++) {
        item->other_pokemon_name[i] = peer_data->pokemon_name[i];
    }

    item->other_pokemon_flags = setup_flags(peer_data->pokemon_flags_1, peer_data->pokemon_flags_2);
}
