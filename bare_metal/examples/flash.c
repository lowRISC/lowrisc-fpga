// A dram test program

#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "memory.h"

// 16MB
#define FLASH_SIZE (1<<24)

int main() {
  uint64_t *raddr = get_flash_base();
  uint64_t rdata;
  uint64_t cnt = 0;

  uart_init();
  printf("\nFLASH test program.\n");

  while(cnt < (FLASH_SIZE >> 3)) {
    if(cnt % 4 == 0)
      printf("\n%8x ", raddr);
    rdata = *(raddr++);
    printf("%16x ", rdata);
    cnt++;
  }
}

