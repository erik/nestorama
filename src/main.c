#include <string.h>

#include "def.h"
#include "ines.h"
#include "6502.h"
#include "2C02.h"
#include "nes.h"
#include "mapper.h"
#include "rom.h"

int usage(void)
{
  fprintf(stderr, "Usage: nestorama NESROM\n");
  return 1;
}

int main(int argc, char** argv)
{
  if(argc < 2) {
    return usage();
  }

  LOGF("Trying to load ROM: %s", argv[1]);

  struct NES* nes = nes_create();

  FILE* fp = fopen(argv[1], "rb");
  nes_load_rom(nes, fp);
  fclose(fp);

  nes_run(nes);

  nes_inspect(nes);
  nes_free(nes);
  return 0;
}
