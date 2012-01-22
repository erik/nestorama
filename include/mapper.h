/* */

#pragma once

#ifndef _MAPPER_H_
#define _MAPPER_H_

#include "def.h"

struct ROM;

struct mapper {
  u8 num;
  u8* rom_banks[8];
  u8* vrom_banks[8];

  struct ROM* rom;
};

// functions
struct mapper* mapper_create(struct ROM* rom);
void           mapper_free(struct mapper* map);

u8             mapper_fetch(struct mapper* map, u16 addr);
void           mapper_set(struct mapper* map, u16 addr, u8 val);

#endif /* _MAPPER_H_ */
