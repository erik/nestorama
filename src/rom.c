#include "rom.h"
#include "ines.h"
#include "mapper.h"

#include <string.h>

struct ROM* rom_load_file(FILE* fp, struct NES* nes)
{

  struct ROM* rom = malloc(sizeof(struct ROM));
  memset(rom, 0, sizeof(struct ROM));

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

    struct iNES_ROM* ines = ines_rom_load_file(fp, nes);
    if(!ines) goto fail;

    rom->type = INES;
    rom->rom.ines = ines;

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

  switch(rom->type) {
  case INES:
    ines_rom_free(rom->rom.ines);
    break;
  default:
    LOGF("That type isn't supported yet");
    break;
  }

  free(rom);
}


void rom_inspect(struct ROM* rom)
{
  switch(rom->type) {
  case INES:
    ines_rom_inspect(rom->rom.ines);
    break;
  default:
    LOGF("That type isn't supported yet");
    break;
  }
}
