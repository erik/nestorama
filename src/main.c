#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6502.h"
#include "nes.h"
#include "2C02.h"
int main(void) {

  struct NES* nes = nes_create();

  u8 prog[] = {0xA9, 0x55,        // LDA imm $55
               0x85, 0x65,        // STA zp  $65
               0xE6, 0x65,        // INC zp  $65
               0xA5, 0x65,        // LDA zp  $65
               0x6C, 0x01, 0x0};  // JMP ind $0100

  memcpy(nes->mem.lowmem, prog, sizeof(prog));
  nes->cpu->r.pc = 0;

  nes_inspect(nes);

  for(int i = 0; i < 5; ++i) {
    nes_tick(nes);
  }

  nes_set_memory(nes, 0x2002, 0x44);

  nes_inspect(nes);

  nes_free(nes);
  return 0;
}
