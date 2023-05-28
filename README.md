# picowalker-core

## About

**Core files for the picowalker**

Sister project: [picowalker](https://github.com/mamba2410/picowalker).

This project aims to recreate a Pokewalker from Pokemon HeartGold/SoulSilver using custom hardware based around the Raspberry Pi Pico.
People should be able to build their own fully functioning device which can interact with the original HG/SS games as the Pokewalker did.
We will try to stay faithful to the original use and intent of the Pokewalker, but on a new, relatively easily buildable device, since working, original Pokewalkers are becoming more and more rare.

There are other projects based around emulating the code that is on the Pokewalker, however this project aims to create a new drop-in replacement which is capable of emulating all of the features of the original Pokewalker, with room for improvement.

The project is written in C and is meant to be platform agnostic and will try to remain faithful to the original pokewalker code, with some more modern and high level approaches.

Most of the functionality is tested with a raspberry pi pico (arm cortex-m0+).

## Project state

What's working (tested with rpi pico):

- Screen functions
- Most of the common IR functionality
- Button functions (interrupts)
- Splash screen
- Apps: battle (including catching), dowsing, IR (including pairing/erasing, walk start/end, peer play), trainer card and inventory.
- EEPROM functions
- Accelerometer

Still to do:

- RTC
- Battery
- Sound
- Pokewalker event logging
- Random events (eg smiley faces, random watts, pokemon joined etc)
- Add the animations for send/receive etc.

## Help Wanted

This is a very large project and I can't do it alone, so extra hands would be extremely welcome and appreciated.

Help is needed to:

- Translate and modernise the code on the original Pokewalker to the Pico.
- Find hardware that can be used as the peripherals which are able to be controlled by the Pico.
- Write drivers/interface code for the hardware chosen.
- Design the physical layout and connections of the hardware.
- Design shells/casing for the end product.
- Find/create a good license that won't get us in trouble. (see License section)

If you would like to try out the current implementation or contribute to the project, please read
the [design doc](./docs/DESIGN.md).

For things that need doing, see the [todo doc](./docs/TODO.md).

## Resources

### Pokewalker

- [Original pokewalker hack by Dmitry.GR](http://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker)
- [H8/300h Series software manual (for looking at the original disassembly)](https://www.renesas.com/us/en/document/mah/h8300h-series-software-manual)

## Building

### Linux for ARM cortex-m0+

It should be as easy as

```sh
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="../toolchain-pico.cmake" -DCMAKE_BUILD_TYPE=Debug ..
cd ..
make
```

### Mac

Should be the same as Linux?

### Windows

As long as you have CMake and the right toolchain installed, it should be the same?

## License

As this is technically not an original project, I am unsure about the license.
I would like as much of this project to be as free and open source as possible, with the exception of being able to sell this as a product, since that will probably get everyone in trouble with Nintendo licensing and nobody wants that.

Licensing suggestions would be welcome. In the meantime, I guess this is fully copyrighted to the contributors.
