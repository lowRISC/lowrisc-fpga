// See LICENSE for license details.

#include "uart.h"
#include "minion_lib.h"

volatile uint32_t *uart_base_ptr = (uint32_t *)(UART_BASE);

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
  static int addr_int = 0;
  volatile uint32_t * const video_base = (volatile uint32_t*)(10<<20);
  switch (ch)
    {
    case '\b':
      if (addr_int & 127) addr_int--;
      break;
    case '\r':
      break;
    case '\n':
      while ((addr_int & 127) < 127)
	queue_write(&(video_base[addr_int++]), ' ', 1);
      ++addr_int;
      break;
    default:
      queue_write(&video_base[addr_int++], ch, 0);
      break;
    }
  addr_int &= 4095;
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

uint8_t uart_recv() {
  // wait until RBR has data
  while(! (*(uart_base_ptr + UART_LSR) & 0x01u))
    {
      uint32_t key;
      volatile uint32_t * const keyb_base = (volatile uint32_t*)(9<<20);
      key = queue_read(keyb_base);
      if ((1<<28) & ~key) /* FIFO not empty */
	{
	  int ch;
	  queue_write(keyb_base+1, 0, 0);
	  ch = (queue_read(keyb_base+1) >> 8) & 127; /* strip off the scan code (default ascii code is UK) */
	  if (ch == '\r') ch = '\n'; /* translate CR to LF, because nobody else will */
	  return ch;
	}
    }
  return *(uart_base_ptr + UART_RBR);
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
