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
  if (addr_int >= LOWRISC_MEM)
    {
      // this is where we scroll
      for (addr_int = 0; addr_int < LOWRISC_MEM; addr_int++)
        if (addr_int < LOWRISC_MEM-128)
          hid_vga_ptr[addr_int] = hid_vga_ptr[addr_int+128];
        else
          hid_vga_ptr[addr_int] = blank;
      addr_int = LOWRISC_MEM-128;
    }
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_XCUR] = addr_int & 127;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_YCUR] = addr_int >> 7;
}

void uart_console_putchar(unsigned char ch)
{
  while (uart_base[0] & 0x400)
    ;
  uart_base[0] = ch;
}  

void hid_init(void *base)
{
  enum {width=1024, height=768};
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_SCROLL] = 0;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_CURSV] = 10;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_XCUR] = 0;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_YCUR] = 32;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_HSTART] = width*2;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_HSYN] = width*2+20;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_HSTOP] = width*2+51;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VSTART] = height;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VSTOP] = height+19;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VBLSTOP] = 16;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VBLSTART ] = height+16;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VPIXSTART ] = 16;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VPIXSTOP ] = height+16;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_HPIXSTART ] = 128*3;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_HPIXSTOP ] = 128*3+256*6;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_HPIXREG ] = 5;
  hid_vga_ptr[LOWRISC_REGS+LOWRISC_REGS_VPIXREG ] = 11; // squashed vertical display uses 10
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
  return -1;
}

// check hid IRQ for read
uint8_t hid_check_read_irq() {
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
