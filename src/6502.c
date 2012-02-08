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


/*
 * Emulation of the 6502 microcontroller used in the NES (part of the 2A03 chip)
 */

#include <string.h>

#include "6502.h"
#include "nes.h"
#include "rom.h"

struct _6502* cpu_6502_create(struct NES* nes)
{
  struct _6502* cpu = malloc(sizeof(struct _6502));
  memset(cpu, 0, sizeof(struct _6502));
  cpu->nes = nes;

  return cpu;
}

void cpu_6502_powerup(struct _6502* cpu)
{
  LOGF("Powering on CPU");

  // power on state
  cpu->r.flags = u8_to_flag(0x34);
  cpu->r.a = cpu->r.x = cpu->r.y = 0;
  cpu->r.sp = cpu->r.pc = 0x00;

  memset(cpu->nes->mem->lowmem, 0xFF, 0x800);

  cpu->nes->mem->lowmem[0x08] = 0xF7;
  cpu->nes->mem->lowmem[0x09] = 0xEF;
  cpu->nes->mem->lowmem[0x0A] = 0xDF;
  cpu->nes->mem->lowmem[0x0F] = 0xBF;

  // reset triggered on startup
  cpu_6502_reset(cpu);
}

void cpu_6502_reset(struct _6502* cpu)
{
  LOGF("Putting CPU into reset state");

  // reset state
  u8 f = flag_to_u8(cpu->r.flags) | 0x04;

  cpu->r.flags = u8_to_flag(f);
  cpu->r.sp -= 3;
  cpu->r.flags.i = 1;

  cpu->intr.reset = true;
}

void cpu_6502_free(struct _6502* cpu)
{
  free(cpu);
}

// give a nice output of the current state of the CPU
void cpu_6502_inspect(struct _6502* cpu)
{
  struct flag f = cpu->r.flags;
  char flags[9] = { f.c, f.z, f.i, f.d, f.b, f.u, f.v, f.n, 0 };
  for(int i = 0; i < 8; ++i) {
    flags[i] = flags[i] ? '1' : '0';
  }

  printf("6502 = {\n"                                     \
         "  ticks=%d"                                     \
         "  registers = {\n"                              \
         " a=0x%X, x=0x%X, y=0x%x, sp=0x%X, "             \
         "pc=0x%X, flags (CZIDBUVN)=0b%s }\n"
         "}\n",
         cpu->ticks,
         cpu->r.a, cpu->r.x, cpu->r.y, cpu->r.sp, cpu->r.pc, flags
         );
}

// push a value onto the stack
void cpu_6502_push_stack(struct _6502* cpu, u8 val)
{
  u16 addr = 0xFF + cpu->r.sp--;
  nes_set_memory(cpu->nes, addr, val);
}

// pop a value off of the stack
u8 cpu_6502_pop_stack(struct _6502* cpu)
{
  u16 addr = 0xFF + ++cpu->r.sp;
  return nes_fetch_memory(cpu->nes, addr);
}

enum proc_flags {
  C = 1 << 0,  Z = 1 << 1,  I = 1 << 2,  D = 1 << 3,
  B = 1 << 4,  U = 1 << 5,  V = 1 << 6,  N = 1 << 7
};

static struct flag set_flags(struct flag f, unsigned which, u8 val)
{
  if(which & Z) f.z = val == 0;
  if(which & N) f.n = val & 0x80;
  return f;
}

#define SET_FLAGS(flg, val) FLAGS = set_flags(FLAGS, flg, val)

// memory at addr                     *addr
#define MEM(addr)          (nes_fetch_memory(cpu->nes, addr))
#define SETMEM(addr, val)  (nes_set_memory(cpu->nes, addr, val))

#define X         (cpu->r.x)
#define Y         (cpu->r.x)
#define A         (cpu->r.a)
#define SP        (cpu->r.sp)
#define PC        (cpu->r.pc)
#define FLAGS     (cpu->r.flags)

#define POP       cpu_6502_pop_stack(cpu)
#define PUSH(v)   cpu_6502_push_stack(cpu, v)

// value of memory at program counter *pc
#define PCVAL     (MEM(cpu->r.pc++))

// these procedures are for ops that manipulate 16 bit values (addresses)
// I do some terrifying things here, please forgive me.
#define ZP16  addr = PCVAL
#define ZPX16 addr = PCVAL + cpu->r.x
#define ZPY16 addr = PCVAL + cpu->r.y

#define IZX16 {                                             \
    u8 pcval = PCVAL;                                       \
    addr = create_u16(MEM(pcval + X), MEM(pcval + X + 1));  \
  }

#define IZY16 {                                             \
    u8 pcval = PCVAL;                                       \
    addr = create_u16(MEM(pcval), MEM(pcval + 1)) + Y;      \
  }

#define ABS16 {                                     \
    PC += 2;                                        \
    addr = create_u16(MEM(PC - 2), MEM(PC - 1));    \
  }

#define ABX16 {                                             \
    PC += 2;                                                \
    addr = create_u16(MEM(PC - 2), MEM(PC - 1)) + cpu->r.x; \
  }

#define ABY16 {                                             \
    PC += 2;                                                \
    addr = create_u16(MEM(PC - 2), MEM(PC - 1)) + cpu->r.y; \
  }

// these procedures are common to every opcode
#define IMP /* nothing */
#define IMM val = MEM(PC++)
#define ZP  ZP16;  val = MEM(addr)
#define ZPX ZPX16; val = MEM(addr)
#define ZPY ZPX16; val = MEM(addr)
#define IZX IZX16; val = MEM(addr)
#define IZY IZY16; val = MEM(addr)
#define ABS ABS16; val = MEM(addr)
#define ABX ABX16; val = MEM(addr)
#define ABY ABY16; val = MEM(addr)

// save some keystrokes
#define OP(num, fam, type) case num: {              \
    printf("0x%02X\t%s %5s\t", num, #fam, #type);   \
    type;                                           \
    goto fam;                                       \
    break;                                          \
  }
// implicit op
#define IMP_OP(num, fam, code) case num: {      \
    printf("0x%02X\t%s   IMP\t", num, #fam);    \
    code;                                       \
    break;                                      \
  }

// relative op
#define REL_OP(num, fam, code) case num: {      \
    printf("0x%02X\t%s   REL\t", num, #fam);    \
    code;                                       \
    break;                                      \
  }

#define BRANCH_IF(cond) u8 jmp = PCVAL; if(cond) { PC += jmp; printf("branching to 0x%X", PC); }

void cpu_6502_tick(struct _6502 *cpu)
{

  if(cpu->intr.reset) {
    cpu->r.pc = create_u16(MEM(0xFFFC), MEM(0xFFFD));

    if(cpu->nes->rom->map->num == NROM) {
      cpu->r.pc = 0xC000 ;
    }

    cpu->intr.reset = false;

    LOGF("Jumping to reset address of: 0x%X", cpu->r.pc);
    return;
  }

  u8 op = PCVAL;
  printf("0x%X ", PC - 1);

  u8  val  = 0; // temporary value for instructions to use
  u16 addr = 0; // temporary 16 bit value (for addresses)

  switch (op) {
    ///// Logical / Arithmetic operations

    // ORA
    OP(0x09, ORA, IMM); // ORA imm
    OP(0x05, ORA, ZP);  // ORA zp
    OP(0x15, ORA, ZPX); // ORA zpx
    OP(0x01, ORA, IZX); // ORA izx
    OP(0x11, ORA, IZY); // ORA izy
    OP(0x0D, ORA, ABS); // ORA abs
    OP(0x1D, ORA, ABX); // ORA abx
    OP(0x19, ORA, ABY); // ORA aby
  ORA:
    printf("0x%X | 0x%X => 0x%X", A, val, A | val);
    A |= val;

    SET_FLAGS(N|Z, A);
    break;

    // AND
    OP(0x29, AND, IMM); // AND imm
    OP(0x25, AND, ZP);  // AND zp
    OP(0x35, AND, ZPX); // AND zpx
    OP(0x21, AND, IZX); // AND izx
    OP(0x31, AND, IZY); // AND izy
    OP(0x2D, AND, ABS); // AND abs
    OP(0x3D, AND, ABX); // AND abx
    OP(0x39, AND, ABY); // AND aby
  AND:
    printf("0x%X & 0x%X => 0x%X", A, val, A & val);
    A &= val;

    SET_FLAGS(N|Z, A);
    break;

    // EOR
    OP(0x49, EOR, IMM); // EOR imm
    OP(0x45, EOR, ZP);  // EOR zp
    OP(0x55, EOR, ZPX); // EOR zpx
    OP(0x41, EOR, IZX); // EOR izx
    OP(0x51, EOR, IZY); // EOR izy
    OP(0x4D, EOR, ABS); // EOR abs
    OP(0x5D, EOR, ABX); // EOR abx
    OP(0x59, EOR, ABY); // EOR aby
  EOR:
    printf("0x%X ^ 0x%X => 0x%X", A, val, A ^ val);
    A ^= val;

    SET_FLAGS(N|Z, A);
    break;

    // ADC
    OP(0x69, ADC, IMM); // ADC imm
    OP(0x65, ADC, ZP);  // ADC zp
    OP(0x75, ADC, ZPX); // ADC zpx
    OP(0x61, ADC, IZX); // ADC izx
    OP(0x71, ADC, IZY); // ADC izy
    OP(0x6D, ADC, ABS); // ADC abs
    OP(0x7D, ADC, ABX); // ADC abx
    OP(0x79, ADC, ABY); // ADC aby
  ADC: {
      u16 v16 = val + A + (FLAGS.c ? 1 : 0);

      FLAGS.c = v16 > 0xFF;
      FLAGS.v = !((A ^ val) & 0x80) && ((A ^ v16) & 0x80);

      printf("0x%X + 0x%X => 0x%X(trunc:0x%X)", A, val, v16, (u8)v16);
      val = v16 & 0xFF;
      A = val;

      SET_FLAGS(N|Z, A);
      break;
    }

    // SBC
    OP(0xE9, SBC, IMM); // SBC imm
    OP(0xE5, SBC, ZP);  // SBC zp
    OP(0xF5, SBC, ZPX); // SBC zpx
    OP(0xE1, SBC, IZX); // SBC izx
    OP(0xF1, SBC, IZY); // SBC izy
    OP(0xED, SBC, ABS); // SBC abs
    OP(0xFD, SBC, ABX); // SBC abx
    OP(0xF9, SBC, ABY); // SBC aby
  SBC: {
      // XXX: at least IZY is broken for SBC

      unsigned v = A - val - (FLAGS.c ? 0 : 1);
      FLAGS.v = ((A ^ v) & 0x80) && ((A ^ val) & 0x80);
      FLAGS.c = v < 0x100;

      printf("0x%X - 0x%X => 0x%X(truc:0x%X)", A, val, v, v & 0xFF);
      A = v & 0xFF;

      SET_FLAGS(N|Z, A);
      break;
    }

    // CMP
    OP(0xC9, CMP, IMM); // CMP imm
    OP(0xC5, CMP, ZP);  // CMP zp
    OP(0xD5, CMP, ZPX); // CMP zpx
    OP(0xC1, CMP, IZX); // CMP izx
    OP(0xD1, CMP, IZY); // CMP izy
    OP(0xCD, CMP, ABS); // CMP abs
    OP(0xDD, CMP, ABX); // CMP abx
    OP(0xD9, CMP, ABY); // CMP aby
  CMP: {
      printf("0x%X CMP 0x%X => %d", A, val, A - val);

      u16 v = A - val;
      FLAGS.c = v < 0x100;
      SET_FLAGS(N|Z, v & 0xFF);
      break;
    }

    // CPX
    OP(0xE0, CPX, IMM); // CPX imm
    OP(0xE4, CPX, ZP);  // CPX zp
    OP(0xEC, CPX, ABS); // CPX abs
  CPX: {
      printf("0x%X CPX 0x%X => %d", X, val, X - val);

      u16 v = X - val;
      FLAGS.c = v < 0x100;
      FLAGS.n = v;
      FLAGS.z = v & 0xFF;
      break;
    }

    // CPY
    OP(0xC0, CPY, IMM); // CPY imm
    OP(0xC4, CPY, ZP);  // CPY zp
    OP(0xCC, CPY, ABS); // CPY abs
  CPY: {
      printf("0x%X CPY 0x%X => %d", Y, val, Y - val);

      u16 v = Y - val;
      FLAGS.c = v < 0x100;
      FLAGS.n = v;
      FLAGS.z = v & 0xFF;
      break;
    }

    // DEC
    OP(0xC6, DEC, ZP);  //DEC zp
    OP(0xD6, DEC, ZPX); //DEC zpx
    OP(0xCE, DEC, ABS); //DEC abs
    OP(0xDE, DEC, ABX); //DEC abx
  DEC: {
      val -= 1;
      printf("0x%X => 0x%X", addr, val);
      SETMEM(addr, val);
      printf("==> 0x%X", MEM(addr));

      SET_FLAGS(N|Z, val);
      break;
    }

    IMP_OP(0xCA, DEX,
           X -= 1;
           SET_FLAGS(N|Z, X)); // DEX imp
    IMP_OP(0x88, DEY,
           Y -= 1;
           SET_FLAGS(N|Z, Y)); // DEY imp

    // INC
    OP(0xE6, INC, ZP);  //INC zp
    OP(0xF6, INC, ZPX); //INC zpx
    OP(0xEE, INC, ABS); //INC abs
    OP(0xFE, INC, ABX); //INC abx
  INC: {
      val += 1;
      printf("0x%X => 0x%X", addr, val);
      SETMEM(addr, val);

      SET_FLAGS(N|Z, val);
      break;
    }

    IMP_OP(0xE8, INX,
           X += 1;
           SET_FLAGS(N|Z, X)); // INX imp

    IMP_OP(0xC8, INY,
           Y += 1;
           SET_FLAGS(N|Z, Y)); // INY imp

    // ASL
    OP(0x0A, ASL, val = A);    // ASL imp

    OP(0x06, ASL, ZP);       // ASL zp
    OP(0x16, ASL, ZPX);      // ASL zpx
    OP(0x0E, ASL, ABS);      // ASL abs
    OP(0x1E, ASL, ABX);      // ASL abx
  ASL: {
      FLAGS.c = (val & 0x80) ? 1 : 0;

      val <<= 1;
      printf("0x%X => 0x%X", addr, val);
      if(op == 0x0A) // ASL imp
        A = val;
      else
        SETMEM(addr, val);

      SET_FLAGS(N|Z, val);
      break;
    }

    // ROL
    OP(0x2A, ROL, val = A);  // ROL imp

    OP(0x26, ROL, ZP);      // ROL zp
    OP(0x36, ROL, ZPX);     // ROL zpx
    OP(0x2E, ROL, ABS);     // ROL abs
    OP(0x3E, ROL, ABX);     // ROL abx
  ROL: {
      u16 v16 = (u16)val;
      FLAGS.c = (val & 0x80) ? 1 : 0;

      v16 <<= 1;
      if(FLAGS.c) val |= 0x01;

      FLAGS.c = v16 > 0xFF;

      val = v16 & 0xFF;

      printf("0x%X => 0x%X", addr, val);

      if(op == 0x2A)  // ROL imp
        A = val;
      else
        SETMEM(addr, val);

      SET_FLAGS(N|Z, val);
      break;
    }

    // LSR
    OP(0x4A, LSR, val = A);  // LSR imp

    OP(0x46, LSR, ZP);      // LSR zp
    OP(0x56, LSR, ZPX);     // LSR zpx
    OP(0x4E, LSR, ABS);     // LSR abs
    OP(0x5E, LSR, ABX);     // LSR abx
  LSR: {
      FLAGS.c = val & 0x01;
      val >>= 1;

      printf("0x%X => 0x%X", addr, val);
      if(op == 0x4A) // LSR imp
        A = val;
      else
        SETMEM(addr, val);

      SET_FLAGS(N|Z, val);
      break;
    }

    // ROR
    OP(0x6A, ROR, val = A);  // ROR imp

    OP(0x66, ROR, ZP);       // ROR zp
    OP(0x76, ROR, ZPX);      // ROR zpx
    OP(0x6E, ROR, ABS);      // ROR abs
    OP(0x7E, ROR, ABX);      // ROR abx
  ROR: {
      u16 v16 = (u16) val;
      if(FLAGS.c) v16 |= 0x100;
      FLAGS.c = v16 & 0x01;
      v16 >>= 1;
      val = v16 & 0xFF;

      printf("0x%X => 0x%X", addr, val);

      if(op == 0x6A) // ROR imp
        A = val;
      else
        SETMEM(addr, val);

      SET_FLAGS(N|Z, val);
      break;
    }

    ///// Movement Operations

    // LDA
    OP(0xA9, LDA, IMM); // LDA imm
    OP(0xA5, LDA, ZP);  // LDA zp
    OP(0xB5, LDA, ZPX); // LDA zpx
    OP(0xA1, LDA, IZX); // LDA izx
    OP(0xB1, LDA, IZY); // LDA izy
    OP(0xAD, LDA, ABS); // LDA abs
    OP(0xBD, LDA, ABX); // LDA abx
    OP(0xB9, LDA, ABY); // LDA aby
  LDA:
    printf("A = 0x%X", val);
    A = val;

    SET_FLAGS(N|Z, A);
    break;

    // STA
    OP(0x85, STA, ZP);  // STA zp
    OP(0x95, STA, ZPX); // STA zpx
    OP(0x81, STA, IZX); // STA izx
    OP(0x91, STA, IZY); // STA izy
    OP(0x8D, STA, ABS); // STA abs
    OP(0x9D, STA, ABX); // STA abx
    OP(0x99, STA, ABY); // STA aby
  STA: {
      printf("address 0x%X -> 0x%X", addr, A);
      SETMEM(addr, A);
      break;
    }

    // LDX
    OP(0xA2, LDX, IMM); // LDX imm
    OP(0xA6, LDX, ZP);  // LDX zp
    OP(0xB6, LDX, ZPY); // LDX zpy
    OP(0xAE, LDX, ABS); // LDX abs
    OP(0xBE, LDX, ABY); // LDX aby
  LDX: {
      printf("X = 0x%X", val);
      X = val;

      SET_FLAGS(N|Z, X);
      break;
    }

    // STX
    OP(0x86, STX, ZP);  // STX zp
    OP(0x96, STX, ZPY); // STX zpy
    OP(0x8E, STX, ABS); // STX abs
  STX: {
      printf("0x%X -> 0x%X", addr, X);
      SETMEM(addr, X);
      break;
    }

    // LDY
    OP(0xA0, LDY, IMM); // LDY imm
    OP(0xA4, LDY, ZP);  // LDY zp
    OP(0xB4, LDY, ZPY); // LDY zpy
    OP(0xAC, LDY, ABS); // LDY abs
    OP(0xBC, LDY, ABX); // LDY abx
  LDY:
    printf("Y = 0x%X", val);
    Y = val;

    SET_FLAGS(N|Z, Y);
    break;


    // STY
    OP(0x84, STY, ZP);  // STY zp
    OP(0x94, STY, ZPX); // STY zpx
    OP(0x8C, STY, ABS); // STY abs
  STY:
    printf("0x%X -> 0x%X", addr, Y);
    SETMEM(addr, Y);
    break;


    IMP_OP(0xAA, TAX,
           X = A;
           SET_FLAGS(N|Z, X));  // TAX imp

    IMP_OP(0x8A, TXA,
           A = X;
           SET_FLAGS(N|Z, A));  // TXA imp

    IMP_OP(0xA8, TAY,
           Y = A;
           SET_FLAGS(N|Z, Y));  // TAY imp

    IMP_OP(0x98, TYA,
           A = Y;
           SET_FLAGS(N|Z, A));  // TYA imp

    IMP_OP(0xBA, TSX, X = SP);       // TSX imp
    IMP_OP(0x9A, TXS, SP = X);       // TXS imp

    IMP_OP(0x68, PLA,
           A = POP;
           SET_FLAGS(N|Z, A));       // PLA imp

    IMP_OP(0x48, PHA, PUSH(A));                   // PHA imp
    IMP_OP(0x08, PHP, PUSH(flag_to_u8(FLAGS)));   // PHP imp
    IMP_OP(0x28, PLP, FLAGS = u8_to_flag(POP));   // PLP imp

    ///// Jump / flag operations

    // branching
    REL_OP(0x10, BPL, BRANCH_IF(!FLAGS.n)); // BPL rel
    REL_OP(0x30, BMI, BRANCH_IF(FLAGS.n));  // BMI rel
    REL_OP(0x50, BVC, BRANCH_IF(!FLAGS.v)); // BVC rel
    REL_OP(0x70, BVS, BRANCH_IF(FLAGS.v));  // BVS rel
    REL_OP(0x90, BCC, BRANCH_IF(!FLAGS.c)); // BCC rel
    REL_OP(0xB0, BCS, BRANCH_IF(FLAGS.c));  // BCS rel
    REL_OP(0xD0, BNE, BRANCH_IF(!FLAGS.z)); // BNE rel
    REL_OP(0xF0, BEQ, BRANCH_IF(FLAGS.z));  // BEQ rel

    IMP_OP(0x00, BRK,                       // BRK imp
           cpu->nes->is_active = false;
           PUSH((--PC >> 8) & 0xFF);
           PUSH(PC & 0xFF);
           FLAGS.c = 1;
           PUSH(flag_to_u8(FLAGS));
           FLAGS.i = 1;
           PC = (MEM(0xFFFE) | (MEM(0xFFFF) << 8)));

    IMP_OP(0x40, RTI,                      // RTI imp
           FLAGS = u8_to_flag(POP);
           PC = POP; PC |= (POP << 8));

    OP(0x20, JSR, ABS16);                  // JSR abs
  JSR: {
      PC -= 1;
      PUSH((PC >> 8) & 0xFF);
      PUSH(PC & 0xFF);

      printf("jumping to 0x%X PC=>0x%X (0x%X 0x%X)", addr, PC, (PC >> 8) & 0xFF, (PC & 0xFF));

      PC = addr;
      break;
    }

    IMP_OP(0x60, RTS,                      // RTS imp
           PC = POP;
           PC += (POP << 8) + 1;
           printf("returning to addr: 0x%X", PC));

    OP(0x4C, JMP, ABS);               // JMP abs
    OP(0x6C, JMP, IMP);               // JMP ind
  JMP:
    if(op == 0x6c) { // ind
      addr = PCVAL;
      addr |= PCVAL << 8;

      // XXX: this is an ugly hack
      if((cpu->nes->rom->map->num == NROM || cpu->nes->rom->map->num == CNROM)
         && addr == 0x2FF) {
        PC = 0x300;
      } else {
        u8 b1 = MEM(addr), b2 = MEM(addr + 1);
        PC = b2 << 8;
        PC |= b1;
      }

    } else {
      PC = addr;
    }

    printf("PC = 0x%X", PC);
    break;

    OP(0x24, BIT, ZP);        // BIT zp
    OP(0x2C, BIT, ABS);       // BIT abs
  BIT: {
      FLAGS.n = val;
      FLAGS.v = 0x40 & val;
      FLAGS.z = val & A;
      break;
    }

    // set flags
    IMP_OP(0x18, CLC, FLAGS.c = 0); // CLC imp
    IMP_OP(0x38, SEC, FLAGS.c = 1); // SEC imp
    IMP_OP(0xD8, CLD, FLAGS.d = 0); // CLD imp
    IMP_OP(0xF8, SED, FLAGS.d = 1); // SED imp
    IMP_OP(0x58, CLI, FLAGS.i = 0); // CLI imp
    IMP_OP(0x78, SEI, FLAGS.i = 1); // SEI imp
    IMP_OP(0xB8, CLV, FLAGS.v = 0); // CLV imp

    ///// NOP

    IMP_OP(0xEA, NOP, /**/);   // NOP imp
    IMP_OP(0x1A, NOP, /**/);   // NOP imp
    IMP_OP(0x3A, NOP, /**/);   // NOP imp
    IMP_OP(0x5A, NOP, /**/);   // NOP imp
    IMP_OP(0x7A, NOP, /**/);   // NOP imp
    IMP_OP(0xDA, NOP, /**/);   // NOP imp
    IMP_OP(0xFA, NOP, /**/);   // NOP imp

    IMP_OP(0x80, NOP, IMM);         // NOP imm
    IMP_OP(0x82, NOP, IMM);         // NOP imm
    IMP_OP(0x89, NOP, IMM);         // NOP imm
    IMP_OP(0xC2, NOP, IMM);         // NOP imm
    IMP_OP(0xE2, NOP, IMM);         // NOP imm

    IMP_OP(0x04, NOP, ZP);          // NOP zp
    IMP_OP(0x44, NOP, ZP);          // NOP zp
    IMP_OP(0x64, NOP, ZP);          // NOP zp

    IMP_OP(0x14, NOP, ZPX);         // NOP zpx
    IMP_OP(0x34, NOP, ZPX);         // NOP zpx
    IMP_OP(0x54, NOP, ZPX);         // NOP zpx
    IMP_OP(0x74, NOP, ZPX);         // NOP zpx
    IMP_OP(0xD4, NOP, ZPX);         // NOP zpx
    IMP_OP(0xF4, NOP, ZPX);         // NOP zpx

    IMP_OP(0x0C, NOP, ABS);         // NOP abs

    IMP_OP(0x1C, NOP, ABX);         // NOP abx
    IMP_OP(0x3C, NOP, ABX);         // NOP abx
    IMP_OP(0x5C, NOP, ABX);         // NOP abx
    IMP_OP(0x7C, NOP, ABX);         // NOP abx
    IMP_OP(0xDC, NOP, ABX);         // NOP abx
    IMP_OP(0xFC, NOP, ABX);         // NOP abx

    ///// KIL
    OP(0x02, KIL, IMP);  // KIL imp
    OP(0x12, KIL, IMP);  // KIL imp
    OP(0x22, KIL, IMP);  // KIL imp
    OP(0x32, KIL, IMP);  // KIL imp
    OP(0x42, KIL, IMP);  // KIL imp
    OP(0x52, KIL, IMP);  // KIL imp
    OP(0x62, KIL, IMP);  // KIL imp
    OP(0x72, KIL, IMP);  // KIL imp
    OP(0x92, KIL, IMP);  // KIL imp
    OP(0xB2, KIL, IMP);  // KIL imp
    OP(0xD2, KIL, IMP);  // KIL imp
    OP(0xF2, KIL, IMP);  // KIL imp
  KIL:
    LOGF("Killing processor.");
    cpu->nes->is_active = false;
    break;

  default:
    LOGF("WARNING: Opcode 0x%X isn't implemented, halting", op);
    cpu->nes->is_active = false;
    break;
  } // switch (op)

  puts("");

  cpu->ticks += cycles[op];
}


// TODO: ditch cycles table and do it by hand (every mem r/w is a cycle, etc.)
const u8 cycles[0x100] = {
  //0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
  7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, // 0
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 1
  6, 6, 0, 8, 3, 3, 5, 5, 4, 3, 3, 3, 4, 4, 6, 6, // 2
  2, 5, 0, 8, 4, 4, 6, 6, 3, 4, 3, 7, 4, 4, 7, 7, // 3
  6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, // 4
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 5
  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, // 6
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 7
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, // 8
  2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, // 9
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, // A
  2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, // B
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // C
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // D
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // E
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7  // F
};
