// A hello eth program

#include <stdio.h>
#include "uart.h"
#include "eth.h"

static volatile unsigned int * const eth_base = (volatile unsigned int*)ETH_BASE;

static void axi_write(size_t addr, int data, int strb)
{
  eth_base[addr>>2] = data;
}

static int axi_read(size_t addr)
{
  int rslt = eth_base[addr>>2];
  return rslt;
}

static void eth_test(void)
{
  int rslt;
  axi_write(0x00000000,0xffffffff,0xf);
  axi_write(0x00000004,0xffffffff,0xf);
  axi_write(0x00000008,0xffffffff,0xf);
  axi_write(0x0000000c,0x11111111,0xf);
  axi_write(0x00000010,0x22222222,0xf);
  axi_write(0x00000014,0x33333333,0xf);
  axi_read(0x0000001c);
  axi_write(0x00000020,0x55555555,0xf);
  axi_write(0x000007f8,0x80000000,0xf);
  axi_write(0x000007f4,0x00000014,0xf);
  axi_write(0x000007fc,0x00000009,0xf);
 do
   {
     rslt = axi_read(0x000007fc);
     printf("Status = %x\n", rslt);
   }
 while (rslt == 0x00000009);
}

int main() {
  uart_init();
  printf("Hello Ethernet!\n");
  do {
    printf("hit key to continue..\n");
    uart_recv();
    printf("sending packet..\n");
    eth_test();
  } while (1);
}

