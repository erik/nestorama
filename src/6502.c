/*
 * Emulation of the 6502 microcontroller used in the NES (part of the 2A07 chip)
 */

#include "6502.h"

struct _6502* cpu_6502_create(void)
{
  struct _6502* cpu = malloc(sizeof(struct _6502));

  cpu_6502_init_pages(cpu);

  return cpu;
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
         "\tticks=%d\n"                                 \
         "\tregisters = {"                              \
         " a=0x%X, x=0x%X, y=0x%x, sp=0x%X, "           \
         "pc=0x%X, flags (CZIDBUVN)=%s }\n"
         "}\n",
         cpu->ticks,
         cpu->r.a, cpu->r.x, cpu->r.y, cpu->r.sp, cpu->r.pc, flags
         );
}

void cpu_6502_init_pages(struct _6502* cpu)
{

#define SETPAGE(addr, size, val) {                          \
    int _pgi = addr >> 8, _s = size < 0x100 ? 0x100 : size; \
    for(int _i = 0; _i < _s / 0x100; _i++) {                \
      pages[_pgi + _i] = val + 0x100 * _i;                  \
    }                                                       \
  }

  const u8 **pages = (const u8**)cpu->pages;
  const struct cpu_memory *mem = &cpu->mem;

  // Internal RAM
  SETPAGE(0x0000, 0x0800, mem->lowmem);

  // Mirrors of RAM
  SETPAGE(0x0800, 0x0800, mem->lowmem);
  SETPAGE(0x1000, 0x0800, mem->lowmem);
  SETPAGE(0x1800, 0x0800, mem->lowmem);

  // PPU registers
  SETPAGE(0x2000, 0x0008, mem->ppureg);

  // Mirrors of PPU registers
  for(int base_addr = 0x2008; base_addr < 0x4000; base_addr += 0x0008) {
    // TODO: This segment may prove to be problematic
    SETPAGE(base_addr, 0x0100, mem->ppureg);
  }

  // TODO: finish this
  // special locations (0xFFFA-0xFFE, etc.) should be treated specially, and not
  // directly mapped.

#undef SETPAGE
}

static u8 fetch_memory(struct _6502* cpu, u16 addr)
{
  int page_index  = addr >> 8;
  int page_offset = addr & 0xFF;

  return cpu->pages[page_index][page_offset];
}

static void set_memory(struct _6502* cpu, u16 addr, u8 value)
{
  int page_index  = addr >> 8;
  int page_offset = addr & 0xFF;

  cpu->pages[page_index][page_offset] = value;
}

// 6502 is little endian
static u16 create_u16(u8 msb, u8 lsb) {
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

// value of memory at program counter *pc
#define PCVAL     (MEM(++cpu->r.pc))

// these procedures are common to every opcode
#define IMP /* nothing */
#define IMM val = MEM(++PC);
#define ZP  val = MEM(PCVAL);
#define ZPX val = MEM(PCVAL + cpu->r.x);
#define ZPY val = MEM(PCVAL + cpu->r.y);
#define IZX val = PCVAL + cpu->r.x;
#define IZY /* TODO */
#define ABS val = MEM(create_u16(PCVAL, PCVAL));
#define ABX /* TODO */
#define ABY /* TODO */

// these procedures are for ops that manipulate 16 bit values (addresses)
#define ZP16  val16 = PCVAL;
#define ZPX16 val16 = PCVAL + cpu->r.x;
#define ZPY16 val16 = PCVAL + cpu->r.y
#define IZX16 /* TODO */
#define IZY16 /* TODO */
#define ABS16 val16 = create_u16(PCVAL, PCVAL);
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
  CMP:
    printf("CMP NOT IMPLEMENTED\n");
    break;

    // CPX
    OP(0xE0, CPX, IMM); // CPX imm
    OP(0xE4, CPX, ZP);  // CPX zp
    OP(0xEC, CPX, ABS); // CPX abs
  CPX:
    printf("CPX NOT IMPLEMENTED\n");
    break;

    // CPY
    OP(0xC0, CPY, IMM); // CPY imm
    OP(0xC4, CPY, ZP);  // CPY zp
    OP(0xCC, CPY, ABS); // CPY abs
  CPY:
    printf("CPY NOT IMPLEMENTED\n");
    break;

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
  LDX:
    printf("LDX NOT IMPLEMENTED\n");
    break;

    // STX
    OP(0x86, STX, ZP16);  // STX zp
    OP(0x96, STX, ZPY16); // STX zpy
    OP(0x8E, STX, ABS16); // STX abs
  STX:
    printf("STX NOT IMPLEMENTED\n");
    break;

    // LDY
    OP(0xA0, LDY, IMM); // LDY imm
    OP(0xA4, LDY, ZP);  // LDY zp
    OP(0xB4, LDY, ZPY); // LDY zpy
    OP(0xAC, LDY, ABS); // LDY abs
    OP(0xBC, LDY, ABX); // LDY abx
  LDY:
    printf("LDY NOT IMPLEMENTED\n");
    break;

    // STY
    OP(0x84, STY, ZP16);  // STY zp
    OP(0x94, STY, ZPX16); // STY zpx
    OP(0x8C, STY, ABS16); // STY abs
  STY:
    printf("STY NOT IMPLEMENTED\n");
    break;

    IMP_OP(0xAA, X = A); // TAX imp
    IMP_OP(0x8A, A = X); // TXA imp
    IMP_OP(0xA8, Y = A); // TAY imp
    IMP_OP(0x98, A = Y); // TYA imp
    IMP_OP(0xBA, X = SP); // TSX imp
    IMP_OP(0x9A, SP = X); // TXS imp

    IMP_OP(0x68, /* TODO */); // PLA imp
    IMP_OP(0x48, /* TODO */); // PHA imp
    IMP_OP(0x08, /* TODO */); // PHP imp

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

    IMP_OP(0x00, /* TODO */); // BRK imp
    IMP_OP(0x40, /* TODO */); // RTI imp

    OP(0x20, JSR, ABS);       // JSR abs
  JSR: /* TODO */ break;

    IMP_OP(0x60, /* TODO */); // RTS imp

    OP(0x4C, JMP, ABS16);               // JMP abs
    OP(0x6C, JMP, val16 = MEM(PCVAL));  // JMP ind
  JMP:
    // val - 1 because there is a ++PC at the end of the switch
    printf("JMP: PC = 0x%X\n", PC = val16 - 1);
    break;

    OP(0x24, BIT, ZP);        // BIT zp
    OP(0x2C, BIT, ABS);       // BIT abs
  BIT: /* TODO */ break;

    // set flags
    IMP_OP(0x18, FLAGS.c = 0); // CLC imp
    IMP_OP(0x38, FLAGS.c = 1); // SEC imp
    IMP_OP(0xD8, FLAGS.d = 0); // CLD imp
    IMP_OP(0xF8, FLAGS.d = 1); // SED imp
    IMP_OP(0x58, FLAGS.i = 0); // CLI imp
    IMP_OP(0x78, FLAGS.i = 1); // SEI imp
    IMP_OP(0xB8, FLAGS.v = 0); // CLV imp

    IMP_OP(0xEA, /* NOP */);   // NOP imp

    ///// Invalid operations

    // TODO: adapt some of these as debug ops
  case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
  case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
    goto invalid;

  invalid:
    printf("OPCODE %X IS INVALID\n", op);
    break;

  default:
    printf("WARNINGWARNINGWARNING: OPCODE %X IS NOT IMPLEMENTED\n", op);
    break;
  } // switch (op)

  FLAGS.n = FLAGS.c = A;
  PC += 1;
  cpu->ticks += cycles[op];
}

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
