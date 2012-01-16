/*
 * Emulation of the 6502 microcontroller used in the NES (part of the 2A07 chip)
 */

#include "6502.h"

#include <string.h>

struct _6502* cpu_6502_create(void)
{
  struct _6502* cpu = malloc(sizeof(struct _6502));

  // power on state

  cpu->r.flags = u8_to_flag(0x34);
  cpu->r.a = cpu->r.x = cpu->r.y = 0;
  cpu->r.sp = 0xFD;

  memset(cpu->mem.lowmem, 0xFF, 0x800);

  cpu->mem.lowmem[0x08] = 0xF7;
  cpu->mem.lowmem[0x09] = 0xEF;
  cpu->mem.lowmem[0x0A] = 0xDF;
  cpu->mem.lowmem[0x0F] = 0xBF;

  // TODO: initiate high memory (PPU regs, etc.) to proper power on state

  return cpu;
}

void cpu_6502_reset(struct _6502* cpu)
{
  cpu->r.sp -= 3;
  cpu->r.flags.i = 1;

  // TODO: put APU into reset state
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

  printf("6502 = {\n"                                   \
         "  ticks=%d\n"                                 \
         "  registers = {"                              \
         " a=0x%X, x=0x%X, y=0x%x, sp=0x%X, "           \
         "pc=0x%X, flags (CZIDBUVN)=%s }\n"
         "}\n",
         cpu->ticks,
         cpu->r.a, cpu->r.x, cpu->r.y, cpu->r.sp, cpu->r.pc, flags
         );
}

static u8 fetch_memory(struct _6502* cpu, u16 addr)
{
  // Low memory
  if(addr < 0x2000) {
    return cpu->mem.lowmem[addr & 0x7FF];
  }

  // PPU registers
  if(addr < 0x4000) {
    return cpu->mem.ppureg[addr & 0x7];
  }

  // APU registers
  if(addr < 0x4018) {
    return cpu->mem.apureg[addr & 0x7];
  }

  // TODO: SRAM, PRG-RAM, etc.

  else {
    printf("Warning: accessing 0x%X, which is unknown\n", addr);
    return ~0;
  }
}

static void set_memory(struct _6502* cpu, u16 addr, u8 value)
{

  // Low memory
  if(addr < 0x2000) {
    cpu->mem.lowmem[addr & 0x7FF] = value;
  }

  // PPU registers
  else if(addr < 0x4000) {
    cpu->mem.ppureg[addr & 0x7] = value;
  }

  // APU registers
  else if(addr < 0x4018) {
    cpu->mem.apureg[addr & 0x17] = value;
  }

  // TODO: SRAM, PRG-RAM, etc.

  else {
    printf("Warning: writing 0x%X, which is unknown\n", addr);
  }
}

// push a value onto the stack
void cpu_6502_push_stack(struct _6502* cpu, u8 val)
{
  u16 addr = 0x100 + cpu->r.sp--;
  set_memory(cpu, addr, val);
}

// pop a value off of the stack
u8 cpu_6502_pop_stack(struct _6502* cpu)
{
  u16 addr = 0x100 + cpu->r.sp++;
  return fetch_memory(cpu, addr);
}

// 6502 is little endian
static u16 create_u16(u8 msb, u8 lsb)
{
  return msb | lsb << 8;
}

// memory at addr                     *addr
#define MEM(addr) (fetch_memory(cpu, addr))
#define X         (cpu->r.x)
#define Y         (cpu->r.x)
#define A         (cpu->r.a)
#define SP        (cpu->r.sp)
#define PC        (cpu->r.pc)
#define FLAGS     (cpu->r.flags)

#define POP       cpu_6502_pop_stack(cpu)
#define PUSH(v)   cpu_6502_push_stack(cpu, v)

// value of memory at program counter *pc
#define PCVAL     (MEM(++cpu->r.pc))

// these procedures are common to every opcode
#define IMP /* nothing */
#define IMM val = PCVAL;
#define ZP  val = MEM(PCVAL);
#define ZPX val = MEM(PCVAL + X);
#define ZPY val = MEM(PCVAL + Y);
#define IZX val = MEM(MEM(PCVAL + X));
#define IZY val = MEM(MEM(PCVAL)) + Y;
#define ABS val = MEM(create_u16(MEM(PC + 1), MEM(PC + 2))); PC += 2;
#define ABX /* TODO */
#define ABY /* TODO */

// these procedures are for ops that manipulate 16 bit values (addresses)
#define ZP16  val16 = PCVAL;
#define ZPX16 val16 = PCVAL + cpu->r.x;
#define ZPY16 val16 = PCVAL + cpu->r.y
#define IZX16 /* TODO */
#define IZY16 /* TODO */
#define ABS16 val16 = create_u16(MEM(PC + 1), MEM(PC + 2)); PC += 2;
#define ABX16 /* TODO */
#define ABY16 /* TODO */

// save some keystrokes
#define OP(num, fam, type) case num: { type; goto fam; break; }
// implicit op
#define IMP_OP(num, code) case num: { code; break; }
#define REL_OP(num, code) case num: { code; break; }

#define BRANCH_IF(cond) u8 jmp = ++PC; if(cond) { PC += (jmp > 0x7F ? jmp - 0xFF : jmp); }

void cpu_6502_evaluate(struct _6502 *cpu)
{
  u8 op = fetch_memory(cpu, PC);

  // temporary values for instructions to use
  u8  val   = 0;
  u16 val16 = 0;

  printf("OP => %X, PC => %X\n", op, PC);

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
    printf("ORA: 0x%X | 0x%X => 0x%X\n", A, val, A | val);
    A |= val;
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
    printf("AND: 0x%X & 0x%X => 0x%X\n", A, val, A & val);
    A &= val;
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
    printf("EOR: 0x%X ^ 0x%X => 0x%X\n", A, val, A ^ val);
    A ^= val;
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
  ADC:
    printf("EOR: 0x%X ^ 0x%X => 0x%X\n", A, val, A + val);
    A += val;
    break;

    // SBC
    OP(0xE9, SBC, IMM); // SBC imm
    OP(0xE5, SBC, ZP);  // SBC zp
    OP(0xF5, SBC, ZPX); // SBC zpx
    OP(0xE1, SBC, IZX); // SBC izx
    OP(0xF1, SBC, IZY); // SBC izy
    OP(0xED, SBC, ABS); // SBC abs
    OP(0xFD, SBC, ABX); // SBC abx
    OP(0xF9, SBC, ABY); // SBC aby
  SBC:
    printf("EOR: 0x%X ^ 0x%X => 0x%X\n", A, val, A - val);
    A -= val;
    break;

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
      printf("CMP: 0x%X CMP 0x%X => %d\n", A, val, A - val);

      u8 v = A - val;
      FLAGS.n = v & ~0;
      FLAGS.z = v;
      FLAGS.c = A >= v;
      break;
    }

    // CPX
    OP(0xE0, CPX, IMM); // CPX imm
    OP(0xE4, CPX, ZP);  // CPX zp
    OP(0xEC, CPX, ABS); // CPX abs
  CPX: {
      printf("CPX: 0x%X CPX 0x%X => %d\n", X, val, X - val);

      u8 v = X - val;
      FLAGS.n = v & ~0;
      FLAGS.z = v;
      FLAGS.c = X >= v;
      break;
    }

    // CPY
    OP(0xC0, CPY, IMM); // CPY imm
    OP(0xC4, CPY, ZP);  // CPY zp
    OP(0xCC, CPY, ABS); // CPY abs
  CPY: {
      printf("CPY: 0x%X CPX 0x%X => %d\n", Y, val, Y - val);

      u8 v = Y - val;
      FLAGS.n = v & ~0;
      FLAGS.z = v;
      FLAGS.c = Y >= v;
      break;
    }

    // DEC
    OP(0xC6, DEC, ZP16);  //DEC zp
    OP(0xD6, DEC, ZPX16); //DEC zpx
    OP(0xCE, DEC, ABS16); //DEC abs
    OP(0xDE, DEC, ABX16); //DEC abx
  DEC: {
      val = fetch_memory(cpu, val16) - 1;
      printf("DEC: 0x%X => 0x%X\n", val16, val);
      set_memory(cpu, val16, val);
      printf("==> 0x%X\n", fetch_memory(cpu, val16));
      break;
    }

    IMP_OP(0xCA, X -= 1); // DEX imp
    IMP_OP(0x88, Y -= 1); // DEY imp

    // INC
    OP(0xE6, INC, ZP16);  //INC zp
    OP(0xF6, INC, ZPX16); //INC zpx
    OP(0xEE, INC, ABS16); //INC abs
    OP(0xFE, INC, ABX16); //INC abx
  INC: {
      val = fetch_memory(cpu, val16) + 1;
      printf("INC: 0x%X => 0x%X\n", val16, val);
      set_memory(cpu, val16, val);
      printf("==> 0x%X\n", fetch_memory(cpu, val16));
      break;
    }

    IMP_OP(0xE8, X += 1); // INX imp
    IMP_OP(0xC8, Y += 1); // INY imp

    // ASL
    OP(0x0A, ASL, val16 = A); // ASL imp
    OP(0x06, ASL, ZP16);      // ASL zp
    OP(0x16, ASL, ZPX16);     // ASL zpx
    OP(0x0E, ASL, ABS16);     // ASL abs
    OP(0x1E, ASL, ABX16);     // ASL abx
  ASL: {
      val = fetch_memory(cpu, val16) * 2;
      printf("ASL: 0x%X => 0x%X\n", val16, val);
      set_memory(cpu, val16, val);
      break;
    }

    // ROL
    OP(0x2A, ROL, val16 = A); // ROL imp
    OP(0x26, ROL, ZP16);      // ROL zp
    OP(0x36, ROL, ZPX16);     // ROL zpx
    OP(0x2E, ROL, ABS16);     // ROL abs
    OP(0x3E, ROL, ABX16);     // ROL abx
  ROL: {
      val = MEM(val16) << 1;
      printf("ROL: 0x%X => 0x%X\n", val16, val);
      set_memory(cpu, val16, val);
      break;
    }

    // LSR
    OP(0x4A, LSR, val16 = A); // LSR imp
    OP(0x46, LSR, ZP16);      // LSR zp
    OP(0x56, LSR, ZPX16);     // LSR zpx
    OP(0x4E, LSR, ABS16);     // LSR abs
    OP(0x5E, LSR, ABX16);     // LSR abx
  LSR: {
      val = fetch_memory(cpu, val16) / 2;
      printf("LSR: 0x%X => 0x%X\n", val16, val);
      set_memory(cpu, val16, val);
      break;
    }

    // ROR
    OP(0x6A, ROR, val16 = A); // ROR imp
    OP(0x66, ROR, ZP16);      // ROR zp
    OP(0x76, ROR, ZPX16);     // ROR zpx
    OP(0x6E, ROR, ABS16);     // ROR abs
    OP(0x7E, ROR, ABX16);     // ROR abx
  ROR: {
      val = MEM(val16) >> 1;
      printf("ROR: 0x%X => 0x%X\n", val16, val);
      set_memory(cpu, val16, val);
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
    printf("LDA: A = 0x%X\n", val);
    A = val;
    break;

    // STA
    OP(0x85, STA, ZP16);  // STA zp
    OP(0x95, STA, ZPX16); // STA zpx
    OP(0x81, STA, IZX16); // STA izx
    OP(0x91, STA, IZY16); // STA izy
    OP(0x8D, STA, ABS16); // STA abs
    OP(0x9D, STA, ABX16); // STA abx
    OP(0x99, STA, ABY16); // STA aby
  STA: {
      printf("STA: address 0x%X -> 0x%X\n", val16, A);
      set_memory(cpu, val16, A);
      printf("STA: ==> 0x%X\n", MEM(val16));
      break;
    }

    // LDX
    OP(0xA2, LDX, IMM); // LDX imm
    OP(0xA6, LDX, ZP);  // LDX zp
    OP(0xB6, LDX, ZPY); // LDX zpy
    OP(0xAE, LDX, ABS); // LDX abs
    OP(0xBE, LDX, ABY); // LDX aby
  LDX: {
      printf("LDX: X = 0x%X\n", val);
      X = val;
      break;
    }

    // STX
    OP(0x86, STX, ZP16);  // STX zp
    OP(0x96, STX, ZPY16); // STX zpy
    OP(0x8E, STX, ABS16); // STX abs
  STX: {
      printf("STX: 0x%X -> 0x%X\n", val16, X);
      set_memory(cpu, val16, X);
      break;
    }

    // LDY
    OP(0xA0, LDY, IMM); // LDY imm
    OP(0xA4, LDY, ZP);  // LDY zp
    OP(0xB4, LDY, ZPY); // LDY zpy
    OP(0xAC, LDY, ABS); // LDY abs
    OP(0xBC, LDY, ABX); // LDY abx
  LDY:
    printf("LDX: Y = 0x%X\n", val);
    Y = val;
    break;


    // STY
    OP(0x84, STY, ZP16);  // STY zp
    OP(0x94, STY, ZPX16); // STY zpx
    OP(0x8C, STY, ABS16); // STY abs
  STY:
    printf("STY: 0x%X -> 0x%X\n", val16, Y);
    set_memory(cpu, val16, Y);
    break;


    IMP_OP(0xAA, X = A);  // TAX imp
    IMP_OP(0x8A, A = X);  // TXA imp
    IMP_OP(0xA8, Y = A);  // TAY imp
    IMP_OP(0x98, A = Y);  // TYA imp
    IMP_OP(0xBA, X = SP); // TSX imp
    IMP_OP(0x9A, SP = X); // TXS imp

    IMP_OP(0x68, A = POP);                 // PLA imp
    IMP_OP(0x48, PUSH(A));                 // PHA imp
    IMP_OP(0x08, PUSH(flag_to_u8(FLAGS))); // PHP imp

    ///// Jump / flag operations

    // branching
    REL_OP(0x10, BRANCH_IF(!FLAGS.n)); // BPL rel
    REL_OP(0x30, BRANCH_IF(FLAGS.n));  // BMI rel
    REL_OP(0x50, BRANCH_IF(!FLAGS.v)); // BVC rel
    REL_OP(0x70, BRANCH_IF(FLAGS.v));  // BVS rel
    REL_OP(0x90, BRANCH_IF(!FLAGS.c)); // BCC rel
    REL_OP(0xB0, BRANCH_IF(FLAGS.c));  // BCS rel
    REL_OP(0xD0, BRANCH_IF(!FLAGS.z)); // BNE rel
    REL_OP(0xF0, BRANCH_IF(FLAGS.z));  // BEQ rel

    IMP_OP(0x00,                       // BRK imp
           PUSH(PC);
           PUSH(flag_to_u8(FLAGS));
           PC = 0xFFFE - 1);

    IMP_OP(0x40,                       // RTI imp
           FLAGS = u8_to_flag(POP);
           PC = POP -1);

    OP(0x20, JSR, ABS16);              // JSR abs
  JSR: {
      PUSH(PC);
      PC = val16 - 1;
      break;
    }

    IMP_OP(0x60, PC = POP - 1);        // RTS imp

    OP(0x4C, JMP, ABS16);              // JMP abs
    OP(0x6C, JMP, val16 = MEM(PCVAL)); // JMP ind
  JMP:
    printf("JMP: PC = 0x%X\n", PC = val16 - 1);
    break;

    OP(0x24, BIT, ZP);        // BIT zp
    OP(0x2C, BIT, ABS);       // BIT abs
  BIT: {
      // XXX: This is probably incorrect
      FLAGS.n = val & (1 << 7);
      FLAGS.v = val & (1 << 6);
      FLAGS.z = val & A;
      break;
    }

    // set flags
    IMP_OP(0x18, FLAGS.c = 0); // CLC imp
    IMP_OP(0x38, FLAGS.c = 1); // SEC imp
    IMP_OP(0xD8, FLAGS.d = 0); // CLD imp
    IMP_OP(0xF8, FLAGS.d = 1); // SED imp
    IMP_OP(0x58, FLAGS.i = 0); // CLI imp
    IMP_OP(0x78, FLAGS.i = 1); // SEI imp
    IMP_OP(0xB8, FLAGS.v = 0); // CLV imp

    ///// NOP

    IMP_OP(0xEA, /* NOP */);   // NOP imp
    IMP_OP(0x1A, /* NOP */);   // NOP imp
    IMP_OP(0x3A, /* NOP */);   // NOP imp
    IMP_OP(0x5A, /* NOP */);   // NOP imp
    IMP_OP(0x7A, /* NOP */);   // NOP imp
    IMP_OP(0xDA, /* NOP */);   // NOP imp
    IMP_OP(0xFA, /* NOP */);   // NOP imp

    IMP_OP(0x80, IMM);         // NOP imm
    IMP_OP(0x82, IMM);         // NOP imm
    IMP_OP(0x89, IMM);         // NOP imm
    IMP_OP(0xC2, IMM);         // NOP imm
    IMP_OP(0xE2, IMM);         // NOP imm

    IMP_OP(0x04, ZP);          // NOP zp
    IMP_OP(0x44, ZP);          // NOP zp
    IMP_OP(0x64, ZP);          // NOP zp

    IMP_OP(0x14, ZPX);         // NOP zpx
    IMP_OP(0x34, ZPX);         // NOP zpx
    IMP_OP(0x54, ZPX);         // NOP zpx
    IMP_OP(0x74, ZPX);         // NOP zpx
    IMP_OP(0xD4, ZPX);         // NOP zpx
    IMP_OP(0xF4, ZPX);         // NOP zpx

    IMP_OP(0x0C, ABS);         // NOP abs

    IMP_OP(0x1C, ABX);         // NOP abx
    IMP_OP(0x3C, ABX);         // NOP abx
    IMP_OP(0x5C, ABX);         // NOP abx
    IMP_OP(0x7C, ABX);         // NOP abx
    IMP_OP(0xDC, ABX);         // NOP abx
    IMP_OP(0xFC, ABX);         // NOP abx

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
    printf("KIL: kill proc here\n");
    break;

    ///// Invalid operations

  default:
    printf("WARNINGWARNINGWARNING: OPCODE %X IS NOT IMPLEMENTED\n", op);
    break;
  } // switch (op)

  FLAGS.n = FLAGS.c = A;
  cpu->ticks += cycles[op];

  // TODO: this is annoying, factor out the +1
  PC += 1;
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
