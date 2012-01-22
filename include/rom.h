#pragma once

#ifndef _ROM_H
#define _ROM_H

#include "def.h"

struct NES;
struct iNES_ROM;
struct mapper;

// for alignment
static struct generic_rom { u8 mapper_num; };

enum rom_type { INES, NSF, NES2 };

struct ROM {
  enum rom_type type;
  struct mapper* map;

  union {
    struct generic_rom* gen;
    struct iNES_ROM* ines;
    // TODO: NSF, NES2.0, etc.
  } rom;
};

// functions
struct ROM* rom_load_file(FILE* f, struct NES* nes);
void        rom_free(struct ROM* rom);
void        rom_inspect(struct ROM* rom);

#endif /* _ROM_H */
