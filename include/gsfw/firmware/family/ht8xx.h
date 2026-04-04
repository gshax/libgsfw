#pragma once

/*
 * firmware/ht8xx.h
 * Definitions for reading and modifying HT8xx firmware images
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

#include <gsfw/firmware/family_defs.h>

// validity check stuff
#define GS_IMAGE_ID_KNOWN(id) (id >= GS_IMG_BOOT && id <= GS_IMG_PROG)

typedef enum ht8_known_image {
    // uboot
    GS_IMG_BOOT = 0x0b,
    // linux kernel
    GS_IMG_CORE,
    // squashfs /
    GS_IMG_BASE,
    // squashfs /app
    GS_IMG_PROG
} ht8v1_known_image_t;

PACKED_STRUCT(ht8_v1_update_hdr, {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    gs_filename_t filenames[GS_HT8_DVF101_FW_FILE_SLOTS];
    // file sizes
    uint32_t sizes[GS_HT8_DVF101_FW_FILE_SLOTS];
    // file versions
    gs_version_t versions[GS_HT8_DVF101_FW_FILE_SLOTS];
});

PACKED_STRUCT(ht8_v2_update_hdr, {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    gs_filename_t filenames[GS_HT8_ROCKCHIP_FW_FILE_SLOTS];
    // file sizes
    uint32_t sizes[GS_HT8_ROCKCHIP_FW_FILE_SLOTS];
    // file versions
    gs_version_t versions[GS_HT8_ROCKCHIP_FW_FILE_SLOTS];
});
