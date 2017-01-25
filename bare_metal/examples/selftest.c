// SD test program

#include <stdio.h>
#include <stdlib.h>
#include "elf.h"
#include "diskio.h"
#include "ff.h"
#include "uart.h"
#include "memory.h"
#include "trace.h"
#include "encoding.h"
#include "bits.h"
#include "minion_lib.h"

/* Read a text file and display it */

FATFS FatFs;   /* Work area (file system object) for logical drive */

void board_mmc_power_init(void);
void minion_dispatch(const char *ucmd);

int sdcard_test(void)
{  
  FIL fil;                /* File object */
  FRESULT fr;             /* FatFs return code */
  uint32_t br;            /* Read count */
  uint32_t i;
  uint8_t buffer[64];     /* File copy buffer */
 /* Register work area to the default drive */
  if(f_mount(&FatFs, "", 1)) {
    printf("Fail to mount SD driver!\n");
    return 1;
  }

  /* Open a text file */
  fr = f_open(&fil, "test.txt", FA_READ);
  if (fr) {
    printf("failed to open test.txt!\n");
    return (int)fr;
  } else {
    printf("test.txt opened\n");
  }

  /* Read all lines and display it */
  uint32_t fsize = 0;
  for (;;) {
    fr = f_read(&fil, buffer, sizeof(buffer)-1, &br);  /* Read a chunk of source file */
    if (fr || br == 0) break; /* error or eof */
    buffer[br] = 0;
    printf("%s", buffer);
    fsize += br;
  }

  printf("file size %d\n", fsize);

  /* Close the file */
  if(f_close(&fil)) {
    printf("fail to close file!");
    return 1;
  }
  if(f_mount(NULL, "", 1)) {         /* unmount it */
    printf("fail to umount disk!");
    return 1;
  }

  printf("test.txt closed.\n");

  return 0;
}

// A dram test program

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

int sdram_test() {
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
      *(get_ddr_base() + waddr) = wkey;
      waddr = (waddr + 1) & 0x3ffffff;
      wkey = lfsr64(wkey);
    }
    
    if(distance < VERIFY_DISTANCE) distance++;

    if(distance == VERIFY_DISTANCE) {
      printf("Check block @%lx using key %llx\n", raddr, rkey);
      for(i=0; i<STEP_SIZE; i++) {
        unsigned long long rd = *(get_ddr_base() + raddr);
        if(rkey != rd) {
          printf("Error! key %llx stored @%lx does not match with %llx\n", rd, raddr, rkey);
          error_cnt++;
          exit(1);
        }
        raddr = (raddr + 1) & 0x3ffffff;
        rkey = lfsr64(rkey);
        if(error_cnt > 10) exit(1);
      }
    }
  }
}

// just jump from BRAM to DDR

int just_jump (void)
{
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);
  asm volatile ("mret");
}

// memory size

int memory_size (void)
{
  int i, j, k;
  uint32_t *memory_base = (uint32_t *)(get_ddr_base());
  for (i = 0; i < 25; i++)
    {
      uint32_t *off = memory_base + (1<<i);
      for (j = 0; j < i; j++)
	{
	  uint32_t *off2 = off + (1<<j);	  
	  for (k = 0; k < j; k++)
	    {
	      uint32_t pat = lfsr64(i+j+k);
	      uint32_t *off3 = off2 + (1<<k);	  
	      printf("Writing address(%d,%d,%d) => %x\n", i, j, k, off3);
	      *off3 = pat;
	    }
	}
    }
  for (i = 0; i < 25; i++)
    {
      uint32_t *off = memory_base + (1<<i);
      for (j = 0; j < i; j++)
	{
	  uint32_t *off2 = off + (1<<j);	  
	  for (k = 0; k < j; k++)
	    {
	      uint32_t pat = lfsr64(i+j+k);
	      uint32_t *off3 = off2 + (1<<k);
	      printf("Reading address(%d,%d,%d) => %x\n", i, j, k, off3);
	      if (*off3 != pat)
		{
		  printf("Address readback error, found %x, should be %x\n", *off3, pat);
		}
	    }
	}
    }
}

// A flash test program

// 16MB
#define FLASH_SIZE (1<<24)

int flash_test() {
  volatile uint64_t *raddr = get_flash_base();
  uint64_t rdata;
  uint64_t cnt = 0;

  uart_init();
  printf("\nFLASH test program.\n");

  while(cnt < (FLASH_SIZE >> 3)) {
    if(cnt % 4 == 0)
      printf("\n%8x ", raddr);
    rdata = *(raddr++);
    printf("%16x ", rdata);
    cnt++;
  }
}

// A hello world trace program

int trace_main() {
  STM_TRACE(0x1234, 0xdeadbeef);

  trace_event0();
  trace_event1(23);
  trace_event2(0xabcd, 0x0123456789abcdef);
}

// max size of file image is 16M
#define MAX_FILE_SIZE 0x1000000

// size of DDR RAM (128M for NEXYS4-DDR) 
#define DDR_SIZE 0x8000000

// 4K size read burst
#define SD_READ_SIZE 4096

int boot (void)
{
  FIL fil;                // File object
  FRESULT fr;             // FatFs return code
  uint8_t *boot_file_buf = (uint8_t *)(get_ddr_base()) + DDR_SIZE - MAX_FILE_SIZE; // at the end of DDR space
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());

  printf("lowRISC boot program\n=====================================\n");

  // Register work area to the default drive
  if(f_mount(&FatFs, "", 1)) {
    printf("Fail to mount SD driver!\n");
    return 1;
  }

  // Open a file
  printf("Load boot.bin into memory\n");
  fr = f_open(&fil, "boot.bin", FA_READ);
  if (fr) {
    printf("Failed to open boot!\n");
    return (int)fr;
  }

  // Read file into memory
  uint8_t *buf = boot_file_buf;
  uint32_t fsize = 0;           // file size count
  uint32_t br;                  // Read count
  do {
    fr = f_read(&fil, buf, SD_READ_SIZE, &br);  // Read a chunk of source file
    buf += br;
    fsize += br;
  } while(!(fr || br == 0));

  printf("Load %lld bytes to memory address %llx from boot.bin of %lld bytes.\n", fsize, boot_file_buf, fil.fsize);

  // read elf
  printf("load elf to DDR memory\n");
  if(br = load_elf(boot_file_buf, fil.fsize))
    printf("elf read failed with code %0d", br);

  // Close the file
  if(f_close(&fil)) {
    printf("fail to close file!");
    return 1;
  }
  if(f_mount(NULL, "", 1)) {         // unmount it
    printf("fail to umount disk!");
    return 1;
  }

  printf("Boot the loaded program...\n");

  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);
  asm volatile ("mret");
}

static char linbuf[80];

void mygets(char *cmd)
{
  int ch;
  char *chp = cmd;
  do
    {
      ch = uart_recv();
      uart_send(ch);
      *chp++ = ch;
    }
  while (ch != '\n');
  *--chp = 0;
}

uint32_t sd_transaction_v(int sdcmd, uint32_t arg, uint32_t setting)
{
  int i, mask = setting > 7 ? 0x500 : 0x100;
  uint32_t resp[10];
  sd_arg(arg);
  sd_setting(setting);
  sd_cmd(sdcmd);
  mysleep(10);
  sd_cmd_start(1);
  sd_transaction_wait(mask);
  for (i = 10; i--; ) resp[i] = sd_resp(i);
  sd_transaction_finish(mask);
  myputhex(resp[7], 4);
  myputchar(':');
  myputhex(resp[6], 8);
  myputchar('-');
  myputchar('>');
  for (i = 4; i--; )
    {
      myputhex(resp[i], 8);
      myputchar(',');
    }
  myputhex(resp[5], 8);
  myputchar(',');
  myputhex(resp[4], 8);
  myputchar('\n');
  return resp[0] & 0xFFFF0000U;
}

void old_init2(void)
{
  uint32_t card_status[32];
  int i, rca, busy, timeout = 0;
  size_t addr, addr2, data, sdcmd, arg, setting;
  queue_read_array(sd_base, 32, card_status);
  if (card_status[12])
    {
    myputs("card slot is empty\n");
    return;
    }
  sd_transaction_v(0,0x00000000,0x0);
  sd_transaction_v(8,0x000001AA,0x1);
  do {
    sd_transaction_v(55,0x00000000,0x1);
    busy = sd_transaction_v(41,0x40300000,0x1);
  } while ((0x80000000U & ~busy) && (timeout++ < 100));
  sd_transaction_v(2,0x00000000,0x3);
  rca = sd_transaction_v(3,0x00000000,0x1);
  myputs("rca=");
  myputhex(rca, 8);
  myputchar('\n');
  sd_transaction_v(9,rca,0x3);
  sd_transaction_v(13,rca,0x1);
  sd_transaction_v(7,rca,0x1);
  sd_transaction_v(55,rca,0x1);
  sd_transaction_v(51,0x00000000,0x1);
  sd_transaction_v(55,rca,0x1);
  sd_transaction_v(13,0x00000000,0x1);
  for (i = 0; i < 16; i=(i+1)|1)
    {
      sd_transaction_v(16,0x00000200,0x1);
      sd_transaction_v(17,i,0x1);
      sd_transaction_v(16,0x00000200,0x1);
    }
  sd_transaction_v(16,0x00000200,0x1);
  sd_transaction_v(18,0x00000040,0x1);
  sd_transaction_v(12,0x00000000,0x1);
}

const char *scan(const char *start, size_t *data, int base)
{
  *data = 0;
  while (*start)
    {
      if (*start >= '0' && *start <= '9') *data = *data * base + *start++ - '0';
      else if (*start >= 'A' && *start <= 'F') *data = *data * base + *start++ - 'A' + 10;
      else if (*start >= 'a' && *start <= 'f') *data = *data * base + *start++ - 'a' + 10;
      else if (*start == ' ') ++start;
      else return start+1;
    }
  return start;
}

size_t mystrtol(const char *nptr, char **endptr, int base)
{
  size_t data;
  const char *last = scan(nptr, &data, base);
  if (endptr) *endptr = (char *)last;
  return data;
}

void card_response()
{
  int i, data;
  uint32_t card_status[32];
  queue_read_array(sd_base, 32, card_status);
  for (i = 0; i < 32; i++)
    {
      int empty = 0;
      switch(i)
	{
	case 0: myputs("sd_cmd_response[38:7]"); break;
	case 1: myputs("sd_cmd_response[70:39]"); break;
	case 2: myputs("sd_cmd_response[102:71]"); break;
	case 3: myputs("sd_cmd_response[133:103]"); break;
	case 4: myputs("sd_cmd_wait"); break;
	case 5: myputs("sd_status"); break;
	case 6: myputs("sd_cmd_packet[31:0]"); break;
	case 7: myputs("sd_cmd_packet[47:32]"); break;
	case 8: myputs("sd_data_wait"); break;
	case 9: myputs("sd_transf_cnt"); break;
	case 10: myputs("rx_fifo_status"); break;
	case 11: myputs("tx_fifo_status"); break;
	case 12: myputs("sd_detect"); break;
	case 16: myputs("sd_align"); break;
	case 17: myputs("clock_divider_sd_clk"); break;
	case 18: myputs("sd_cmd_arg"); break;
	case 19: myputs("sd_cmd_i"); break;
	case 20: myputs("{sd_data_start,sd_cmd_setting[2:0]}"); break;
	case 21: myputs("sd_cmd_start"); break;
	case 22: myputs("{sd_reset,sd_clk_rst,sd_data_rst,sd_cmd_rst}"); break;
	case 23: myputs("sd_blkcnt"); break;
	case 24: myputs("sd_blksize"); break;
	case 25: myputs("sd_cmd_timeout"); break;
	default: empty = 1; break;
	}
      if (!empty)
	{
	  myputchar(':');
	  data = card_status[i];
	  myputhex(data, 8);
	  myputchar('\n');
	}
    }
}

void minion_dispatch(const char *ucmd)
{
  int i, rca, busy;
  size_t addr, addr2, data, sdcmd, arg, setting;
  const char *nxt;
  switch(*ucmd)
      {
      case 4:
	break;
      case 'B':
	boot();
	break;
      case 'c':
	card_response();
	break;
      case 'd':
	printf("DEV_MAP__io_ext_bram__BASE = %x\n", DEV_MAP__io_ext_bram__BASE);
	printf("DEV_MAP__io_ext_bram__MASK = %x\n", DEV_MAP__io_ext_bram__MASK);
	printf("DEV_MAP__mem__BASE = %x\n", DEV_MAP__mem__BASE);
	printf("DEV_MAP__mem__MASK = %x\n", DEV_MAP__mem__MASK);
	printf("DEV_MAP__io_ext_flash__BASE = %x\n", DEV_MAP__io_ext_flash__BASE);
	printf("DEV_MAP__io_ext_flash__MASK = %x\n", DEV_MAP__io_ext_flash__MASK);
	printf("DEV_MAP__io_int_prci0__BASE = %x\n", DEV_MAP__io_int_prci0__BASE);
	printf("DEV_MAP__io_int_prci0__MASK = %x\n", DEV_MAP__io_int_prci0__MASK);
	printf("DEV_MAP__io_int_rtc__BASE = %x\n", DEV_MAP__io_int_rtc__BASE);
	printf("DEV_MAP__io_int_rtc__MASK = %x\n", DEV_MAP__io_int_rtc__MASK);
	printf("DEV_MAP__io_ext_uart__BASE = %x\n", DEV_MAP__io_ext_uart__BASE);
	printf("DEV_MAP__io_ext_uart__MASK = %x\n", DEV_MAP__io_ext_uart__MASK);
	printf("DEV_MAP__io_ext_spi__BASE = %x\n", DEV_MAP__io_ext_spi__BASE);
	printf("DEV_MAP__io_ext_spi__MASK = %x\n", DEV_MAP__io_ext_spi__MASK);
	printf("DEV_MAP__io_int_bootrom__BASE = %x\n", DEV_MAP__io_int_bootrom__BASE);
	printf("DEV_MAP__io_int_bootrom__MASK = %x\n", DEV_MAP__io_int_bootrom__MASK);
	break;
      case 'f':
	sdcard_test();
	break;
      case 'F':
	flash_test();
	break;
      case 'i':
	old_init2();
	break;
      case 'J':
	just_jump();
	break;
      case 'l':
	nxt = scan(ucmd+1, &data, 16);
	myputchar('l');
	myputchar(' ');
	myputhex(data, 2);
	myputchar('\n');
	write_led(data);
      case 'M':
	memory_size();
	break;
      case 'r':
	nxt = scan(ucmd+1, &addr, 16);
	myputchar('r');
	myputchar(' ');
	myputhex(addr, 8);
	myputchar(',');
	data = queue_read((unsigned *)addr);
	myputhex(data, 2);
	myputchar('\n');
	break;
      case 'R':
	nxt = scan(ucmd+1, &addr, 16);
	addr &= ~3;
	nxt = scan(nxt, &addr2, 16);
	while (addr <= addr2)
	  {
	    myputchar('R');
	    myputchar(' ');
	    myputhex(addr, 8);
	    myputchar(':');
	    data = queue_read((unsigned *)addr);
	    myputhex(data, 8);
	    myputchar('\n');
	    addr += 4;
	  }
	break;
      case 's':
	nxt = scan(ucmd+1, &sdcmd, 16);
	myputchar('s');
	myputchar(' ');
	myputhex(sdcmd,2);
	nxt = scan(nxt, &arg, 16);
	myputchar(',');
	myputhex(arg, 8);
	nxt = scan(nxt, &setting, 16);
	myputchar(',');
	myputhex(setting, 1);
        myputchar('\n');
	sd_transaction_v(sdcmd, arg, setting);
	myputchar('\n');
	break;
      case 't':
	nxt = scan(ucmd+1, &data, 16);
	myputchar('t');
	myputchar(' ');
	myputhex(data, 8);
	myputchar('\n');
	sd_timeout(data);
	break;
      case 'T':
	sdram_test();
	break;
      case 'W':
	nxt = scan(ucmd+1, &addr, 16);
	myputchar('W');
	myputchar(' ');
	myputhex(addr, 8);
	nxt = scan(nxt, &data, 16);
	myputchar(',');
	myputhex(data, 8);
	myputchar('\n');
	queue_write((unsigned *)addr, data, 0);
	break;
      case 'q':
	break;
      default: printf("%c: unknown command\n");
      }
}

int main (void)
{
  trace_main();
  uart_init();
  board_mmc_power_init();  
  do {
    uart_send_string("selftest> ");
    mygets(linbuf);
    minion_dispatch(linbuf);
  } while (*linbuf != 'q');
  return 0;
}

