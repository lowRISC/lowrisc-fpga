// See LICENSE for license details.

#ifndef HID_HEADER_H
#define HID_HEADER_H

#include <stdint.h>

#define HID_VGA 0x2000
#define HID_LED 0x400F
#define HID_DIP 0x401F

extern void hid_init(void *base);
extern void hid_send(uint8_t);
extern void hid_send_irq(uint8_t);
extern void hid_send_string(const char *str);
extern void hid_send_buf(const char *buf, const int32_t len);
extern uint8_t hid_recv();
extern uint8_t hid_read_irq();
extern uint8_t hid_check_read_irq();
extern void hid_enable_read_irq();
extern void hid_disable_read_irq();

#endif
