#ifndef PW_FLASH_H
#define PW_FLASH_H

#include <stdint.h>

#include "picowalker_structures.h"

/// @file flash.h

extern void pw_flash_read(pw_flash_img_t img_index, uint8_t *buf);

#endif /* PW_FLASH_H */
