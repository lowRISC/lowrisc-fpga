// SD test program

#include <stdio.h>
#include "diskio.h"
#include "ff.h"
#include "uart.h"
#include "trace.h"

/* Read a text file and display it */

FATFS FatFs;   /* Work area (file system object) for logical drive */

void mygets(char *cmd)
{
  int ch;
  char *chp = cmd;
  do
    {
      ch = uart_recv();
      uart_send(ch);
      *chp++ = ch;
    }
  while (ch != '\n');
  *--chp = 0;
}

void board_mmc_power_init(void);
void minion_dispatch(const char *ucmd);

int main (void)
{
  FIL fil;                /* File object */
  uint8_t buffer[64];     /* File copy buffer */
  FRESULT fr;             /* FatFs return code */
  uint32_t br;            /* Read count */
  uint32_t i;

  STM_TRACE(0x1234, 0xdeadbeef);

  uart_init();
#if 1
  static char linbuf[80];
  board_mmc_power_init();
  
  do {
    uart_send_string("sdcard> ");
    mygets(linbuf);
    minion_dispatch(linbuf);
  } while (*linbuf != 'q');
#endif  
  /* Register work area to the default drive */
  if(f_mount(&FatFs, "", 1)) {
    printf("Fail to mount SD driver!\n");
    return 1;
  }

  /* Open a text file */
  fr = f_open(&fil, "test.txt", FA_READ);
  if (fr) {
    printf("failed to open test.txt!\n");
    return (int)fr;
  } else {
    printf("test.txt opened\n");
  }

  /* Read all lines and display it */
  uint32_t fsize = 0;
  for (;;) {
    fr = f_read(&fil, buffer, sizeof(buffer)-1, &br);  /* Read a chunk of source file */
    if (fr || br == 0) break; /* error or eof */
    buffer[br] = 0;
    printf("%s", buffer);
    fsize += br;
  }

  printf("file size %d\n", fsize);

  /* Close the file */
  if(f_close(&fil)) {
    printf("fail to close file!");
    return 1;
  }
  if(f_mount(NULL, "", 1)) {         /* unmount it */
    printf("fail to umount disk!");
    return 1;
  }

  printf("test.txt closed.\n");

  return 0;
}
