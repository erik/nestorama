#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6502.h"

int main(void) {

  struct _6502* cpu = cpu_6502_create();

  u8 prog[] = {0xA9, 0x55, 0x85, 0x65, 0xE6, 0x65, 0xA5, 0x65, 0x6C, 0x01, 0x0};
  memcpy(cpu->mem.lowmem, prog, sizeof(prog));
  cpu->r.pc = 0;

  cpu_6502_evaluate(cpu);
  cpu_6502_evaluate(cpu);
  cpu_6502_evaluate(cpu);
  cpu_6502_evaluate(cpu);
  cpu_6502_evaluate(cpu);

  cpu->r.flags.z = 1;
  cpu->r.flags.n = 1;
  cpu->r.flags.c = 1;

  cpu_6502_inspect(cpu);
  return 0;
}
