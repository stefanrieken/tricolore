# Tricolore

### Goal:
A virtual machine in the spirit of CHIP-8
optimized to show a game of go

Simple to implement e.g. on:
- M5Stack
- LilyGo SmartWatch

### So, features:
- 240x240 pixel screen
- Addressable as 30x30 8x8 sprite blocks
- 64 monochrome sprites, default to alphanumerical characters
- 64 2-bit indexed sprites

### Display memory:
- 1kb (1024 bytes) of 6-bit sprite references with 2-bit palette selectors
  - Memory is arranged as if screen were 32x32, but viewport is 30x30
  - Bytes that fall outside of viewport contain registers
  - Smooth scrolling is theoretically possible by further limiting viewport
- palette 00 selects monochrome sprite bank(s)
- Scroll / viewport offset registers
- N independently movable character sprite registers
  - sprite reference (incl. palette) + size (2 bytes)
  - x / y coords (2 bytes)

### Palette / sprite memory:
- In each screen memory byte the lower 6 bits refer a sprite, but the upper
- 2 bits index into the 2 bytes at the end of the screen line, where
- 4 entries of 4 bits refer to any of the 16 possible palettes, where
- 4 entries of 4 bits refer to any of the 16 built-in (or once-definable) colors.

Without this double indirection there would be only 4 colours per screen line;
but with this indirection, there are 4 palettes of 4 colours per screen line,
so in theory all 16 colours may be used. (In practice however, index pattern
00 also selects for monochrome sprites, which use only the first two colours
of the referenced palette.)


### (Display) memory layout:

    0000  -----
    ...   1kb display mem (32 * 32 blocks)
          0000 ...............X
          0040 ...............X  (scaled 1/2 x 1/2 !!)
          0080 ...............X
          00C0 ...............X  Legenda:
          0100 ...............X  . 6-bit sprite reference plus 2-bit index into next border X
          0140 ...............X  X 4*4-bit index into palette P
          0180 ...............X  P 16*4-bit indexed color palette
          01C0 ...............X  r display (and IO?) settings registers (e.g. scroll offset, actual viewport; TBD)
          0200 ...............X  S sprite register (sprite reference, size, x, y)
          0240 ...............X  s (other) sprite register (sprite reference, size, x, y)
          0280 ...............X
          02C0 ...............X
          0300 ...............X
          0340 ...............X
          0380 ...............X
          03C0 PPPPrrrrssSSssSS
    0400  ----
    ...   512b monochrome sprite data (64 sprites, maybe 4 banks of 16)
    0600  ----
    ...   512b indexed sprite data (64 sprites, maybe 4 banks of 16)
    0800  ----


## Using its own display as a sprite editor

Getting data from an image editor into a (virtual) computer is a pain.
So, being self-contained is very useful indeed.

Being 30x30 blocks in size, the screen does not hold sufficient space to edit
all 32x32 pixels in a bank at once. But even if it would, we would still miss a
preview option (4x4 blocks) and perhaps tool selectors. Instead, the editor may
be 16x16 or even 24x24 blocks wide.

In theory this field can be used to either edit a same-sized subset of the
32x32 bank, or a single scaled-up 8x8 block.

The editor will need to carry at least some sprites of its own to use the
display.


## The processor

- To be inspired by CHIP-8, or any existing processor to re-use tooling
  - If 6502, reserve 0000-0200 (zero-page and stack)
  - If 6502, we get highly compatible with the L-Star Plus (but it's monochrome!)
- Having an ESP32-based emulator / interpreter is a big plus, too.
- Maybe address memory by 1kb banks (10 bits),
  leaving 6 bits (32 options) for command data

The display 'chip' can be used without specifying the processor!

