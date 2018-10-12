// See LICENSE for license details.

#ifndef MEMORY_HEADER_H
#define MEMORY_HEADER_H

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

extern volatile uint64_t * get_bram_base();
extern volatile uint64_t * get_ddr_base();
extern volatile uint64_t   get_ddr_size();
extern volatile uint64_t * get_flash_base();
extern void write_led(uint32_t data);

extern volatile uint32_t *const plic;
extern volatile uint64_t *const sd_base;
extern volatile uint64_t *const sd_bram;
extern volatile uint16_t *const hid_vga_ptr;
extern volatile uint64_t *const hid_reg_ptr;
extern volatile uint8_t  *const hid_font_ptr;
extern volatile uint64_t *const uart_base;
extern volatile uint64_t *const eth_base;
extern volatile uint32_t *const keyb_base;

enum { clint_base_addr = 0x02000000,
	plic_base_addr = 0x0c000000,
	bram_base_addr = 0x40000000,
          sd_base_addr = 0x40010000,        
          sd_bram_addr = 0x40018000,
         eth_base_addr = 0x40020000,
        keyb_base_addr = 0x40030000, // These have been relocated
        uart_base_addr = 0x40034000,
         vga_base_addr = 0x40038000,
        ddr_base_addr  = 0x80000000
      };

#endif
