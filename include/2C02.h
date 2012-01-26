/* Emulation of the Ricoh RP2C02, the PPU of the NES */

#pragma once

#ifndef _PPU_H
#define _PPU_H

#include "def.h"

struct NES;

/*

  For a better description, see: http://wiki.nesdev.com/w/index.php/PPU_registers

  2C02 registers (1 byte each, starting at 0x2000)
  port 7 = 0x2007, etc.
  -----------------------
  reg   bit desc
  ---   --- ----
  0     0   X scroll name table selection.
        1   Y scroll name table selection.
        2   increment PPU address by 1/32 (0/1) on access to port 7
        3   object pattern table selection (if bit 5 = 0)
        4   playfield pattern table selection
        5   8/16 scanline objects (0/1)
        6   EXT bus direction (0:input; 1:output)
        7   /VBL disable (when 0)

  1     0   disable composite colorburst (when 1). Effectively causes gfx to go black & white.
        1   left side screen column (8 pixels wide) playfield clipping (when 0).
        2   left side screen column (8 pixels wide) object clipping (when 0).
        3   enable playfield display (on 1).
        4   enable objects display (on 1).
        5   R (to be documented)
        6   G (to be documented)
        7   B (to be documented)

  2     5   more than 8 objects on a single scanline have been detected in the last frame
        6   a primary object pixel has collided with a playfield pixel in the last frame
        7   vblank flag

  3     -   internal object attribute memory index pointer (64 attributes, 32 bits
            each, byte granular access). stored value post-increments on access to port 4

  4     -   returns object attribute memory location indexed by port 3, then
            increments port 3.

  5     -   scroll offset port.

  6     -   PPU address port to access with port 7.

  7     -   PPU memory read/write port.
*/

struct ppu_control_register {   // 0x2000
  int nametable    : 2; // scroll name table selection (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
  int increment    : 1; // 0: increment by 1,  1: increment by 32
  int sprite_table : 1; // 0: $0000; 1: $1000; ignored in 8x16 mode
  int pattern      : 1; // 0: $0000; 1: $1000
  int sprite_size  : 1; // 0: 8x8; 1: 8x16
  int unused       : 1; // ignored
  int nmi          : 1; // 0: off; 1: on
};

struct ppu_mask_register {      // 0x2001
  int greyscale    : 1; // 0: normal color; 1: produce a monochrome display
  int clip_playfield  : 1; // 1: show background in leftmost 8 pixels of screen; 0: Hide
  int clip_object     : 1; // 1: Show sprites in leftmost 8 pixels of screen; 0: Hide
  int show_background : 1; // 1: Show background
  int show_sprites : 1; // 1: Show sprites
  int red          : 1; // Intensify reds (and darken other colors)
  int green        : 1; // Intensify greens (and darken other colors)
  int blue         : 1; // Intensify blues (and darken other colors)
};

struct ppu_status_register {    // 0x2002
  int lsb       : 5; // 5 least significant bits, unused
  int overflow  : 1; // sprite scanline overflow
  int sprite_hit: 1; // set when sprite 0 hits nonzero background pixel
  int vblank    : 1; // set when in vblank
};

struct __attribute__ ((aligned)) ppu_registers {
  struct ppu_control_register ctrl;        // 0x2000
  struct ppu_mask_register    mask;        // 0x2001
  struct ppu_status_register  status;      // 0x2002
  u8                          oam_addr;    // 0x2003
  u8                          oam_data;    // 0x2004
  u8                          ppu_scroll;  // 0x2005
  u8                          ppu_addr;    // 0x2006
  u8                          ppu_data;    // 0x2007
};

struct _2C02 {
  struct ppu_registers r;

  struct NES* nes;
};

// functions
struct _2C02* ppu_2C02_create(struct NES* nes);
void          ppu_2C02_free(struct _2C02* ppu);
void          ppu_2C02_powerup(struct _2C02* ppu);
void          ppu_2C02_reset(struct _2C02* ppu);

void          ppu_2C02_tick(struct _2C02* ppu);
void          ppu_2C02_set_register(struct _2C02* ppu, u8 reg, u8 val);
u8            ppu_2C02_get_register(struct _2C02* ppu, u8 reg);
void          ppu_2C02_inspect(struct _2C02* ppu);

#endif /* _PPU_H */
