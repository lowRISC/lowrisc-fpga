// See LICENSE for license details.

#include "hid.h"

volatile uint32_t *hid_base_ptr;

void hid_console_putchar(unsigned char ch)
{
  static int addr_int = 4096-256;
  switch(ch)
    {
    case 8: case 127: if (addr_int & 127) --addr_int; break;
    case 13: addr_int = addr_int & -128; break;
    case 10: addr_int = (addr_int|127)+1; break;
    default: hid_base_ptr[HID_VGA+addr_int++] = ch;
    }
  if (addr_int >= 4096-128)
    {
      // this is where we scroll
      for (addr_int = 0; addr_int < 4096; addr_int++)
        if (addr_int < 4096-128)
          hid_base_ptr[HID_VGA+addr_int] = hid_base_ptr[HID_VGA+addr_int+128];
        else
          hid_base_ptr[HID_VGA+addr_int] = ' ';
      addr_int = 4096-256;
    }
}

void hid_init(void *base) {
    hid_base_ptr = (uint32_t *)base;
}

void hid_send_irq(uint8_t data) {

}

void hid_send(uint8_t data) {
  hid_console_putchar(data);
    *(hid_base_ptr + HID_LED) = data;
}

void hid_send_string(const char *str) {
  while (*str) hid_send(*str++);
}

void hid_send_buf(const char *buf, const int32_t len) {
  int32_t i;
  for (i=0; i<len; i++) hid_send(buf[i]);
}

uint8_t hid_recv() {
  return *(hid_base_ptr + HID_DIP);
}

// IRQ triggered read
uint8_t hid_read_irq() {
  return *(hid_base_ptr + HID_DIP);
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
