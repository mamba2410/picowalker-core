# TODO

## Features

- Trainer card
  - Display time properly
- RTC
  - New timer driver function to get current time in pokewalker timestamp
  - Interrupts every so long (hour, maybe minute, maybe day)
- Logs
  - Add logging driver (printf wrapper)

## Bugs

- General
  - Remove reliance on `pw_screen_clear()` on most `xx_init_display()` functions
    since sleep doesn't clear when it wakes up.
    Also allows for smoother state transitions eg radar/battle
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
- Comms
  - Peer play doesn't work (`slave_perform_action()` commands)
- Power
  - Periodically (as long as possible?) check battery voltage and test it.
    - May require RTC interrupts every minute/hour

## Apps

- Settings
  - Sound
  - Shade
  - Secret pico settings?
- Sleep

