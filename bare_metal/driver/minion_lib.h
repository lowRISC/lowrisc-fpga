// See LICENSE for license details.

#ifndef MINION_HEADER_H
#define MINION_HEADER_H

#include <stddef.h>

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
typedef __u32 le32;
typedef __u16 le16;
typedef __u64 u64;
typedef __u32 u32;
typedef __s32 s32;
typedef __u16 le16;
typedef __u16 u16;
typedef __u8 u8;
typedef __s8 s8;
// typedef int bool;

// MINION_LIB APIs
extern int echo;
void write_led(uint32_t data);
void myputhex(unsigned n, unsigned width);
void sd_setting(int setting);
void sd_cmd_start(int sd_cmd);

extern int printf (const char *, ...);
extern void uart_send (uint8_t);
extern void uart_send_string (const char *);
extern void uart_send_buf (const char *, const int32_t);
extern uint8_t uart_recv (void);
extern uint8_t uart_read_irq (void);
extern uint8_t uart_check_read_irq (void);
extern void uart_enable_read_irq (void);
extern void uart_disable_read_irq (void);
extern void uart_send (uint8_t data);
extern void uart_send_buf (const char *buf, const const int32_t len);
extern uint8_t uart_recv (void);
extern void uart_send_string (const char *str);
extern void cpu_perf_set (unsigned int counterId, unsigned int value);
extern void illegal_insn_handler_c (void);
extern void int_time_cmp (void);
extern void int_main (void);
extern void uart_set_cfg (int parity, uint16_t clk_counter);
extern void __libc_init_array (void);
extern char uart_getchar (void);
extern void uart_wait_tx_done (void);
extern void uart_sendchar (const const char c);
extern void mystatus (void);
// SDCARD entry point
void spi_init(void);
unsigned sd_transaction_v(int sdcmd, unsigned arg, unsigned setting);
int sd_transaction(unsigned read, unsigned val, unsigned resp[], unsigned iobuf[], unsigned iobuflen);
int mysleep(int delay);
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
int sd_read_sector(int, void *, int);
void open_handle(void);
void uart_printf(const char *fmt, ...);
void log_printf(const char *fmt, ...);
void uart_write(volatile unsigned int * const sd_ptr, unsigned val);
int cli_readline_into_buffer(const char *const prompt, char *buffer, int timeout);

  int edcl_main(void);
  void edcl_loadelf(const char *elf);
  void edcl_close(void);
  int edcl_read(uint64_t addr, int bytes, uint8_t *obuf);
  int edcl_write(uint64_t addr, int bytes, uint8_t *ibuf);

/*
 * Controller registers
 */

#define MINION_UART_DMA_ADDRESS	0x00

#define MINION_UART_BLOCK_SIZE	0x04

#define MINION_UART_BLOCK_COUNT	0x06

#define MINION_UART_ARGUMENT		0x08

#define MINION_UART_TRANSFER_MODE	0x0C
#define  MINION_UART_TRNS_DMA		0x01
#define  MINION_UART_TRNS_BLK_CNT_EN	0x02
#define  MINION_UART_TRNS_ACMD12	0x04
#define  MINION_UART_TRNS_READ	0x10
#define  MINION_UART_TRNS_MULTI	0x20

#define MINION_UART_COMMAND		0x0E
#define  MINION_UART_CMD_RESP_MASK	0x03
#define  MINION_UART_CMD_CRC		0x08
#define  MINION_UART_CMD_INDEX	0x10
#define  MINION_UART_CMD_DATA		0x20
#define  MINION_UART_CMD_ABORTCMD	0xC0

#define  MINION_UART_CMD_RESP_NONE	0x00
#define  MINION_UART_CMD_RESP_LONG	0x01
#define  MINION_UART_CMD_RESP_SHORT	0x02
#define  MINION_UART_CMD_RESP_SHORT_BUSY 0x03

#define MINION_UART_MAKE_CMD(c, f) (((c & 0xff) << 8) | (f & 0xff))
#define MINION_UART_GET_CMD(c) ((c>>8) & 0x3f)

#define MINION_UART_RESPONSE		0x10

#define MINION_UART_BUFFER		0x20

#define MINION_UART_PRESENT_STATE	0x24
#define  MINION_UART_CMD_INHIBIT	0x00000001
#define  MINION_UART_DATA_INHIBIT	0x00000002
#define  MINION_UART_DOING_WRITE	0x00000100
#define  MINION_UART_DOING_READ	0x00000200
#define  MINION_UART_SPACE_AVAILABLE	0x00000400
#define  MINION_UART_DATA_AVAILABLE	0x00000800
#define  MINION_UART_CARD_PRESENT	0x00010000
#define  MINION_UART_CARD_STATE_STABLE	0x00020000
#define  MINION_UART_CARD_DETECT_PIN_LEVEL	0x00040000
#define  MINION_UART_WRITE_PROTECT	0x00080000

#define MINION_UART_HOST_CONTROL	0x28
#define  MINION_UART_POWER_ON		0x01
#define  MINION_UART_CTRL_LED		0x01
#define  MINION_UART_CTRL_4BITBUS	0x02
#define  MINION_UART_CTRL_HISPD	0x04
#define  MINION_UART_CTRL_DMA_MASK	0x18
#define   MINION_UART_CTRL_SDMA	0x00
#define   MINION_UART_CTRL_ADMA1	0x08
#define   MINION_UART_CTRL_ADMA32	0x10
#define   MINION_UART_CTRL_ADMA64	0x18
#define  MINION_UART_CTRL_8BITBUS	0x20
#define  MINION_UART_CTRL_CD_TEST_INS	0x40
#define  MINION_UART_CTRL_CD_TEST	0x80

#define MINION_UART_POWER_CONTROL	0x29
#define  MINION_UART_POWER_180	0x0A
#define  MINION_UART_POWER_300	0x0C
#define  MINION_UART_POWER_330	0x0E

#define MINION_UART_BLOCK_GAP_CONTROL	0x2A
#define MINION_UART_WAKE_UP_CONTROL	0x2B
#define MINION_UART_TIMEOUT_CONTROL	0x2E
#define MINION_UART_SOFTWARE_RESET	0x2F

#define  MINION_UART_WAKE_ON_INT	0x01
#define  MINION_UART_WAKE_ON_INSERT	0x02
#define  MINION_UART_WAKE_ON_REMOVE	0x04

#define MINION_UART_CLOCK_CONTROL	0x2C
#define  MINION_UART_DIVIDER_SHIFT	8
#define  MINION_UART_DIV_MASK	        0xFFFFFF
#define  MINION_UART_CLOCK_CARD_EN	0x0004
#define  MINION_UART_CLOCK_INT_STABLE	0x0002
#define  MINION_UART_CLOCK_INT_EN	0x0001

#define  MINION_UART_RESET_ALL	0x01
#define  MINION_UART_RESET_CMD	0x02
#define  MINION_UART_RESET_DATA	0x04

#define MINION_UART_INT_STATUS	0x30
#define MINION_UART_INT_ENABLE	0x34
#define MINION_UART_SIGNAL_ENABLE	0x38
#define  MINION_UART_INT_RESPONSE	0x00000001
#define  MINION_UART_INT_DATA_END	0x00000002
#define  MINION_UART_INT_DMA_END	0x00000008
#define  MINION_UART_INT_SPACE_AVAIL	0x00000010
#define  MINION_UART_INT_DATA_AVAIL	0x00000020
#define  MINION_UART_INT_CARD_INSERT	0x00000040
#define  MINION_UART_INT_CARD_REMOVE	0x00000080
#define  MINION_UART_INT_CARD_INT	0x00000100
#define  MINION_UART_INT_ERROR	0x00008000
#define  MINION_UART_INT_TIMEOUT	0x00010000
#define  MINION_UART_INT_CRC		0x00020000
#define  MINION_UART_INT_END_BIT	0x00040000
#define  MINION_UART_INT_INDEX	0x00080000
#define  MINION_UART_INT_DATA_TIMEOUT	0x00100000
#define  MINION_UART_INT_DATA_CRC	0x00200000
#define  MINION_UART_INT_DATA_END_BIT	0x00400000
#define  MINION_UART_INT_BUS_POWER	0x00800000
#define  MINION_UART_INT_ACMD12ERR	0x01000000
#define  MINION_UART_INT_ADMA_ERROR	0x02000000

#define  MINION_UART_INT_NORMAL_MASK	0x00007FFF
#define  MINION_UART_INT_ERROR_MASK	0xFFFF8000

#define  MINION_UART_INT_CMD_MASK	(MINION_UART_INT_RESPONSE | MINION_UART_INT_TIMEOUT | \
		MINION_UART_INT_CRC | MINION_UART_INT_END_BIT | MINION_UART_INT_INDEX)
#define  MINION_UART_INT_DATA_MASK	(MINION_UART_INT_DATA_END | MINION_UART_INT_DMA_END | \
		MINION_UART_INT_DATA_AVAIL | MINION_UART_INT_SPACE_AVAIL | \
		MINION_UART_INT_DATA_TIMEOUT | MINION_UART_INT_DATA_CRC | \
		MINION_UART_INT_DATA_END_BIT | MINION_UART_INT_ADMA_ERROR)
#define MINION_UART_INT_ALL_MASK	((unsigned int)-1)

#define MINION_UART_ACMD12_ERR	0x3C

/* 3E-3F reserved */

#define MINION_UART_CAPABILITIES	0x40
#define  MINION_UART_TIMEOUT_CLK_MASK	0x0000003F
#define  MINION_UART_TIMEOUT_CLK_SHIFT 0
#define  MINION_UART_TIMEOUT_CLK_UNIT	0x00000080
#define  MINION_UART_CLOCK_BASE_MASK	0x00003F00
#define  MINION_UART_CLOCK_V3_BASE_MASK	0x0000FF00
#define  MINION_UART_CLOCK_BASE_SHIFT	8
#define  MINION_UART_MAX_BLOCK_MASK	0x00030000
#define  MINION_UART_MAX_BLOCK_SHIFT  16
#define  MINION_UART_CAN_DO_8BIT	0x00040000
#define  MINION_UART_CAN_DO_ADMA2	0x00080000
#define  MINION_UART_CAN_DO_ADMA1	0x00100000
#define  MINION_UART_CAN_DO_HISPD	0x00200000
#define  MINION_UART_CAN_DO_SDMA	0x00400000
#define  MINION_UART_CAN_VDD_330	0x01000000
#define  MINION_UART_CAN_VDD_300	0x02000000
#define  MINION_UART_CAN_VDD_180	0x04000000
#define  MINION_UART_CAN_64BIT	0x10000000

#define MINION_UART_CAPABILITIES_1	0x44
#define  MINION_UART_CLOCK_MUL_MASK	0x00FF0000
#define  MINION_UART_CLOCK_MUL_SHIFT	16

#define MINION_UART_MAX_CURRENT	0x48

/* 4C-4F reserved for more max current */

#define MINION_UART_SET_ACMD12_ERROR	0x50
#define MINION_UART_SET_INT_ERROR	0x52

#define MINION_UART_ADMA_ERROR	0x54

/* 55-57 reserved */

#define MINION_UART_ADMA_ADDRESS	0x58

/* 60-FB reserved */

#define MINION_UART_SLOT_INT_STATUS	0xFC

#define MINION_UART_HOST_VERSION	0xFE
#define  MINION_UART_VENDOR_VER_MASK	0xFF00
#define  MINION_UART_VENDOR_VER_SHIFT	8
#define  MINION_UART_SPEC_VER_MASK	0x00FF
#define  MINION_UART_SPEC_VER_SHIFT	0
#define   MINION_UART_SPEC_100	0
#define   MINION_UART_SPEC_200	1
#define   MINION_UART_SPEC_300	2

#define MINION_UART_GET_VERSION(x) (x->version & MINION_UART_SPEC_VER_MASK)

/*
 * End of controller registers.
 */

#define MINION_UART_MAX_DIV_SPEC_200	256
#define MINION_UART_MAX_DIV_SPEC_300	2046

typedef unsigned int uint;

struct minion_uart_host {
	const char *name;
	unsigned int quirks;
	unsigned int host_caps;
	unsigned int version;
	unsigned int clock;
	struct mmc *mmc;
	const struct minion_uart_ops *ops;
	int index;
	int bus_width;
	uint	voltages;
        uint32_t *start_addr;
};

void myputchar(char ch);
void myputs(const char *str);
void minion_uart_write(struct minion_uart_host *host, uint32_t val, int reg);
uint32_t minion_uart_read(struct minion_uart_host *host, int reg);
void minion_uart_reset(struct minion_uart_host *host, uint8_t mask);
//void minion_uart_cmd_done(struct minion_uart_host *host, uint resp_type, uint cmd_response[]);
//int sd_flush(unsigned iobuf[], unsigned iobuflen, unsigned trans);
unsigned sd_transaction_v(int sdcmd, unsigned arg, unsigned setting);
void minion_dispatch(const char *ucmd);
void sd_cmd_setting(int cmd_flags);
void sd_reset(int sd_rst, int clk_rst, int data_rst, int cmd_rst);
void sd_arg(unsigned arg);
void sd_align(int d_align);
void sd_clk_div(int clk_div);
void sd_cmd(unsigned cmd);
void board_mmc_power_init(void);
int init_sd(void);

/*
 * Host SDMA buffer boundary. Valid values from 4K to 512K in powers of 2.
 */
#define MINION_UART_DEFAULT_BOUNDARY_SIZE	(512 * 1024)
#define MINION_UART_DEFAULT_BOUNDARY_ARG	(7)
struct minion_uart_ops {
#ifdef CONFIG_MMC_MINION_UART_IO_ACCESSORS
	uint32_t             (*read_l)(struct minion_uart_host *host, int reg);
	uint16_t             (*read_w)(struct minion_uart_host *host, int reg);
	uint8_t              (*read_b)(struct minion_uart_host *host, int reg);
	void            (*write_l)(struct minion_uart_host *host, uint32_t val, int reg);
	void            (*write_w)(struct minion_uart_host *host, uint16_t val, int reg);
	void            (*write_b)(struct minion_uart_host *host, uint8_t val, int reg);
#endif
};

/*
 * No command will be sent by driver if card is busy, so driver must wait
 * for card ready state.
 * Every time when card is busy after timeout then (last) timeout value will be
 * increased twice but only if it doesn't exceed global defined maximum.
 * Each function call will use last timeout value.
 */
#define MINION_UART_CMD_MAX_TIMEOUT			3200
#define MINION_UART_CMD_DEFAULT_TIMEOUT		100
#define MINION_UART_READ_STATUS_TIMEOUT		1000
#endif

#define get_card_status(_verbose) _get_card_status(__LINE__, _verbose)

extern uint32_t card_status[32];
extern int sd_read_sector1(int sect, void *buf, int max);
extern void card_response(void);
extern void _get_card_status(int line, int verbose);
extern void sd_transaction_show(void);
extern int sd_transaction_finish2(void *buf);
extern u8 *minion_iobuf(int sect);
extern void show_sector(u8 *buf);
extern void myhash(size_t addr);
extern uint8_t *hash_buf(const void *in_buf, int count);
extern int minion_cache_map(int sect, int clr);
extern uint32_t sd_resp(int sel);
extern int rand (void);
extern unsigned int rand32(void);
extern uint64_t rand64(void);

