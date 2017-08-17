#pragma once

#include <stddef.h>

void* operator new(size_t size, void* buf);
void* operator new(size_t size);
void operator delete(void* obj, void* buf) noexcept;
void operator delete(void* obj) noexcept;
