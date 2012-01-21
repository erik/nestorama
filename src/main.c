#include <string.h>

#include "def.h"
#include "ines.h"
#include "6502.h"
#include "2C02.h"
#include "nes.h"

static int builtin_test(void)
{
  struct NES* nes = nes_create();

  u8 prog[] = {0xA9, 0x55,        // LDA imm $55
               0x85, 0x65,        // STA zp  $65
               0xE6, 0x65,        // INC zp  $65
               0xA5, 0x65,        // LDA zp  $65
               0x6C, 0x01, 0x0};  // JMP ind $0100

  memcpy(nes->mem.lowmem, prog, sizeof(prog));
  nes->cpu->r.pc = 0;

  nes_inspect(nes);

  for(int i = 0; i < 6; ++i) {
    nes_tick(nes);
  }

  nes_inspect(nes);

  nes_free(nes);
  return 0;
}

int main(int argc, char** argv)
{
  if(argc < 2) {
    LOGF("Running built in test.");
    return builtin_test();
  } else {
    LOGF("Trying to load ROM: %s", argv[1]);

    struct NES* nes = nes_create();

    FILE* fp = fopen(argv[1], "rb");
    nes_load_rom(nes, fp);

    nes_inspect(nes);

    fclose(fp);

    nes_free(nes);
    return 0;
  }
}
