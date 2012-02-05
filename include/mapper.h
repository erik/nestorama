/* */

#pragma once

#ifndef _MAPPER_H_
#define _MAPPER_H_

#include "def.h"

struct ROM;
struct NES;

#define NUM_BANKS 8
static const int ROM_BANK_SIZE = 0x2000;
static const int VROM_BANK_SIZE = 0x0400;

// for MMC1
struct mmc1_data {
  u8 write_count;
  u8 chr_mode;
  u8 reg_cache;
  u8 regs[4];
};

// registers and data values that may be specific to a mapper
union mapper_data {
  struct mmc1_data mmc1;
};

// all bit representations are 7   ->  0
enum rom_mapper {
  NROM = 0,
  MMC1 = 1,
  CNROM = 3,

  /* Any write to 0x8000 - 0xFFFF:
    xxxM xPPP
    PPP - Select 32 KB PRG ROM bank for CPU $8000-$FFFF
    M   - Select 1 KB VRAM page for all 4 nametables
  */
  AXROM = 7
};

struct mapper {
  enum rom_mapper num;

  u8 sram[0x2000];

  union mapper_data data;

  u8* rom_banks[NUM_BANKS];
  u8* vrom_banks[NUM_BANKS];

  struct ROM* rom;
};

// functions
struct mapper* mapper_create(struct ROM* rom);
void           mapper_free(struct mapper* map);

void           mapper_init_banks(struct mapper* map);
void           mapper_set_rom_bank(struct mapper* map, u16 index, u16 addr, u16 size);
void           mapper_set_vrom_bank(struct mapper* map, u16 index, u16 addr, u16 size);

u8             mapper_fetch_memory(struct mapper* map, u16 addr);
void           mapper_set_memory(struct mapper* map, u16 addr, u8 val);

#endif /* _MAPPER_H_ */
