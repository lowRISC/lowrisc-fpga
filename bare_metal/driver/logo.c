#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "hid.h"
#include "lowrisc_memory_map.h"
#include "logo.h"

void draw_logo(void)
{
  size_t i = 0;
  size_t j = 0;
  uint64_t cline = 0;
  uint32_t palette[16], used = 0;

  unsigned int __bpp = 1;
  size_t __ip = 0;
  size_t __il = (image_width*image_height) * __bpp;
  const unsigned char *image_ptr = logo;

  for (i = 0; i < sizeof(palette_logo2)/sizeof(*palette_logo2); i++)
    hid_reg_ptr[LOWRISC_REGS_PALETTE + i] = palette_logo2[i];
  memset(palette, 0, sizeof(palette));
  while (__ip < __il)
    {
      unsigned int __l = *(image_ptr++);
      unsigned int cnt = __l & 127;
      do {
          uint64_t k = 0;
          k = *image_ptr;
          cline |= k << ((j&15)*4);
          if ((j&15)==15)
            {
              if ((j < 512) && (i < 768))
                hid_fb_ptr[i*32+((j-15)>>4)] = cline;
              cline = 0;
            }
          if (++j >= image_width)
            {
              j = 0;
              ++i;
            }
          __ip++;
          if (~__l & 128)
            image_ptr++;
        }
        while (--cnt);
        if (__l & 128)
          image_ptr++;
    }
}
