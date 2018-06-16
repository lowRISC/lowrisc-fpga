// See LICENSE for license details.

#ifndef MEMORY_HEADER_H
#define MEMORY_HEADER_H

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>

extern volatile uint64_t * get_bram_base();
extern volatile uint64_t * get_ddr_base();
extern volatile uint64_t   get_ddr_size();
extern volatile uint64_t * get_flash_base();

enum { clint_base_addr = 0x02000000,
	plic_base_addr = 0x0c000000,
	bram_base_addr = 0x40000000,
          sd_base_addr = 0x40010000,        
          sd_bram_addr = 0x40018000,
         eth_base_addr = 0x40020000,
        keyb_base_addr = 0x40030000, // These have been relocated
        uart_base_addr = 0x40034000,
         vga_base_addr = 0x40038000,
      };

#endif
