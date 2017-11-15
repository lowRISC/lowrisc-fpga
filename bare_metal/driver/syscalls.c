// See LICENSE for license details.

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>

#include "mini-printf.h"
#include "encoding.h"
#include "memory.h"
#include "bits.h"
#include "uart.h"

#define SYS_write 64
#define SYS_exit 93
#define SYS_stats 1234

// initialized in crt.S
int have_vec;

#define static_assert(cond) switch(0) { case 0: case !!(long)(cond): ; }

// In setStats, we might trap reading uarch-specific counters.
// The trap handler will skip over the instruction and write 0,
// but only if a0 is the destination register.
#define read_csr_safe(reg) ({ register long __tmp asm("a0"); \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define NUM_COUNTERS 18
static long counters[NUM_COUNTERS];
static char* counter_names[NUM_COUNTERS];
static int handle_stats(int enable)
{
  int i = 0;
#define READ_CTR(name) do { \
    while (i >= NUM_COUNTERS) ; \
    long csr = read_csr_safe(name); \
    if (!enable) { csr -= counters[i]; counter_names[i] = #name; } \
    counters[i++] = csr; \
  } while (0)
  READ_CTR(mcycle);  READ_CTR(minstret);
  READ_CTR(0xcc0); READ_CTR(0xcc1); READ_CTR(0xcc2); READ_CTR(0xcc3);
  READ_CTR(0xcc4); READ_CTR(0xcc5); READ_CTR(0xcc6); READ_CTR(0xcc7);
  READ_CTR(0xcc8); READ_CTR(0xcc9); READ_CTR(0xcca); READ_CTR(0xccb);
  READ_CTR(0xccc); READ_CTR(0xccd); READ_CTR(0xcce); READ_CTR(0xccf);
#undef READ_CTR
  return 0;
}

void tohost_exit(long code)
{
  // halt
  if(code) {
    char str[] = "error! exit(0xFFFFFFFFFFFFFFFF)\n";
    int i;
    for (i = 0; i < 16; i++) {
      str[29-i] = (code & 0xF) + ((code & 0xF) < 10 ? '0' : 'a'-10);
      code >>= 4;
    }
    uart_send_string(str);
  }
  uart_send_string("tohost_exit was called, and this version does not return\n");
  for(;;)
    ;
}

long handle_trap(long cause, long epc, long regs[32])
{
  int* csr_insn;
    // do some report
    printf("mcause=%x\n", cause);
    printf("mepc=%x\n", epc);
    printf("mbadaddr=%x\n", read_csr_safe(mbadaddr));
    printf("einsn=%x\n", *(int*)epc);
    printf("sp=%x\n", regs[2]);
    printf("tp=%x\n", regs[4]);
    tohost_exit(1337);
}

void __attribute__((weak)) thread_entry(int cid, int nc)
{
  // multi-threaded programs override this function.
  // for the case of single-threaded programs, only let core 0 proceed.
  while (cid != 0);
}

static void init_tls()
{
  register void* thread_pointer asm("tp");
  extern char _tls_data;
  extern __thread char _tdata_begin, _tdata_end, _tbss_end;
  size_t tdata_size = &_tdata_end - &_tdata_begin;
  memcpy(thread_pointer, &_tls_data, tdata_size);
  size_t tbss_size = &_tbss_end - &_tdata_end;
  memset(thread_pointer + tdata_size, 0, tbss_size);
}

void _init(int cid, int nc)
{
  /*
  init_tls();
  thread_entry(cid, nc);
  */

  // only single-threaded programs should ever get here.
  int ret = main(0, 0);

  char buf[NUM_COUNTERS * 32] __attribute__((aligned(64)));
  char* pbuf = buf;
  for (int i = 0; i < NUM_COUNTERS; i++)
    if (counters[i])
      pbuf += sprintf(pbuf, "%s = %d\n", counter_names[i], counters[i]);
  if (pbuf != buf)
    uart_send_string(buf);

  mini_printf("normal exit reached, code=%d\n", ret);
  for(;;);
}

void printhex(uint64_t x)
{
  char str[17];
  int i;
  for (i = 0; i < 16; i++)
  {
    str[15-i] = (x & 0xF) + ((x & 0xF) < 10 ? '0' : 'a'-10);
    x >>= 4;
  }
  str[16] = 0;

  uart_send_string(str);
}

static inline void printnum(void (*putch)(int, void**), void **putdat,
                    unsigned long long num, unsigned base, int width, int padc)
{
  unsigned digs[sizeof(num)*CHAR_BIT];
  int pos = 0;

  while (1)
  {
    digs[pos++] = num % base;
    if (num < base)
      break;
    num /= base;
  }

  while (width-- > pos)
    putch(padc, putdat);

  while (pos-- > 0)
    putch(digs[pos] + (digs[pos] >= 10 ? 'a' - 10 : '0'), putdat);
}

