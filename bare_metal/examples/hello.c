// A hello world program

#include <stdio.h>
#include "uart.h"
#include "mini-printf.h"

int main() {
  uart_init();
  uart_send_string("\nBare metal UART access\n");
  mini_printf("Hello World! "__DATE__" "__TIME__"\n");
  uart_drain();
}

