#pragma once

#include <stddef.h>
#include <stdint.h>

#define GS_CRYPTO_BLOCK_SIZE 0x20
#define GS_CRYPTO_KEY_SIZE 0x10
#define GS_CRYPTO_KEY_STR_SIZE (GS_CRYPTO_KEY_SIZE * 2)

#define GS_CRYPTO_MAX_KNOWN_KEYS 0x10

extern char gs_known_keys[GS_CRYPTO_MAX_KNOWN_KEYS][GS_CRYPTO_KEY_STR_SIZE + 1];

// bugged Grandstream hex string decoder
extern void gs_key_decode(const char* str, uint16_t* buff);

// bugged Grandstream AES CBC encryption
void gs_aes_encrypt(char* buff, size_t len, const uint16_t* key);

// bugged Grandstream AES CBC decryption
void gs_aes_decrypt(char* buff, size_t len, const uint16_t* key);