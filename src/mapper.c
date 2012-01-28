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
    // bits 2,3 of 0x8000 are set
    mapper_set_memory(map, 0x8000, (1 << 1) | (1 << 2));
    break;
  case AXROM:
    // unknown(?) power up state
    break;
  default:
    LOGF("TODO: Write init routine for mapper %d", map->num);
  }
}

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
    LOGF("TODO: MMC1");
    break;
  }
  case AXROM: {
    // (val & 7) to grap lowest 3 bits (representing bank index)
    mapper_set_rom_bank(map, val & 0x7, 0x8000, 0x8000);

    // (val >> 4) & 1 to grab bit 4 (vrom)
    break;
  }

  default:
    LOGF("XXX: Tried to access memory location 0x%X, which doesn't seem to be mapped", addr);
  }
}
