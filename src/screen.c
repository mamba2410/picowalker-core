#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "screen.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "globals.h"

/*
 *  Most of the heavy lifting is done by the driver code
 */

// TODO: move to global buffers.h
//static uint8_t *eeprom_buf = 0;

void pw_screen_draw_from_eeprom(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t addr, size_t len) {
    pw_img_t img = {.height=h, .width=w, .data=eeprom_buf, .size=len};
    pw_eeprom_read(addr, eeprom_buf, len);
    pw_screen_draw_img(&img, x, y);
}

size_t pw_screen_draw_integer(uint32_t n, size_t right_x, size_t y) {

    size_t x = right_x;
    uint32_t m = n;
    do {
        size_t idx = m%10;
        m = m/10;
        x -= 8;
        pw_screen_draw_from_eeprom(
            x, y,
            8, 16,
            PW_EEPROM_ADDR_IMG_DIGITS+PW_EEPROM_SIZE_IMG_CHAR*idx,
            PW_EEPROM_SIZE_IMG_CHAR
        );
    } while(m>0);

    return x;
}


size_t pw_screen_draw_integer_with_overline(uint32_t n, size_t right_x, size_t y, screen_colour_t c) {

    size_t x = right_x;
    uint32_t m = n;
    do {
        size_t idx = m%10;
        m = m/10;
        x -= 8;

        pw_img_t img = {.width=8, .height=16, .data=eeprom_buf, .size=PW_EEPROM_SIZE_IMG_CHAR};
        pw_eeprom_read(PW_EEPROM_ADDR_IMG_DIGITS+PW_EEPROM_SIZE_IMG_CHAR*idx, eeprom_buf, PW_EEPROM_SIZE_IMG_CHAR);
        pw_screen_overlay_overline(&img, 8, c);
        pw_screen_draw_img(&img, x, y);
    } while(m>0);

    return x;
}

void pw_screen_draw_time(uint8_t hour, uint8_t minute, uint8_t second, size_t x, size_t y) {
    pw_screen_draw_subtime(hour, x, y, true);
    x += 24;
    pw_screen_draw_subtime(minute, x, y, true);
    x += 24;
    pw_screen_draw_subtime(second, x, y, false);
}

void pw_screen_draw_subtime(uint8_t n, size_t x, size_t y, bool draw_colon) {
    uint8_t idx;

    idx = n/10;
    pw_screen_draw_from_eeprom(
        x, y,
        8, 16,
        PW_EEPROM_ADDR_IMG_DIGITS+PW_EEPROM_SIZE_IMG_CHAR*idx,
        PW_EEPROM_SIZE_IMG_CHAR
    );

    x += 8;
    idx = n%10;
    pw_screen_draw_from_eeprom(
        x, y,
        8, 16,
        PW_EEPROM_ADDR_IMG_DIGITS+PW_EEPROM_SIZE_IMG_CHAR*idx,
        PW_EEPROM_SIZE_IMG_CHAR
    );
    if(draw_colon) {
        x += 8;
        pw_screen_draw_from_eeprom(
            x, y,
            8, 16,
            PW_EEPROM_ADDR_IMG_CHAR_COLON,
            PW_EEPROM_SIZE_IMG_CHAR
        );
    }
}

// always draws at x=0
void pw_screen_draw_message(screen_pos_t y, uint8_t message_index, screen_pos_t h) {
    if(h != 16 && h != 32) {
        return;    // can only do 16 or 32 height messages
    }

    eeprom_addr_t addr = PW_EEPROM_ADDR_TEXT_CONNECTING + message_index * PW_EEPROM_SIZE_TEXT_CONNECTING;
    size_t sz = PW_EEPROM_SIZE_TEXT_CONNECTING*h/16;

    pw_eeprom_read(addr, eeprom_buf, sz);

    pw_img_t img = {
        .width=SCREEN_WIDTH, .height=h,
        .data=eeprom_buf,
        .size=sz
    };

    pw_screen_draw_img(&img, 0, y);
}

void pw_screen_overlay_text_box(pw_img_t *img, screen_pos_t w, screen_pos_t h, screen_colour_t c) {
    // If dimensions are too small, extend source image.
    // TODO: this assumes there's enough space in the buffer. this is bad.
    if(w > img->width) {
        //printf("[Debug] Increased text box size from %dx%d to %dx%d\n", img->width, img->height, w, img->height);
        // Need to copy from the back to not override data
        for(int i = img->height/8-1; i >= 0; i--) {
            //memcpy(&img->data[i*2*w], &img->data[i*2*img->width], 2*img->width);
            // Still need to memcpy backwards
            for(int j = 2*img->width - 1; j >= 0; j--) {
                img->data[i*2*w+j] = img->data[i*2*img->width + j];
            }
            if(i > 0) {
                memset(&img->data[i*2*img->width], 0, 2*(w-img->width));
            }
        }
        img->width = w;
        img->size = img->height * img->width;
    }

    if(h > img->height) {
        //printf("[Debug] Increased text box size from %dx%d to %dx%d\n", img->width, img->height, img->width, h);
        memset(&img->data[(img->height/8)*2*img->width], 0, 2*img->width*(h-img->height));
        img->height = h;
        img->size = img->height * img->width;
    }

    uint8_t top_mask = ~(1);
    uint8_t c_upper_top = (c&0x02)>>1;
    uint8_t c_lower_top = (c&0x01)>>0;

    uint8_t bottom_mask = ~(1<<7);
    uint8_t c_upper_bottom = (c&0x02)<<6;
    uint8_t c_lower_bottom = (c&0x01)<<7;

    // Fill in top line
    for(size_t i = 0; i < w; i++) {
        img->data[2*i+0] = (img->data[2*i+0] & top_mask) | c_upper_top;
        img->data[2*i+1] = (img->data[2*i+1] & top_mask) | c_lower_top;
    }

    // Fill in bottom line
    size_t offs = 2*(img->height/8-1);
    for(size_t i = 0; i < w; i++) {
        img->data[offs*img->width+2*i+0] = (img->data[offs*img->width+2*i+0] & bottom_mask) | c_upper_bottom;
        img->data[offs*img->width+2*i+1] = (img->data[offs*img->width+2*i+1] & bottom_mask) | c_lower_bottom;
    }

    uint8_t upper_splat = (c&0x02)>>1;
    upper_splat |= upper_splat << 1;
    upper_splat |= upper_splat << 2;
    upper_splat |= upper_splat << 4;

    uint8_t lower_splat = c&0x01;
    lower_splat |= lower_splat << 1;
    lower_splat |= lower_splat << 2;
    lower_splat |= lower_splat << 4;

    // Fill in sides
    for(size_t i = 0; i < img->height/8; i++) {
        img->data[i*2*img->width+0] = upper_splat;
        img->data[i*2*img->width+1] = lower_splat;

        img->data[(i+1)*2*img->width-2] = upper_splat;
        img->data[(i+1)*2*img->width-1] = lower_splat;
    }

}

void pw_screen_overlay_overline(pw_img_t *img, screen_pos_t w, screen_colour_t c) {
    if(w > img->width) {
        printf("[Error] Trying to draw a %d pixel line over a %dx%d image\n", w, img->width, img->height);
        return;
    }

    uint8_t top_mask = ~(1);
    uint8_t c_upper_top = (c&0x02)>>1;
    uint8_t c_lower_top = (c&0x01)>>0;

    for(size_t i = 0; i < w; i++) {
        img->data[2*i+0] = (img->data[2*i+0] & top_mask) | c_upper_top;
        img->data[2*i+1] = (img->data[2*i+1] & top_mask) | c_lower_top;
    }
    
}

