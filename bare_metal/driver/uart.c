// See LICENSE for license details.

#include "uart.h"
extern int printf (const char *, ...);

volatile uint32_t *uart_base_ptr = (uint32_t *)(UART_BASE);
volatile uint32_t *hid_base_ptr = (uint32_t *)(DEV_MAP__io_ext_hid__BASE);

void uart_init() {
  // set 0x0080 to UART.LCR to enable DLL and DLM write
  // configure baud rate
  *(uart_base_ptr + UART_LCR) = 0x0080;

  // System clock 25 MHz, 115200 baud rate
  // divisor = clk_freq / (16 * Baud)
  *(uart_base_ptr + UART_DLL) = 25*1000*1000u / (16u * 115200u) % 0x100u;
  *(uart_base_ptr + UART_DLM) = 25*1000*1000u / (16u * 115200u) >> 8;

  // 8-bit data, 1-bit odd parity
  *(uart_base_ptr + UART_LCR) = 0x000Bu;

  // Enable read IRQ
  *(uart_base_ptr + UART_IER) = 0x0001u;

}

void minion_console_putchar(unsigned char ch)
{
#ifdef DEV_MAP__io_ext_hid__BASE
  static int addr_int = 4096-256;
  volatile uint32_t * const vid_base = (volatile uint32_t*)(DEV_MAP__io_ext_hid__BASE+0x8000);
  switch(ch)
    {
    case 8: case 127: if (addr_int & 127) --addr_int; break;
    case 13: addr_int = addr_int & -128; break;
    case 10: addr_int = (addr_int|127)+1; break;
    default: vid_base[addr_int++] = ch;
    }
  if (addr_int >= 4096-128)
    {
      // this is where we scroll
      for (addr_int = 0; addr_int < 4096; addr_int++)
        if (addr_int < 4096-128)
          vid_base[addr_int] = vid_base[addr_int+128];
        else
          vid_base[addr_int] = ' ';
      addr_int = 4096-256;
    }
#endif  
}

void uart_send(uint8_t data) {
  // wait until THR empty
  while(! (*(uart_base_ptr + UART_LSR) & 0x40u));
  *(uart_base_ptr + UART_THR) = data;
  minion_console_putchar(data);
}

void uart_send_string(const char *str) {
  while (*str) uart_send(*str++);
}

void uart_send_buf(const char *buf, const int32_t len) {
  int32_t i;
  for (i=0; i<len; i++) uart_send(buf[i]);
}

uint8_t cr2lf(uint8_t ch)
{
  if (ch == '\r') ch = '\n'; /* translate CR to LF, because nobody else will */
  return ch;
}

uint8_t uart_recv() {
  // wait until RBR has data
  while(! (*(uart_base_ptr + UART_LSR) & 0x01u))
    {
#ifdef DEV_MAP__io_ext_hid__BASE
      volatile uint32_t * const keyb_base = (volatile uint32_t*)(DEV_MAP__io_ext_hid__BASE+0x0000);
      uint32_t key = keyb_base[0];
      if ((1<<16) & ~key) /* FIFO not empty */
	{
	  int ch;
	  *keyb_base = 0;
	  ch = (*keyb_base >> 8) & 127; /* strip off the scan code (default ascii code is UK) */
          return cr2lf(ch);
	}
#endif      
    }
  return cr2lf(*(uart_base_ptr + UART_RBR));
}

// IRQ triggered read
uint8_t uart_read_irq() {
  return *(uart_base_ptr + UART_RBR);
}

// check uart IRQ for read
uint8_t uart_check_read_irq() {
  return (*(uart_base_ptr + UART_LSR) & 0x01u);
}

// enable uart read IRQ
void uart_enable_read_irq() {
  *(uart_base_ptr + UART_IER) = 0x0001u;
}

// disable uart read IRQ
void uart_disable_read_irq() {
  *(uart_base_ptr + UART_IER) = 0x0000u;
}
