#include <stdint.h>
#include <stddef.h>

#include "utils.h"
#include "states.h"
#include "types.h"
#include "eeprom_map.h"
#include "eeprom.h"

extern uint16_t swap_bytes_u16(uint16_t x);
extern uint32_t swap_bytes_u32(uint32_t x);

/*
 * Sets sv->reg{a,b,c}
 *
 * reg_a = [0]=walking_mon, [1..3]=caught_pokemon, [4]=event_pokemon
 * reg_b = [1..3]=found_items, [4]=event_item
 * reg_c = [0..3]=stamps
 */
void pw_read_inventory(pw_brief_inventory_t *brief, pw_detailed_inventory_t *detailed) {
    if(!brief || !detailed) return;

    *brief = (pw_brief_inventory_t) {
        0
    };
    *detailed = (pw_detailed_inventory_t) {
        0
    };

    pokemon_summary_t caught_pokemon[3];

    // walking pokemon
    pw_eeprom_read(PW_EEPROM_ADDR_ROUTE_INFO, (uint8_t*)(caught_pokemon), sizeof(pokemon_summary_t));

    detailed->walking_pokemon = caught_pokemon[0].le_species;
    if(caught_pokemon[0].le_species != 0 && caught_pokemon[0].le_species != 0xffff) {
        brief->caught_pokemon |= INV_WALKING_POKEMON;
    }

    // normal caught pokemon
    pw_eeprom_read(
        PW_EEPROM_ADDR_CAUGHT_POKEMON_SUMMARY,
        (uint8_t*)caught_pokemon,
        PW_EEPROM_SIZE_CAUGHT_POKEMON_SUMMARY
    );

    for(uint8_t i = 0; i < 3; i++) {
        detailed->caught_pokemon[i] = caught_pokemon[i].le_species;
        if(caught_pokemon[i].le_species != 0 && caught_pokemon[i].le_species != 0xffff) {
            brief->caught_pokemon |= (1<<(i+1));
        }
    }

    // event pokemon
    pw_eeprom_read(
        PW_EEPROM_ADDR_EVENT_POKEMON_BASIC_DATA,
        (uint8_t*)(caught_pokemon),
        sizeof(pokemon_summary_t)
    );

    detailed->event_pokemon = caught_pokemon[0].le_species;
    if(caught_pokemon[0].le_species != 0 && caught_pokemon[0].le_species != 0xffff) {
        brief->caught_pokemon |= INV_EXTRA_POKEMON;
    }


    struct {
        uint16_t le_item;
        uint16_t pad;
    } items[10];

    // normal dowsing items
    pw_eeprom_read(
        PW_EEPROM_ADDR_OBTAINED_ITEMS,
        (uint8_t*)(items),
        PW_EEPROM_SIZE_OBTAINED_ITEMS
    );

    for(uint8_t i = 0; i < 3; i++) {
        detailed->dowsed_items[i] = items[i].le_item;
        if(items[i].le_item != 0 && items[i].le_item != 0xffff) {
            brief->dowsed_items |= (1<<(i+1));
        }
    }

    // event dowsing item
    pw_eeprom_read(
        PW_EEPROM_ADDR_EVENT_ITEM+6,    // ignore first 6 bytes of zeroes
        (uint8_t*)items,
        2
    );

    detailed->event_item = items[0].le_item;
    if(items[0].le_item != 0 && items[0].le_item != 0xffff) {
        brief->dowsed_items |= INV_EXTRA_ITEM;
    }


    // special inventory (stamps, etc)
    uint8_t special_inventory;
    pw_eeprom_read(
        PW_EEPROM_ADDR_RECEIVED_BITFIELD,
        &(brief->received_bitfield),
        1
    );

    // gifted items from peer play
    pw_eeprom_read(
        PW_EEPROM_ADDR_PEER_PLAY_ITEMS,
        (uint8_t*)items,
        PW_EEPROM_SIZE_PEER_PLAY_ITEMS
    );

    brief->n_peer_play_items = 0;
    for(size_t i = 0; i < 10; i++) {
        detailed->gifted_items[i] = items[i].le_item;
        if(items[i].le_item != 0 && items[i].le_item != 0xffff) {
            brief->n_peer_play_items++;
            brief->peer_play_items |= 1<<i;
        }
    }


    /*
     * packing
     * [0]    = walking mon
     * [1..3] = caught mons
     * [4]    = event mon
     * [5]    = empty
     * [6..8] = dowsed items
     * [9]    = event item
     */
    brief->packed = (brief->caught_pokemon) | (brief->dowsed_items<<5);

}

void pw_pokemon_index_to_small_sprite(pokemon_index_t idx, uint8_t *buf, uint8_t frame) {
    eeprom_addr_t addr;

    switch(idx) {
    case PIDX_WALKING: {
        addr = PW_EEPROM_ADDR_IMG_POKEMON_SMALL_ANIMATED +
               frame*PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED_FRAME;
        break;
    }
    case PIDX_EXTRA: {
        addr = PW_EEPROM_ADDR_IMG_EVENT_POKEMON_SMALL_ANIMATED +
               frame*PW_EEPROM_SIZE_IMG_EVENT_POKEMON_SMALL_ANIMATED_FRAME;
        break;
    }
    case PIDX_OPTION_A:
    case PIDX_OPTION_B:
    case PIDX_OPTION_C: {
        uint8_t offs = 2*(idx-1);
        addr = PW_EEPROM_ADDR_IMG_ROUTE_POKEMON_SMALL_ANIMATED
               + (offs+frame)*PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED_FRAME;
        break;
    }
    default: {
        return;
        break;
    }
    }

    pw_eeprom_read(addr, buf, PW_EEPROM_SIZE_IMG_POKEMON_SMALL_ANIMATED_FRAME);

}

/**
 *
 */
void pw_pokemon_index_to_name(pokemon_index_t idx, uint8_t *buf) {
    eeprom_addr_t addr;

    switch(idx) {
    case PIDX_WALKING: {
        addr = PW_EEPROM_ADDR_TEXT_POKEMON_NAME;
        break;
    }
    case PIDX_EXTRA: {
        addr = PW_EEPROM_ADDR_TEXT_SPECIAL_POKEMON_NAME;
        break;
    }
    case PIDX_OPTION_A:
    case PIDX_OPTION_B:
    case PIDX_OPTION_C: {
        uint8_t offs = 2*(idx-1);
        addr = PW_EEPROM_ADDR_TEXT_POKEMON_NAMES + offs*(PW_EEPROM_SIZE_TEXT_POKEMON_NAME);
        break;
    }
    default: {
        return;
        break;
    }
    }

    pw_eeprom_read(addr, buf, PW_EEPROM_SIZE_TEXT_POKEMON_NAME);
}


/**
 *
 */
void pw_item_index_to_name(uint8_t idx, uint8_t *buf) {
    uint16_t addr;

    if(idx < 10) {
        addr = PW_EEPROM_ADDR_TEXT_ITEM_NAMES + idx*PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE;
    } else if(idx == 10) {
        addr = PW_EEPROM_ADDR_TEXT_EVENT_ITEM_NAME;
    } else {
        return;
    }

    pw_eeprom_read(addr, buf, PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE);
}


