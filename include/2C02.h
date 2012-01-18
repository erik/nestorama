/* Emulation of the Ricoh RP2C02, the PPU of the NES */

#pragma once

#ifndef _PPU_H
#define _PPU_H

#include "def.h"

struct NES;

static struct ppu_mem {
  int placeholder;
};

struct _2C02 {
  struct ppu_mem mem;

  struct NES* nes;
};

// functions
struct _2C02* ppu_2C02_create(struct NES* nes);
void          ppu_2C02_free(struct _2C02* ppu);
void          ppu_2C02_powerup(struct _2C02* ppu);
void          ppu_2C02_reset(struct _2C02* ppu);
void          ppu_2C02_tick(struct _2C02* ppu);
void          ppu_2C02_inspect(struct _2C02* ppu);

#endif /* _PPU_H */
