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
