// See LICENSE for license details.

#include "lowrisc_memory_map.h"

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
