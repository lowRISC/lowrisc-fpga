// A dram test program

#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "mini-printf.h"
#include "lowrisc_memory_map.h"

// 16MB
#define FLASH_SIZE (1<<24)
#define CMD_RDID 0x9F
#define CMD_MIORDID 0xAF
#define CMD_RDSR 0x05
#define CMD_RFSR 0x70
#define CMD_RDVECR 0x65
#define CMD_WRVECR 0x61
#define CMD_WREN 0x06
#define CMD_SE 0xD8
#define CMD_BE 0xC7
#define CMD_PP 0x02
#define CMD_QCFR 0x0B

#define JEDEC_ID 0x20

#define tPPmax 5 //ms
#define tBEmax 250_000 //ms
#define tSEmax 3_000 //ms
#define input_freq 31_250 //kHz

enum {trigger = 0, cmd = 1, data_send = 2, quad = 3};
enum {readout = 0, status = 2};
enum {busy = 1, error = 2};

int main()
{
  uint64_t *raddr = (uint64_t *)flash_base_addr;
  uint64_t rdata;
  uint64_t cnt = 0;
  
  printf("\nFLASH test program.\n");

  raddr[cmd] = CMD_RDID;
  raddr[trigger] = 1;   
  while (raddr[status] == busy)
    ;
  printf ("readout = %x (expected %x)\n", readout, JEDEC_ID);
  // enable quad IO
  raddr[cmd] = CMD_WRVECR;
  raddr[data_send] = 0x4f;  // quad protocol, hold/accelerator disabled, default drive strength
  raddr[trigger] = 1;   
  while (raddr[status] == busy)
    ;
  raddr[quad] = 1;
  raddr[cmd] = CMD_WREN;
  raddr[trigger] = 1;   
  while (raddr[status] == busy)
    ;
  raddr[cmd] = CMD_PP;
  // 3 bytes of address, then random data
  raddr[data_send] = 0xA30000010203040;
  raddr[trigger] = 1;   
  while (raddr[status] == busy)
    ;
}
