#pragma once

/*
 * fwhdr.h
 * Definitions for reading and modifying HT818 firmware images
 * Copyright (C) 2025 myriad research
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>

#pragma pack(1)

// magic number for header
// this is the same for all other GS devices, as well as the firmware files,
// despite the actual format being very different
#define GS_MAGIC 0x23c97af9

#define GS_UPDATE_FILES 7
#define GS_UPDATE_FILENAME_SIZE 0x40
#define GS_UPDATE_CHECKSUM_SIZE 0x20a
#define GS_UPDATE_START 0x4000

// validity check stuff
#define GS_IMAGE_ID_KNOWN(id) (id >= GS_IMG_BOOT && id <= GS_IMG_PROG)
#define GS_IMAGE_TRUNCATED(hdr, size) (hdr->size_image != size)
#define GS_CHECKSUM_VALID(hdr, sum) (hdr->checksum == sum)

typedef enum gs_known_image {
    GS_IMG_BOOT = 0x0b,
    GS_IMG_CORE,
    GS_IMG_BASE,
    GS_IMG_PROG
} gs_known_image_t;

typedef enum gs_known_hwid {
    GS_HT818 = 0xfd23
} gs_known_hwid_t;

typedef struct gs_version {
    uint8_t revision;
    uint16_t minor;
    uint8_t major;
} gs_version_t;

typedef struct gs_timestamp {
    uint16_t year;
    uint8_t day;
    uint8_t month;
    uint8_t minute;
    uint8_t hour;
} gs_timestamp_t;

typedef struct gs_update_hdr {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    char filenames[GS_UPDATE_FILES][GS_UPDATE_FILENAME_SIZE];
    // file sizes
    uint32_t sizes[GS_UPDATE_FILES];
    // file versions
    gs_version_t versions[GS_UPDATE_FILES];
    // rollback protection bits?
    uint16_t support_bits[4];
    // "FW V Mask"? unknown function.
    uint16_t v_mask;
    // the size of the header itself
    uint32_t header_size;
    // the checksum of the header itself (up to header_size)
    uint16_t checksum;
    // oem id
    // CHECK: would this be different for, say, vonage?
    uint16_t oem_id;
    // unknown function
    uint16_t encryption_type;
} gs_update_hdr_t;

typedef struct ht818_image_hdr {
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
    uint16_t checksum;
    // build timestamp
    gs_timestamp_t timestamp;
    // device-specific id?
    // CHECK: is this different for other HT8xx devices?
    uint16_t hw_id;
    // "FW V Mask"? unknown function.
    uint16_t v_mask;
    // image flags?
    // bit 0: CVE-2021-37748 rollback protection?
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
} ht818_image_hdr_t;

// calculates the firmware checksum for a block of data
// hint: begin at header->start and use header->size as the length
extern uint16_t gs_sum(uint16_t* buff, size_t len);
