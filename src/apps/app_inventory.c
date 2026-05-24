#include <stdint.h>

#include "app_inventory.h"

#include "../eeprom.h"
#include "../eeprom_map.h"
#include "../screen.h"
#include "../audio.h"
#include "../buttons.h"
#include "../globals.h"
#include "../types.h"
#include "../utils.h"

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
    (void)sf;
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


void pw_inventory_handle_input(pw_state_t *s, const screen_flags_t *sf, pw_buttons_t b) {
    (void)sf;
    switch(b) {
    case PW_BUTTON_L: {
        pw_inventory_move_cursor(s, -1);
        break;
    };
    case PW_BUTTON_M: {
        if(s->inventory.current_substate == SUBSCREEN_FOUND && gbrief.peer_play_items != 0) {
            s->inventory.current_substate = SUBSCREEN_PRESENTS;
        } else {
            s->inventory.current_substate = SUBSTATE_GO_TO_SPLASH;
        }
        break;
    };
    case PW_BUTTON_R: {
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
    uint8_t old_substate = s->inventory.current_substate;

    switch(s->inventory.current_substate) {
    case SUBSCREEN_FOUND: {
        uint16_t is_filled = 0;

        // move cursor by `m` until it hits a nonzero bit, cursor<0 or cursor>=10
        do {
            s->inventory.current_cursor += m;
            is_filled = gbrief.packed & (1<<s->inventory.current_cursor);
        } while( (!is_filled) && (s->inventory.current_cursor>=0) && (s->inventory.current_cursor<=9) );

        if(s->inventory.current_cursor < 0) {
            s->inventory.current_substate = SUBSTATE_GO_TO_MENU;
        }
        if(s->inventory.current_cursor > 9) {
            if(gbrief.n_peer_play_items > 0) {
                s->inventory.current_substate = SUBSCREEN_PRESENTS;  // change to presents screen
                s->inventory.current_cursor = 0;
                pw_audio_play_sound(SOUND_CURSOR_MOVE);
            } else {
                s->inventory.current_cursor = 9;
                pw_audio_play_sound(SOUND_NAVIGATE_BACK);
                pw_inventory_move_cursor(s, -1);   // laziest way of setting cursor to last non-empty slot
            }
        } else if(old_substate == s->inventory.current_substate) {
            pw_audio_play_sound(SOUND_CURSOR_MOVE);
        }
        break;
    }
    case SUBSCREEN_PRESENTS: {
        s->inventory.current_cursor += m;
        if(s->inventory.current_cursor < 0) {
            s->inventory.current_substate = SUBSCREEN_FOUND;
            s->inventory.current_cursor = 10;
            pw_audio_play_sound(SOUND_CURSOR_MOVE);
            pw_inventory_move_cursor(s, -1);   // laziest way of setting cursor to last non-empty slot
        } else if(s->inventory.current_cursor >= gbrief.n_peer_play_items) {
            s->inventory.current_cursor = gbrief.n_peer_play_items-1;
            pw_audio_play_sound(SOUND_NAVIGATE_BACK);
        } else {
            pw_audio_play_sound(SOUND_CURSOR_MOVE);
        }

        break;
    }
    default:
        break;
    }

    PW_SET_REQUEST(s->requests, PW_REQUEST_REDRAW);
}


static void get_cursor_coords(pw_state_t *s, pw_screen_pos_t *cx, pw_screen_pos_t *cy) {
    uint8_t xs[] = {8, 24, 32, 40, 48};
    const uint8_t yp = 24, yi = 40;
    uint8_t x0 = 16, y0 = 24;

    switch(s->inventory.current_substate) {
    case SUBSCREEN_FOUND: {
        *cx = xs[ (s->inventory.current_cursor)%5 ];
        *cy = (s->inventory.current_cursor>5)?yi:yp;
        *cy -= 8;

        break;
    }
    case SUBSCREEN_PRESENTS: {
        *cx = x0 + 8*(s->inventory.current_cursor%5);
        *cy = y0 + 16*(s->inventory.current_cursor/5) - 8;

        break;
    }
    default:
        break;
    }
}

static void draw_cursor(pw_state_t *s, const screen_flags_t *sf) {

    uint16_t addr = (sf->frame&ANIM_FRAME_NORMAL_TIME)?PW_EEPROM_ADDR_IMG_ARROW_DOWN_NORMAL:PW_EEPROM_ADDR_IMG_ARROW_DOWN_OFFSET;

    pw_screen_pos_t cx=0, cy=0;
    get_cursor_coords(s, &cx, &cy);
    pw_screen_draw_from_eeprom(
        cx, cy,
        8, 8,
        addr,
        PW_EEPROM_SIZE_IMG_ARROW,
        true
    );
}


static void draw_animated_sprite(pw_state_t *s, const screen_flags_t *sf) {

    uint8_t *buf = eeprom_buf;
    pw_img_t sprite = {
        .width=32,
        .height=24,
        .data=buf,
        .size=192, // 32*24/4
        .is_flipped=false,
        .lookup_table = {
            .addr=-1,
            .use_alt=true
        }
    };

    if(s->inventory.current_cursor == PI_EMPTY_SLOT) return;
    bool is_pokemon = s->inventory.current_substate == SUBSCREEN_FOUND && s->inventory.current_cursor < PI_EMPTY_SLOT;


    if(is_pokemon) {
        pw_eeprom_addr_t addr;
        pokemon_summary_t pokemon;
        pokemon_index_t pokemon_index = pw_pokemon_id_to_pokemon_index(gdetailed.entries[s->inventory.current_cursor], &pokemon);
        pw_pokemon_index_to_small_sprite(pokemon_index, buf, (sf->frame&ANIM_FRAME_DOUBLE_TIME)>>ANIM_FRAME_DOUBLE_TIME_OFFSET, &addr);
        sprite.lookup_table.addr = addr;
        sprite.lookup_table.metadata.pokemon.species = pokemon.le_species;
        sprite.lookup_table.metadata.pokemon.pokemon_flags_1 = pokemon.pokemon_flags_1;
        sprite.lookup_table.metadata.pokemon.pokemon_flags_2 = pokemon.pokemon_flags_2;
    } else {
        sprite.lookup_table.addr = PW_EEPROM_ADDR_IMG_TREASURE_LARGE;
        pw_eeprom_read(
            PW_EEPROM_ADDR_IMG_TREASURE_LARGE,
            buf,
            PW_EEPROM_SIZE_IMG_TREASURE_LARGE
        );
    }

    pw_screen_draw_img(&sprite, PW_SCREEN_WIDTH-32-4, PW_SCREEN_HEIGHT-16-24);

}



static void draw_name(pw_state_t *s, const screen_flags_t *sf) {
    (void)sf;
    uint8_t *buf = eeprom_buf;
    pw_img_t sprite;

    if(s->inventory.current_cursor == PI_EMPTY_SLOT) return;

    bool is_pokemon = s->inventory.current_substate == SUBSCREEN_FOUND && s->inventory.current_cursor < PI_EMPTY_SLOT;

    if(is_pokemon) {
        // we're looking at a pokemon
        pokemon_summary_t pokemon;
        pokemon_index_t pokemon_index = pw_pokemon_id_to_pokemon_index(gdetailed.entries[s->inventory.current_cursor], &pokemon);
        pw_pokemon_index_to_name(pokemon_index, buf);
        sprite = (pw_img_t) {
            .width=80,
            .height=16,
            .data=buf,
            .size=PW_EEPROM_SIZE_TEXT_POKEMON_NAME,
            .is_flipped=false,
            .lookup_table = {
                .addr=-1,
                .use_alt=false
            }
        };
    } else {
        // we're looking at an item
        uint8_t item_idx = pw_item_id_to_item_index(gdetailed.entries[s->inventory.current_cursor]);
        pw_item_index_to_name(item_idx, buf);
        sprite = (pw_img_t) {
            .width=96,
            .height=16,
            .data=buf,
            .size=PW_EEPROM_SIZE_TEXT_ITEM_NAME_SINGLE,
            .is_flipped=false,
            .lookup_table = {
                .addr=-1,
                .use_alt=false
            }
        };
    }

    pw_screen_overlay_text_box(&sprite, PW_SCREEN_WIDTH, 16, PW_SCREEN_BLACK);
    pw_screen_draw_img(&sprite, 0, PW_SCREEN_HEIGHT-16);
    //pw_screen_draw_text_box(0, PW_SCREEN_HEIGHT-16, PW_SCREEN_WIDTH, 16, PW_SCREEN_BLACK);

}

static void pw_inventory_draw_screen1(pw_state_t *s, const screen_flags_t *sf) {

    pw_screen_draw_from_eeprom(
        0, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RETURN,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RETURN,
        true
    );

    pw_screen_draw_from_eeprom(
        PW_SCREEN_WIDTH-8, 0,
        8, 16,
        PW_EEPROM_ADDR_IMG_MENU_ARROW_RIGHT,
        PW_EEPROM_SIZE_IMG_MENU_ARROW_RIGHT,
        true
    );

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_IMG_MENU_TITLE_INVENTORY,
        PW_EEPROM_SIZE_IMG_MENU_TITLE_INVENTORY,
        false
    );

    // Draw icons
    uint8_t buf_pokeball[PW_EEPROM_SIZE_IMG_BALL];
    uint8_t buf_item[PW_EEPROM_SIZE_IMG_ITEM];

    pw_img_t pokeball = {
        .width=8,
        .height=8,
        .data=buf_pokeball,
        .size=PW_EEPROM_SIZE_IMG_BALL,
        .is_flipped=false,
        .lookup_table = {
            .addr=PW_EEPROM_ADDR_IMG_BALL,
            .use_alt=true
        }
    };
    pw_eeprom_read(PW_EEPROM_ADDR_IMG_BALL, buf_pokeball, PW_EEPROM_SIZE_IMG_BALL);

    pw_img_t item = {
        .width=8,
        .height=8,
        .data=buf_item,
        .size=PW_EEPROM_SIZE_IMG_ITEM,
        .is_flipped=false,
        .lookup_table = {
            .addr=PW_EEPROM_ADDR_IMG_ITEM,
            .use_alt=true
        }
    };
    pw_eeprom_read(PW_EEPROM_ADDR_IMG_ITEM, buf_item, PW_EEPROM_SIZE_IMG_ITEM);

    uint8_t xs[] = {8, 24, 32, 40, 48};
    const uint8_t yp = 24, yi = 40;

    // draw normal pokeballs (walking and caught)
    for(uint8_t i = 0; i < 4; i++) {
        if(gbrief.caught_pokemon & (1<<i)) {
            pw_screen_draw_img(&pokeball, xs[i], yp);
        }
    }

    // draw normal items
    for(uint8_t i = 0; i < 3; i++) {
        if(gbrief.dowsed_items & (1<<(i+1))) {
            pw_screen_draw_img(&item, xs[i+1], yi);
        }
    }


    // draw special pokeball
    if(gbrief.caught_pokemon & INV_EXTRA_POKEMON) {
        pw_screen_draw_from_eeprom(
            xs[4], yp,
            8, 8,
            PW_EEPROM_ADDR_IMG_BALL_LIGHT,
            PW_EEPROM_SIZE_IMG_BALL_LIGHT,
            true
        );
    }

    // draw special item
    if(gbrief.dowsed_items & INV_EXTRA_ITEM) {
        pw_screen_draw_from_eeprom(
            xs[4], yi,
            8, 8,
            PW_EEPROM_ADDR_IMG_ITEM_LIGHT,
            PW_EEPROM_SIZE_IMG_ITEM_LIGHT,
            true
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
        PW_EEPROM_SIZE_IMG_MENU_ARROW_LEFT,
        true
    );

    pw_screen_draw_from_eeprom(
        8, 0,
        80, 16,
        PW_EEPROM_ADDR_IMG_MENU_TITLE_INVENTORY,
        PW_EEPROM_SIZE_IMG_MENU_TITLE_INVENTORY,
        false
    );


    uint8_t buf_item[PW_EEPROM_SIZE_IMG_ITEM];
    pw_img_t item = {
        .width=8,
        .height=8,
        .data=buf_item,
        .size=PW_EEPROM_SIZE_IMG_ITEM,
        .is_flipped=false,
        .lookup_table = {
            .addr=PW_EEPROM_ADDR_IMG_ITEM,
            .use_alt=true
        }
    };
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
            PW_SCREEN_WIDTH-32-4, PW_SCREEN_HEIGHT-16-24,
            32, 24,
            PW_EEPROM_ADDR_IMG_PRESENT_LARGE,
            PW_EEPROM_SIZE_IMG_PRESENT_LARGE,
            true
        );

        draw_name(s, sf);
    }
}


static void pw_inventory_update_screen1(pw_state_t *s, const screen_flags_t *sf) {
    pw_screen_pos_t cx=0, cy=0;
    get_cursor_coords(s, &cx, &cy);

    if(cy == 16) {
        pw_screen_clear_area(0, 16, cx, 8);
        pw_screen_clear_area(cx+8, 16, 56-(cx+8), 8);
        pw_screen_clear_area(0, 32, 56, 8);
    } else if(cy == 32) {
        pw_screen_clear_area(0, 16, 56, 8);
        pw_screen_clear_area(0, 32, cx, 8);
        pw_screen_clear_area(cx+8, 32, 56-(cx+8), 8);
    }

    draw_cursor(s, sf);
    draw_name(s, sf);
    draw_animated_sprite(s, sf);

}


static void pw_inventory_update_screen2(pw_state_t *s, const screen_flags_t *sf) {

    pw_screen_pos_t cx=0, cy=0;
    get_cursor_coords(s, &cx, &cy);
    uint8_t x0 = 16, y0 = 24;

    if(cy == 16) {
        pw_screen_clear_area(x0, y0-8, cx, 8);
        pw_screen_clear_area(cx+8, y0-8, 40-(cx+8), 8);
        pw_screen_clear_area(x0, y0-8+16, 40, 8);
    } else if(cy == 32) {
        pw_screen_clear_area(x0, y0-8, 40, 8);
        pw_screen_clear_area(x0, y0-8+16, cx, 8);
        pw_screen_clear_area(cx+8, y0-8+16, 40-(cx+8), 8);
    }


    // don't draw this if we don't have presents
    if(gbrief.n_peer_play_items > 0) {
        draw_cursor(s, sf);
        draw_name(s, sf);
    }
}

void pw_inventory_event_loop(pw_state_t *s, pw_state_t *p, const screen_flags_t *sf) {
    (void)sf;
    switch(s->inventory.current_substate) {
    case SUBSTATE_GO_TO_SPLASH: {
        p->sid = STATE_SPLASH;
        pw_audio_play_sound(SOUND_NAVIGATE_MENU);
        break;
    }
    case SUBSTATE_GO_TO_MENU: {
        p->sid = STATE_MAIN_MENU;
        p->menu.cursor = 4;
        pw_audio_play_sound(SOUND_NAVIGATE_BACK);
        break;
    }
    default:
        break;
    }
}

