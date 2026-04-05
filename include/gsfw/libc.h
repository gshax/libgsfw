#pragma once

#include <stddef.h>

extern void* memcpy(void *dest, const void *src, size_t len);
extern void* memmove(void* dest, const void* src, size_t len);
extern void* memset(void* dest, int val, size_t len);
extern int strncmp(const char* s1, const char* s2, size_t n);
extern char* itoa(int value, char* buf, int base);
extern long strtol(const char *s, char **endptr, int base);
