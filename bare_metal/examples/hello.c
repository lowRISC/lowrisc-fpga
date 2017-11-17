// A hello world program

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "uart.h"
#include "mini-printf.h"

int main() {
  uart_send_string("\nBare metal UART access\n");
  printf("Hello World! "__DATE__" "__TIME__"\n");
}

