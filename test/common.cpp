#include <cstdio>
#include "printk.hpp"

int printk(const char* format, ...)
{
  va_list ap;
  int result;

  va_start(ap, format);
  result = vprintf(format, ap);
  va_end(ap);

  return result;
}
