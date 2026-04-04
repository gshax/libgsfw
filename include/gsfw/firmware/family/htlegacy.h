#pragma once

#include <stddef.h>
#include <stdint.h>

#include <gsfw/firmware/family_defs.h>
#include <gsfw/util.h>

PACKED_STRUCT(ht5_update_hdr, {
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
});

PACKED_STRUCT(ht7_update_hdr, {
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
});
