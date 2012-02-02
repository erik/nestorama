/* iNES ROM loading */

#pragma once

#ifndef _INES_H
#define _INES_H

#include "def.h"

struct NES;
struct mapper;
struct ROM;

/*
  Header (16 bytes)
  Trainer, if present (0 or 512 bytes)
  PRG ROM data (16384 * x bytes)
  CHR ROM data, if present (8192 * y bytes)
  PlayChoice hint screen, if present (0 or 8192 bytes)

  The format of the header is as follows:
  ---------------------------------------
  - 0-3: Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
  - 4: Size of PRG ROM in 16 KB units
  - 5: Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
  - 6: Flags 6
  - 7: Flags 7
  - 8: Size of PRG RAM in 8 KB units (Value 0 infers 8 KB for compatibility; see PRG RAM circuit)
  - 9: Flags 9
  - 10: Flags 10 (unofficial)
  - 11-15: Zero filled
*/

struct iNES_ROM_header {
  u8 mapper_num;

  u8 prg_rom_count; // blocks of PRG ROM (16KB units)
  u8 chr_rom_count; // blocks of CHR ROM (8KB units) (0 value means ROM uses CHR RAM)
  u8 flags6;        //
  u8 flags7;        //
  u8 prg_ram_count; // blocks of PRG RAM (8KB units) (value 0 implies 8KB for compat)
  u8 format;        // 0 is NTSC, 1 is PAL (not widely used)
  u8 unofficial;    // unofficial flags

  // remainder of 16 bit header is left zero filled
};

static u8 INES_HEADER[4] = { 0x4E, 0x45, 0x53, 0x1A };

// functions
struct ROM*      ines_rom_load_file(FILE* f, struct ROM* nes);

#endif /* _INES_H */
