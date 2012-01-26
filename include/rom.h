#pragma once

#ifndef _ROM_H
#define _ROM_H

#include "def.h"
#include "mapper.h"

struct NES;
struct mapper;

enum rom_type { INES, NSF, NES2 };
enum rom_format { NTSC, PAL };

// some generic ROM header information
struct rom_header {
  enum rom_type type;
  enum rom_format format;
  enum rom_mapper mapper;

  u8 prg_rom_count; // blocks of PRG ROM (16KB units)
  u8 chr_rom_count; // blocks of CHR ROM (8KB units)

  bool has_prg_ram; // has 0x2000 bytes of SRAM at 0x6000
};

struct ROM {
  struct rom_header hdr;

  struct mapper* map;
  struct NES* nes;
};

// functions
struct ROM* rom_load_file(FILE* f, struct NES* nes);
void        rom_free(struct ROM* rom);
void        rom_inspect(struct ROM* rom);
u8          rom_fetch_memory(struct ROM* rom, u16 addr);
void        rom_set_memory(struct ROM* rom, u16 addr, u8 val);

#endif /* _ROM_H */
