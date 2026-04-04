#include "log.h"
#include "firmware/shared.h"
#include "firmware/crypto.h"
#include "firmware/family_defs.h"
//#include "htlegacy.h"
//#include "ht8xx.h"

#include <stdio.h>
#include <string.h>

void gs_swap_bytes(char* buff, size_t len) {
    for (int i = 0; i < len; i += 2) {
        char c = buff[i];
        buff[i] = buff[i + 1];
        buff[i + 1] = c;
    }
}

uint16_t gs_sum(uint16_t* buff, size_t len) {
    uint16_t sum = 0;
    for (size_t i = 0; i < len / 2; i++) {
        sum += buff[i];
    }
    return 0x10000 - sum;
}

gs_family_def_t* gs_family_fw_fingerprint(char* start) {
    for (int i = 0; i < GS_MAX_FAMILIES; i++) {
        gs_family_def_t* family = &gs_device_families[i];
        gs_stub_update_hdr_t* header = (gs_stub_update_hdr_t*)(start + family->fw_start);

        // check if the magic matches
        if (family->fw_magic != header->magic) {
            //printf("not %s, magic wrong\n", family->name);
            continue;
        }

        // check if the filename for the first file starts with ascii a-z
        if (!VALID_START_CHAR(header->file0[0])) {
            //printf("not %s, invalid file0 name\n", family->name);
            continue;
        }

        // check all available file0s for the family
        // XXX: it would probably be more bulletproof to check for prefixes
        //      this works for now, though
        int matched = 0;
        for (int j = 0; j < GS_FAMILY_MEMBERS; j++) {
            if (!family->fw_first_file[j][0]) { break; }
            if (strncmp(header->file0, family->fw_first_file[j], GS_UPDATE_FILENAME_SIZE) == 0) {
                matched = 1;
                break;
            }
        }
        if (!matched) {
            //printf("not %s, no file0 match\n", family->name);
            continue;
        }

        // this seems to match, let's go with it
        return family;
    }

    // tough luck
    return NULL;
}

void gs_family_capability_string(gs_family_def_t* family, char* output, size_t maxlen) {
    snprintf(output, maxlen, "%c%c%c%c",
        // Unpack firmware updates
        family->methods.fw_parse_header ? 'U' : '.',
        // Rebuild firmware updates (not implemented yet)
        (family->methods.fw_build_header && family->methods.img_set_body_size) ? 'P' : '.',
        // Display family-specific details about firmware updates
        //family->methods.fw_infodump ? 'Q' : '.',

        // Decrypt firmware images
        family->capabilities.img_decrypt ? 'D' : '.',
        // Patch firmware images (not implemented yet)
        family->capabilities.img_encrypt ? 'E' : '.'
        // Display family-specific details about firmware images
        //family->methods.img_infodump ? 'S' : '.'
    );
}

int gs_family_fw_build_directory(gs_family_def_t* family, char* start, gs_update_directory_t* directory) {
    //directory->family = family;
    start += family->fw_start;

    // common elements
    directory->body = start + family->fw_body_start;

    if (!family->methods.fw_parse_header) {
        LOGV(RED, "directory parser not implemented for %s\n", family->name);
        return 1;
    }
    else {
        return family->methods.fw_parse_header(start, directory);
    }

    return 0;
}

char* gs_family_img_key_probe(gs_family_def_t* family, char* image) {
    // try all known keys
    uint16_t decoded[GS_CRYPTO_KEY_SIZE];
    for (int i = 0; i < GS_CRYPTO_MAX_KNOWN_KEYS; i++) {
        char* key = gs_known_keys[i];
        if (!key[0]) { break; }

        // get decrypted magic from first block
        gs_key_decode(key, decoded);
        gs_aes_decrypt(image, GS_CRYPTO_BLOCK_SIZE, decoded);
        uint32_t magic = *((uint32_t*)image);
        gs_aes_encrypt(image, GS_CRYPTO_BLOCK_SIZE, decoded);

        // is the magic correct?
        if (magic == family->img_magic) {
            return key;
        }
    }
    // no luck
    return NULL;
}

int gs_family_img_decrypt(gs_family_def_t* family, char* image, size_t len, char* key) {
    if (key == NULL) {
        key = gs_family_img_key_probe(family, image);
    }
    if (key == NULL) {
        return 1;
    }
    //fprintf(stderr, "using key %s\n", key);

    uint16_t head_key[GS_CRYPTO_KEY_SIZE];

    // decrypt image header
    gs_key_decode(key, head_key);
    gs_aes_decrypt(image, family->img_crypt_size, head_key);

    // get body key
    char* body_key = image + family->img_key_sample;
    gs_swap_bytes(body_key, GS_CRYPTO_KEY_SIZE);

    // decrypt body
    char* body = image + family->img_body_start;
    gs_aes_decrypt(body, len - family->img_body_start, (uint16_t*)body_key);
    
    // restore header section
    gs_swap_bytes(body_key, GS_CRYPTO_KEY_SIZE);

    return 0;
}

int gs_family_img_encrypt(gs_family_def_t* family, char* image, size_t len, char* key) {
    if (key == NULL) {
        key = gs_known_keys[0];
    }

    // copy body key from header and swap to get the AES key
    // (don't modify the header — body key must remain in original form for header encryption)
    char body_key[GS_CRYPTO_KEY_SIZE];
    memcpy(body_key, image + family->img_key_sample, GS_CRYPTO_KEY_SIZE);
    gs_swap_bytes(body_key, GS_CRYPTO_KEY_SIZE);

    // encrypt body
    char* body = image + family->img_body_start;
    gs_aes_encrypt(body, len - family->img_body_start, (uint16_t*)body_key);

    // encrypt header (body key at img_key_sample is still in original form)
    uint16_t head_key[GS_CRYPTO_KEY_SIZE];
    gs_key_decode(key, head_key);
    gs_aes_encrypt(image, family->img_crypt_size, head_key);

    return 0;
}
