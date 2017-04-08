#if 0
#define SDHCI_VERBOSE
#define SDHCI_VERBOSE2
#define SDHCI_VERBOSE3
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "errno.h"
#include "memory.h"
#include "minion_lib.h"
#include "sdhci-minion-hash-md5.h"

#undef putchar
void myputchar(char ch)
{
   putchar(ch);
}

struct completion { void *dummy; };
struct device { void *dummy; };
struct timer_list { void *dummy; };
struct delayed_work { void *dummy; };
struct sg_mapping_iter { int length, consumed; void *addr; };
struct device_driver { void *dummy; };
struct tasklet_struct { void *dummy; };
struct bootstrap_host { int flags; const struct sdhci_ops *ops; unsigned long private[0]; };
struct mmc_request;

typedef void *atomic_t;
typedef void *wait_queue_head_t;
typedef void *spinlock_t;
typedef void *dma_addr_t;

enum {false, true};

#define __iomem
#define ____cacheline_aligned
#define __maybe_unused
#define uninitialized_var(x) x;
#define BUG_ON(x) if (x) printf("bug at line %d\n", __LINE__)

bool sg_miter_next(struct sg_mapping_iter *miter)
{
  return true;
}

void sg_miter_stop(struct sg_mapping_iter *miter)
{

}

#include "sdhci-minion-hash-md5.h"
#include "pm.h"
#include "card.h"
#include "host.h"
#include "core.h"
#include "sdhci.h"

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

uint32_t __be32_to_cpu(uint32_t arg)
{
  return (__builtin_constant_p((__u32)(( __u32)(__be32)(arg))) ? ((__u32)( (((__u32)((( 
__u32)(__be32)(arg))) & (__u32)0x000000ffUL) << 24) | (((__u32)((( __u32)(__be32)(arg)))
 & (__u32)0x0000ff00UL) << 8) | (((__u32)((( __u32)(__be32)(arg))) & (__u32)0x00ff0000UL
) >> 8) | (((__u32)((( __u32)(__be32)(arg))) & (__u32)0xff000000UL) >> 24) )) : __fswab32((( __u32)(__be32)(arg))));
}

void sd_disable(void) {

}

uint8_t sd_send(uint8_t dat) {
  return 0;
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

void myputs(const char *str)
{
  while (*str)
    {
      myputchar(*str++);
    }
}

void myputn(uint32_t n)
{
  if (n > 9) myputn(n / 10);
  myputchar(n%10 + '0');
}

void myputhex(uint32_t n, uint32_t width)
{
  if (width > 1) myputhex(n >> 4, width-1);
  n &= 15;
  if (n > 9) myputchar(n + 'A' - 10);
  else myputchar(n + '0');
}

uint32_t sd_resp(int);
void sd_timeout(int d_timeout);
void sd_blksize(int d_blksize);
void sd_blkcnt(int d_blkcnt);
void tx_write_fifo(uint32_t data);
uint32_t rx_read_fifo(void);
void queue_read_array(volatile uint32_t * const sd_ptr, uint32_t cnt, uint32_t iobuf[]);

void open_handle(void);
void uart_printf(const char *fmt, ...);
void log_printf(const char *fmt, ...);
void uart_write(volatile uint32_t * const sd_ptr, uint32_t val);
int cli_readline_into_buffer(const char *const prompt, char *buffer, int timeout);

extern volatile uint32_t * const sd_base;

void myputchar(char ch);
void myputs(const char *str);
int sdhci_write(u8 *buf, uint32_t val, int reg);
uint32_t sdhci_read(int reg);
void sdhci_reset(struct bootstrap_host *host, uint8_t mask);

void minion_dispatch(const char *ucmd);

/* minion address space pointers */
volatile uint32_t * const led_base = (volatile uint32_t*)(7<<20);
volatile uint32_t * const sd_base = (volatile uint32_t*)(6<<20);
volatile uint32_t * const rxfifo_base = (volatile uint32_t*)(4<<20);
volatile uint32_t * const txfifo_base = (volatile uint32_t*)(5<<20);

enum edcl_mode {edcl_mode_unknown, edcl_mode_read, edcl_mode_write, edcl_mode_block_read, edcl_max=256};

#pragma pack(4)

static struct etrans {
  enum edcl_mode mode;
  volatile uint32_t *ptr;
  uint32_t val;
} edcl_trans[edcl_max+1];

#pragma pack()

static int edcl_cnt;

/* shared address space pointer (appears at 0x800000 in minion address map */
volatile static struct etrans *shared_base = (struct etrans *)0x40010000;

int shared_read(volatile struct etrans *addr, int cnt, struct etrans *obuf)
  {
    int i;
    for (i = 0; i < cnt; i++)
      {
	obuf[i] = addr[i];
#ifdef VERBOSE
	printf("shared_read(%d, %p) => %x,%x;\n", i, addr+i, obuf[i].ptr, obuf[i].val);
#endif
      }
    return 0;
  }

int shared_write(volatile struct etrans *addr, int cnt, struct etrans *ibuf)
  {
    int i,j;
    for (i = 0; i < cnt; i++)
      {
	addr[i] = ibuf[i];
#ifdef VERBOSE
	printf("shared_write(%d, %p, 0x%x, 0x%x);\n", i, addr+i);
	for (j = 0; j < sizeof(struct etrans); j++)
	  printf("%x ", ((volatile uint8_t *)(&addr[i]))[j]);
	printf("\n");
#endif	
      }
    return 0;
  }

int queue_flush(void)
{
  int cnt;
  struct etrans tmp;
  tmp.val = 0xDEADBEEF;
  edcl_trans[edcl_cnt++].mode = edcl_mode_unknown;
#ifdef VERBOSE
  printf("sizeof(struct etrans) = %d\n", sizeof(struct etrans));
  for (int i = 0; i < edcl_cnt; i++)
    {
      switch(edcl_trans[i].mode)
	{
	case edcl_mode_write:
	  printf("queue_mode_write(%p, 0x%x);\n", edcl_trans[i].ptr, edcl_trans[i].val);
	  break;
	case edcl_mode_read:
	  printf("queue_mode_read(%p, 0x%x);\n", edcl_trans[i].ptr, edcl_trans[i].val);
	  break;
	case edcl_mode_unknown:
	  if (i == edcl_cnt-1)
	    {
	    printf("queue_end();\n");
	    break;
	    }
	default:
	  printf("queue_mode %d\n", edcl_trans[i].mode);
	  break;
	}
    }
#endif
  shared_write(shared_base, edcl_cnt, edcl_trans);
  shared_write(shared_base+edcl_max, 1, &tmp);
  do {
#ifdef VERBOSE
    int i = 10000000;
    int tot = 0;
    while (i--) tot += i;
    printf("waiting for minion %x\n", tot);
#endif
    shared_read(shared_base, 1, &tmp);
  } while (tmp.ptr);
  tmp.val = 0;
  shared_write(shared_base+edcl_max, 1, &tmp);
  cnt = edcl_cnt;
  edcl_cnt = 1;
  edcl_trans[0].mode = edcl_mode_read;
  edcl_trans[0].ptr = (volatile uint32_t*)(8<<20);
  return cnt;
}

void queue_write(volatile uint32_t *const sd_ptr, uint32_t val, int flush)
 {
   struct etrans tmp;
#if 1
   flush = 1;
#endif   
   tmp.mode = edcl_mode_write;
   tmp.ptr = sd_ptr;
   tmp.val = val;
   edcl_trans[edcl_cnt++] = tmp;
   if (flush || (edcl_cnt==edcl_max-1))
     {
       queue_flush();
     }
#ifdef VERBOSE  
   printf("queue_write(%p, 0x%x);\n", tmp.ptr, tmp.val);
#endif
 }

uint32_t queue_read(volatile uint32_t * const sd_ptr)
 {
   int cnt;
   struct etrans tmp;
   tmp.mode = edcl_mode_read;
   tmp.ptr = sd_ptr;
   tmp.val = 0xDEADBEEF;
   edcl_trans[edcl_cnt++] = tmp;
   cnt = queue_flush();
   shared_read(shared_base+(cnt-2), 1, &tmp);
#ifdef VERBOSE
   printf("queue_read(%p, %p, 0x%x);\n", sd_ptr, tmp.ptr, tmp.val);
#endif   
   return tmp.val;
 }

void queue_read_array(volatile uint32_t * const sd_ptr, uint32_t cnt, uint32_t iobuf[])
 {
   int i, n, cnt2;
   struct etrans tmp;
   if (edcl_cnt+cnt >= edcl_max)
     {
     queue_flush();
     }
   for (i = 0; i < cnt; i++)
     {
       tmp.mode = edcl_mode_read;
       tmp.ptr = sd_ptr+i;
       tmp.val = 0xDEADBEEF;
       edcl_trans[edcl_cnt++] = tmp;
     }
   cnt2 = queue_flush();
   n = cnt2-1-cnt;
   shared_read(shared_base+n, cnt, edcl_trans+n);
   for (i = n; i < n+cnt; i++) iobuf[i-n] = edcl_trans[i].val;
 }

void write_led(uint32_t data)
{
  queue_write(led_base, data, 1);
}

void tx_write_fifo(uint32_t data)
{
  queue_write(txfifo_base, data, 1);
}

void rx_write_fifo(uint32_t data)
{
  queue_write(rxfifo_base, data, 0);
}

uint32_t rx_read_fifo(void)
{
  return queue_read(rxfifo_base);
}

uint32_t sd_resp(int sel)
{
  uint32_t rslt = queue_read(sd_base+sel);
  return rslt;
}

void sd_align(int d_align)
{
  queue_write(sd_base+0, d_align, 0);
}

void sd_clk_div(int clk_div)
{
  if (clk_div < 16) clk_div = 16;
  if (clk_div > 255) clk_div = 255;
  printf("Clock divider = %d\n", clk_div);
  queue_write(sd_base+1, clk_div, 1);
}

void sd_arg(uint32_t arg)
{
  queue_write(sd_base+2, arg, 0);
}

void sd_cmd(uint32_t cmd)
{
  queue_write(sd_base+3, cmd, 0);
}

void sd_setting(int setting)
{
  queue_write(sd_base+4, setting, 1);
}

void sd_cmd_start(int sd_cmd)
{
  queue_write(sd_base+5, sd_cmd, 1);
}

void sd_reset(int sd_rst, int clk_rst, int data_rst, int cmd_rst)
{
  queue_write(sd_base+6, ((sd_rst&1) << 3)|((clk_rst&1) << 2)|((data_rst&1) << 1)|((cmd_rst&1) << 0), 1);
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

uint32_t queue_block_read2(int i)
{
  uint32_t rslt = __be32_to_cpu(((volatile uint32_t *)(shared_base+1))[i]);
  return rslt;
}

int queue_block_read1(void)
{
   struct etrans tmp;
   queue_flush();
   tmp.mode = edcl_mode_block_read;
   tmp.ptr = rxfifo_base;
   tmp.val = 1;
   shared_write(shared_base, 1, &tmp);
   tmp.val = 0xDEADBEEF;
   shared_write(shared_base+edcl_max, 1, &tmp);
   do {
    shared_read(shared_base, 1, &tmp);
  } while (tmp.ptr);
#ifdef SDHCI_VERBOSE3
   printf("queue_block_read1 completed\n");
#endif
   return tmp.mode;
}

static int sdhci_host_control;
static int sdhci_power_control;
static int sdhci_block_gap;
static int sdhci_wake_up;
static int sdhci_timeout_control;
static int sdhci_software_reset;
static int sdhci_clock_div;
static int sdhci_int_status;
static int sdhci_int_enable;
static int sdhci_signal_enable;
static int sdhci_present_state;
static int sdhci_max_current;
static int sdhci_set_acmd12_error;
static int sdhci_acmd12_err;
static int sdhci_set_int;
static int sdhci_slot_int_status;
static int sdhci_host_version;
static int sdhci_transfer_mode;
static int sdhci_dma_address;
static int sdhci_block_count;
static int sdhci_block_size;
static int sdhci_command;
static int sdhci_argument;
static int sdhci_host_control2;

uint32_t card_status[32];

void _get_card_status(int line, int verbose)
{
  int i;
  static uint32_t old_card_status[32];
  queue_read_array(sd_base, 32, card_status);
#ifdef SDHCI_VERBOSE3
  for (i = 0; i < 26; i++) if (verbose || (card_status[i] != old_card_status[i]))
      {
	printf("line(%d), card_status[%d]=%8x\n", line, i, card_status[i]);
	old_card_status[i] = card_status[i];
      }
#endif      
}

void board_mmc_power_init(void)
{
  sd_clk_div(0x20);
  sd_reset(1,1,0,0);
  get_card_status(0);
  sd_blkcnt(1);
  sd_blksize(1);

  sd_align(0);

  sd_timeout(500000);
  get_card_status(0);
  sd_reset(0,1,1,1);
  get_card_status(10);
}

int sd_read_sector(int sect, void *buf, int max)
{
  int rslt, i = 0;
  do {
    rslt = sd_read_sector1(sect, buf, max);
  }
  while (i++ < 3 && rslt);
  if (rslt)
    {
      printf("More than 3 attempts failed on sector %d\n", sect);
      return rslt;
    }
}

static void minion_sdhci_read_block_pio(u8 *buf)
{
	unsigned long flags;
	size_t blksize, len, chunk;
	u32 uninitialized_var(scratch);
	int i = 0;
#ifdef SDHCI_MD5
	md5_ctx_t context;
#endif
#ifdef SDHCI_VERBOSE3
	printf("Sector reading\n");
#endif
	blksize = 512;
	chunk = 0;

#ifdef SDHCI_MD5
	md5_begin(&context);
#endif	
	while (blksize) {
	  int idx = 0;
	  len = blksize;
	  
	  blksize -= len;
	  
	  while (len) {
	    if (chunk == 0) {
	      scratch = queue_block_read2(i++);
	      chunk = 4;
	    }
	    
	    buf[idx] = scratch & 0xFF;	    
	    idx++;
	    scratch >>= 8;
	    chunk--;
	    len--;
	  }
#ifdef SDHCI_MD5
	  md5_hash(&context, buf, idx);
#endif	  
	}
#ifdef SDHCI_MD5	
	md5_end(&context);
	printf("arg=%x, md5 = %s\n", sdhci_argument, hash_bin_to_hex(&context));
#endif	
#ifdef SDHCI_VERBOSE4	
	      {
		int i;
		for (i = 0; i < 512; i++)
		  {
		    if ((i & 31) == 0) printf("\n%4x: ", i);
		    printf("%2x ", buf[i]);
		  }
		printf("\n");
	      }
#endif	
}

static void minion_sdhci_write_block_pio(u8 *buf)
{
	unsigned long flags;
	size_t blksize, len, chunk;
	u32 scratch;	

	blksize = 512;
	chunk = 0;
	scratch = 0;

	while (blksize) {
		len = blksize;

		blksize -= len;

		while (len) {
			scratch |= (u32)*buf << (chunk * 8);

			buf++;
			chunk++;
			len--;

			if ((chunk == 4) || ((len == 0) && (blksize == 0))) {
				tx_write_fifo(scratch);
				chunk = 0;
				scratch = 0;
			}
		}
	}

}

void card_response(void)
{
  int i, data;
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

int sd_transaction_finish(void *buf, int cmd_flags)
{
  uint32_t timeout;
  int rslt, setting = 0;
  switch(sdhci_command & SDHCI_CMD_RESP_MASK)
      {
      case SDHCI_CMD_RESP_NONE: setting = 0; break;
      case SDHCI_CMD_RESP_SHORT: setting = 1; break;
      case SDHCI_CMD_RESP_SHORT_BUSY: setting = 1; break;
      case SDHCI_CMD_RESP_LONG: setting = 3; break;
      }
  if (sdhci_host_control & SDHCI_CTRL_4BITBUS) setting |= 0x20;
  if (sdhci_command & SDHCI_CMD_DATA)
      {
	timeout = sdhci_timeout_control;
	setting |= (sdhci_transfer_mode & SDHCI_TRNS_READ ? 0x10 : 0x8) | 0x4;
      }
  else
    timeout = 15;
  sd_reset(0,1,0,1);
  sd_align(0);
  sd_arg(sdhci_argument);
  sd_cmd(cmd_flags >> 8);
  sd_setting(setting);
  sd_cmd_start(0);
  sd_reset(0,1,1,1);
  sd_blkcnt(sdhci_block_count);
  sd_blksize(sdhci_block_size&0xFFF);
  sd_timeout(500000);
  get_card_status(0);
  /* drain rx fifo, if needed */
  queue_block_read1();
  rslt = sd_transaction_finish2(buf);
  return rslt;
}

int sd_transaction_finish2(void *buf)
{	
  static int good, bad;	   
  uint32_t timeout, stat, wait, timedout, rslt = 0;
  int retry = 0;
  sd_align(0);
  if ((sdhci_command & SDHCI_CMD_DATA) && !(sdhci_transfer_mode & SDHCI_TRNS_READ))
        {
          minion_sdhci_write_block_pio(buf);
        }
  sd_cmd_start(1);
  get_card_status(1);
  timeout = 0;
  do
    {
      get_card_status(0);
      stat = card_status[5];
      wait = stat & 0x100;
    }
  while ((wait != 0x100) && (card_status[4] < card_status[25]) && (timeout++ < 1000000));
#ifdef SDHCI_VERBOSE2
  {
  int i;
  printf("%4x:%8x->", card_status[7], card_status[6]);
  for (i = 4; i--; )
    {
      printf("%8x,", card_status[i]);
    }
  printf("%8x,%8x\n", card_status[5], card_status[4]);
  }
#endif
  queue_read_array(sd_base, 32, card_status);
  if (card_status[4] >= card_status[25])
    {
      printf("cmd timeout\n");
      card_response();
      sd_cmd_start(0);
      sd_setting(0);
      rslt = -1;
    }
 if (sdhci_command & SDHCI_CMD_DATA)
    {
     do
         {
           get_card_status(0);
           stat = card_status[5];
           wait = stat & 0x400;
         }
      while ((wait != 0x400) && (card_status[8] < card_status[25]));
      if ((card_status[8] < card_status[25]) && card_status[9])
	{
	      if (sdhci_transfer_mode & SDHCI_TRNS_READ)
	    {
		{
		  int cnt = queue_block_read1();
		  if (cnt != 129)
		    printf("transf_cnt = %d, fifo_cnt = %d\n", card_status[9], cnt);
		  minion_sdhci_read_block_pio(buf);
		}
	    }
	}
      else
	{
		rslt = -1;
        }
	}
  if (sdhci_transfer_mode & SDHCI_TRNS_READ)
    {
      if (rslt == -1)
	{
	  ++bad;
	    printf("bad = %d/%d (%d%%)\n", bad, good, bad*100/good);
	}
      else
	++good;
    }
#ifdef SDHCI_VERBOSE3
  printf("sd_transaction_finish stopping\n");
#endif
  sd_cmd_start(0);
  sd_setting(0);
#ifdef SDHCI_VERBOSE3
  printf("sd_transaction_finish ended\n");
#endif
  return rslt;
}

void sdhci_minion_hw_reset(void)
{
  printf("sdhci_minion_hw_reset();\n");
}

#ifdef SDHCI_VERBOSE

const char *sdhci_kind(int reg)
{  
  switch (reg)
    {
    case SDHCI_DMA_ADDRESS      : return "SDHCI_DMA_ADDRESS";
    case SDHCI_ARGUMENT	        : return "SDHCI_ARGUMENT";
    case SDHCI_BLOCK_COUNT	: return "SDHCI_BLOCK_COUNT";
    case SDHCI_BLOCK_GAP_CONTROL: return "SDHCI_BLOCK_GAP_CONTROL";
    case SDHCI_BLOCK_SIZE	: return "SDHCI_BLOCK_SIZE";
    case SDHCI_BUFFER           : return "SDHCI_BUFFER ";
    case SDHCI_CAPABILITIES     : return "SDHCI_CAPABILITIES";
    case SDHCI_CLOCK_CONTROL	: return "SDHCI_CLOCK_CONTROL";
    case SDHCI_COMMAND	        : return "SDHCI_COMMAND";
    case SDHCI_HOST_CONTROL	: return "SDHCI_HOST_CONTROL";
    case SDHCI_HOST_VERSION	: return "SDHCI_HOST_VERSION";
    case SDHCI_INT_ENABLE	: return "SDHCI_INT_ENABLE";
    case SDHCI_INT_STATUS	: return "SDHCI_INT_STATUS";
    case SDHCI_MAX_CURRENT	: return "SDHCI_MAX_CURRENT";
    case SDHCI_POWER_CONTROL	: return "SDHCI_POWER_CONTROL";
    case SDHCI_PRESENT_STATE	: return "SDHCI_PRESENT_STATE";
    case SDHCI_RESPONSE+12      : return "SDHCI_RESPONSE+12 ";
    case SDHCI_RESPONSE+4       : return "SDHCI_RESPONSE+4";
    case SDHCI_RESPONSE+8       : return "SDHCI_RESPONSE+8";
    case SDHCI_RESPONSE         : return "SDHCI_RESPONSE";
    case SDHCI_SET_ACMD12_ERROR	: return "SDHCI_SET_ACMD12_ERROR";
    case SDHCI_SET_INT_ERROR	: return "SDHCI_SET_INT_ERROR";
    case SDHCI_SIGNAL_ENABLE	: return "SDHCI_SIGNAL_ENABLE";
    case SDHCI_SLOT_INT_STATUS	: return "SDHCI_SLOT_INT_STATUS";
    case SDHCI_SOFTWARE_RESET	: return "SDHCI_SOFTWARE_RESET";
    case SDHCI_TIMEOUT_CONTROL	: return "SDHCI_TIMEOUT_CONTROL";
    case SDHCI_TRANSFER_MODE	: return "SDHCI_TRANSFER_MODE";
    case SDHCI_WAKE_UP_CONTROL	: return "SDHCI_WAKE_UP_CONTROL";
    case SDHCI_ACMD12_ERR       : return "SDHCI_ACMD12_ERR";
    case SDHCI_HOST_CONTROL2    : return "SDHCI_HOST_CONTROL2";
    case SDHCI_CAPABILITIES_1   : return "SDHCI_CAPABILITIES_1";
    default                     : return "unknown";
    }
}
#endif  

int sdhci_write(u8 *buf, uint32_t val, int reg)
{
  int rslt = 0;
#ifdef SDHCI_VERBOSE2
  if (reg != SDHCI_HOST_CONTROL) printf("sdhci_write(&host, 0x%x, %s);\n", val, sdhci_kind(reg));
#endif
  switch (reg)
    {
    case SDHCI_DMA_ADDRESS      :
      printf("DMA_address = %x\n", val);
      sdhci_dma_address = val;
      break;
    case SDHCI_BLOCK_COUNT	:
      sdhci_block_count = val;
      break;
    case SDHCI_BLOCK_SIZE	        :
      sdhci_block_size = val;
      break;
    case SDHCI_HOST_CONTROL	:
      if ((val^sdhci_host_control) & SDHCI_CTRL_4BITBUS)
	{
	  if (val & SDHCI_CTRL_4BITBUS)
	    printf("4-bit bus enabled\n");
	  else
	    printf("4-bit bus disabled\n");
	}
      if (sdhci_host_control != val)
	write_led(sdhci_host_control);
      sdhci_host_control = val;
      break;
    case SDHCI_ARGUMENT	        :
      sdhci_argument = val;
      break;
    case SDHCI_TRANSFER_MODE	:
      sdhci_transfer_mode = val;
      break;
    case SDHCI_POWER_CONTROL	:
      if (val & SDHCI_POWER_ON)
	{
	  sdhci_minion_hw_reset();
	  sd_reset(0,1,0,0);
	  get_card_status(0);
	  sd_align(0);
	  sd_reset(0,1,1,1);
	  switch (val & ~SDHCI_POWER_ON)
	    {
	    case SDHCI_POWER_180: printf("Power = 1.8V\n"); break;
	    case SDHCI_POWER_300: printf("Power = 3.0V\n"); break;
	    case SDHCI_POWER_330: printf("Power = 3.3V\n"); break;
	    }
	}
      else
	{
	printf("Power off\n"); 
	sd_reset(1,0,0,0);
	}
      sdhci_power_control = val;
      break;
    case SDHCI_COMMAND	        :
      sdhci_command = val;
      rslt = sd_transaction_finish(buf, sdhci_command);
      break;
    case SDHCI_BLOCK_GAP_CONTROL	: sdhci_block_gap = val; break;
    case SDHCI_WAKE_UP_CONTROL	: sdhci_wake_up = val; break;
    case SDHCI_TIMEOUT_CONTROL	:
      sdhci_timeout_control = 500000;
      if (sdhci_timeout_control < val) sdhci_timeout_control = val;
#ifdef SDHCI_VERBOSE3
      printf("Actual timeout control = %d\n", sdhci_timeout_control);
#endif
      break;
    case SDHCI_SOFTWARE_RESET	:
      sdhci_software_reset = val;
      sdhci_transfer_mode = 0;
      if (val & SDHCI_RESET_ALL) sdhci_minion_hw_reset();
      get_card_status(0);      
      break;
    case SDHCI_CLOCK_CONTROL	:
      sdhci_clock_div = val >> SDHCI_DIVIDER_SHIFT;
      if (sdhci_clock_div)
	{
	  printf("Trying clock div = %d\n", sdhci_clock_div);
	  if (sdhci_clock_div < 5) sdhci_clock_div = 5;
	  if (sdhci_clock_div > 255) sdhci_clock_div = 255;
	  printf("Actual clock divider = %d\n", sdhci_clock_div);
	  sd_clk_div(sdhci_clock_div*2);
	  sdhci_timeout_control = 500000;
#ifdef SDHCI_VERBOSE3
	  printf("Actual timeout control = %d\n", sdhci_timeout_control);
#endif
	  get_card_status(0);
	}
      if (val & SDHCI_CLOCK_CARD_EN)
	{
	  sd_reset(0,1,1,1);
	  printf("Card clock enabled\n");
	  get_card_status(0);
	}
      else
	{
	  sd_reset(0,0,1,1);
	  printf("Card clock disabled\n");
	  get_card_status(0);
	}
      break;
    case SDHCI_INT_STATUS	:
      sdhci_int_status = val;
      break;
    case SDHCI_INT_ENABLE	: sdhci_int_enable = val; break;
    case SDHCI_SIGNAL_ENABLE	: sdhci_signal_enable = val; break;
    case SDHCI_PRESENT_STATE	: sdhci_present_state = val; break;
    case SDHCI_MAX_CURRENT	: sdhci_max_current = val; break;
    case SDHCI_BUFFER           : tx_write_fifo(val); break;
    case SDHCI_SET_ACMD12_ERROR	: sdhci_set_acmd12_error = val; break;
    case SDHCI_SET_INT_ERROR	: sdhci_set_int = val; break;
    case SDHCI_HOST_VERSION	: sdhci_host_version = val; break;
    case SDHCI_HOST_CONTROL2    : sdhci_host_control2 = val; break;
    case SDHCI_ACMD12_ERR       : sdhci_acmd12_err = val; break;
    case SDHCI_SLOT_INT_STATUS  : sdhci_slot_int_status = val; break;
    default: printf("unknown(0x%x)", reg); rslt = -1;
    }
  return rslt;
}

uint32_t sdhci_read(int reg)
{
  uint32_t rslt = 0;
  switch (reg)
    {
    case SDHCI_DMA_ADDRESS       : rslt = sdhci_dma_address; break;
    case SDHCI_BLOCK_COUNT	 : rslt = sdhci_block_count; break;
    case SDHCI_BLOCK_SIZE	 : rslt = sdhci_block_size; break;
    case SDHCI_HOST_CONTROL      : rslt = sdhci_host_control; break;
    case SDHCI_ARGUMENT          : rslt = sdhci_argument; break;
    case SDHCI_TRANSFER_MODE	 : rslt = sdhci_transfer_mode; break;
    case SDHCI_COMMAND	         : rslt = sdhci_command; break;
    case SDHCI_RESPONSE          : rslt = card_status[0]; break;
    case SDHCI_RESPONSE+4        : rslt = card_status[1]; break;
    case SDHCI_RESPONSE+8        : rslt = card_status[2]; break;
    case SDHCI_RESPONSE+12       : rslt = card_status[3]; break;
    case SDHCI_INT_STATUS	 : rslt = sdhci_int_status; break;
    case SDHCI_INT_ENABLE	 : rslt = sdhci_int_enable; break;
    case SDHCI_PRESENT_STATE	 : 
      sdhci_present_state = card_status[12] ? 0 : SDHCI_CARD_PRESENT;
      rslt = sdhci_present_state; break;
    case SDHCI_HOST_VERSION	 : rslt = SDHCI_SPEC_300; break;
    case SDHCI_CAPABILITIES      :
      rslt = SDHCI_CAN_VDD_330|(25 << SDHCI_CLOCK_BASE_SHIFT)|SDHCI_CAN_DO_HISPD;
      break;
    case SDHCI_SOFTWARE_RESET    : rslt = 0; break;
    case SDHCI_BLOCK_GAP_CONTROL : rslt = sdhci_block_gap; break;
    case SDHCI_CLOCK_CONTROL     : rslt = (sdhci_clock_div << SDHCI_DIVIDER_SHIFT)|SDHCI_CLOCK_INT_STABLE; break;
    case SDHCI_BUFFER            : rslt = 0; break;
    case SDHCI_MAX_CURRENT       : rslt = 0; break;
    case SDHCI_POWER_CONTROL	 : rslt = sdhci_power_control; break;
    case SDHCI_WAKE_UP_CONTROL	 : rslt = sdhci_wake_up; break;
    case SDHCI_TIMEOUT_CONTROL	 : rslt = sdhci_timeout_control; break;
    case SDHCI_SIGNAL_ENABLE	 : rslt = sdhci_signal_enable; break;
    case SDHCI_SET_ACMD12_ERROR	 : rslt = sdhci_set_acmd12_error; break;
    case SDHCI_HOST_CONTROL2     : rslt = sdhci_host_control2; break;
    case SDHCI_ACMD12_ERR        : rslt = sdhci_acmd12_err; break;
    case SDHCI_CAPABILITIES_1    : rslt = MMC_CAP2_NO_SDIO; break;
    case SDHCI_SLOT_INT_STATUS   : rslt = SDHCI_INT_RESPONSE; break;
    default: printf("unknown(0x%x)", reg);
    }
#ifdef SDHCI_VERBOSE2
  if ((reg != SDHCI_PRESENT_STATE) && (reg != SDHCI_HOST_CONTROL))
    printf("sdhci_read(%s) => %x;\n", sdhci_kind(reg), rslt);
#endif  
  return rslt;
}

static struct bootstrap_host host;	

uint32_t sd_transaction_v(int sdcmd, uint32_t arg, uint32_t setting)
{
  static char iobuf[512];
  sd_cmd_start(0);
  sd_setting(0);
  sd_arg(arg);
  sd_cmd(sdcmd);
  sd_setting(setting);
  get_card_status(0);
  sd_cmd_start(1);
  sd_transaction_finish2(iobuf);
  get_card_status(0);
  sd_transaction_show();
  return card_status[0] & 0xFFFF0000U;
}

void sd_transaction_show(void)
{
int i;
  printf("CMD%d:", card_status[19]);
  myputhex(card_status[7], 4);
  myputchar(':');
  myputhex(card_status[6], 8);
  myputchar('-');
  myputchar('>');
  for (i = 4; i--; )
    {
      myputhex(card_status[i], 8);
      myputchar(',');
    }
  myputhex(card_status[5], 8);
  myputchar(',');
  myputhex(card_status[4], 8);
  myputchar('\n');
}

int sd_read_sector1(int sect, void *buf, int max)
{
  int rslt = 0;
#ifdef SDHCI_VERBOSE3
  printf("sd_read_sector1(%d)\n", sect);
#endif
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x00000200, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000101A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x000001F4, SDHCI_TIMEOUT_CONTROL);
sdhci_write(buf, 0x00000200, SDHCI_BLOCK_SIZE);
sdhci_write(buf, 0x00000001, SDHCI_BLOCK_COUNT);
sdhci_write(buf, 0x00000012, SDHCI_TRANSFER_MODE);
sdhci_write(buf, sect, SDHCI_ARGUMENT);
rslt = sdhci_write(buf, 0x0000113A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
 sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
 sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 return rslt;
}

int init_sd(void)
{
  u8 buf[512];
  int div = 255 << SDHCI_DIVIDER_SHIFT;
  int div2 = 12 << SDHCI_DIVIDER_SHIFT;
  size_t addr = 0;
  size_t addr2 = 1;
  int i, busy, rca, timeout = 0;
  get_card_status(0);
  if (card_status[12])
    {
    myputs("card slot is empty\n");
    return -1;
    }
int sdhci_capabilities = sdhci_read(SDHCI_CAPABILITIES);
int bootstrap_host_version = sdhci_read(SDHCI_HOST_VERSION);
sdhci_write(buf, 0x00000001, SDHCI_SOFTWARE_RESET);
int sdhci_software_reset = sdhci_read(SDHCI_SOFTWARE_RESET);
sdhci_write(buf, 0x0000000F, SDHCI_POWER_CONTROL);
sdhci_write(buf, 0x027F003B, SDHCI_INT_ENABLE);
sdhci_write(buf, 0x00000000, SDHCI_SIGNAL_ENABLE);
int bootstrap_host_control = sdhci_read(SDHCI_HOST_CONTROL);
sdhci_write(buf, 0x00000000, SDHCI_HOST_CONTROL);
int sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
int sdhci_clock_control = sdhci_read(SDHCI_CLOCK_CONTROL);
sdhci_write(buf, 0x00000002, SDHCI_CLOCK_CONTROL);
sdhci_write(buf, div | 0x001, SDHCI_CLOCK_CONTROL);
 sdhci_clock_control = sdhci_read(SDHCI_CLOCK_CONTROL);
sdhci_write(buf, div | 0x006, SDHCI_CLOCK_CONTROL);
 bootstrap_host_control = sdhci_read(SDHCI_HOST_CONTROL);
sdhci_write(buf, 0x00000000, SDHCI_HOST_CONTROL);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x00000000, SDHCI_ARGUMENT);
sdhci_write(buf, 0x00000000, SDHCI_COMMAND);
#if 0
 int sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
int cnt = queue_block_read(iobuf, 0);
#endif
 sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x000001AA, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000081A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
do {
   sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
   sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
   sdhci_write(buf, 0x00000000, SDHCI_ARGUMENT);
   sdhci_write(buf, 0x0000371A, SDHCI_COMMAND);
   sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
   sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
   sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
   sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
   sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
   sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
   sdhci_write(buf, 0x40300000, SDHCI_ARGUMENT);
   sdhci_write(buf, 0x00002903, SDHCI_COMMAND);
   sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
   sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
   sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
   sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
   busy = sd_resp(0) & 0xFFFF0000U;
   printf("busy = %x\n", busy);
 } while ((0x80000000U & ~busy) && (timeout++ < 100));

sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x00000000, SDHCI_ARGUMENT);
sdhci_write(buf, 0x00000209, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x00000000, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000031A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 rca = sd_resp(0) & 0xFFFF0000U;
 printf("RCA = %x\n", rca);
 sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, rca, SDHCI_ARGUMENT);
sdhci_write(buf, 0x00000909, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, rca, SDHCI_ARGUMENT);
sdhci_write(buf, 0x00000D1A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, rca, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000071A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, rca, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000371A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x000001F4, SDHCI_TIMEOUT_CONTROL);
sdhci_write(buf, 0x00000008, SDHCI_BLOCK_SIZE);
sdhci_write(buf, 0x00000001, SDHCI_BLOCK_COUNT);
sdhci_write(buf, 0x00000012, SDHCI_TRANSFER_MODE);
sdhci_write(buf, 0x00000000, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000333A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
#if 0
 cnt = queue_block_read(iobuf, 8);
#endif
 sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x000001F4, SDHCI_TIMEOUT_CONTROL);
sdhci_write(buf, 0x00000040, SDHCI_BLOCK_SIZE);
sdhci_write(buf, 0x00000001, SDHCI_BLOCK_COUNT);
sdhci_write(buf, 0x00000012, SDHCI_TRANSFER_MODE);
sdhci_write(buf, 0x00FFFFF1, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000063A, SDHCI_COMMAND);
#if 0
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
cnt = queue_block_read(iobuf, 64);
 #endif
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x000001F4, SDHCI_TIMEOUT_CONTROL);
sdhci_write(buf, 0x00000040, SDHCI_BLOCK_SIZE);
sdhci_write(buf, 0x00000001, SDHCI_BLOCK_COUNT);
sdhci_write(buf, 0x00000012, SDHCI_TRANSFER_MODE);
sdhci_write(buf, 0x80FFFFF1, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000063A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
#if 0
 cnt = queue_block_read(iobuf, 64);
 #endif
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, rca, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000371A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x00000002, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000061A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 bootstrap_host_control = sdhci_read(SDHCI_HOST_CONTROL);
sdhci_write(buf, 0x00000002, SDHCI_HOST_CONTROL);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, rca, SDHCI_ARGUMENT);
sdhci_write(buf, 0x0000371A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
sdhci_write(buf, 0x000001F4, SDHCI_TIMEOUT_CONTROL);
sdhci_write(buf, 0x00000040, SDHCI_BLOCK_SIZE);
sdhci_write(buf, 0x00000001, SDHCI_BLOCK_COUNT);
sdhci_write(buf, 0x00000012, SDHCI_TRANSFER_MODE);
sdhci_write(buf, 0x00000000, SDHCI_ARGUMENT);
sdhci_write(buf, 0x00000D3A, SDHCI_COMMAND);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0x00000001, SDHCI_INT_STATUS);
 sdhci_int_status = sdhci_read(SDHCI_INT_STATUS);
sdhci_write(buf, 0xFFFFFFFF, SDHCI_INT_STATUS);
 sdhci_present_state = sdhci_read(SDHCI_PRESENT_STATE);
 sdhci_clock_control = sdhci_read(SDHCI_CLOCK_CONTROL);
sdhci_write(buf, div2 | 0x002, SDHCI_CLOCK_CONTROL);
sdhci_write(buf, div2 | 0x001, SDHCI_CLOCK_CONTROL);
 sdhci_clock_control = sdhci_read(SDHCI_CLOCK_CONTROL);
sdhci_write(buf, div2 | 0x006, SDHCI_CLOCK_CONTROL);
 bootstrap_host_control = sdhci_read(SDHCI_HOST_CONTROL);
sdhci_write(buf, 0x00000002, SDHCI_HOST_CONTROL);
 return 0;
}

uint8_t send_cmd (uint8_t cmd, uint32_t arg, uint32_t flag)
{
  return -1;
}

int rcvr_datablock (uint8_t *buff,uint32_t btr)
{
  return 0;
}

int xmit_datablock (const uint8_t *buff, uint8_t token)
{
  return 0;
}
