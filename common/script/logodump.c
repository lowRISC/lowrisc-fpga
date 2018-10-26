#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "sublime-digital_lowrisc-identity-01.c"

// VGA tuning registers
uint64_t palette_ptr[16];
// VGA frame buffer
uint64_t hid_fb_ptr[512*413];
uint8_t rslt_ptr[1<<20];
unsigned char *rslt = rslt_ptr;
uint32_t palette[16], used = 0;

size_t logo(void)
{
  size_t i = 0;
  size_t j = 0;
  size_t max = 0;
  uint64_t cline = 0;

  unsigned int __bpp;
  size_t __ip, __il;
  const unsigned char *image_ptr;

  __bpp = (gimp_image.bytes_per_pixel);
  __ip = 0;
  __il = (gimp_image.width*gimp_image.height) * __bpp;    
  image_ptr = (gimp_image.rle_pixel_data);
  memset(palette, 0, sizeof(palette));
  while (__ip < __il)
    {
      unsigned int __l = *(image_ptr++);
      unsigned int cnt = __l & 127;
      uint64_t k = 0;
      *rslt++ = __l;
      do {
          uint32_t work = 0;
          memcpy(&work, image_ptr, 3);
          //        printf("work[%d]=%x\n", image_ptr-image_buf, work);
          while ((k < used) && (work != palette[k]))
            ++k;
          if (k == used)
            {
              if (used == 16)
                {
                  printf("Pallette is full (i=%ld,j=%ld,work=0x%x)", i, j, work);
                  return 0;
                }
              else
                {
                  uint32_t palrev = (image_ptr[0]<<16)|(image_ptr[1]<<8)|(image_ptr[2]<<0);
                  palette_ptr[k] = palrev;
                  printf("Palette[%ld] = %X\n", k, palrev);
                  palette[used++] = work;
                }
            }
          cline |= k << ((j&15)*4);
          if ((j&15)==15)
            {
              if ((j < 512) && (i < 768))
                {
                  max = i*32+((j-15)>>4);
                  hid_fb_ptr[max] = cline;
                }
              cline = 0;
            }
          if (++j >= gimp_image.width)
            {
              j = 0;
              ++i;
            }
          __ip += 3;
          if (~__l & 128)
            {
              *rslt++ = k;
              image_ptr += 3;
            }
        }
        while (--cnt);
        if (__l & 128)
          {
              *rslt++ = k;
              image_ptr += 3;
          }
    }
  return rslt-rslt_ptr;
}

int main()
{
  int i;
  FILE *dmp = fopen("logo.h", "w");
  size_t siz = logo();
  fprintf(dmp, "const uint32_t image_width = %d;\n", gimp_image.width);
  fprintf(dmp, "const uint32_t image_height = %d;\n", gimp_image.height);
  fprintf(dmp, "const uint32_t palette_logo2[] = {\n");
  for (i = 0; i < used; i++)
    {
      fprintf(dmp, "0x%.6lX,\n", palette_ptr[i]);
    }
  fprintf(dmp, "};\n\n");
  fprintf(dmp, "const uint8_t logo[] = {\n");
  for (i = 0; i < siz; i++)
    {
      fprintf(dmp, "0x%.2X,\n", rslt_ptr[i]);
    }
  fprintf(dmp, "};\n");
  fclose(dmp);
}
