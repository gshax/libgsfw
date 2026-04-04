/** collection of various public domain stdlib method implementations */

#include <gsfw/libc.h>

void* c_memcpy(void *dest, const void *src, size_t len) {
    char* d = dest;
    const char* s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

void* c_memmove(void* dest, const void* src, size_t len) {
    char* d = dest;
    const char* s = src;
    if (d < s)
        while (len--)
        *d++ = *s++;
    else {
        const char* lasts = s + (len - 1);
        char* lastd = d + (len - 1);
        while (len--)
        *lastd-- = *lasts--;
    }
    return dest;
}

void* c_memset(void* dest, int val, size_t len) {
    unsigned char* ptr = dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

int c_strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    }
    else {
        return (*(unsigned char *)s1 - *(unsigned char *)s2);
    }
}
