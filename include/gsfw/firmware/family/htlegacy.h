#pragma once

#include <stddef.h>
#include <stdint.h>

#include <gsfw/firmware/family_defs.h>

#pragma pack(push, 1)

typedef struct ht5_update_hdr {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    gs_filename_t filenames[GS_HT5_FW_FILE_SLOTS];
    // file sizes
    uint32_t sizes[GS_HT5_FW_FILE_SLOTS];
    // ???
    uint32_t unknown[GS_HT5_FW_FILE_SLOTS];
    // file versions
    gs_version_t versions[GS_HT5_FW_FILE_SLOTS];
    // model-specific header
    char model_header[1];
} ht5_update_hdr_t;

typedef struct ht7_update_hdr {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    gs_filename_t filenames[GS_HT7_FW_FILE_SLOTS];
    // file sizes
    uint32_t sizes[GS_HT7_FW_FILE_SLOTS];
    // ???
    uint32_t unknown[GS_HT7_FW_FILE_SLOTS];
    // file versions
    gs_version_t versions[GS_HT7_FW_FILE_SLOTS];
    // model-specific header
    char model_header[1];
} ht7_update_hdr_t;

#pragma pack(pop)