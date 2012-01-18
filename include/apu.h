/* Emulation of the APU chip built into the NES' 2A03 */

#pragma once

#ifndef _APU_H
#define _APU_H

static struct apu_registers {
  int placeholder;
};

struct APU {
  struct apu_registers r;
  /* TODO: Write me */
};

#endif /* _APU_H */
