# On9 NMEA

Dumb & tiny state machine based NMEA parser

"[On 9](https://en.wiktionary.org/wiki/on9)" means "戆鸠" in Cantonese, means "dumb"

This library does NOT have any C standard library used. Only `size_t` and `uintXX_t` types used.

Currently, this library only supports RMC and GGA sentences.

## Usage

0. Copy `on9nmea.c` and `on9nmea.h` to your project, or clone this repo as a submodule
1. Initialise the context by `on9_nmea_init()`
2. Call `on9_nmea_feed_char()` till it returns `ON9_NMEA_STATE_DONE` or `ON9_NMEA_STATE_ERROR_*`

## Size

As of commit [99b8ee66eabf6](https://github.com/huming2207/on9nmea/tree/99b8ee66eabf6381f98a952cdfdd2ef94f613773):

- `.text`
  - 1119 bytes
- `.rodata`
  - 44 bytes

