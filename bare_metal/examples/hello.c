// A hello world program

#include <stdio.h>
#include "uart.h"
#include "dev_map.h"

static volatile uint32_t *sd_base = (uint32_t *)(DEV_MAP__io_ext_hid__BASE + 0x00010000);

static void write_led(uint32_t data)
{
  sd_base[15] = data;
}

static uint32_t sw_resp(void)
{
  return sd_base[31];
}

int main() {
  int sw;
  uart_init();
  printf("Hello World!\n");
  for(;;)
	{
	sw = sw_resp();
	write_led(sw);
	}
}

void external_interrupt(void)
{
  int handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif
  if (!handled)
    {
      printf("unhandled interrupt!\n");
    }
}
