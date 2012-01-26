/* Emulation of the APU chip built into the NES' 2A03 */

#pragma once

#ifndef _APU_H
#define _APU_H

#include "def.h"

struct NES;

static struct apu_registers {
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
