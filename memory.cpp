#include "memory.hpp"


void* operator new(size_t size, void* buf)
{
    return buf;
}

void* operator new(size_t size)
{
    return nullptr;
}

void operator delete(void* obj, void* buf) noexcept
{
}

void operator delete(void* obj) noexcept
{
}
