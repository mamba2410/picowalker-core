# picowalker-core

## About

**Core files for the picowalker**

Drivers for the picowalker custom hardware that use this code: [picowalker](https://github.com/mamba2410/picowalker).

Windows/linux port (with limited connectivity): [picowalker-sdl](https://github.com/mamba2410/picowalker-sdl).

The overall Picowalker project aims to recreate a Pokewalker from Pokemon HeartGold/SoulSilver using custom hardware based around the Raspberry Pi Pico/rp2040 series of chips.
This includes custom hardware, software and drivers, with some modern convenience features added in as well.
Users should also be able to build their own based on a Raspberry Pi Pico, given that they write the drivers for it.

This repo contains the "core" code, a (hopefully) platform-agnostic set of code which needs to be compiled and linked with [driver code](https://github.com/mamba2410/picowalker) for it to work.

This code has been tested on a Pico, Pico 2, custom hardware based on the RP2350, Windows and Linux.

All issues related to the picowalker project software will be in this repo, unless its really specific.

## Project state

What's working (tested with rpi pico):

- Screen
- Most of the IR functionality
- All Apps: battle (including catching), dowsing, IR (including pairing/erasing, walk start/end, peer play), trainer card, inventory and settings (doesn't do anything yet).
- EEPROM save data
- Accelerometer for step counting
- RTC for resetting and logging steps daily

Still to do:

- Battery monitoring for reporting the level and safely shutting down.
- Sound.
- Pokewalker event logging (for walk summary when returned from a walk).
- Random events (eg smiley faces, random watts, pokemon joined etc).
- More obscure IR functions like adding stamps.
- Support for colour images on a colour screen.

## Contributing

If you'd like to contribute, any form is welcome!
Issues, suggestions, code contributions etc.

The one large outstanding thing to do is to license this project.

If you would like to try out the current implementation or contribute to the project, please read
the [design doc](./docs/DESIGN.md).

For things that need doing, see the [todo doc](./docs/TODO.md).

## Resources

### Pokewalker

- [Original pokewalker hack by Dmitry.GR](http://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker)

## Building

### Linux for ARM cortex-m0+

It should be as easy as

```sh
cmake -B build/arm-cortexm0plus -DCMAKE_TOOLCHAIN_FILE="../toolchain-pico.cmake" .
cmake --build build/arm-cortexm0plus
```

### Mac

Should be the same as Linux?

### Windows natively

As long as you have CMake and the right toolchain installed, it should be the same.

```sh
cmake -B build/x86-windows .
cmake --build build/x86-windows
```

## License

As this is heavily inspired by an existing product, I am unsure about the license.
I would like as much of this project to be as free and open source as possible.

Licensing suggestions would be welcome. In the meantime, I guess this is fully copyrighted to the contributors.

