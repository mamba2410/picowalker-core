#include "buttons.h"

#include <stdint.h>

#include "picowalker_structures.h"
#include "states.h"

void pw_button_callback(pw_buttons_t b) {
    pw_state_handle_input(b);
}
