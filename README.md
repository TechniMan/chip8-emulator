# CHIP-8 Emulator

Writing an emulator for the [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) platform, in C using [SDL2](https://libsdl.org/).

Following the [Emulator 101](http://www.emulator101.com/welcome.html) tutorial as a guide, with some help from Cowgod's [CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

I used [corax89's test rom](https://github.com/corax89/chip8-test-rom) to test the emulator ran correctly.

Built on Windows 11 using Visual Studio to build.

## To-Dos

* Change SDL rendering to use a renderer and scale up an image, rather than draw each pixel as a 16x16 super-pixel
* Implement reset button
* Implement save state(s) (and load of said state)
* Allow customisation of on/off colours
* Allow customisation of ops per second
* Re-implement in TypeScript to run on a webpage
