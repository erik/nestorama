#include "2C02.h"

#include "nes.h"

struct _2C02* ppu_2C02_create(struct NES* nes)
{
  struct _2C02* ppu = malloc(sizeof(struct _2C02));
  ppu->nes = nes;

  return ppu;
}

void ppu_2C02_free(struct _2C02* ppu)
{
  free(ppu);
}

void ppu_2C02_powerup(struct _2C02* ppu)
{
  (void)ppu;
  // TODO
}

void ppu_2C02_reset(struct _2C02* ppu)
{
  (void)ppu;
  // TODO
}

void ppu_2C02_tick(struct _2C02* ppu)
{
  (void)ppu;
  // TODO
}

void ppu_2C02_inspect(struct _2C02* ppu)
{
  printf("TODO: ppu_2C02_inspect %p\n", ppu);
}
