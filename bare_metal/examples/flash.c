// A dram test program

#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "memory.h"

// 16MB
#define FLASH_SIZE (1<<24)

int main() {
  volatile uint32_t *raddr = (volatile uint32_t *)get_flash_base();
  uint32_t rdata;
  uint64_t cnt = 0;

  uart_init();
  printf("\nFLASH test program.\n");

  while(cnt < (FLASH_SIZE >> 2)) {
    rdata = *(raddr++);
    if (rdata != 0xCCCCCCCC)
      {
	if(cnt % 4 == 0)
	  printf("\n%8x ", raddr);
      printf("%x ", rdata);
      }
    cnt++;
  }
}

