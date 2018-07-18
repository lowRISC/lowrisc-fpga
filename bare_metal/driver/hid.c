// See LICENSE for license details.

#include "hid.h"
#include "lowrisc_memory_map.h"

static int addr_int = 0;

void hid_console_putchar(unsigned char ch)
{
  int lmt, blank = ' '|0xF00;
  switch(ch)
    {
    case 8: case 127: if (addr_int & 127) hid_vga_ptr[--addr_int] = blank; break;
    case 13: addr_int = addr_int & -128; break;
    case 10:
      {
        int lmt = (addr_int|127)+1; while (addr_int < lmt) hid_vga_ptr[(addr_int++)] = blank;
        break;
      }
    default: hid_vga_ptr[addr_int++] = ch|0xF00;
    }
  if (addr_int >= 4096-128)
    {
      // this is where we scroll
      for (addr_int = 0; addr_int < 4096; addr_int++)
        if (addr_int < 4096-128)
          hid_vga_ptr[addr_int] = hid_vga_ptr[addr_int+128];
        else
          hid_vga_ptr[addr_int] = blank;
      addr_int = 4096-256;
    }
}

void uart_console_putchar(unsigned char ch)
{
  while (uart_base[0] & 0x400)
    ;
  uart_base[0] = ch;
}  

void hid_init(void *base)
{

}

void hid_send_irq(uint8_t data)
{

}

void hid_send(uint8_t data)
{
  uart_console_putchar(data);
  hid_console_putchar(data);
}

void hid_send_string(const char *str) {
  while (*str) hid_send(*str++);
}

void hid_send_buf(const char *buf, const int32_t len)
{
  int32_t i;
  for (i=0; i<len; i++) hid_send(buf[i]);
}

uint8_t hid_recv()
{
  return -1;
}

// IRQ triggered read
uint8_t hid_read_irq() {
  int ch = 0;
  do {
    if (hid_check_read_irq())
      {
        uart_base[0x200] = 0;
        ch = uart_base[0] & 0x7f;
      }
  } while (!ch);
  return ch;
}

// check hid IRQ for read
uint8_t hid_check_read_irq() {
  int retval, rxwrcnt, rxrdcnt;
  uint64_t stat = uart_base[0];
  uint64_t counts = uart_base[0x400];
  rxrdcnt = counts & 0x7ff;
  rxwrcnt = (counts >> 16) & 0x7ff;
  retval = rxrdcnt != rxwrcnt;
  
  return 0;
}

// enable hid read IRQ
void hid_enable_read_irq() {

}

// disable hid read IRQ
void hid_disable_read_irq() {

}

#undef putc
int putc(int c, __FILE *ptr) {
  hid_send(c);
}

int puts(const char *str) {
  while (*str) hid_send(*str++);
  hid_send('\n');
  return 0;
}
