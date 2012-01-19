/* */

#pragma once

#ifndef _MAPPER_H_
#define _MAPPER_H_

#include "def.h"

struct NES;

struct mapper {
  u8 num;

  struct NES* nes;
};

// functions
struct mapper* mapper_create(struct NES* nes);
void           mapper_free(struct mapper* map);

u8             mapper_fetch(struct mapper* map, u16 addr);
void           mapper_set(struct mapper* map, u16 addr, u8 val);

#endif /* _MAPPER_H_ */
