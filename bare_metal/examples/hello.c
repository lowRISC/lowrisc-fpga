// A hello world program

#include <stdio.h>
#include "uart.h"

int main() {
  uart_init();
  printf("Hello World!\n");
}

