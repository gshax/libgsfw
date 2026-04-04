#pragma once

#include "firmware/family/ht8xx.h"

typedef enum ht8_dvf101_known_hwid {
    GS_HT818 = 0xfd23
} gs_known_hwid_t;

#pragma pack(push, 1)

typedef struct ht8_dvf101_update_hdr {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    gs_filename_t filenames[GS_HT8_DVF101_FW_FILE_SLOTS];
    // file sizes
    uint32_t sizes[GS_HT8_DVF101_FW_FILE_SLOTS];
    // file versions
    gs_version_t versions[GS_HT8_DVF101_FW_FILE_SLOTS];

    // rollback protection bits?
    uint16_t support_bits[4];
    // "FW V Mask"? unknown function
    uint16_t v_mask;
    // the size of the header itself
    uint32_t header_size;
    // the checksum of the header itself (up to GS_UPDATE_CHECKSUM_SIZE)
    uint16_t checksum;
    // oem id
    // CHECK: would this be different for, say, vonage?
    uint16_t oem_id;
    // unknown function
    uint16_t encryption_type;
} ht8_dvf101_update_hdr_t;

typedef struct ht8_dvf101_image_hdr {
    // should be GS_MAGIC
    uint32_t magic;
    // image version
    gs_version_t version;
    // start of data, checksum begins from here
    uint32_t start;
    // unknown purpose, always same as start?
    uint32_t start2;
    // full size of the partition
    uint32_t size_image;
    // length of data, checksum ends here
    uint32_t size;
    // file number?
    uint16_t id;
    // the checksum of the image data
    // hint: begin at header->start and use header->size as the length
    uint16_t checksum;
    // build timestamp
    gs_timestamp_t timestamp;
    // device-specific id?
    // CHECK: is this different for other HT8xx devices?
    uint16_t hw_id;
    // "FW V Mask"? unknown function.
    uint16_t v_mask;
    // image flags?
    // bit 0: maybe CVE-2021-37748 rollback protection?
    // bit 1: unknown, present on 1.63.1 beta
    // bit 2: unknown, present on 1.63.1 beta
    // bit 3: unknown, present on 1.63.1 beta
    uint16_t support_bits[4];
    // oem id
    // CHECK: would this be different for, say, vonage?
    uint16_t oem_id;
    // compatible versions
    gs_version_t compat_version[2];
    // reflash count
    uint32_t prov_counter;
    // unknown function
    uint32_t pad_size;
    // TODO: there's a byte at 0x240 that supposedly marks dev builds
} ht8_dvf101_image_hdr_t;

#pragma pack(pop)