# TODO

Most of this stuff has been moved to focalboard, but here will be more detailed looks at ideas/suggestions and bugs

## Ideas

- redo state machine code to use tagged unions

## Bugs

- General
  - Change some calls of `pw_now_us()` to `pw_now_ms()` for things that need >65ms
    causes infinite loops since some hardware timers are only 16 bit, so have a max us
    difference of 65535us.
  - Standardise timer calls `pw_timer_x()` where x = `now_us`, `now_ms`, `delay_us`, `delay_ms`.
- Inventory
  - Do not open if no inventory
  - Cursor bugs on going backwards
  - do not open page 2 if no gifted items
  - going backwards from page 2 sets weird cursor value?

## Apps

- Settings
  - Sound
  - Shade
  - Secret pico settings?
- Sleep

