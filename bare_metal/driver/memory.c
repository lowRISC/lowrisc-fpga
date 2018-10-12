// See LICENSE for license details.
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "eth.h"

#include "lowrisc_memory_map.h"

uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask;

volatile int rxhead, rxtail, txhead, txtail;

inqueue_t *rxbuf;
outqueue_t *txbuf;

// LowRISC simple UART base address
volatile uint64_t *const uart_base = (uint64_t *)uart_base_addr;
// LowRISC VGA-compatible display base address
volatile uint16_t *const hid_vga_ptr = (uint16_t *)vga_base_addr;
// VGA tuning registers
volatile uint64_t *const hid_reg_ptr = (volatile uint64_t *)(vga_base_addr+16384);
// Downloadable font pointer
volatile uint8_t *const hid_font_ptr = (volatile uint8_t *)(vga_base_addr+24576);
// HID keyboard
volatile uint32_t *const keyb_base = (volatile uint32_t *)keyb_base_addr;
// LowRISC Ethernet base address
volatile uint64_t *const eth_base = (volatile uint64_t *)eth_base_addr;
// LowRISC SD-card base address
volatile uint64_t *const sd_base = (volatile uint64_t *)sd_base_addr;
// LowRISC SD-card buffer RAM address
volatile uint64_t *const sd_bram = (volatile uint64_t *)sd_bram_addr;
// Rocket PLIC base address
volatile uint32_t *const plic = (volatile uint32_t *)plic_base_addr;

volatile uint64_t * get_bram_base() {
  return (uint64_t *)bram_base_addr;
}

volatile uint64_t * get_ddr_base() {
  return (uint64_t *)ddr_base_addr;
}

volatile uint64_t  get_ddr_size() {
  return (uint64_t)0x8000000;
}

volatile uint64_t * get_flash_base() {
#ifdef FLASH_BASE
  return (uint64_t *)(FLASH_BASE);
#else
  return (uint64_t *)0;         /* boot ROM, raise error */
#endif
}

void write_led(uint32_t data)
{
  sd_base[15] = data;
}
