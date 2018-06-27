// A hello world program

#include <stdio.h>
#include <stdint.h>
#include "lowrisc_memory_map.h"
#include "hid.h"
#include "mini-printf.h"

const struct { char scan,lwr,upr; } scancode[] = {
#include "scancode.h"
  };

int main() {
  volatile uint32_t *keyb = (volatile uint32_t *)keyb_base_addr;
  hid_send_string("\nBare metal HID access\n");
  printf("Hello World! "__DATE__" "__TIME__"\n");
  for (;;)
    {
    int event = *keyb;
    if (0x200 & ~event)
      {
        *keyb = 0; // pop FIFO
        event = *keyb & ~0x200;
        printf("Keyboard event = %X, scancode = %X, ascii = '%c'\n", event, scancode[event&~0x100].scan, scancode[event&~0x100].lwr);
      }
    }
}

void external_interrupt(void)
{
  int i, claim, handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif  
}
