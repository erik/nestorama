#include "2C02.h"

#include "nes.h"

struct _2C02* ppu_2C02_create(struct NES* nes)
{
  struct _2C02* ppu = malloc(sizeof(struct _2C02));
  ppu->nes = nes;

  return ppu;
}

void ppu_2C02_free(struct _2C02* ppu)
{
  free(ppu);
}

// leaving powerup and reset stubs in, but reset / power on state is basically
// setting each PPU register to 0 or an undefined value

void ppu_2C02_powerup(struct _2C02* ppu)
{
  (void)ppu;
}

void ppu_2C02_reset(struct _2C02* ppu)
{
  (void)ppu;
  // TODO
}

void ppu_2C02_tick(struct _2C02* ppu)
{
  (void)ppu;
  // TODO
}

void ppu_2C02_set_register(struct _2C02* ppu, u8 reg, u8 val)
{
  LOGF("set reg 0x%X to 0x%X", reg, val);

  // TODO: shorten this up
  if(reg == 0) // PPUCTRL
    ppu->r.ctrl = *(struct ppu_control_register*)&val;
  if(reg == 1) // PPUMASK
    ppu->r.mask = *(struct ppu_mask_register*)&val;
  if(reg == 2) // PPUSTATUS
    ppu->r.status = *(struct ppu_status_register*)&val;
  if(reg == 3) // OAMADDR
    ppu->r.oam_addr = val;
  if(reg == 4) // OAMDATA
    ppu->r.oam_data = val;
  if(reg == 5) // PPUSCROLL
    ppu->r.ppu_scroll = val;
  if(reg == 6) // PPU_ADDR
    ppu->r.ppu_addr = val;
  if(reg == 7) // PPU_DATA
    ppu->r.ppu_data = val;
}

u8 ppu_2C02_get_register(struct _2C02* ppu, u8 reg)
{
  LOGF("get reg 0x%X", reg);

  if(reg == 0) // PPUCTRL
    return *(u8*)&ppu->r.ctrl;
  if(reg == 1) // PPUMASK
    return *(u8*)&ppu->r.mask;
  if(reg == 2) // PPUSTATUS
    return *(u8*)&ppu->r.status;
  if(reg == 3) // OAMADDR
    return ppu->r.oam_addr;
  if(reg == 4) // OAMDATA
    return ppu->r.oam_data;
  if(reg == 5) // PPUSCROLL
    return ppu->r.ppu_scroll;
  if(reg == 6) // PPU_ADDR
    return ppu->r.ppu_addr;
  if(reg == 7) // PPU_DATA
    return ppu->r.ppu_data;

  return 0;
}

void ppu_2C02_inspect(struct _2C02* ppu)
{

  // TODO: fix this

  printf("2C02 = {\n");

  // Prints high bits first

#define BIT(v, x)  ((v >> x) & 1) ? '1' : '0'
#define U8_TO_BIN(v) (char[]){ BIT(v, 7), BIT(v, 6), BIT(v, 5), BIT(v, 4), \
      BIT(v, 3), BIT(v, 2), BIT(v, 1), BIT(v, 0), 0 }

  printf("  PPUCTRL=0b%s\t",      U8_TO_BIN((*(u8*)&ppu->r.ctrl)));
  printf("  PPUMASK=0b%s\t",      U8_TO_BIN((*(u8*)&ppu->r.mask)));
  printf("  PPUSTATUS=0b%s\t",    U8_TO_BIN((*(u8*)&ppu->r.status)));
  printf("  OAMADDR=0x%X\n",    ppu->r.oam_addr);
  printf("  OAMDATA=0x%X\t\t",    ppu->r.oam_data);
  printf("  PPUSCROLL=0x%X\t\t",  ppu->r.ppu_scroll);
  printf("  PPUADDR=0x%X\t\t",    ppu->r.ppu_addr);
  printf("  PPUDATA=0x%X\n}\n", ppu->r.ppu_data);
}
