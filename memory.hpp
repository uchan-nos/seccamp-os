#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <stddef.h>

void* operator new(size_t size, void* buf);
void* operator new(size_t size);
void operator delete(void* obj, void* buf) noexcept;
void operator delete(void* obj) noexcept;

namespace bitnos
{

}

#endif // MEMORY_HPP_
