#pragma once

#include <stddef.h>

extern void* c_memcpy (void *dest, const void *src, size_t len);
extern void* c_memmove(void* dest, const void* src, size_t len);
extern void* c_memset (void* dest, int val, size_t len);
extern int c_strncmp(const char* s1, const char* s2, size_t n);
