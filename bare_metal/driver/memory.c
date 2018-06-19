// See LICENSE for license details.

#include "lowrisc_memory_map.h"

// LowRISC simple UART base address
volatile uint64_t *const uart_base = (uint64_t *)uart_base_addr;
// LowRISC VGA-compatible display base address
volatile uint8_t *const hid_vga_ptr = (uint8_t *)vga_base_addr;
// LowRISC Ethernet base address
volatile uint64_t *const eth_base = (volatile uint64_t *)eth_base_addr;
// LowRISC SD-card base address
volatile uint64_t *const sd_base = (volatile uint64_t *)sd_base_addr;
// LowRISC SD-card buffer RAM address
volatile uint32_t *const sd_bram = (volatile uint32_t *)sd_bram_addr;
// Rocket PLIC base address
volatile uint32_t *const plic = (volatile uint32_t *)plic_base_addr;

volatile uint64_t * get_bram_base() {
  return (uint64_t *)bram_base_addr;
}

volatile uint64_t * get_ddr_base() {
  return (uint64_t *)0x80000000;
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
