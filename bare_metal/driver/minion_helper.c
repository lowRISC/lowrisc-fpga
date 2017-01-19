#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "minion_lib.h"
#ifdef __x86_64
#define shared_read log_edcl_read
#define shared_write log_edcl_write
#include "tester/main.h"
#include "bbl.h"
#include "driver/diskio.h"

void uart_send_buf(const char *buf, const int32_t len) 
{
  write(2, buf, len);
}

void die(int code)
{
  exit(code);
}

int main(void)
{
  //  uart_init();
  vm_init();
  f_mount(&FatFs, "", 0);
  boot_loader();
}

void myputchar(char ch)
{
   putchar(ch);
   if (ch == '\n') fflush(stdout);
}

#else
#include "trace.h"
static unsigned char *shared_base = (unsigned char *)0x40010000;

int shared_read(uint64_t addr, int bytes, uint8_t *obuf)
  {
    memcpy(obuf, shared_base+addr, bytes);
    STM_TRACE(1, shared_base+addr);
    STM_TRACE(2, bytes);
    STM_TRACE(3, *(uint64_t *)obuf);
  }

int shared_write(uint64_t addr, int bytes, uint8_t *ibuf)
  {
    STM_TRACE(4, shared_base+addr);
    STM_TRACE(5, bytes);
    STM_TRACE(6, *(uint64_t *)ibuf);
    memcpy(shared_base+addr, ibuf, bytes);
  }

#undef putchar
void myputchar(char ch)
{
   putchar(ch);
}

#endif

typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;

#  define __arch__swab32(x) ___swab32(x)
#define ___swab32(x) \
	((__u32)( \
		(((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
		(((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
		(((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(x) & (__u32)0xff000000UL) >> 24) ))

static __inline__ __attribute__((const)) __u32 __fswab32(__u32 x)
{
	return __arch__swab32(x);
}

uint32_t to_cpu(uint32_t arg)
{
  return (__builtin_constant_p((__u32)(( __u32)(__be32)(arg))) ? ((__u32)( (((__u32)((( 
__u32)(__be32)(arg))) & (__u32)0x000000ffUL) << 24) | (((__u32)((( __u32)(__be32)(arg)))
 & (__u32)0x0000ff00UL) << 8) | (((__u32)((( __u32)(__be32)(arg))) & (__u32)0x00ff0000UL
) >> 8) | (((__u32)((( __u32)(__be32)(arg))) & (__u32)0xff000000UL) >> 24) )) : __fswab32((( __u32)(__be32)(arg))));
}

void sd_disable() {

}

uint8_t sd_send(uint8_t dat) {
}

void sd_send_multi(const uint8_t* dat, uint8_t n) {
}

void sd_recv_multi(uint8_t* dat, uint8_t n) {
}

void sd_select_slave(uint8_t id) {
}

void sd_deselect_slave(uint8_t id) {
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

int wait_ready (                /* 1:Ready, 0:Timeout */
                uint32_t wt     /* Timeout [ms] */
                                )
{
  return 1;
}



/*-----------------------------------------------------------------------*/
/* Deselect the card                                                     */
/*-----------------------------------------------------------------------*/

void sd_deselect (void)
{
  sd_deselect_slave(0);
}

/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/

int sd_select (void)   /* 1:Successful, 0:Timeout */
{
  sd_select_slave(0);
  if (wait_ready(500)) return 1;  /* Wait for card ready */

  sd_deselect();
  return 0;   /* Timeout */
}

/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/

int xmit_datablock (
                    const uint8_t *buff,   /* 512 byte data block to be transmitted */
                    uint8_t token          /* Data/Stop token */
                    )
{
  uint8_t resp;
  return 1;
}


int echo = 0;
static char linbuf[99];

void myputs(const char *str)
{
  while (*str)
    {
      myputchar(*str++);
    }
}

void myputn(unsigned n)
{
  if (n > 9) myputn(n / 10);
  myputchar(n%10 + '0');
}

void myputhex(unsigned n, unsigned width)
{
  if (width > 1) myputhex(n >> 4, width-1);
  n &= 15;
  if (n > 9) myputchar(n + 'A' - 10);
  else myputchar(n + '0');
}

unsigned sd_transaction_v(int sdcmd, unsigned arg, unsigned setting);
int sd_transaction(unsigned read, unsigned val, unsigned resp[], unsigned iobuf[], unsigned iobuflen);
void mysleep(int delay);
unsigned int sd_resp(int);
unsigned int sd_stat(int);
void sd_timeout(int d_timeout);
void sd_blksize(int d_blksize);
void sd_blkcnt(int d_blkcnt);
void rx_write_fifo(unsigned int data);
unsigned int rx_read_fifo(void);
void sd_transaction_start(int cmd_flags);
void sd_transaction_wait(int mask);
int sd_transaction_flush(int flush, unsigned resp[], unsigned iobuf[], unsigned iobuflen);
void sd_transaction_finish(int mask);
void queue_read_array(volatile unsigned int * const sd_ptr, unsigned cnt, unsigned iobuf[]);

void open_handle(void);
void uart_printf(const char *fmt, ...);
void log_printf(const char *fmt, ...);
void uart_write(volatile unsigned int * const sd_ptr, unsigned val);
int cli_readline_into_buffer(const char *const prompt, char *buffer, int timeout);

extern volatile unsigned int * const sd_base;

void myputchar(char ch);
void myputs(const char *str);
void minion_uart_write(struct minion_uart_host *host, uint32_t val, int reg);
uint32_t minion_uart_read(struct minion_uart_host *host, int reg);
void minion_uart_reset(struct minion_uart_host *host, uint8_t mask);

void minion_dispatch(const char *ucmd);

volatile unsigned int * const led_base = (volatile unsigned int*)(7<<20);
volatile unsigned int * const sd_base = (volatile unsigned int*)(6<<20);
volatile unsigned int * const sd_stat_ = (volatile unsigned int*)(5<<20);
volatile unsigned int * const rxfifo_base = (volatile unsigned int*)(4<<20);

enum edcl_mode {edcl_mode_unknown, edcl_mode_read, edcl_mode_write, edcl_mode_block_read, edcl_max=256};

#pragma pack(4)

static struct etrans {
  enum edcl_mode mode;
  volatile uint32_t *ptr;
  uint32_t val;
} edcl_trans[edcl_max+1];

#pragma pack()

static int edcl_cnt;

void queue_flush(void)
{
  struct etrans tmp;
  tmp.val = 0xDEADBEEF;
  edcl_trans[edcl_cnt++].mode = edcl_mode_unknown;
  shared_write(0, edcl_cnt*sizeof(struct etrans), (uint8_t*)edcl_trans);
  shared_write(edcl_max*sizeof(struct etrans), sizeof(struct etrans), (uint8_t*)&tmp);
  do {
    shared_read(0, sizeof(tmp), (uint8_t *)&(tmp));
  } while (tmp.ptr);
}

void queue_write(volatile unsigned int *const sd_ptr, unsigned val, int flush)
 {
   struct etrans tmp;
   tmp.mode = edcl_mode_write;
   tmp.ptr = sd_ptr;
   tmp.val = val;
   edcl_trans[edcl_cnt++] = tmp;
   if (flush || (edcl_cnt==edcl_max))
     {
       queue_flush();
       edcl_cnt = 0;
     }
 }

unsigned queue_read(volatile unsigned int * const sd_ptr)
 {
   struct etrans tmp;
   tmp.mode = edcl_mode_read;
   tmp.ptr = sd_ptr;
   tmp.val = 0xDEADBEEF;
   edcl_trans[edcl_cnt++] = tmp;
   queue_flush();
   shared_read((edcl_cnt-2)*sizeof(struct etrans), sizeof(tmp), (uint8_t *)&tmp);
   edcl_cnt = 0;
   return tmp.val;
 }

void queue_read_array(volatile unsigned int * const sd_ptr, unsigned cnt, unsigned iobuf[])
 {
   int i, n;
   struct etrans tmp;
   if (edcl_cnt+cnt >= edcl_max)
     queue_flush();
   for (i = 0; i < cnt; i++)
     {
       tmp.mode = edcl_mode_read;
       tmp.ptr = sd_ptr+i;
       tmp.val = 0xDEADBEEF;
       edcl_trans[edcl_cnt++] = tmp;
     }
   queue_flush();
   n = edcl_cnt-1-cnt;
   shared_read(n*sizeof(struct etrans), cnt*sizeof(struct etrans), (uint8_t *)(edcl_trans+n));
   for (i = n; i < n+cnt; i++) iobuf[i-n] = edcl_trans[i].val;
   edcl_cnt = 0;
 }

void rx_write_fifo(unsigned int data)
{
  queue_write(rxfifo_base, data, 0);
}

unsigned int rx_read_fifo(void)
{
  return queue_read(rxfifo_base);
}

unsigned int sd_resp(int sel)
{
  unsigned int rslt = queue_read(sd_base+sel);
  return rslt;
}

unsigned int sd_stat(int sel)
{
  unsigned int rslt = queue_read(sd_stat_+sel);
  return rslt;
}

void sd_align(int d_align)
{
  queue_write(sd_base+0, d_align, 0);
}

void sd_clk_div(int clk_div)
{
  printf("Clock divider = %d\n", clk_div);
  queue_write(sd_base+1, clk_div, 0);
}

void sd_cmd(unsigned cmd)
{
  queue_write(sd_base+3, cmd, 0);
}

void sd_arg(unsigned arg)
{
  queue_write(sd_base+2, arg, 0);
}

void sd_setting(int setting)
{
  queue_write(sd_base+4, setting, 0);
}

void sd_cmd_start(int sd_cmd)
{
  queue_write(sd_base+5, sd_cmd, 0);
}

void sd_reset(int sd_rst, int clk_rst, int data_rst, int cmd_rst)
{
  queue_write(sd_base+6, ((sd_rst&1) << 3)|((clk_rst&1) << 2)|((data_rst&1) << 1)|((cmd_rst&1) << 0), 0);
}

void sd_blkcnt(int d_blkcnt)
{
  queue_write(sd_base+7, d_blkcnt&0xFFFF, 0);
}

void sd_blksize(int d_blksize)
{
  queue_write(sd_base+8, d_blksize&0xFFF, 0);
}

void sd_timeout(int d_timeout)
{
  queue_write(sd_base+9, d_timeout, 0);
}

void mysleep(int delay)
{
}

int queue_block_read(unsigned iobuf[], unsigned iobuflen)
{
  int cnt;
   struct etrans tmp;
   queue_flush();
   tmp.mode = edcl_mode_block_read;
   tmp.ptr = rxfifo_base;
   tmp.val = 1;
   shared_write(0, sizeof(struct etrans), (uint8_t*)&tmp);
   tmp.val = 0xDEADBEEF;
   shared_write(edcl_max*sizeof(struct etrans), sizeof(struct etrans), (uint8_t*)&tmp);
   do {
    shared_read(0, sizeof(tmp), (uint8_t *)&tmp);
  } while (tmp.ptr);
   cnt = tmp.mode;
   if (cnt > iobuflen) cnt = iobuflen;
   if (cnt) shared_read(sizeof(struct etrans), cnt*sizeof(uint32_t), (uint8_t *)iobuf);
   return cnt;
}

void board_mmc_power_init(void)
{
  sd_clk_div(200);
  sd_reset(0,1,0,0);
  mysleep(74);
  sd_blkcnt(1);
  sd_blksize(1);

  sd_align(0);

  sd_timeout(14);
  mysleep(10);
  sd_reset(0,1,1,1);
  mysleep(10);
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

static int minion_uart_host_control;
static int minion_uart_ctrl_dma;
static int minion_uart_ctrl_cd;
static int minion_uart_power_control;
static int minion_uart_power_180;
static int minion_uart_block_gap;
static int minion_uart_wake_up;
static int minion_uart_timeout_control;
static int minion_uart_software_reset;
static int minion_uart_clock_div;
static int minion_uart_int_status;
static int minion_uart_int_enable;
static int minion_uart_signal_enable;
static int minion_uart_present_state;
static int minion_uart_max_current;
static int minion_uart_set_acmd12;
static int minion_uart_set_int;
static int minion_uart_slot_int;
static int minion_uart_host_version;
static int minion_uart_transfer_mode;

void sd_cmd_setting(int cmd_flags)
{
  int setting = 0;
  switch(cmd_flags & MINION_UART_CMD_RESP_MASK)
      {
      case MINION_UART_CMD_RESP_NONE: setting = 0; break;
      case MINION_UART_CMD_RESP_SHORT: setting = 1; break;
      case MINION_UART_CMD_RESP_SHORT_BUSY: setting = 1; break;
      case MINION_UART_CMD_RESP_LONG: setting = 3; break;
      }
    if (minion_uart_transfer_mode & MINION_UART_TRNS_READ)
      {
	setting |= 0x14;
	if (minion_uart_host_control & MINION_UART_CTRL_4BITBUS) setting |= 0x20;
      }
    sd_setting(setting);
}

void sd_transaction_start(int cmd_flags)
  {
    sd_cmd(cmd_flags >> 8);
    sd_cmd_setting(cmd_flags & 255);
    mysleep(10);
    sd_cmd_start(1);
  }

void sd_transaction_wait(int mask)
{
  unsigned wait;
  do {
    unsigned stat = sd_stat(0);
    wait = stat & mask;
  }
    while (wait != mask);
}

 int sd_transaction_flush(int flush, unsigned resp[], unsigned iobuf[], unsigned iobuflen)
{
  int i, cnt = 0;
  queue_read_array(sd_base, 10, resp);
  if (flush) cnt = queue_block_read(iobuf, iobuflen);
    return cnt;
}

void sd_transaction_finish(int mask)
{
    sd_cmd_start(0);
    sd_cmd_setting(0);
    while ((sd_stat(0) & mask) != 0);
}

int sd_transaction(unsigned read, unsigned val, unsigned resp[], unsigned iobuf[], unsigned iobuflen)
  {
    int cnt = 0;
    int cmd = val >> 8;
    int mask = read ? 0x500 : 0x100;
    sd_transaction_start(val);

    sd_transaction_wait(mask);

    cnt = sd_transaction_flush(read || !cmd, resp, iobuf, cmd ? iobuflen : 0);
    sd_transaction_finish(mask);

    return cnt;
  }

unsigned sd_transaction_v(int sdcmd, unsigned arg, unsigned setting)
{
  int i, mask = setting > 7 ? 0x500 : 0x100;
  unsigned resp[10];
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

void minion_uart_write(struct minion_uart_host *host, uint32_t val, int reg)
{
  int read, mask;
  switch (reg)
    {
    case MINION_UART_BLOCK_COUNT	:
      sd_blkcnt(val);
      break;
    case MINION_UART_BLOCK_SIZE	        :
      sd_blksize(val);
      break;
    case MINION_UART_HOST_CONTROL	:
      minion_uart_host_control = val;
      printf("host_control = %d\n", val);
      if (val & MINION_UART_CTRL_4BITBUS)
	printf("4-bit bus enabled\n");
      else
	printf("4-bit bus disabled\n");
      break;
    case MINION_UART_ARGUMENT	        :
      sd_arg(val);
      break;
    case MINION_UART_TRANSFER_MODE	: minion_uart_transfer_mode = val; break;

    case MINION_UART_CTRL_CD_TEST_INS   : minion_uart_ctrl_cd = val; break;
    case MINION_UART_CTRL_CD_TEST	: minion_uart_ctrl_cd = val; break;
    case MINION_UART_POWER_CONTROL	:
      minion_uart_power_control = val;
      printf("power control = %d\n", val);
      break;
    case MINION_UART_POWER_180	        : minion_uart_power_180 = val; break;
    case MINION_UART_COMMAND	        :
      read = minion_uart_transfer_mode & MINION_UART_TRNS_READ;
      mask = read ? 0x500 : 0x100;
      sd_transaction_start(val);
      sd_transaction_wait(mask);
      sd_transaction_finish(mask);
      minion_uart_int_status = MINION_UART_INT_RESPONSE;
      break;
    case MINION_UART_BLOCK_GAP_CONTROL	: minion_uart_block_gap = val; break;
    case MINION_UART_WAKE_UP_CONTROL	: minion_uart_wake_up = val; break;
    case MINION_UART_TIMEOUT_CONTROL	:
      minion_uart_timeout_control = val;
      sd_timeout(minion_uart_timeout_control);
      break;
    case MINION_UART_SOFTWARE_RESET	:
      minion_uart_software_reset = val;
      minion_uart_timeout_control = 1000; 
      minion_uart_transfer_mode = 0;
      break;
    case MINION_UART_CLOCK_CONTROL	:
      minion_uart_clock_div = val >> MINION_UART_DIVIDER_SHIFT;
      if (minion_uart_clock_div) sd_clk_div(40000000/minion_uart_clock_div);
      if (val & MINION_UART_CLOCK_CARD_EN) printf("Card clock enabled\n"); else printf("Card clock disabled\n");
      break;
    case MINION_UART_INT_STATUS	:
      minion_uart_int_status = val;
      minion_uart_transfer_mode = 0;
      break;
    case MINION_UART_INT_ENABLE	: minion_uart_int_enable = val; break;
    case MINION_UART_SIGNAL_ENABLE	: minion_uart_signal_enable = val; break;
    case MINION_UART_PRESENT_STATE	: minion_uart_present_state = val; break;
    case MINION_UART_MAX_CURRENT	: minion_uart_max_current = val; break;
    case MINION_UART_SET_ACMD12_ERROR	: minion_uart_set_acmd12 = val; break;
    case MINION_UART_SET_INT_ERROR	: minion_uart_set_int = val; break;
    case MINION_UART_SLOT_INT_STATUS	: minion_uart_slot_int = val; break;
    case MINION_UART_HOST_VERSION	: minion_uart_host_version = val; break;
    default: printf("unknown(%d)", reg);
    }
}

uint32_t minion_uart_read(struct minion_uart_host *host, int reg)
{
  switch (reg)
    {
    case MINION_UART_RESPONSE          : return sd_resp(0);
    case MINION_UART_RESPONSE+4        : return sd_resp(1);
    case MINION_UART_RESPONSE+8        : return sd_resp(2);
    case MINION_UART_RESPONSE+12       : return sd_resp(3);
    case MINION_UART_INT_STATUS	:
      return sd_resp(4) < minion_uart_timeout_control ? MINION_UART_INT_RESPONSE|MINION_UART_INT_DATA_AVAIL : MINION_UART_INT_ERROR;
    case MINION_UART_INT_ENABLE	: return minion_uart_int_enable;
    case MINION_UART_PRESENT_STATE	: return MINION_UART_DATA_AVAILABLE;
    case MINION_UART_HOST_VERSION	: return minion_uart_host_version;
    case MINION_UART_CAPABILITIES      : return MINION_UART_CAN_VDD_330;
    case MINION_UART_SOFTWARE_RESET : return 0;
    case MINION_UART_HOST_CONTROL: return minion_uart_host_control;
    case MINION_UART_CLOCK_CONTROL: return (minion_uart_clock_div << MINION_UART_DIVIDER_SHIFT)|MINION_UART_CLOCK_INT_STABLE;
    case MINION_UART_BUFFER : return 0;
    default: printf("unknown(%d)", reg);
    }
  return 0;
}

static unsigned my_resp[10];

static unsigned iobuf[512];
static struct minion_uart_host host;

int sd_read_sector(int sect, void *buf, int max)
{
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000200, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000101A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x000001F4, MINION_UART_TIMEOUT_CONTROL);
minion_uart_write(&host, 0x00000200, MINION_UART_BLOCK_SIZE);
minion_uart_write(&host, 0x00000001, MINION_UART_BLOCK_COUNT);
minion_uart_write(&host, 0x00000012, MINION_UART_TRANSFER_MODE);
minion_uart_write(&host, sect, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000113A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
 queue_read_array(sd_base, 10, my_resp);
 int i, len = queue_block_read(iobuf, 512);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 printf("block_read(%d) = %d\n", sect, len);
 for (i = 0; i < len; i++)
   ((uint32_t *)buf)[i] = to_cpu(iobuf[i]); 
 return len;
}

#if 1
uint8_t send_cmd (     /* Returns R1 resp (bit7==1:Send failed) */
                  uint8_t cmd,       /* Command index */
                  uint32_t arg,      /* Argument */
		  uint32_t resp_type        /* response type */
                       )
{
  switch(cmd)
    {
#if 0
    case 17:
      sd_read_sector(arg);
      return 0;
      break;
#endif
    default:
      printf("unhandled send_cmd\n");
    }
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

int rcvr_datablock (
                    uint8_t *buff,         /* Data buffer to store received data */
                    uint32_t btr            /* Byte count (must be multiple of 4) */
                    )
{
  printf("unhandled rcvr_datablock\n");
}
#endif

void init_sd(void)
{
  int i;
  host.name = "minion_uart";
  
int minion_uart_capabilities = minion_uart_read(&host, MINION_UART_CAPABILITIES);
int minion_uart_host_version = minion_uart_read(&host, MINION_UART_HOST_VERSION);
minion_uart_write(&host, 0x00000001, MINION_UART_SOFTWARE_RESET);
int minion_uart_software_reset = minion_uart_read(&host, MINION_UART_SOFTWARE_RESET);
minion_uart_write(&host, 0x0000000F, MINION_UART_POWER_CONTROL);
minion_uart_write(&host, 0x027F003B, MINION_UART_INT_ENABLE);
minion_uart_write(&host, 0x00000000, MINION_UART_SIGNAL_ENABLE);
int minion_uart_host_control = minion_uart_read(&host, MINION_UART_HOST_CONTROL);
minion_uart_write(&host, 0x00000000, MINION_UART_HOST_CONTROL);
int minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
int minion_uart_clock_control = minion_uart_read(&host, MINION_UART_CLOCK_CONTROL);
minion_uart_write(&host, 0x00000002, MINION_UART_CLOCK_CONTROL);
minion_uart_write(&host, 0x0186A001, MINION_UART_CLOCK_CONTROL);
 minion_uart_clock_control = minion_uart_read(&host, MINION_UART_CLOCK_CONTROL);
minion_uart_write(&host, 0x0186A006, MINION_UART_CLOCK_CONTROL);
 minion_uart_host_control = minion_uart_read(&host, MINION_UART_HOST_CONTROL);
minion_uart_write(&host, 0x00000000, MINION_UART_HOST_CONTROL);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00000000, MINION_UART_COMMAND);
int minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
int cnt = queue_block_read(iobuf, 0);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x000001AA, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000081A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x40300000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00002903, MINION_UART_COMMAND);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00000209, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000031A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00070000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00000909, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00070000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00000D1A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00070000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000071A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00070000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x000001F4, MINION_UART_TIMEOUT_CONTROL);
minion_uart_write(&host, 0x00000008, MINION_UART_BLOCK_SIZE);
minion_uart_write(&host, 0x00000001, MINION_UART_BLOCK_COUNT);
minion_uart_write(&host, 0x00000012, MINION_UART_TRANSFER_MODE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000333A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
cnt = queue_block_read(iobuf, 8);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x000001F4, MINION_UART_TIMEOUT_CONTROL);
minion_uart_write(&host, 0x00000040, MINION_UART_BLOCK_SIZE);
minion_uart_write(&host, 0x00000001, MINION_UART_BLOCK_COUNT);
minion_uart_write(&host, 0x00000012, MINION_UART_TRANSFER_MODE);
minion_uart_write(&host, 0x00FFFFF1, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000063A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
cnt = queue_block_read(iobuf, 64);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x000001F4, MINION_UART_TIMEOUT_CONTROL);
minion_uart_write(&host, 0x00000040, MINION_UART_BLOCK_SIZE);
minion_uart_write(&host, 0x00000001, MINION_UART_BLOCK_COUNT);
minion_uart_write(&host, 0x00000012, MINION_UART_TRANSFER_MODE);
minion_uart_write(&host, 0x80FFFFF1, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000063A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
cnt = queue_block_read(iobuf, 64);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00070000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00000002, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000061A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_host_control = minion_uart_read(&host, MINION_UART_HOST_CONTROL);
minion_uart_write(&host, 0x00000002, MINION_UART_HOST_CONTROL);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x00070000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x0000371A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
minion_uart_write(&host, 0x000001F4, MINION_UART_TIMEOUT_CONTROL);
minion_uart_write(&host, 0x00000040, MINION_UART_BLOCK_SIZE);
minion_uart_write(&host, 0x00000001, MINION_UART_BLOCK_COUNT);
minion_uart_write(&host, 0x00000012, MINION_UART_TRANSFER_MODE);
minion_uart_write(&host, 0x00000000, MINION_UART_ARGUMENT);
minion_uart_write(&host, 0x00000D3A, MINION_UART_COMMAND);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
cnt = queue_block_read(iobuf, 64);
minion_uart_write(&host, 0x00000001, MINION_UART_INT_STATUS);
 minion_uart_int_status = minion_uart_read(&host, MINION_UART_INT_STATUS);
minion_uart_write(&host, 0xFFFFFFFF, MINION_UART_INT_STATUS);
 minion_uart_present_state = minion_uart_read(&host, MINION_UART_PRESENT_STATE);
 minion_uart_clock_control = minion_uart_read(&host, MINION_UART_CLOCK_CONTROL);
minion_uart_write(&host, 0x0186A002, MINION_UART_CLOCK_CONTROL);
minion_uart_write(&host, 0x0C350001, MINION_UART_CLOCK_CONTROL);
 minion_uart_clock_control = minion_uart_read(&host, MINION_UART_CLOCK_CONTROL);
minion_uart_write(&host, 0x0C350006, MINION_UART_CLOCK_CONTROL);
 minion_uart_host_control = minion_uart_read(&host, MINION_UART_HOST_CONTROL);
minion_uart_write(&host, 0x00000002, MINION_UART_HOST_CONTROL);
 for (i = 0; i < 16; i++) sd_read_sector(i, iobuf, 512);
}


