// See LICENSE.Cambridge for license details.
// Ethernet loader program main function
#include "encoding.h"
#include "bits.h"
#include "elfriscv.h"
#include "hid.h"
#include "lowrisc_memory_map.h"
#include "eth.h"
#include "mini-printf.h"
#include "minion_lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sdhci-minion-hash-md5.h"

int main()
{
  int i, sw = sd_base[31];
  loopback_test(8, (sw & 0xF) == 0xF);

  hid_send_string("lowRISC etherboot program\n=====================================\n");

  for (i = 10000000; i; i--)
    write_led(i);
  eth_main();
}
