#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++)
    {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *s_ = s;
    for (size_t i = 0; i < n; i++)
    {
        *s_++ = c;
    }
    return s;
}

caddr_t sbrk(int incr)
{
    return 0;
}

int close(int file)
{
    return -1;
}

int fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

int isatty(int file)
{
    return 1;
}

int lseek(int file, int ptr, int dir)
{
    return 0;
}

int read(int file, char *ptr, int len)
{
    return 0;
}
