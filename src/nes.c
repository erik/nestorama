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


/* */

#include "nes.h"

#include "6502.h"
#include "2C02.h"
#include "apu.h"
#include "mapper.h"
#include "rom.h"

struct NES* nes_create(void)
{
  struct NES* nes = calloc(1, sizeof(struct NES));

  nes->cpu = cpu_6502_create(nes);
  nes->ppu = ppu_2C02_create(nes);
  nes->apu = apu_create(nes);

  nes->mem = calloc(1, sizeof(struct memory));

  nes->mem->rom_size = nes->mem->vrom_size = 0;
  nes->is_active = false;

  nes->rom = NULL;

  return nes;
}


void nes_free(struct NES* nes)
{
  cpu_6502_free(nes->cpu);
  ppu_2C02_free(nes->ppu);
  apu_free(nes->apu);


  free(nes->mem->rom);
  free(nes->mem->vrom);
  free(nes->mem);

  if(nes->rom) rom_free(nes->rom);

  free(nes);
}

void nes_powerup(struct NES* nes)
{
  cpu_6502_powerup(nes->cpu);
  ppu_2C02_powerup(nes->ppu);
  apu_powerup(nes->apu);
}

void nes_reset(struct NES* nes)
{
  cpu_6502_reset(nes->cpu);
  ppu_2C02_reset(nes->ppu);
  apu_reset(nes->apu);
}

bool nes_load_rom(struct NES* nes, FILE* fp)
{
  // free any previous ROM
  if(nes->rom) free(nes->rom);

  nes->rom = rom_load_file(fp, nes);

  if(!nes->rom) {
    LOGF("ROM load failed");
    return false;
  }

  return true;
}

void nes_run(struct NES* nes)
{
  LOGF("Beginning execution");
  nes_powerup(nes);
  nes->is_active = true;

  printf("PC     OP  \tNAM  TYPE\tINFO\n");
  while(nes->is_active) {
    nes_tick(nes);
  }

  return;
}

void nes_tick(struct NES* nes)
{
  // PPU ticks at 3 times CPU rate
  for(int i = 0; i < 3; ++i)
    ppu_2C02_tick(nes->ppu);

  // APU ticks at 1 times CPU rate
  for(int i = 0; i < 1; ++i)
    apu_tick(nes->apu);

  cpu_6502_tick(nes->cpu);
}


void nes_inspect(struct NES* nes)
{
  cpu_6502_inspect(nes->cpu);

  ppu_2C02_inspect(nes->ppu);
  apu_inspect(nes->apu);

  if(nes->rom) rom_inspect(nes->rom);
}

// the bitwise ANDing in set_memory and fetch_memory are to compensate for memory mirroring
u8 nes_fetch_memory(struct NES* nes, u16 addr)
{
  // Low memory
  if(addr < 0x2000) {
    return nes->mem->lowmem[addr & 0x7FF];
  }

  // PPU registers
  if(addr < 0x4000) {
    return ppu_2C02_get_register(nes->ppu, addr & 0x07);
  }

  // APU registers
  if(addr < 0x4018) {
    return nes->mem->apureg[addr & 0x7];
  }

  // ROM memory
  else {
    return rom_fetch_memory(nes->rom, addr);
  }
}

void nes_set_memory(struct NES* nes, u16 addr, u8 value)
{

  // Low memory
  if(addr < 0x2000) {
    nes->mem->lowmem[addr & 0x7FF] = value;
  }

  // PPU registers
  else if(addr < 0x4000) {
    ppu_2C02_set_register(nes->ppu, addr & 0x07, value);
  }

  // APU registers
  else if(addr < 0x4018) {
    nes->mem->apureg[addr & 0x17] = value;
  }

  // ROM memory
  else {
    rom_set_memory(nes->rom, addr, value);
  }
}
