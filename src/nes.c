/* */

#include "nes.h"

u8 nes_fetch_memory(struct NES* nes, u16 addr)
{
  // Low memory
  if(addr < 0x2000) {
    return nes->mem.lowmem[addr & 0x7FF];
  }

  // PPU registers
  if(addr < 0x4000) {
    return nes->mem.ppureg[addr & 0x7];
  }

  // APU registers
  if(addr < 0x4018) {
    return nes->mem.apureg[addr & 0x7];
  }

  // TODO: SRAM, PRG-RAM, etc.

  else {
    printf("Warning: accessing 0x%X, which is unknown\n", addr);
    return ~0;
  }
}

void nes_set_memory(struct NES* nes, u16 addr, u8 value)
{

  // Low memory
  if(addr < 0x2000) {
    nes->mem.lowmem[addr & 0x7FF] = value;
  }

  // PPU registers
  else if(addr < 0x4000) {
    nes->mem.ppureg[addr & 0x7] = value;
  }

  // APU registers
  else if(addr < 0x4018) {
    nes->mem.apureg[addr & 0x17] = value;
  }

  // TODO: SRAM, PRG-RAM, etc.

  else {
    printf("Warning: writing 0x%X, which is unknown\n", addr);
  }
}


struct NES* nes_create(void)
{
  struct NES* nes = malloc(sizeof(struct NES));

  nes->cpu = cpu_6502_create(nes);

  // nes->ppu = ppu_2C02_create();
  // nes->apu = apu_create();

  return nes;
}


void nes_free(struct NES* nes)
{
  cpu_6502_free(nes->cpu);

  // ppu_2C02_free(nes->ppu);
  // apu_free(nes->apu);

  free(nes);
}


void nes_reset(struct NES* nes)
{
  cpu_6502_reset(nes->cpu);

  // ppu_2C02_reset(nes->ppu);
  // apu_reset(nes->apu);
}


void nes_tick(struct NES* nes)
{
  for(int i = 0; i < 3; ++i);
    // ppu_2C02_tick(nes->ppu);
  for(int i = 0; i < 1; ++i);
    // apu_tick(nes->apu);

  cpu_6502_tick(nes->cpu);
}


void nes_inspect(struct NES* nes)
{
  cpu_6502_inspect(nes->cpu);

  // ppu_2C02_inspect(nes->ppu);
  // apu_inspect(nes->apu);
}
