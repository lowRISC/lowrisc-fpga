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
  switch(cause)
    {
    case 0:
      printf("mcause=misalign\n", cause);
      break;
    case 1:
      printf("mcause=instr access fault\n", cause);
      break;
    case 2:
      printf("mcause=instr illegal\n", cause);
      break;
    case 3:
      printf("mcause=breakpoint\n", cause);
      break;
    case 4:
      printf("mcause=load address misaligned\n", cause);
      break;
    case 5:
      printf("mcause=load access fault\n", cause);
      break;
    case 6:
      printf("mcause=store address misaligned\n", cause);
      break;
    case 7:
      printf("mcause=store access fault\n", cause);
      break;
    default:
      printf("mcause=%x\n", cause);
      break;
    }
  
    
    printf("mepc=%x\n", epc);
    printf("mbadaddr=%x\n", read_csr_safe(mbadaddr));
    //    printf("einsn=%x\n", *(int*)epc);
    for (int i = 0; i < 32; i++)
      {
        if (i == 2)
          printf("sp =%x ", regs[i]);
        else if (i == 1)
          printf("ra =%x ", regs[i]);
        else if (i == 2)
          printf("sp =%x ", regs[i]);
        else if (i == 3)
          printf("gp =%x ", regs[i]);
        else if ((i >= 10) && (i <= 17))
          printf("a%d=%x ", i-10, regs[i]);
        else if ((i >= 18) && (i <= 27))
          printf("s%d=%x ", i-16, regs[i]);
        else
          printf("x%d=%x ", i, regs[i]);
        if ((i&3) == 3) printf("\n"); 
      }
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

size_t err = 0, eth = 0, ddr = 0, rom = 0, bram = 0, intc = 0, spi = 0, uart = 0, clin = 0, dumh = 0;

void _init(int cid, int nc)
{
  size_t unknown = 0;
  char *unknownstr, *config = (char *)0x10000;
  for (int i = 128; i < 4096; i++)
    {
      char ch = config[i] & 0x7f; 
      if (ch == '@')
        {
          int j = i+1;
          unsigned addr = 0;
          while ((config[j] >= '0' && config[j] <= '9') || (config[j] >= 'a' && config[j] <= 'f'))
            {
              int hex = config[j] - '0';
              if (hex > 9) hex = config[j] - 'a' + 10;
              addr = (addr << 4) | hex;
              j++;
            }
          j = i - 1;
          while ((config[j] >= 'a' && config[j] <= 'z') || config[j] == '-')
            j--;
          if ((++j < i) && addr)
            {
              uint32_t label = (config[j]<<24) | (config[j+1]<<16) | (config[j+2]<<8) |(config[j+3]);
              switch (label)
                {
                case 'memo':
                  ddr = addr;
                  break;
                case 'bram':
                  bram = addr;
                  break;
                case 'clin':
                  clin = addr;
                  break;
                case 'dumh':
                  dumh = addr;
                  break;
                case 'erro':
                  err = addr;
                  break;
                case 'eth@':
                  eth = addr;
                  break;
                case 'inte':
                  intc = addr;
                  break;
                case 'rom@':
                  rom = addr;
                  break;
                case 'seri':
                  uart = addr;
                  break;
                case 'spi@':
                  spi = addr;
                  break;
                default:
                  unknown = addr;
                  unknownstr = config+j;
                  break;
                }
            }
        }
    }
  if (uart)
    {
      uart_init((void *)uart);
      printf("Serial controller start 0x%x\n", uart);
      if (eth) printf("Ethernet start 0x%x\n", eth);
      if (err) printf("Error device start 0x%x\n", err);
      if (rom) printf("ROM start 0x%x\n", rom);
      if (bram) printf("Block RAM start 0x%x\n", bram);
      if (ddr) printf("DDR memory start 0x%x\n", ddr);
      if (intc) printf("Interrupt controller start 0x%x\n", intc);
      if (spi) printf("SPI controller start 0x%x\n", spi);
      if (unknown)
        printf("Unknown %s, start = 0x%x\n", unknownstr, unknown);
    }
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

