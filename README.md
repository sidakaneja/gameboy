# Gameboy Emulator (In progress)

A multiplatform Gameboy emulator written in C; currently available for: Windows, OS X, Linux based OSes that support the SDL2 library.

Note:
This emulation is meant for educational purposes only.

## Installation
### Mac
```bash
brew install sdl2
make
./bin/main
```

## Dependency 
SDL2 library.


## Blargg Test Rom Status
### Blargg CPU instructions test
- Passed: 01, 02, 03, 04, 05, 06, 07, 08, 09, 10, 11

## Internals
- Will be periodically updated. 
- May be turned into a more in-depth guide.
#### CPU
- 4.194304MHz clock speed.
- 8 Bit CPU with 16 bit address bus.
- Instructions between 1 and 4 bytes.
- 8 8-bit general purpose registers A,B,C,D,E,F,H,L that can be used as 16 bit registers AF, BC, DE, HL.
- 16-bit PC and Stack register.