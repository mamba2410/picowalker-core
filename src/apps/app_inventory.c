#include <stdint.h>

#include "app_inventory.h"

#include "../eeprom.h"
#include "../eeprom_map.h"
#include "../screen.h"
#include "../buttons.h"
#include "../globals.h"
#include "../types.h"

/// @file app_inventory.c

static pw_brief_inventory_t gbrief;
static pw_detailed_inventory_t gdetailed;

enum search_type {
    SEARCH_POKEMON_NAME,
    SEARCH_POKEMON_SPRITE,
    SEARCH_ITEM_NAME,
    SEARCH_ITEM_SPRITE,
};

enum subscreen_type {
    SUBSCREEN_FOUND,
    SUBSCREEN_PRESENTS,
    SUBSTATE_GO_TO_SPLASH,
    SUBSTATE_GO_TO_MENU,
    N_SUBSCREENS,
};

static void pw_inventory_draw_screen1(pw_state_t *s, const screen_flags_t *sf);
static void pw_inventory_draw_screen2(pw_state_t *s, const screen_flags_t *sf);
static void pw_inventory_update_screen1(pw_state_t *s, const screen_flags_t *sf);
static void pw_inventory_update_screen2(pw_state_t *s, const screen_flags_t *sf);

static void pw_inventory_move_cursor(pw_state_t *s, int8_t m);
static pokemon_index_t pw_pokemon_id_to_pokemon_index(uint16_t id);
static uint8_t pw_item_id_to_item_index(uint16_t id);

state_void_func_t const draw_funcs[N_SUBSCREENS] = {
    [SUBSCREEN_FOUND]       = pw_inventory_draw_screen1,
    [SUBSCREEN_PRESENTS]    = pw_inventory_draw_screen2,
    [SUBSTATE_GO_TO_SPLASH] = pw_empty_event,
    [SUBSTATE_GO_TO_MENU]   = pw_empty_event,
};

state_void_func_t const update_funcs[N_SUBSCREENS] = {
    [SUBSCREEN_FOUND]       = pw_inventory_update_screen1,
    [SUBSCREEN_PRESENTS]    = pw_inventory_update_screen2,
    [SUBSTATE_GO_TO_SPLASH] = pw_empty_event,
    [SUBSTATE_GO_TO_MENU]   = pw_empty_event,
};


void pw_inventory_init(pw_state_t *s, const screen_flags_t *sf) {

    pw_read_inventory(&gbrief, &gdetailed);

    s->inventory.current_cursor = 0;
    s->inventory.previous_cursor = 0;
    s->inventory.current_substate = SUBSCREEN_FOUND;
    s->inventory.previous_substate = SUBSCREEN_FOUND;

}


void pw_inventory_init_display(pw_state_t *s, const screen_flags_t *sf) {
    draw_funcs[s->inventory.current_substate](s, sf);
}


void pw_inventory_update_display(pw_state_t *s, const screen_flags_t *sf) {
    if(s->inventory.previous_substate != s->inventory.current_substate) {
        pw_screen_clear();
        draw_funcs[s->inventory.current_substate](s, sf);
    } else {
        update_funcs[s->inventory.current_substate](s, sf);
    }

    s->inventory.previous_substate = s->inventory.current_substate;
}


void pw_inventory_handle_input(pw_state_t *s, const screen_flags_t *sf, uint8_t b) {
    switch(b) {
    case BUTTON_L: {
        pw_inventory_move_cursor(s, -1);
        break;
    };
    case BUTTON_M: {
        if(s->inventory.current_substate == SUBSCREEN_FOUND && gbrief.peer_play_items != 0) {
            s->inventory.current_substate = SUBSCREEN_PRESENTS;
        } else {
            s->inventory.current_substate = SUBSTATE_GO_TO_SPLASH;
        }
    };
    case BUTTON_R: {
        pw_inventory_move_cursor(s, +1);
        break;
    };
    };

}

/*
 *  cursor found screen:
 *
 *  0  1 2 3 4  // pokemon
 *  5  6 7 8 9  // items
 *
 *  0 = walking mon
 *  1-3 = caught mon
 *  4 = special/gifted mon
 *  5 = always empty
 *  6-8 = dowsed items
 *  9 = special/gifted item
 */
static void pw_inventory_move_cursor(pw_state_t *s, int8_t m) {

    switch(s->inventory.current_substate) {
    case SUBSCREEN_FOUND: {
        uint16_t is_filled = 0;

        // move cursor by `m` until it hits a nonzero bit, cursor<0 or cursor>=10
        do {
            s->inventory.current_cursor += m;
            is_filled = gbrief.packed & s->inventory.current_cursor;
        } while( (!is_filled) && (s->inventory.current_cursor>=0) && (s->inventory.current_cursor<=9) );

        if(s->inventory.current_cursor < 0) {
            s->inventory.current_substate = SUBSTATE_GO_TO_MENU;
        }
        if(s->inventory.current_cursor > 9) {
            s->inventory.current_substate = SUBSCREEN_PRESENTS;  // change to presents screen
            s->inventory.current_cursor = 0;
        }
        break;
    }
    case SUBSCREEN_PRESENTS: {
        s->inventory.current_cursor += m;
        if(s->inventory.current_cursor < 0) {
            s->inventory.current_substate = SUBSCREEN_FOUND;
            s->inventory.current_cursor = 10;
            pw_inventory_move_cursor(s, -1);   // laziest way of setting cursor to last non-empty slot
        }

        if(s->inventory.current_cursor >= gbrief.n_peer_play_items)
            s->inventory.current_cursor = gbrief.n_peer_play_items-1;

        break;
    }
    default:
        break;
    }

    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}

static void draw_cursor(pw_state_t *s, const screen_flags_t *sf) {
    uint8_t cx=0, cy=0;
    uint8_t xs[] = {8, 24, 32, 40, 48};
    const uint8_t yp = 24, yi = 40;
    uint8_t x0 = 16, y0 = 24;

    switch(s->inventory.current_substate) {
    case SUBSCREEN_FOUND: {
        cx = xs[ (s->inventory.current_cursor)%5 ];
        cy = (s->inventory.current_cursor>5)?yi:yp;
        cy -= 8;

        break;
    }
    case SUBSCREEN_PRESENTS: {
        cx = x0 + 8*(s->inventory.current_cursor%5);
        cy = y0 + 16*(s->inventory.current_cursor/5) - 8;

        break;
    }
    default:
        break;
    }

    uint16_t addr = (sf->frame&ANIM_FRAME_NORMAL_TIME)?PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL:PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;

    pw_screen_draw_from_eeprom(
        cx, cy,
        8, 8,
        addr,
        PW_EEPROM_SIZE_IMG_ARROW
    );
}


static void draw_animated_sprite(pw_state_t *s, const screen_flags_t *sf) {

    uint8_t *buf = eeprom_buf;
    uint8_t idx, w;
    size_t size;
    pw_img_t sprite;
    enum search_type type;

    if(s->inventory.current_cursor == PI_EMPTY_SLOT) return;
    bool is_pokemon = s->inventory.current_substate == SUBSCREEN_FOUND && s->inventory.current_cursor < PI_EMPTY_SLOT;


    if(is_pokemon) {
        pokemon_index_t pokemon_index = pw_pokemon_id_to_pokemon_index(gdetailed.entries[s->inventory.current_cursor]);
        pw_pokemon_index_to_small_sprite(pokemon_index, buf, (sf->frame&ANIM_FRAME_NORMAL_TIME)>>ANIM_FRAME_NORMAL_TIME_OFFSET);
    } else {
        pw_eeprom_read(
            PW_EEPROM_ADDR_IMG_ITEM,
            buf,
            PW_EEPROM_SIZE_IMG_ITEM
        );
    }

    sprite = (pw_img_t) {
        .data=buf, .width=32, .height=24, .size=size
    };
    pw_screen_draw_img(&sprite, SCREEN_WIDTH-32-4, SCREEN_HEIGHT-16-24);

}



static void draw_name(pw_state_t *s, const screen_flags_t *sf) {

    uint8_t *buf = eeprom_buf;
    uint8_t w;
    size_t size;
    pw_img_t sprite;

    if(s->inventory.current_cursor == PI_EMPTY_SLOT) return;

    bool is_pokemon = s->inventory.current_substate == SUBSCREEN_FOUND && s->inventory.current_cursor < PI_EMPTY_SLOT;

    if(is_pokemon) {
        // we're looking at a pokemon
        pokemon_index_t pokemon_index = pw_pokemon_id_to_pokemon_index(gdetailed.entries[s->inventory.current_cursor]);
        pw_pokemon_index_to_name(pokemon_index, buf);
        sprite = (pw_img_t) {
            .width=80, .height=16, .data=buf, .size=PW_EEPROM_SIZE_TEXT_POKEMON_NAME
        };
    } else {
        // we're looking at an item
        uint8_t item_idx = pw_item_id_to_item_index(gdetailed.entries[s->inventory.current_cursor]);
        pw_item_index_to_name(item_idx, buf);
        sprite = (pw_img_t) {
            .width=96, .height=16, .data=buf, PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE
        };
    }

    pw_screen_draw_img(&sprite, 0, SCREEN_HEIGHT-16);
    pw_screen_draw_text_box(0, SCREEN_HEIGHT-16, SCREEN_WIDTH, 16, SCREEN_BLACK);

}

static void pw_inventory_draw_screen1(pw_state_t *s, const screen_flags_t *sf) {

    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN
    );

    pw_screen_draw_from_eeprom(
        SCREEN_WIDTH-8, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RIGHT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RIGHT
    );

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_IMG_MENU_TITLE_INVENTORY,
        PW_EEPROM_SIZE_IMG_MENU_TITLE_INVENTORY
    );

    // Draw icons
    uint8_t buf_pokeball[PW_EEPROM_SIZE_IMG_BALL];
    uint8_t buf_item[PW_EEPROM_SIZE_IMG_ITEM];

    pw_img_t pokeball = {.height=8, .width=8, .data=buf_pokeball, .size=PW_EEPROM_SIZE_IMG_BALL};
    pw_eeprom_read(PW_EEPROM_ADDR_IMG_BALL, buf_pokeball, PW_EEPROM_SIZE_IMG_BALL);

    pw_img_t item = {.height=8, .width=8, .data=buf_item, .size=PW_EEPROM_SIZE_IMG_ITEM};
    pw_eeprom_read(PW_EEPROM_ADDR_IMG_ITEM, buf_item, PW_EEPROM_SIZE_IMG_ITEM);

    uint8_t xs[] = {8, 24, 32, 40, 48};
    const uint8_t yp = 24, yi = 40;

    // draw normal pokeballs
    for(uint8_t i = 0; i < 3; i++) {
        if(gbrief.caught_pokemon & (1<<(i+1))) {
            pw_screen_draw_img(&pokeball, xs[i], yp);
        }
    }

    // draw normal items
    for(uint8_t i = 0; i < 3; i++) {
        if(gbrief.dowsed_items & (1<<(i+1))) {
            pw_screen_draw_img(&item, xs[i], yi);
        }
    }


    // draw special pokeball
    if(gbrief.caught_pokemon & INV_EXTRA_POKEMON) {
        pw_screen_draw_from_eeprom(
            xs[4], yp,
            8, 8,
            PW_EEPROM_ADDR_IMG_BALL_LIGHT,
            PW_EEPROM_SIZE_IMG_BALL_LIGHT
        );
    }

    // draw special item
    if(gbrief.dowsed_items & INV_EXTRA_ITEM) {
        pw_screen_draw_from_eeprom(
            xs[4], yi,
            8, 8,
            PW_EEPROM_ADDR_IMG_ITEM_LIGHT,
            PW_EEPROM_SIZE_IMG_ITEM_LIGHT
        );
    }


    draw_cursor(s, sf);
    draw_name(s, sf);
    draw_animated_sprite(s, sf);

}


static void pw_inventory_draw_screen2(pw_state_t *s, const screen_flags_t *sf) {
    // PEER_PLAY_ITEMS {u16 item, u16 pad}[10]
    // PRESENT_LARGE

    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_LEFT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_LEFT
    );

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_IMG_MENU_TITLE_INVENTORY,
        PW_EEPROM_SIZE_IMG_MENU_TITLE_INVENTORY
    );


    uint8_t buf_item[PW_EEPROM_SIZE_IMG_ITEM];
    pw_img_t item = {.height=8, .width=8, .data=buf_item, .size=PW_EEPROM_SIZE_IMG_ITEM};
    pw_eeprom_read(PW_EEPROM_ADDR_IMG_ITEM, buf_item, PW_EEPROM_SIZE_IMG_ITEM);

    uint8_t x0 = 16, y0 = 24;
    for(uint8_t i = 0; i < gbrief.n_peer_play_items; i++) {
        pw_screen_draw_img(
            &item,
            x0 + 8*(i%5),
            y0 + 16*(i/5)
        );
    }

    // don't draw this if we don't have presents
    if(gbrief.n_peer_play_items > 0) {

        draw_cursor(s, sf);

        pw_screen_draw_from_eeprom(
            SCREEN_WIDTH-32-4, SCREEN_HEIGHT-16-24,
            32, 24,
            PW_EEPROM_ADDR_IMG_PRESENT_LARGE,
            PW_EEPROM_SIZE_IMG_PRESENT_LARGE
        );

        draw_name(s, sf);
    }
}


static void pw_inventory_update_screen1(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_clear_area(0, 16, 56, 8);
    pw_screen_clear_area(0, 32, 56, 8);

    draw_cursor(s, sf);
    draw_name(s, sf);
    draw_animated_sprite(s, sf);

}


static void pw_inventory_update_screen2(pw_state_t *s, const screen_flags_t *sf) {

    uint8_t x0 = 16, y0 = 24;
    pw_screen_clear_area(x0, y0-8, 40, 8);
    pw_screen_clear_area(x0, y0-8+16, 40, 8);


    // don't draw this if we don't have presents
    if(gbrief.n_peer_play_items > 0) {
        draw_cursor(s, sf);
        draw_name(s, sf);
    }
}

void pw_inventory_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    switch(s->inventory.current_substate) {
    case SUBSTATE_GO_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        break;
    }
    case SUBSTATE_GO_TO_MENU: {
        p->sid = STATE_MAIN_MENU;
        p->menu.cursor = 4;
        break;
    }
    default:
        break;
    }
}

/**
 *  Convert a Pokemon species id into a `pokemon_index_t` enum.
 *  For use with getting sprites and data.
 *
 *  @param id Pokemon species ID.
 *
 *  @return `pokemon_index_t` containing which route pokemon slot it is
 */
static pokemon_index_t pw_pokemon_id_to_pokemon_index(uint16_t id) {
    pokemon_summary_t pokes[N_PIDX];

    pw_eeprom_read(
        PW_EEPROM_ADDR_ROUTE_INFO+0x0000, // offset 0 = current pokemon
        (uint8_t*)(&pokes[0]),
        sizeof(pokemon_summary_t)
    );

    pw_eeprom_read(
        PW_EEPROM_ADDR_ROUTE_POKEMON,
        (uint8_t*)(&pokes[1]),
        3*sizeof(pokemon_summary_t)
    );

    pw_eeprom_read(
        PW_EEPROM_ADDR_EVENT_POKEMON_BASIC_DATA, // in inventory so look here for both gifted and special pokemon
        (uint8_t*)(&pokes[4]),
        sizeof(pokemon_summary_t)
    );

    for(size_t i = 0; i < N_PIDX; i++) {
        if(pokes[i].le_species == id) return (pokemon_index_t)i;
    }

    // unreachable, hopefully
    return (pokemon_index_t)0;
}


/**
 * Convert item id into index of route-available items
 */
static uint8_t pw_item_id_to_item_index(uint16_t id) {

    uint16_t items[11];

    pw_eeprom_read(
        PW_EEPROM_ADDR_ROUTE_ITEMS,
        (uint8_t*)items,
        PW_EEPROM_SIZE_ROUTE_ITEMS
    );

    pw_eeprom_read(
        PW_EEPROM_ADDR_EVENT_ITEM+0x0006, // in inventory, so look here for gifted and special item.
        (uint8_t*)(&items[10]),
        2
    );

    for(size_t i = 0; i < 11; i++) {
        if(items[i] == id) return (uint8_t)i;
    }

    // unreachable, hopefully
    return 0;
}

