// See LICENSE for license details.

#include "memory.h"

volatile uint64_t * get_bram_base() {
#ifdef BRAM_BASE
  return BRAM_BASE;
#else
  return (uint64_t *)0;         /* boot ROM, raise error */
#endif
}

volatile uint64_t * get_ddr_base() {
  return (uint64_t *)(MEM_BASE);
}

volatile uint64_t  get_ddr_size() {
  return (uint64_t)MEM_SIZE;
}

volatile uint64_t * get_flash_base() {
#ifdef FLASH_BASE
  return (uint64_t *)(FLASH_BASE);
#else
  return (uint64_t *)0;         /* boot ROM, raise error */
#endif
}
