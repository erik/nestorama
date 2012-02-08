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
  printf("%s:%d\t%-20s\t", __FILE__, __LINE__, __func__);               \
  printf( __VA_ARGS__);                                                 \
  printf("\n");

#endif /* _DEF_H */
