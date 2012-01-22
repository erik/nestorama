#include "mapper.h"

#include <string.h>

struct mapper* mapper_create(struct ROM* rom)
{
  struct mapper* map = malloc(sizeof(struct mapper));
  memset(map, 0, sizeof(struct mapper));

  map->rom = rom;

  return map;
}

void mapper_free(struct mapper* map)
{
  free(map);
}

u8 mapper_fetch(struct mapper* map, u16 addr)
{
  (void)map;
  return (u8)addr;
}

void mapper_set(struct mapper* map, u16 addr, u8 val)
{
  (void)map;
  addr = val;
}
