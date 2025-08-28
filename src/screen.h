#ifndef PW_SCREEN_H
#define PW_SCREEN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "picowalker-defs.h"
#include "eeprom.h"

/// @file screen.h

#define SCREEN_REDRAW_DELAY_US  250000  // 250ms


/*
 *  Derived functions
 */
void pw_screen_draw_from_eeprom(
    screen_pos_t x, screen_pos_t y,
    screen_pos_t w, screen_pos_t h,
    eeprom_addr_t addr,
    size_t len
);
size_t pw_screen_draw_integer(uint32_t n, size_t right_x, size_t y);
void pw_screen_draw_time(uint8_t hour, uint8_t minute, uint8_t second, size_t x, size_t y);
void pw_screen_draw_subtime(uint8_t n, size_t x, size_t y, bool draw_colon);
void pw_screen_draw_message(screen_pos_t y, uint8_t message_index, screen_pos_t h);
void pw_screen_overlay_overline(pw_img_t *img, screen_pos_t w, screen_colour_t c);
void pw_screen_overlay_text_box(pw_img_t *img, screen_pos_t w, screen_pos_t h, screen_colour_t c);
size_t pw_screen_draw_integer_with_overline(uint32_t n, size_t right_x, size_t y, screen_colour_t c);
void pw_screen_draw_from_eeprom_with_text_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t addr, size_t len, screen_colour_t c);
void pw_screen_draw_message_with_text_box(screen_pos_t y, uint8_t message_index, screen_pos_t h, screen_colour_t c);
void pw_screen_draw_pokemon_name_and_message(uint16_t poke_addr, uint16_t message_addr, screen_colour_t c);
void pw_screen_overlay_img(pw_img_t *base, pw_img_t *img, screen_pos_t x, screen_pos_t y);
void pw_screen_get_blank_image(pw_img_t *img, screen_pos_t w, screen_pos_t h);


#endif /* PW_SCREEN_H */
