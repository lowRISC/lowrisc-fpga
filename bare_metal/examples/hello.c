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

static void axi_write(size_t addr, int data)
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
  uint64_t macaddr = 0x5E00FACE;
  axi_write(MACLO_OFFSET, (macaddr&0xFFFFFFFF));
  axi_write(MACHI_OFFSET, MACHI_COOKED_MASK|(macaddr>>32));
  axi_write(TXBUFF_OFFSET+0x00, 0xffffffff);
  axi_write(TXBUFF_OFFSET+0x04, 0xffffffff);
  axi_write(TXBUFF_OFFSET+0x08, 0xffffffff);
  axi_write(TXBUFF_OFFSET+0x0C, 0x11111111);
  axi_write(TXBUFF_OFFSET+0x10, 0x22222222);
  axi_write(TXBUFF_OFFSET+0x14, 0x33333333);  

  axi_write(TPLR_OFFSET, length);
  printf("MACLO = %x\n", axi_read(MACLO_OFFSET));
  printf("MACHI = %x\n", axi_read(MACHI_OFFSET));

}

int main() {
  uart_init();
  eth_test();
  printf("Hello World from Ethernet! "__TIMESTAMP__"\n");
}

