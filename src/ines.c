#include "ines.h"
#include "nes.h"
#include "mapper.h"

#include <string.h>

struct iNES_ROM* ines_rom_load_file(FILE* fp, struct NES* nes)
{
  struct iNES_ROM* rom = malloc(sizeof(struct iNES_ROM));

  char hdr[16] = {0};
  long hdrsz = fread(hdr, 1, 16, fp);

  if(hdrsz != 16 || memcmp(hdr, INES_HEADER, 4)) {
    LOGF("Given magic 0x%X 0x%X 0x%X 0x%X, expected 0x4E 0x45 0x53 0x1A, is this an iNES ROM?",
         hdr[0], hdr[1], hdr[2], hdr[3]);
    goto fail;
  }

  rom->header.prg_rom_count = hdr[4];
  rom->header.chr_rom_count = hdr[5];
  rom->header.flags6        = hdr[6];
  rom->header.flags7        = hdr[7];
  rom->header.prg_ram_count = hdr[8];
  rom->header.format        = hdr[9];

  rom->mapper_num  = rom->header.flags7 | (rom->header.flags6 >> 4);

  u32 rom_size = rom->header.prg_rom_count * 0x4000;
  u32 vrom_size = rom->header.chr_rom_count * 0x2000;

  LOGF("Loading 0x%X bytes of PRG ROM and 0x%X bytes of VROM", rom_size, vrom_size);

  u8* rom_buf = malloc(rom_size);
  u8* vrom_buf = malloc(vrom_size);

  fread(rom_buf, 1, rom_size, fp);
  fread(vrom_buf, 1, vrom_size, fp);

  nes_set_rom(nes, rom_buf, rom_size);
  nes_set_vrom(nes, vrom_buf, vrom_size);

  return rom;

 fail:
  LOGF("ROM load failed, aborting...");
  return NULL;
}

void ines_rom_free(struct iNES_ROM* rom)
{
  free(rom);
}

void ines_rom_inspect(struct iNES_ROM* rom)
{
  // TODO: rest of header and information
  printf("ROM = { PRG ROM=0x%X, CHR ROM=0x%X, PRG RAM=0x%X, format=%s mapper=%d}\n",
         rom->header.prg_rom_count * 0x4000, rom->header.chr_rom_count * 0x2000,
         rom->header.prg_ram_count * 0x2000, rom->header.format == 0 ? "NTSC" : "PAL",
         rom->mapper_num);
}
