# TODO

Most of this stuff has been moved to focalboard, but here will be more detailed looks at ideas/suggestions and bugs

## Features

- Graphics
    - Walking animation when steps are being taken on spash screen
    - Walk start animation
        - Comms app probably needs refactoring or at least a better look at
    - walk end animation


## Bugs

- General
  - Change some calls of `pw_now_us()` to `pw_now_ms()` for things that need >65ms
    causes infinite loops since some hardware timers are only 16 bit, so have a max us
    difference of 65535us.
  - Standardise timer calls `pw_timer_x()` where x = `now_us`, `now_ms`, `delay_us`, `delay_ms`.
- Dowsing
    - Switch screen doesn't cancel on left button
- Battle
    - Can battle with no pokemon present
    - Stop screen flickers by drawing less overlaps - mini frame buffer?
- Inventory
  - Do not open if no inventory
  - Cursor bugs on going backwards
  - do not open page 2 if no gifted items
  - going backwards from page 2 sets weird cursor value?
  - blinking images, don't redraw blank right before overriding text
  - walk end doesn't remove special items?

## Apps

- Settings
  - Sound
  - Shade
  - Secret pico settings?
- Sleep

