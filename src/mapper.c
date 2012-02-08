/*
 * This file is part of Nestorama.
 *
 * Nestorama is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Nestorama is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Nestorama.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "mapper.h"
#include "nes.h"
#include "rom.h"

#include <string.h>

struct mapper* mapper_create(struct ROM* rom)
{
  struct mapper* map = malloc(sizeof(struct mapper));
  memset(map, 0, sizeof(struct mapper));

  map->rom = rom;
  map->num = rom->hdr.mapper;;

  mapper_init_banks(map);

  return map;
}

void mapper_free(struct mapper* map)
{
  free(map);
}

void mapper_init_banks(struct mapper* map)
{
  switch(map->num) {
  case CNROM:
  case NROM:
    mapper_set_rom_bank(map, 0, 0x8000, 0x8000);
    break;
  case MMC1:
  case AXROM:
  default:
    mapper_set_rom_bank(map, 0,  0x0000, 0x4000);
    mapper_set_rom_bank(map, 0,  0x4000, 0x4000);
    mapper_set_rom_bank(map, 0,  0x8000, 0x4000);
    mapper_set_rom_bank(map, -1, 0xC000, 0x4000);
    break;
  }
}

// this function just factors out some functionality common to ROM and VROM bankswitching
static void mapper_set_bank(struct mapper* map, u32 index, u16 addr, u16 size, bool use_rom)
{
  struct NES* nes = map->rom->nes;

  u8* rom    = use_rom ? nes->mem->rom      : nes->mem->vrom;
  u32 rs     = use_rom ? nes->mem->rom_size : nes->mem->vrom_size;
  u8** banks = use_rom ? map->rom_banks     : map->vrom_banks;

  u32 bs = use_rom ? ROM_BANK_SIZE : VROM_BANK_SIZE;

  for(u32 page = addr / bs, idx = rs + index * size;
      page < (addr + size) / bs && page < NUM_BANKS;
      page++, idx += bs) {

    banks[page] = &rom[idx % rs];
  }
}

void mapper_set_rom_bank(struct mapper* map, u16 index, u16 addr, u16 size)
{
  LOGF("Setting 0x%X through 0x%X to ROM index %d", addr, addr + size, index);

  struct NES* nes = map->rom->nes;
  mapper_set_bank(map, index, addr, size, true);
}

void mapper_set_vrom_bank(struct mapper* map, u16 index, u16 addr, u16 size)
{
  LOGF("Setting 0x%X through 0x%X to VROM index %d", addr, addr + size, index);

  struct NES* nes = map->rom->nes;
  mapper_set_bank(map, index, addr, size, false);
}

u8 mapper_fetch_memory(struct mapper* map, u16 addr)
{
  // PRG RAM (SRAM)
  // (addr >> 13) == 3 checks if 0x6000 <= addr <= 0x7FFF
  if((addr >> 13) == 3) {
    return map->sram[addr - 0x6000];
  }

  u8* bank = map->rom_banks[(addr / ROM_BANK_SIZE) % NUM_BANKS];
  return bank[addr % ROM_BANK_SIZE];
}

void mapper_set_memory(struct mapper* map, u16 addr, u8 val)
{
  switch(map->num) {
  case MMC1: {

    // bit 7 is the reset flag
    if(val >> 7) {
      map->data.mmc1.write_count = 0;
      map->data.mmc1.regs[0] = (1 << 2) | (1 << 3);

      LOGF("MMC1 reset bit set");

      goto switch_prg;
    }

    map->data.mmc1.reg_cache |= (val & 1) << map->data.mmc1.write_count;

    // only every 5th write actually does anything
    if(++map->data.mmc1.write_count == 5) {
      u8 reg_index, value;

    switch_prg:

      reg_index = (addr >> 13) & 3;
      value = map->data.mmc1.regs[reg_index] = map->data.mmc1.reg_cache;
      map->data.mmc1.write_count = map->data.mmc1.reg_cache = 0;

      /* control register (actual bankswitching takes place in 0xE000 - 0xFFFF)
         $8000-9FFF:  [...C PSMM]
         - C = CHR Mode (0=8k mode, 1=4k mode)
         - P = PRG Size (0=32k mode, 1=16k mode)

         - S = Slot select:
         -- 0 = $C000 swappable, $8000 fixed to page $00 (mode A)
         -- 1 = $8000 swappable, $C000 fixed to page $0F (mode B)
         -- This bit is ignored when 'P' is clear (32k mode)

         - M = Mirroring control:
         -- %00 = 1ScA
         -- %01 = 1ScB
         -- %10 = Vert
         -- %11 = Horz
      */

      map->data.mmc1.chr_mode = (value >> 4) & 1;

      // TODO: nametable mirroring

      /* CHR Bank 0
         $A000-BFFF:  [...C CCCC]
         - CHR Reg 0

         CHR Bank 1
         $C000-DFFF:  [...C CCCC]
         - CHR Reg 1
      */
      {
        // checks if bit 5 is set (4k)
        u8 use_4k = (map->data.mmc1.regs[0] >> 4) & 1;
        u8 bank = map->data.mmc1.regs[1] & 0x10;

        // TODO: this is almost certainly wrong here
        // mapper_set_vrom_bank(map, use_4k ? bank & 0xFE : bank, 0x0000, 0x0FFF);
        // mapper_set_vrom_bank(map, use_4k ? bank & 0xFE + 1 : bank, 0x1000, 0x0FFF);
      }

      /* PRG Bank
         $E000-FFFF:  [...W PPPP]
         - W = RAM Disable (0=enabled, 1=disabled)
         - P = PRG Reg
      */
      {
        u8 reg0 = map->data.mmc1.regs[0];

        u8 chr = (reg0 >> 4) & 1;
        u8 prg = (reg0 >> 3) & 1;
        u8 slot = (reg0 >> 2) & 1;
        u8 mirror = reg0 & 3;

        u8 bank = map->data.mmc1.regs[3] & 0xF;
        map->rom->hdr.has_prg_ram = (map->data.mmc1.regs[3] >> 4) & 1;

        if(prg == 0) {
          // ignores low bit of bank number
          mapper_set_rom_bank(map, (bank & 0xE) >> 1, 0x8000, 0x8000);
        } else {

          if(slot == 0) {
            mapper_set_rom_bank(map, 0,    0x8000, 0x4000);
            mapper_set_rom_bank(map, bank, 0xC000, 0x4000);
          }

          if(slot == 1) {
            mapper_set_rom_bank(map, ~0, 0xC000, 0x4000);
            mapper_set_rom_bank(map, bank, 0x8000, 0x4000);
          }
        }
      }
    }

    break;
  }
  case AXROM: {
    // (val & 7) to grap lowest 3 bits (representing bank index)
    mapper_set_rom_bank(map, val & 0x7, 0x8000, 0x8000);

    // (val >> 3) & 1 to grab bit 4 (vrom)
    // mapper_set_vrom_bank(map, (val >> 3) & 0x1, ..., ...);
    break;
  }

  case NROM:
  case CNROM: {
    // TODO: simulate bus conflict
  }

  default:
    LOGF("XXX: Tried to access memory location 0x%X, which doesn't seem to be mapped", addr);
  }
}
