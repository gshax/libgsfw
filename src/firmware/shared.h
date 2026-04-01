#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define GS_UPDATE_FILENAME_SIZE 0x40
#define GS_FAMILY_MEMBERS 0x10
#define GS_FAMILY_NAME_SIZE 0x10

#define GS_IMAGE_TRUNCATED(hdr, size) (hdr->size_image != size)
#define GS_CHECKSUM_VALID(hdr, sum) (hdr->checksum == sum)

#define VALID_START_CHAR(c) (c >= 'a' && c <= 'z')

typedef char gs_filename_t[GS_UPDATE_FILENAME_SIZE];

#pragma pack(push, 1)

// common start structure for all variants, used for fingerprinting
typedef struct gs_stub_update_hdr {
    // should be GS_MAGIC
    uint32_t magic;
    // file names
    gs_filename_t file0;
} gs_stub_update_hdr_t;

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

#pragma pack(pop)

// internal directory for update metadata
typedef struct gs_update_directory {
    gs_filename_t* filenames;
    uint32_t* sizes;
    gs_version_t* versions;
    char* body;
} gs_update_directory_t;

typedef struct gs_family_methods {
    // print information about a firmware header
    void (*fw_infodump)(void* header);
    // parse a header into universal format
    int (*fw_parse_header)(void* header, gs_update_directory_t* directory);

    // print information about an image header
    void (*img_infodump)(void* header);

    // get pointer to the checksum field in a firmware header
    // sets *checksum_field to the field location; returns 0 on success
    int (*fw_get_checksum)(void* header, uint16_t** checksum_field);

    // reset support/rollback bits to permissive values (0000 0000 0000 0001)
    // returns 0 on success
    int (*fw_fix_support_bits)(void* header);

    // get checksum context for an image:
    //   stored_out   → pointer to the checksum field (for fixing in place)
    //   body_out     → pointer to body data (pass to gs_sum)
    //   body_size    → length of data to checksum
    // returns 0 on success
    int (*img_get_checksum)(void* image, size_t file_size,
        uint16_t** stored_out, void** body_out, size_t* body_size_out);

    // reset image support/rollback bits to permissive values (0000 0000 0000 0001)
    // returns 0 on success
    int (*img_fix_support_bits)(void* image);

    // update image header body size fields after a patch
    // body_size = number of bytes at img_body_start; file_size = total file size
    // returns 0 on success
    int (*img_set_body_size)(void* image, size_t body_size, size_t file_size);

    // build a firmware update header from a directory + first image header
    // (first_image supplies family-specific metadata: support_bits, oem_id, etc.)
    // checksum is computed by the caller via fw_get_checksum + gs_sum
    // returns 0 on success
    int (*fw_build_header)(void* header, gs_update_directory_t* directory, void* first_image);
} gs_family_methods_t;

typedef struct gs_family_innate_capabilities {
    bool img_encrypt;
    bool img_decrypt;
} gs_family_innate_capabilities_t;

// family format variant definition
typedef struct gs_family_def {
    // internal id of family
    int id;
    // internal name of family
    char name[GS_FAMILY_NAME_SIZE];
    // internal name of members
    gs_filename_t members[GS_FAMILY_MEMBERS];

    // firmware file magic number
    uint32_t fw_magic;
    // first filename of firmware
    gs_filename_t fw_first_file[GS_FAMILY_MEMBERS];
    // start of header in file
    int fw_start;
    // length and position of checksum
    int fw_check_size;
    // start of body content
    int fw_body_start;
    // filename slots supported by variant
    int fw_file_slots;

    // image magic number
    uint32_t img_magic;
    // image header encrypted section
    int img_crypt_size;
    // body key sample position
    int img_key_sample;
    // start of image body
    int img_body_start;

    gs_family_methods_t methods;
    gs_family_innate_capabilities_t capabilities;
} gs_family_def_t;

extern void gs_swap_bytes(char* buff, size_t len);

// calculates the firmware checksum for a block of data
extern uint16_t gs_sum(uint16_t* buff, size_t len);

// determine the family of an update, NULL if unknown
extern gs_family_def_t* gs_family_fw_fingerprint(char* start);

extern void gs_family_capability_string(gs_family_def_t* family, char* output, size_t maxlen);

// build the firmware update directory for a given family
extern int gs_family_fw_build_directory(gs_family_def_t* family, char* start, gs_update_directory_t* directory);

// decrypt a firmware image for a given family
extern int gs_family_img_decrypt(gs_family_def_t* family, char* image, size_t len, char* key);

// encrypt a firmware image for a given family
// image must have been decrypted by gs_family_img_decrypt (body key is expected in swapped form)
extern int gs_family_img_encrypt(gs_family_def_t* family, char* image, size_t len, char* key);