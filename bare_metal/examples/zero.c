#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "memory.h"
#include "hid.h"
#include "mini-printf.h"

int main()
{
    hid_send_string("\nZero DRAM begin\n");
    memset((void *) 0x80000000, 0, 0x8000000);
    hid_send_string("\nZero DRAM end\n");
}

void external_interrupt(void)
{
  int i, claim, handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif  
}
