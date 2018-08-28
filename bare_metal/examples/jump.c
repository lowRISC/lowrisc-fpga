// See LICENSE for license details.

// just jump from BRAM to DDR

#include <stddef.h>
#include <stdint.h>
#include <memory.h>
#include "encoding.h"
#include "mini-printf.h"
#include "diskio.h"
#include "ff.h"
#include "bits.h"
#include "hid.h"
#include "eth.h"
#include "elfriscv.h"
#include "lowrisc_memory_map.h"

int main (void)
{
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);
  asm volatile ("mret");
}
