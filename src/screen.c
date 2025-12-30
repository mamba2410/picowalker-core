#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <stdio.h>

#include "screen.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "globals.h"

/*
 *  Most of the heavy lifting is done by the driver code
 */

void pw_screen_draw_from_eeprom(pw_screen_pos_t x, pw_screen_pos_t y, pw_screen_pos_t w, pw_screen_pos_t h, pw_eeprom_addr_t addr, size_t len) {
    pw_img_t img = {.height=h, .width=w, .data=eeprom_buf, .size=len};
    pw_eeprom_read(addr, eeprom_buf, len);
    pw_screen_draw_img(&img, x, y);
}

void pw_screen_draw_from_eeprom_with_text_box(pw_screen_pos_t x, pw_screen_pos_t y, pw_screen_pos_t w, pw_screen_pos_t h, pw_eeprom_addr_t addr, size_t len, pw_screen_color_t c) {
    pw_img_t img = {.height=h, .width=w, .data=eeprom_buf, .size=len};
    pw_eeprom_read(addr, eeprom_buf, len);
    pw_screen_overlay_text_box(&img, w, h, c);
    pw_screen_draw_img(&img, x, y);
}


pw_screen_pos_t pw_screen_draw_integer(uint32_t n, pw_screen_pos_t right_x, pw_screen_pos_t y) {

    pw_screen_pos_t x = right_x;
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


pw_screen_pos_t pw_screen_draw_integer_with_overline(uint32_t n, pw_screen_pos_t right_x, pw_screen_pos_t y, pw_screen_color_t c) {

    pw_screen_pos_t x = right_x;
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

void pw_screen_draw_time(uint8_t hour, uint8_t minute, uint8_t second, pw_screen_pos_t x, pw_screen_pos_t y) {
    pw_screen_draw_subtime(hour, x, y, true);
    x += 24;
    pw_screen_draw_subtime(minute, x, y, true);
    x += 24;
    pw_screen_draw_subtime(second, x, y, false);
}

void pw_screen_draw_subtime(uint8_t n, pw_screen_pos_t x, pw_screen_pos_t y, bool draw_colon) {
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
void pw_screen_draw_message(pw_screen_pos_t y, uint8_t message_index, pw_screen_pos_t h) {
    if(h != 16 && h != 32) {
        return;    // can only do 16 or 32 height messages
    }

    pw_eeprom_addr_t addr = PW_EEPROM_ADDR_TEXT_CONNECTING + message_index * PW_EEPROM_SIZE_TEXT_CONNECTING;
    size_t sz = PW_EEPROM_SIZE_TEXT_CONNECTING*h/16;

    pw_eeprom_read(addr, eeprom_buf, sz);

    pw_img_t img = {
        .width=PW_SCREEN_WIDTH, .height=h,
        .data=eeprom_buf,
        .size=sz
    };

    pw_screen_draw_img(&img, 0, y);
}

// always draws at x=0
void pw_screen_draw_message_with_text_box(pw_screen_pos_t y, uint8_t message_index, pw_screen_pos_t h, pw_screen_color_t c) {
    if(h != 16 && h != 32) {
        return;    // can only do 16 or 32 height messages
    }

    pw_eeprom_addr_t addr = PW_EEPROM_ADDR_TEXT_CONNECTING + message_index * PW_EEPROM_SIZE_TEXT_CONNECTING;
    size_t sz = PW_EEPROM_SIZE_TEXT_CONNECTING*h/16;

    pw_eeprom_read(addr, eeprom_buf, sz);

    pw_img_t img = {
        .width=PW_SCREEN_WIDTH, .height=h,
        .data=eeprom_buf,
        .size=sz
    };
    pw_screen_overlay_text_box(&img, PW_SCREEN_WIDTH, h, c);

    pw_screen_draw_img(&img, 0, y);
}

void pw_screen_draw_pokemon_name_and_message(pw_eeprom_addr_t poke_addr, pw_eeprom_addr_t message_addr, pw_screen_color_t c) {
    pw_img_t img = {.width=PW_SCREEN_WIDTH, .height=32, .data=eeprom_buf, .size=2*PW_EEPROM_SIZE_TEXT_APPEARED};
    pw_eeprom_read(
        poke_addr,
        img.data,
        PW_EEPROM_SIZE_TEXT_POKEMON_NAME
    );
    pw_eeprom_read(
        message_addr,
        img.data + PW_EEPROM_SIZE_TEXT_APPEARED,
        PW_EEPROM_SIZE_TEXT_APPEARED
    );
    for(int i = 1; i >= 0; i--) {
        for(int j = 2*80 - 1; j >= 0; j--) {
            img.data[i*2*PW_SCREEN_WIDTH+j] = img.data[i*2*80 + j];
        }
        if(i > 0) {
            memset(&img.data[i*2*80], 0, 2*(16));
        }
    }
    memset(img.data+PW_EEPROM_SIZE_TEXT_POKEMON_NAME, 0, PW_EEPROM_SIZE_TEXT_ATTACKED - PW_EEPROM_SIZE_TEXT_POKEMON_NAME);
    pw_screen_overlay_text_box(&img, PW_SCREEN_WIDTH, 32, PW_SCREEN_BLACK);
    pw_screen_draw_img(&img, 0, PW_SCREEN_HEIGHT-32);
}

void pw_screen_overlay_text_box(pw_img_t *img, pw_screen_pos_t w, pw_screen_pos_t h, pw_screen_color_t c) {
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

    uint8_t bottom_mask = (uint8_t)(~(1<<7));
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

void pw_screen_overlay_overline(pw_img_t *img, pw_screen_pos_t w, pw_screen_color_t c) {
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


void pw_screen_get_blank_image(pw_img_t *img, pw_screen_pos_t w, pw_screen_pos_t h) {
    bool invalid = (w > PW_SCREEN_WIDTH) || (h > PW_SCREEN_HEIGHT) || (img == NULL);
    if(invalid) {
        img->data = NULL;
        img->width = 0;
        img->height = 0;
        img->size = 0;
    } else {
        img->data = screen_buf;
        img->width = w;
        img->height = h;
        img->size = w*h*2/8;
        memset(img->data, 0, img->size);
    }
}


/*
 * Gives the visible dimension (width/height) of a smaller image when overlapping with a larger one.
 * Effectively the convolution of two unit top-hat functions of widths img and base.
 */
static pw_screen_pos_t get_overlapping_dimension(pw_screen_pos_t img, pw_screen_pos_t base, pw_screen_pos_t pos) {
    if(img > base) return 0;
    if(pos < (int8_t)(-img)) return 0;
    if(pos < 0) return img + pos;
    if(pos <= base - img) return img;
    if(pos < base) return base - pos;
    return 0;
}


/**
 * Specifically for images with y offset aligned to 8 bytes
 */
static void overlay_img_aligned(pw_img_t *base, pw_img_t *img, pw_screen_pos_t x, pw_screen_pos_t y, pw_screen_pos_t visible_width, pw_screen_pos_t visible_height) {

    // Split y
    uint8_t chunk_spans = visible_height / 8;
    uint8_t x_read_offset = (x < 0) ? -2*x : 0; // Ignore first -2x bytes if its off the screen in the left x direction
    size_t base_write_offset = (base->width*y/8 + (x<0?0:x)) * 2;

    // Things this does not do:
    // - Images overlapping the edge of the screen in the y direction

    // 8-aligned is pretty simple as all chunks are treated equally
    for(size_t chunk = 0; chunk < chunk_spans; chunk++) {
        for(size_t b = 0; b < 2*visible_width; b++) {
            uint8_t adjusted_byte = img->data[2*img->width*chunk + x_read_offset + b];
            base->data[2*base->width*chunk + base_write_offset + b] |= adjusted_byte;
        }
    }
}


/**
 * Specifically for images with y offset aligned to 8 bytes
 */
static void overlay_img_unaligned(pw_img_t *base, pw_img_t *img, pw_screen_pos_t x, pw_screen_pos_t y, pw_screen_pos_t visible_width, pw_screen_pos_t visible_height) {

    // Split y
    uint8_t chunk_spans = (visible_height / 8) + ((y + base->height)%8 + (8-1))/8;
    uint8_t y_shift = (y + base->height)%8;
    uint8_t top_shift = (y_shift > 0) ? 8-y_shift : 0;
    int8_t x_read_offset = (x < 0) ? -2*x : 0; // Ignore first -2x bytes if its off the screen in the left x direction
    size_t base_write_offset = (base->width*(y/8) + (x<0?0:x)) * 2;

    // Things this does not do:
    // - Images overlapping the edge of the screen in the y direction


    // Chunks need to be treated differently since one input chunk spans multiple output chunks
    size_t chunk = 0;

    // First chunk, top is zeros, bottom is top of current chunk
    for(size_t b = 0; b < 2*visible_width; b++) {
        uint8_t top_byte = 0;
        uint8_t bot_byte = img->data[2*img->width*(chunk+0) + x_read_offset + b];

        // LSB is top
        uint8_t adjusted_byte = (top_byte >> top_shift) | (bot_byte << y_shift);
        base->data[2*base->width*chunk + base_write_offset + b] |= adjusted_byte;
    }
    chunk++;

    // Middle chunks, top is bottom of previous chunk, bottom is top of current chunk
    for(; chunk < chunk_spans-1; chunk++) {
        for(size_t b = 0; b < 2*visible_width; b++) {
            uint8_t top_byte = img->data[2*img->width*(chunk-1) + x_read_offset + b];
            uint8_t bot_byte = img->data[2*img->width*(chunk+0) + x_read_offset + b];

            // LSB is top
            uint8_t adjusted_byte = (top_byte >> top_shift) | (bot_byte << y_shift);
            base->data[2*base->width*chunk + base_write_offset + b] |= adjusted_byte;
        }
    }

    // Final chunk, top is bottom of last chunk, bottom is zeros
    for(size_t b = 0; b < 2*visible_width; b++) {
        uint8_t top_byte = img->data[2*img->width*(chunk-1) + x_read_offset + b];
        uint8_t bot_byte = 0;

        // LSB is top
        uint8_t adjusted_byte = (top_byte >> top_shift) | (bot_byte << y_shift);
        base->data[2*base->width*chunk +  base_write_offset + b] |= adjusted_byte;
    }
    chunk++;
}


/**
 * Overlay image `img` on top of the image `base`.
 *
 */
void pw_screen_overlay_img(pw_img_t *base, pw_img_t *img, pw_screen_pos_t x, pw_screen_pos_t y) {
    // Dimension checks
    pw_screen_pos_t visible_width = get_overlapping_dimension(img->width, base->width, x);
    pw_screen_pos_t visible_height = get_overlapping_dimension(img->height, base->height, y);
    if(visible_width == 0 || visible_height == 0) {
        printf("[Error] Visible width/height of overlapping image is zero.");
        return;
    }

    if(y < 0 || y > base->height - img->height) {
        printf("[Error] Overlays which puts an image off the top or bottom edge of a screen are not supported\n");
        return;
    }
    // Now we know that i + visible_width <= base->height for all 0 <= i <= x. Same for y.
    // This means we can stop worrying about OOB reads/writes.

    if( (y%8) != 0) {
        overlay_img_unaligned(base, img, x, y, visible_width, visible_height);
    } else {
        overlay_img_aligned(base, img, x, y, visible_width, visible_height);
    }

}

