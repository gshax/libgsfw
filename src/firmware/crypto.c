#include <gsfw/firmware/crypto.h>
#include <gsfw/firmware/shared.h>
#include <gsfw/aes.h>
#include <gsfw/libc.h>

// universal (as far as i know) AES IV
char gs_iv[16 + 1] = "Grandstream Inc.";

// known aes keys
char gs_known_keys[GS_CRYPTO_MAX_KNOWN_KEYS][GS_CRYPTO_KEY_STR_SIZE + 1] = {
    // the most common key we all know and love
    "37d6ae8bc920374649426438bde35493",
    // vonage firmware key
    "de395fe3e08d8a47984de86e36b37eb0",
    
    // last entry
    "\0"
};

uint16_t gs_decode_uint16(const char* str) {
    // BUG: lowercase hex digits are not decoded properly
    uint16_t val = 0;
    for (int i = 0; i < sizeof(uint16_t) * 2; i++) {
        char byte = str[i];
        if (byte < 'A') byte = byte - '0';
        else byte = byte - 'A' + 10;
        val *= 0x10;
        val += byte;
    }
    // endianness swap
    return (val >> 8) | (val << 8);
}

void gs_key_decode(const char* str, uint16_t* buff) {
    char dstr[GS_CRYPTO_KEY_STR_SIZE];
    memset(dstr, 0, GS_CRYPTO_KEY_STR_SIZE);
    
    // copy key to workspace buffer and flip nibbles
    memcpy(dstr, str, GS_CRYPTO_KEY_STR_SIZE);
    gs_swap_bytes(dstr, GS_CRYPTO_KEY_STR_SIZE);

    // decode hex shorts
    for (int i = 0; i < GS_CRYPTO_KEY_SIZE / sizeof(uint16_t); i++) {
        buff[i] = gs_decode_uint16(dstr + i * sizeof(uint16_t) * 2);
    }
}

void gs_aes_encrypt(char* buff, size_t len, const uint16_t* key) {
    // if the length is not a multiple of the block size, truncate it
    // BUG: the trailing incomplete block will be left unencrypted (lol)
    len = len - (len % GS_CRYPTO_BLOCK_SIZE);
    // encrypt each block
    for (int i = 0; i < len; i += GS_CRYPTO_BLOCK_SIZE) {
        // init the cipher
        // BUG: this is done from scratch every single block...
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, (uint8_t*)key, (uint8_t*)gs_iv);
        // encrypt the block
        AES_CBC_encrypt_buffer(&ctx, (uint8_t*)&buff[i], GS_CRYPTO_BLOCK_SIZE);
    }
}

void gs_aes_decrypt(char* buff, size_t len, const uint16_t* key) {
    len = len - (len % GS_CRYPTO_BLOCK_SIZE);
    // decrypt each block
    for (int i = 0; i < len; i += GS_CRYPTO_BLOCK_SIZE) {
        // init the cipher
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, (uint8_t*)key, (uint8_t*)gs_iv);
        // decrypt the block
        AES_CBC_decrypt_buffer(&ctx, (uint8_t*)&buff[i], GS_CRYPTO_BLOCK_SIZE);
    }
}
