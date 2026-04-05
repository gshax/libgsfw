/** collection of various public domain stdlib method implementations */

#include <gsfw/libc.h>
#include <limits.h>

#ifdef LIBGSFW_EMBEDDED

void* memcpy(void *dest, const void *src, size_t len) {
    char* d = dest;
    const char* s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t len) {
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

void* memset(void* dest, int val, size_t len) {
    unsigned char* ptr = dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

int strncmp(const char* s1, const char* s2, size_t n) {
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

char* itoa(int value, char* buf, int base) {
    char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[66]; // enough for base-2 + sign + null
    int i = 0;
    int negative = 0;

    // Handle 0 explicitly
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    // Only handle negative for base 10
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }

    // Build digits in reverse
    while (value > 0) {
        tmp[i++] = digits[value % base];
        value /= base;
    }

    if (negative)
        tmp[i++] = '-';

    // Reverse into buf
    int j = 0;
    while (i > 0)
        buf[j++] = tmp[--i];
    buf[j] = '\0';

    return buf;
}

long strtol(const char *s, char **endptr, int base) {
    long result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;

    // Handle optional sign
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }

    // Infer base from prefix if base == 0
    if (base == 0) {
        if (*s == '0' && (*(s+1) == 'x' || *(s+1) == 'X')) base = 16;
        else if (*s == '0')                                  base = 8;
        else                                                 base = 10;
    }

    // Skip optional 0x/0X prefix for base 16
    if (base == 16 && *s == '0' && (*(s+1) == 'x' || *(s+1) == 'X'))
        s += 2;

    // Convert digits
    const char *start = s;
    while (*s) {
        int digit;
        if      (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;

        if (digit >= base) break;

        // Overflow check before accumulating
        if (result > (LONG_MAX - digit) / base) {
            result = (sign == 1) ? LONG_MAX : LONG_MIN;
            // Consume remaining valid digits
            while (*s >= '0' && *s <= '9') s++;
            if (endptr) *endptr = (char *)s;
            return result;
        }

        result = result * base + digit;
        s++;
    }

    // If no digits were consumed, endptr points to the original start
    if (endptr)
        *endptr = (char *)(s == start ? s - (sign == -1) : s);

    return sign * result;
}

#endif // LIBGSFW_EMBEDDED