// See LICENSE.Cambridge for license details.

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void *memcpy(void *o, const void *i, size_t n)
{
  uint8_t *optr = (uint8_t *)((size_t)o & 0xFFFFFFFF);
  const uint8_t *iptr = (const uint8_t *)((size_t)i & 0xFFFFFFFF);

  if ((uint64_t)optr < 0x40000000 || (uint64_t)optr >= 0x88000000 || (uint64_t)iptr < 0x40000000 || (uint64_t)iptr >= 0x88000000)
    {
      printf("memcpy internal error, %x <= %x\n", optr, iptr);
      for(;;)
        ;
    }

  //  printf("memcpy(%x,%x,%x);\n", o, i, n);
  while (n--) *optr++ = *iptr++;
  return optr;
}

size_t strlen (const char *str)
{
  const char *char_ptr = str;

  if ((uint64_t)str < 0x40000000 || (uint64_t)str >= 0x88000000)
    {
      printf("strlen internal error, %x\n", str);
      for(;;)
        ;
    }
  while (*char_ptr)
    ++char_ptr;
  return char_ptr - str;
}
