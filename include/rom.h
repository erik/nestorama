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
