#pragma once

#include <stdint.h>
#include <gsfw/util.h>

PACKED_STRUCT(dspg_pubkey, {
    uint8_t n[256];
    uint32_t e;
});

PACKED_STRUCT(dspg_secure_header, {
    dspg_pubkey_t pubkey;
    uint8_t pre_sha256[32];
    uint8_t load_sha256[32];
    uint8_t signature[256];
});

PACKED_STRUCT(dspg_image_header, {
    uint32_t size;
    uint32_t flags;
    uint32_t pre_offset;
    uint32_t pre_size;
    uint32_t pre_crc32;
    uint32_t load_offset;
    uint32_t load_size;
    uint32_t load_addr;
    uint32_t load_crc32;
    dspg_secure_header_t sb_hdr;
    uint32_t crc32;
});
