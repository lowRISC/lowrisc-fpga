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

void *memcpy(void *o, const void *i, size_t n)
{
  uint8_t *optr = (uint8_t *)((size_t)o & 0xFFFFFFFF);
  const uint8_t *iptr = (const uint8_t *)((size_t)i & 0xFFFFFFFF);

  if ((uint64_t)optr < 0x40000000 || (uint64_t)optr >= 0x88000000 || (uint64_t)iptr < 0x40000000 || (uint64_t)iptr >= 0x88000000)
    {
      printf("memcpy internal error, %x <= %x\n", optr, iptr);
      for(;;)
        ;
    }

  //  printf("memcpy(%x,%x,%x);\n", o, i, n);
  while (n--) *optr++ = *iptr++;
  return optr;
}

size_t strlen (const char *str)
{
  const char *char_ptr = str;

  if ((uint64_t)str < 0x40000000 || (uint64_t)str >= 0x88000000)
    {
      printf("strlen internal error, %x\n", str);
      for(;;)
        ;
    }
  while (*char_ptr)
    ++char_ptr;
  return char_ptr - str;
}
