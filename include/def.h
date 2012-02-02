/* global typedefs / defines / ... */

#pragma once

#ifndef _DEF_H
#define _DEF_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// 6502 is little endian
static u16 create_u16(u8 lsb, u8 msb) { return (msb << 8) | lsb ; }

#define LOGF(...)                                                       \
  fprintf(stderr, "%s:%d\t%-20s\t", __FILE__, __LINE__, __func__);   \
  fprintf(stderr, __VA_ARGS__);                                         \
  fprintf(stderr, "\n");

#endif /* _DEF_H */
