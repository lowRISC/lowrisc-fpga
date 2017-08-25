// See LICENSE for license details.

#ifndef ETH_HEADER_H
#define ETH_HEADER_H

#include <stdint.h>
#include "dev_map.h"

// Xilinx AXI_ETH 16550

#ifdef DEV_MAP__io_ext_eth__BASE
  #define ETH_BASE ((uint32_t)(DEV_MAP__io_ext_eth__BASE))
#else
  #define ETH_BASE 0
#endif

// ETH APIs
extern void eth_init();

#endif
