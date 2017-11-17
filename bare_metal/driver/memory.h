// See LICENSE for license details.

#ifndef MEMORY_HEADER_H
#define MEMORY_HEADER_H

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include "consts.hpp"

extern volatile uint64_t * get_bram_base();
extern volatile uint64_t * get_ddr_base();
extern volatile uint64_t   get_ddr_size();
extern volatile uint64_t * get_flash_base();

extern size_t err, ddr, rom, bram, intc, spi, uart;

#endif
