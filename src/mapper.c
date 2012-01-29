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

  case MMC1:
    LOGF("init MMC1");

    mapper_set_memory(map, 0x8000, 0x80);
    break;
  case AXROM:
    // unknown(?) power up state
    LOGF("init AXROM");
    break;
  default:
    LOGF("TODO: Write init routine for mapper %d", map->num);
  }
}

// this function just factors out some functionality common to ROM and VROM bankswitching
static void mapper_set_bank(struct mapper* map, u8 bank_num, u16 addr, u16 size,
                            unsigned rs, u8* rom, u8* dest[])
{
  struct NES* nes = map->rom->nes;

  unsigned rom_idx = rs + bank_num * size;
  u8 end_idx = (addr + size) / BANK_SIZE;

  for(u8 bank_idx = addr / BANK_SIZE; bank_idx < end_idx && bank_idx < NUM_BANKS; bank_idx++) {
    dest[bank_idx] = rom + (rom_idx % rs);
    rom_idx += BANK_SIZE;
  }
}

void mapper_set_rom_bank(struct mapper* map, u8 bank_num, u16 addr, u16 size)
{
  LOGF("Setting 0x%X through 0x%X to ROM bank %d", addr, addr + size, bank_num);

  struct NES* nes = map->rom->nes;
  mapper_set_bank(map, bank_num, addr, size, nes->mem.rom_size, nes->mem.rom, map->rom_banks);
}

void mapper_set_vrom_bank(struct mapper* map, u8 bank_num, u16 addr, u16 size)
{
  LOGF("Setting 0x%X through 0x%X to VROM bank %d", addr, addr + size, bank_num);

  struct NES* nes = map->rom->nes;
  mapper_set_bank(map, bank_num, addr, size, nes->mem.vrom_size, nes->mem.vrom, map->vrom_banks);
}

u8 mapper_fetch_memory(struct mapper* map, u16 addr)
{
  // PRG RAM (SRAM)
  // (addr >> 13) == 3 checks if 0x6000 <= addr <= 0x7FFF
  if((addr >> 13) == 3) {
    return map->sram[addr - 0x6000];
  }

  u8* bank = map->rom_banks[(addr / BANK_SIZE) % NUM_BANKS];
  return bank[addr % BANK_SIZE];

}

void mapper_set_memory(struct mapper* map, u16 addr, u8 val)
{
  switch(map->num) {
  case MMC1: {

    // bit 7 is the reset flag
    if(val >> 7) {
      map->data.mmc1.write_count = 0;
      map->data.mmc1.regs[0] = (1 << 2) | (1 << 3);
      map->data.mmc1.write_count = map->data.mmc1.reg_cache = 0;

      LOGF("MMC1 reset bit set");

      goto switch_prg;
    }

    map->data.mmc1.reg_cache |= (val & 1) << map->data.mmc1.write_count;

    // only every 5th write actually does anything
    if(++map->data.mmc1.write_count == 5) {
      u8 reg_index = (addr >> 13) & 3;
      u8 value = map->data.mmc1.regs[reg_index] = map->data.mmc1.reg_cache;

      map->data.mmc1.write_count = map->data.mmc1.reg_cache = 0;

      switch(reg_index) {
      case 0: {
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

        break;
      }

      case 1:
      case 2: {
        /* CHR Bank 0
           $A000-BFFF:  [...C CCCC]
           - CHR Reg 0

           CHR Bank 1
           $C000-DFFF:  [...C CCCC]
           - CHR Reg 1
        */

        // checks if bit 5 is set (4k)
        u8 use_4k = (map->data.mmc1.regs[0] >> 4) & 1;

        u8 bank = map->data.mmc1.regs[1] & 0x10;


        // TODO: this is almost certainly wrong here
        mapper_set_vrom_bank(map, use_4k ? bank & 0xFE : bank, 0x0000, 0x0FFF);
        mapper_set_vrom_bank(map, use_4k ? bank & 0xFE + 1 : bank, 0x1000, 0x0FFF);
        break;
      }

      case 3:
      switch_prg: {
        /* PRG Bank
           $E000-FFFF:  [...W PPPP]
           - W = RAM Disable (0=enabled, 1=disabled)
           - P = PRG Reg
        */

        u8 reg0 = map->data.mmc1.regs[0];

        u8 chr = (reg0 >> 4) & 1;
        u8 prg = (reg0 >> 3) & 1;
        u8 slot = (reg0 >> 2) & 1;
        u8 mirror = reg0 & 3;

        u8 bank = map->data.mmc1.regs[3] & 0xF;
        u8 has_ram = (map->data.mmc1.regs[3] >> 4) & 1;

        map->rom->hdr.has_prg_ram = has_ram;

        if(prg == 0) {
          // ignores low bit of bank number
          mapper_set_rom_bank(map, (bank & 0xE) >> 1, 0x8000, 0x7FFF);
        } else {

          if(slot == 0) {
            mapper_set_rom_bank(map, 0, 0x8000, 0x3FFF);
            mapper_set_rom_bank(map, bank, 0xC000, 0x3FFF);
          }

          if(slot == 1) {
            mapper_set_rom_bank(map, 0xFF, 0xC000, 0x3FFF);
            mapper_set_rom_bank(map, bank, 0x8000, 0x3FFF);
          }
        }

        break;
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

  default:
    LOGF("XXX: Tried to access memory location 0x%X, which doesn't seem to be mapped", addr);
  }
}
