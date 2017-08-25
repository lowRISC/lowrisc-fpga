// A hello world program

#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "dev_map.h"
#include "eth.h"

static volatile unsigned int * const eth_base = (volatile unsigned int*)ETH_BASE;

static int axi_read(size_t addr)
{
  int rslt;
  // printf("axi_read(0x%x);\n", addr);
  rslt = eth_base[addr>>2];
  // printf("return (0x%x);\n", rslt);
  return rslt;
}

static void axi_write(size_t addr, int data, int strb)
{
  int rslt;
  // printf("axi_write(0x%x,0x%x);\n", addr, data);
  eth_base[addr>>2] = data;
  // printf("verify;\n");
  rslt = axi_read(addr);
  // printf("return (0x%x);\n", rslt);
}

static void eth_test(void)
{
  int length = 0x14;
  axi_write(0x800, 0x5E00FACE, 0xf);
  axi_write(0x804, 0x10000, 0xf);
  axi_write(0x1000, 0xffffffff, 0xf);
  axi_write(0x1004, 0xffffffff, 0xf);
  axi_write(0x1008, 0xffffffff, 0xf);
  axi_write(0x100C, 0x11111111, 0xf);
  axi_write(0x1010, 0x22222222, 0xf);
  axi_write(0x1014, 0x33333333, 0xf);  

  axi_write(0x808, length, 0xf);
}

int main() {
  uart_init();
  // printf("Hello World from Ethernet! "__TIMESTAMP__"\n");
  eth_test();
}

