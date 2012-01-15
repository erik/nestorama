#pragma once

#ifndef _2A07_H
#define _2A07_H

#include "6502.h"

struct dummy;

struct _2A07 {
  struct _6502 cpu;
  struct dummy   ppu;
  struct dummy   apu;
};

#endif /* _2A07_H */
