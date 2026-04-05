#include <gsfw/firmware/shared.h>

uint16_t gs_sum(uint16_t* buff, size_t len) {
    uint16_t sum = 0;
    for (size_t i = 0; i < len / sizeof(uint16_t); i++) {
        sum += buff[i];
    }
    return 0x10000 - sum;
}
