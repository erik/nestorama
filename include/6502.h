#pragma once

#ifndef _6502_H
#define _6502_H

#include "def.h"

#include <stdlib.h>

/* proc status / flag register layout
   +7 6 5 4 3 2 1 0+ bit number
   +-+-+-+-+-+-+-+-+
   |N|V|1|B|D|I|Z|C|
   +-+-+-+-+-+-+-+-+
*/
struct flag {
  /* C => carry        - set if add/sub produces carry/borrow,
     bit after shift */
  int c : 1;
  /* Z => zero         - load/inc/dec/add/sub was zero */
  int z : 1;
  /* I => IRQ disable  - maskable interrupts are disabled */
  int i : 1;
  /* D => Decimal mode - decimal mode active */
  int d : 1;
  /* B => BRK command  - interrupt caused by BRK */
  int b : 1;
  /* 1 => Unused       - always 1 */
  int u : 1;
  /* V => Overflow     - over/underflow produced  */
  int v : 1;
  /* N => Negative     - bit 7 of accumulator is set */
  int n : 1;

};

struct registers {
  u8 a;                 // accumulator
  u8 x;                 // general purpose / index reg
  u8 y;                 // general purpose / index reg
  u8 sp;                // stack pointer
  u16 pc;               // program counter
  struct flag flags;    // proc status / flag
};

/*
  NES' page size is 256 bytes. Total of 256 pages available. (0xFFFF bytes)

  SADDR           SIZE    DESCRIPTION
  0x0000         0x00FF   Zero page
  0x0100         0x00FF   Stack
  0x0200         0x0600   Internal RAM
  0x0800         0x0800   Mirror of 0x0000-0x07FF
  0x1000         0x0800   Mirror of 0x0000-0x07FF
  0x1800         0x0800   Mirror of 0x0000-0x07FF
  0x2000         0x0008   NES PPU registers
  0x2008         0x1FF8   Mirrors of 0x2000 every 8 bytes
  0x4000         0x0020   Input/Output registers

  0x4020         0x1FE0   Expansion ROM
  0x6000         0x2000   Save RAM
  0x8000         0x4000   PRG-ROM

  0xFFFA         0x0002   Address of Non Maskable Interrupt (NMI) handler routine
  0xFFFC         0x0002   Address of Power on reset handler routine
  0xFFFE         0x0002   Address of Break (BRK instruction) handler routine

*/

// the various memory blocks of significance
struct cpu_memory {
  u8 lowmem[0x800]; // 2K internal RAM                 0x0000-0x0800 (mirrored 3 times after)
  u8 ppureg[0x008]; // 8B dedicated PPU register mem   0x2000-0x2008 (mirrored a lot of times)
  u8 apureg[0x018]; // TODO:
};

struct _6502 {
  struct registers r;
  u32 ticks;

  // important memory areas
  struct cpu_memory mem;
};

// in 6502.c
const extern u8 cycles[0x100];

// functions
struct _6502* cpu_6502_create(void);
void          cpu_6502_evaluate(struct _6502* cpu);
void          cpu_6502_free(struct _6502* cpu);
void          cpu_6502_inspect(struct _6502* cpu);

#endif /* _6502_H */
