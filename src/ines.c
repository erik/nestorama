#include "ines.h"
#include "nes.h"
#include "mapper.h"
#include "rom.h"

#include <string.h>

struct ROM* ines_rom_load_file(FILE* fp, struct ROM* rom)
{

  struct iNES_ROM_header header;

  char hdr[16] = {0};
  long hdrsz = fread(hdr, 1, 16, fp);

  if(hdrsz != 16 || memcmp(hdr, INES_HEADER, 4)) {
    LOGF("Given magic 0x%X 0x%X 0x%X 0x%X, expected 0x4E 0x45 0x53 0x1A, is this an iNES ROM?",
         hdr[0], hdr[1], hdr[2], hdr[3]);
    goto fail;
  }

  header.prg_rom_count = hdr[4];
  header.chr_rom_count = hdr[5];
  header.flags6        = hdr[6];
  header.flags7        = hdr[7];
  header.prg_ram_count = hdr[8];
  header.format        = hdr[9];

  header.mapper_num  = header.flags7 | (header.flags6 >> 4);

  u32 rom_size = header.prg_rom_count * 0x4000;
  u32 vrom_size = header.chr_rom_count * 0x2000;

  LOGF("Loading 0x%X bytes of PRG ROM and 0x%X bytes of VROM", rom_size, vrom_size);

  u8* rom_buf = malloc(rom_size);
  u8* vrom_buf = malloc(vrom_size);

  fread(rom_buf, 1, rom_size, fp);
  fread(vrom_buf, 1, vrom_size, fp);

  nes_set_rom(rom->nes, rom_buf, rom_size);
  nes_set_vrom(rom->nes, vrom_buf, vrom_size);

  rom->hdr.type = INES;
  rom->hdr.format = header.format ? PAL : NTSC;
  rom->hdr.prg_rom_count = header.prg_rom_count;
  rom->hdr.chr_rom_count = header.chr_rom_count;
  rom->hdr.has_prg_ram = (header.prg_ram_count != 0);

  return rom;

 fail:
  LOGF("ROM load failed, aborting...");
  return NULL;
}
