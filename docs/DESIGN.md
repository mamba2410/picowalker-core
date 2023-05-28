# picowalker Design Document

## Goals

Faithful recreation of the original Pokewalker with some minor quality of life improvements.
Intended to be a full drop-in replacement for the Pokewalker.

The core files are intended to be platform-agnostic and nostd so they can run on anything, so long as
there are drivers available.

## Software

Pico C code to recreate all of the features of the original Pokewalker.
The code itself does not need to be as similar to the original as possible, however the output of said code should be.

In the end, it would be nice if multiple peripherals or even boards could be supported so try to keep all specific device/driver code separate, but this isn't a must right now.

## What does the code need to emulate?

- Mainly the IR comms. Want a seamless communication with the games and the walker.
- Pokemon/items/routes consistency with the games.
- Minigames and micro-apps.
- Uh some other stuff that I can't think of right now.

