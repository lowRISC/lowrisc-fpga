// A dram test program

#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "memory.h"

unsigned long long lfsr64(unsigned long long d) {
  // x^64 + x^63 + x^61 + x^60 + 1
  unsigned long long bit = 
    (d >> (64-64)) ^
    (d >> (64-63)) ^
    (d >> (64-61)) ^
    (d >> (64-60)) ^
    1;
  return (d >> 1) | (bit << 63);
}

//#define STEP_SIZE 4
#define STEP_SIZE 1024*16
//#define VERIFY_DISTANCE 2
#define VERIFY_DISTANCE 16


int main() {
  volatile uint64_t * addr_base = get_ddr_base();
  uint64_t addr_mask = (get_ddr_size() - 1) >> 3;
  unsigned long waddr = 0;
  unsigned long raddr = 0;
  unsigned long long wkey = 0;
  unsigned long long rkey = 0;
  unsigned int i = 0;
  unsigned int error_cnt = 0;
  unsigned distance = 0;

  uart_init();
  printf("DRAM test program.\n");

  while(1) {
    printf("Write block @%lx using key %llx\n", waddr, wkey);
    for(i=0; i<STEP_SIZE; i++) {
      *(addr_base + waddr) = wkey;
      waddr = (waddr == addr_mask) ? 0 : waddr + 1;
      wkey = lfsr64(wkey);
    }

    if(distance < VERIFY_DISTANCE) distance++;

    if(distance == VERIFY_DISTANCE) {
      printf("Check block @%lx using key %llx\n", raddr, rkey);
      for(i=0; i<STEP_SIZE; i++) {
        unsigned long long rd = *(addr_base + raddr);
        if(rkey != rd) {
          printf("Error! key %llx stored @%lx does not match with %llx\n", rd, raddr, rkey);
          error_cnt++;
          exit(1);
        }
        raddr = (raddr == addr_mask) ? 0 : raddr + 1;
        rkey = lfsr64(rkey);
        if(error_cnt > 10) exit(1);
      }
    }
  }
}

