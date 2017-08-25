// A hello world program

#include <stdio.h>
#include <stdint.h>
#include "encoding.h"
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

volatile uint64_t * get_ddr_base() {
  return (uint64_t *)(DEV_MAP__mem__BASE);
}

void *sbrk(size_t len)
{
  static unsigned long raddr = 0;
  char *rd = (char *)get_ddr_base();
  rd += raddr;
  raddr += ((len-1)|7)+1;
  return rd;
}

void external_interrupt(void)
{
  int handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif  
  if (axi_read(RSR_OFFSET) & RSR_RECV_DONE_MASK)
    {
      int length = axi_read(RPLR_OFFSET) & RPLR_LENGTH_MASK;
      int fcs = axi_read(RFCS_OFFSET);
      int i, rnd = ((length-1|3)+1);
      uint32_t *alloc = sbrk(rnd);
      for (i = 0; i < rnd/4; i++)
	  {
            alloc[i] = axi_read(RXBUFF_OFFSET+(i<<2));
	  }
      axi_write(RSR_OFFSET, 0); /* acknowledge */
      printf("length = %d\n", length);
      printf("RX FCS = %x\n", fcs);
      handled = 1;
    }
  if (uart_check_read_irq())
    {
      int rslt = uart_read_irq();
      printf("uart interrupt read %x (%c)\n", rslt, rslt);
      handled = 1;
    }
  if (!handled)
    {
      printf("unhandled interrupt!\n");
    }
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
  enum {scale=262144};
  int cnt = scale;
  uart_init();
  set_csr(mstatus, MSTATUS_MIE|MSTATUS_HIE);
  set_csr(mie, ~MIP_MTIP);
  set_csr(mstatus, MSTATUS_MIE|MSTATUS_HIE);
  set_csr(mie, ~(1 << IRQ_M_TIMER));
  uart_enable_read_irq();
  eth_test();
  printf("Hello World from Ethernet! "__TIMESTAMP__"\n");
  do {
    if (++cnt % scale == 0)
      {
        int pp = cnt / scale;
	printf("Count %d\n", pp);
        if (axi_read(RSR_OFFSET) & RSR_RECV_DONE_MASK)
          {
            printf("#");
          }
      }
  } while (1);
}

