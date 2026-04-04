#include <string.h>
#include <gsfw/bootrom/header.h>
#include <gsfw/bootrom/shared.h>
#include "util.h"

#define HDR_SIZE sizeof(dspg_image_header_t)

int main(int argc, char* argv[]) {
    ezmmap_ctx_t input;
    if (!ezmmap(argv[1], ezmmap_ro, 0, &input)) {
        exit(EXIT_FAILURE);
    }

    ezmmap_ctx_t output;
    if (!ezmmap(argv[2], ezmmap_create, input.size + HDR_SIZE, &output)) {
        exit(EXIT_FAILURE);
    }

    // construct header
    dspg_image_header_t header;
    memset(&header, 0, HDR_SIZE);

    // header details
    header.size = HDR_SIZE;
    header.flags = 1; // CRC32 mode

    // preloader details (this is us!)
    header.pre_offset = HDR_SIZE;
    header.pre_size = input.size;
    /*
     * the bootrom CRC32 only processes complete 4-byte words,
     * silently ignoring trailing bytes. match that behavior.
     */
    header.pre_crc32 = dspg_sum(input.ptr, input.size & ~3);

    // uboot load stuff (maybe not actually read?)
    header.load_offset = 0x40000;
    header.load_size = 0x140000;
    header.load_addr = 0x41000000;

    // calculate header crc
    header.crc32 = dspg_sum((void*)&header, HDR_SIZE - sizeof(uint32_t));

    // write file
    memcpy(output.ptr, &header, HDR_SIZE);
    memcpy(output.ptr + HDR_SIZE, input.ptr, input.size);

    // close files
    ezmunmap(&output);
    ezmunmap(&input);
}
