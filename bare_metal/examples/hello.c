// A hello world program

#include <stdio.h>
#include "uart.h"

int main() {
  int delay = 1000000;
  uart_init();
  uart_send_irq('*');
  uart_send_string("Hello World!\n");
  while (delay--)
    ;
}

