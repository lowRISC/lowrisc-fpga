__attribute__ ((__noinline__))
int
memcmp (const void *s1, const void *s2, __SIZE_TYPE__ len)
{
  const unsigned char *sp1, *sp2;

  sp1 = s1;
  sp2 = s2;
  while (len != 0 && *sp1 == *sp2)
    sp1++, sp2++, len--;

  if (len == 0)
  return 0;
  return *sp1 - *sp2;
}
