#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

static unsigned short csum(unsigned short *buf, int nwords)
    {       //
            unsigned long sum;
            for(sum=0; nwords>0; nwords--)
              {
                unsigned short src = *buf++;
                sum += src;
              }
            sum = (sum >> 16) + (sum & 0xffff);
            sum += (sum >> 16);
            return (unsigned short)(~sum);
    }

int main(int argc, char **argv)
{
  char str[99];
  int i, hex, cnt2, cnt = 0;
  unsigned char array[2048];
  unsigned short crc[2048];
  FILE *fd = fopen(argv[1], "w");
  while (scanf("%s", str)==1)
    {
      if ((strlen(str)==2) && (sscanf(str, "%x", &hex)==1))
        array[cnt++] = hex;
    }
  cnt2 = 0;
  for (i = 34; i < 64+34; i += 2)
    {
      crc[cnt2++] = array[i]*256+array[i+1];
    }
  for (i = 0; i < cnt2; i += 1)
    {
      fprintf(fd, "crc[%d] = %X\n", i, crc[i]);
    }
  crc[1] = 0;
  fprintf(fd, "csum = %.4X\n", csum(crc, cnt2));
  fclose(fd);
}
