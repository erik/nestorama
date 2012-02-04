#pragma once

#ifndef _6502_H
#define _6502_H

#include "def.h"

/* proc status / flag register layout
   +7 6 5 4 3 2 1 0+ bit number
   +-+-+-+-+-+-+-+-+
   |N|V|1|B|D|I|Z|C|
   +-+-+-+-+-+-+-+-+
*/
struct flag {
  /* C => carry        - set if add/sub produces carry/borrow,
     bit after shift */
  bool c : 1;
  /* Z => zero         - load/inc/dec/add/sub was zero */
  bool z : 1;
  /* I => IRQ disable  - maskable interrupts are disabled */
  bool i : 1;
  /* D => Decimal mode - decimal mode active */
  bool d : 1;
  /* B => BRK command  - interrupt caused by BRK */
  bool b : 1;
  /* 1 => Unused       - always 1 */
  bool u : 1;
  /* V => Overflow     - over/underflow produced  */
  bool v : 1;
  /* N => Negative     - bit 7 of accumulator is set */
  bool n : 1;
};

struct registers {
  u8 a;                 // accumulator
  u8 x;                 // general purpose / index reg
  u8 y;                 // general purpose / index reg
  u8 sp;                // stack pointer
  u16 pc;               // program counter
  struct flag flags;    // proc status / flag
};

struct interrupts {
  bool nmi;
  bool reset;
  bool brk;

  // XXX: these are temporary until NES files are actually loaded into memory
  u16 nmi_addr;   // 0xFFFA
  u16 reset_addr; // 0xFFFC
  u16 brk_addr;   // 0xFFFE

};

struct NES;
struct memory;

struct _6502 {
  struct registers r;
  struct interrupts intr; // interrupt state
  u32 ticks;

  struct NES* nes;   // pointer to parent NES struct
};

// functions
struct _6502* cpu_6502_create(struct NES* nes);
void          cpu_6502_free(struct _6502* cpu);
void          cpu_6502_powerup(struct _6502* cpu);
void          cpu_6502_reset(struct _6502* cpu);
void          cpu_6502_tick(struct _6502* cpu);
void          cpu_6502_inspect(struct _6502* cpu);

void          cpu_6502_push_stack(struct _6502* cpu, u8 value);
u8            cpu_6502_pop_stack(struct _6502* cpu);

static struct flag u8_to_flag(u8 u) { return *(struct flag*)&u; }
static u8 flag_to_u8(struct flag f) { return *(u8*)&f; }

// in 6502.c
extern const u8 cycles[0x100];

#endif /* _6502_H */
