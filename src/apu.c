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
