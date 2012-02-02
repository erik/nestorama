/* emulation of the NES' Audio Processing Unit */

#include "apu.h"

struct APU* apu_create(struct NES* nes)
{
  struct APU* apu = malloc(sizeof(struct APU));
  apu->nes = nes;

  return apu;
}

void apu_free(struct APU* apu)
{
  free(apu);
}

void apu_reset(struct APU* apu)
{
  (void)apu;
  // TODO:
}

void apu_powerup(struct APU* apu)
{
  (void)apu;
  // TODO:
}


void apu_tick(struct APU* apu)
{
  (void)apu;
  // TODO
}

void apu_inspect(struct APU* apu)
{
  LOGF("TODO: apu_inspect(struct APU*) %p", (void*)apu);
}
