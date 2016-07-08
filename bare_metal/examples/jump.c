// See LICENSE for license details.

// just jump from BRAM to DDR

#include "encoding.h"
#include "bits.h"
#include "memory.h"

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
