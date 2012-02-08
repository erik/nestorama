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


/* Emulation of the APU chip built into the NES' 2A03 */

#pragma once

#ifndef _APU_H
#define _APU_H

#include "def.h"

struct NES;

struct apu_registers {
  int placeholder;
};

struct APU {
  struct apu_registers r;

  struct NES* nes;
};

// functions
struct APU*   apu_create(struct NES* nes);
void          apu_free(struct APU* apu);
void          apu_powerup(struct APU* apu);
void          apu_reset(struct APU* apu);

void          apu_tick(struct APU* apu);
void          apu_inspect(struct APU* apu);

#endif /* _APU_H */
