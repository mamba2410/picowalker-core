#ifndef PW_UTILS_H
#define PW_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "eeprom.h"

/// @file utils.h

#define INV_WALKING_POKEMON     (1<<0)
#define INV_CAUGHT_POKEMON_1    (1<<1)
#define INV_CAUGHT_POKEMON_2    (1<<2)
#define INV_CAUGHT_POKEMON_3    (1<<3)
#define INV_EXTRA_POKEMON       (1<<4)
#define INV_DOWSED_ITEM_1       (1<<1)
#define INV_DOWSED_ITEM_2       (1<<2)
#define INV_DOWSED_ITEM_3       (1<<3)
#define INV_EXTRA_ITEM          (1<<4)
#define INV_HAVE_HEART          (1<<0)
#define INV_HAVE_SPADE          (1<<1)
#define INV_HAVE_DIAMOND        (1<<2)
#define INV_HAVE_CLUB           (1<<3)

typedef enum {
    PIDX_WALKING,
    PIDX_OPTION_A,
    PIDX_OPTION_B,
    PIDX_OPTION_C,
    PIDX_EXTRA,
    N_PIDX,
} pokemon_index_t;

/*
 * For pokemon and items screen.
 * in context of packed inventory
 */
typedef enum {
    PI_WALKING_POKEMON,
    PI_CAUGHT_POKEMON_1,
    PI_CAUGHT_POKEMON_2,
    PI_CAUGHT_POKEMON_3,
    PI_EXTRA_POKEMON,
    PI_EMPTY_SLOT,
    PI_DOWSED_ITEM_1,
    PI_DOWSED_ITEM_2,
    PI_DOWSED_ITEM_3,
    PI_EXTRA_ITEM,
    N_PACKED_INDICES
} packed_index_t;

typedef struct {
    uint8_t     caught_pokemon; // [0]=walking [1..3]=caught, [4]=event
    uint8_t     dowsed_items; // [1..3] = dowsed, [4]=event
    uint16_t    peer_play_items; // [0..9] = peer play gifted
    uint8_t     n_peer_play_items;
    uint8_t     received_bitfield;
    uint16_t    packed; // [0]=walking, [1..3]=caught, [4]=event mon, [5]=none, [6..8]=dowsed, [9]=event item
} pw_brief_inventory_t;

typedef struct {
    union {
        struct {
            uint16_t walking_pokemon;
            uint16_t caught_pokemon[3];
            uint16_t event_pokemon;
            uint16_t pad;
            uint16_t dowsed_items[3];
            uint16_t event_item;
        };
        uint16_t entries[10];
    };
    uint16_t gifted_items[10];
} pw_detailed_inventory_t;

inline uint16_t swap_bytes_u16(uint16_t x) {
    uint16_t y = (x>>8) | ((x&0xff)<<8);
    return y;
}

inline uint32_t swap_bytes_u32(uint32_t x) {
    uint32_t y = (x>>24) | ((x&0x00ff0000)>>8) | ((x&0x0000ff00)<<8) | ((x&0xff)<<24);
    return y;
}

void pw_read_inventory(pw_brief_inventory_t *brief, pw_detailed_inventory_t *detailed);

void pw_pokemon_index_to_small_sprite(pokemon_index_t idx, uint8_t *buf, uint8_t frame);
void pw_pokemon_index_to_name(pokemon_index_t idx, uint8_t *buf);
void pw_item_index_to_name(uint8_t idx, uint8_t *buf);

//int nintendo_to_ascii(uint8_t *str, char* buf, size_t len);

#endif /* PW_UTILS_H */
