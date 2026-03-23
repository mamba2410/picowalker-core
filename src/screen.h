#ifndef PW_SCREEN_H
#define PW_SCREEN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "picowalker_structures.h"

/// @file screen.h

#define SCREEN_REDRAW_DELAY_US  250000  // 250ms

/**
 * An image queue data structure for pokewalker images 
 * post processing
 */
typedef struct pw_img_queue_s {
    pw_img_t *img;
    pw_screen_pos_t x;
    pw_screen_pos_t y;
} pw_img_queue_t;

/*
 *  Derived functions
 */
void pw_screen_draw_from_eeprom(
    pw_screen_pos_t x, pw_screen_pos_t y,
    pw_screen_dim_t w, pw_screen_dim_t h,
    pw_eeprom_addr_t addr,
    size_t len,
    bool use_alt
);
pw_screen_pos_t pw_screen_draw_integer(uint32_t n, pw_screen_pos_t right_x, pw_screen_pos_t y);
void pw_screen_draw_time(uint8_t hour, uint8_t minute, uint8_t second, pw_screen_pos_t x, pw_screen_pos_t y);
void pw_screen_draw_subtime(uint8_t n, pw_screen_pos_t x, pw_screen_pos_t y, bool draw_colon);
void pw_screen_draw_message(pw_screen_pos_t y, uint8_t message_index, pw_screen_dim_t h);
void pw_screen_overlay_overline(pw_img_t *img, pw_screen_dim_t w, pw_screen_color_t c);
void pw_screen_overlay_text_box(pw_img_t *img, pw_screen_dim_t w, pw_screen_dim_t h, pw_screen_color_t c);
pw_screen_pos_t pw_screen_draw_integer_with_overline(uint32_t n, pw_screen_pos_t right_x, pw_screen_pos_t y, pw_screen_color_t c);
void pw_screen_draw_from_eeprom_with_text_box(pw_screen_pos_t x, pw_screen_pos_t y, pw_screen_dim_t w, pw_screen_dim_t h, pw_eeprom_addr_t addr, size_t len, pw_screen_color_t c);
void pw_screen_draw_message_with_text_box(pw_screen_pos_t y, uint8_t message_index, pw_screen_dim_t h, pw_screen_color_t c);
void pw_screen_draw_pokemon_name_and_message(pw_eeprom_addr_t poke_addr, pw_eeprom_addr_t message_addr, pw_screen_color_t c);
void pw_screen_overlay_img(pw_img_t *base, pw_img_t *img, pw_screen_pos_t x, pw_screen_pos_t y);
void pw_screen_get_blank_image(pw_img_t *img, pw_screen_dim_t w, pw_screen_dim_t h);
void pw_screen_draw_queue(pw_img_queue_t *queue, uint8_t count);

/*
 * Functions defined by the driver
 */
extern void pw_screen_init();
extern void pw_screen_draw_img(
    pw_img_t *img,
    pw_screen_pos_t x, pw_screen_pos_t y
);
extern void pw_screen_clear_area(
    pw_screen_pos_t x, pw_screen_pos_t y,
    pw_screen_dim_t width, pw_screen_dim_t height
);
extern void pw_screen_draw_horiz_line(
    pw_screen_pos_t x, pw_screen_pos_t y,
    pw_screen_dim_t len,
    pw_screen_color_t color
);
extern void pw_screen_draw_text_box(
    pw_screen_pos_t x1, pw_screen_pos_t y1,
    pw_screen_pos_t x2, pw_screen_pos_t y2,
    pw_screen_color_t color
);
extern void pw_screen_clear();
extern void pw_screen_fill_area(
    pw_screen_pos_t x, pw_screen_pos_t y,
    pw_screen_dim_t w, pw_screen_dim_t h,
    pw_screen_color_t color
);
extern void pw_screen_sleep();
extern void pw_screen_wake();
extern void pw_screen_set_brightness(uint8_t brightness);


#endif /* PW_SCREEN_H */
