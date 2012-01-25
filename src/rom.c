#include "rom.h"
#include "ines.h"
#include "mapper.h"

#include <string.h>

struct ROM* rom_load_file(FILE* fp, struct NES* nes)
{

  struct ROM* rom = malloc(sizeof(struct ROM));
  memset(rom, 0, sizeof(struct ROM));

  rom->nes = nes;

  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  rewind(fp);

  LOGF("Loading ROM: %ld bytes", fsize);
  LOGF("Trying to determine ROM type.");

  // check for iNES
  char hdr[4] = {0};
  fread(hdr, 1, 4, fp);
  if(!memcmp(hdr, INES_HEADER, 4)) {
    LOGF("This ROM appears to be an iNES file");

    rewind(fp);

    if(!ines_rom_load_file(fp, rom)) goto fail;

  } else {
    LOGF("Can't determine this ROM's file type");
    goto fail;
  }

  rom->map = mapper_create(rom);

  return rom;
 fail:
  free(rom);
  LOGF("Failed to load ROM, aborting");

  return NULL;
}


void rom_free(struct ROM* rom)
{
  free(rom->map);
  free(rom);
}


void rom_inspect(struct ROM* rom)
{
  printf("ROM = { PRG ROM=0x%X, CHR ROM=0x%X, PRG RAM=%s, format=%s mapper=%d }\n",
         rom->hdr.prg_rom_count * 0x4000, rom->hdr.chr_rom_count * 0x2000,
         rom->hdr.has_prg_ram ? "YES" : "NO", rom->hdr.format == NTSC ? "NTSC" : "PAL",
         rom->map->num);
}


u8 rom_fetch_memory(struct ROM* rom, u16 addr)
{
  return mapper_fetch_memory(rom->map, addr);
}

void rom_set_memory(struct ROM* rom, u16 addr, u8 val)
{
  mapper_set_memory(rom->map, addr, val);
}
