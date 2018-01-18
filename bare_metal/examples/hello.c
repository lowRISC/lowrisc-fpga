// A hello world program

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "hid.h"
#include "mini-printf.h"

int main() {
  hid_send_string("\nBare metal HID access\n");
  printf("Hello World! "__DATE__" "__TIME__"\n");
}

void external_interrupt(void)
{
  int i, claim, handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif  
}
