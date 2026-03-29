import crypto from "crypto";

export const MAGIC = 0x23c97af9;
const KEY = "37d6ae8bc920374649426438bde35493";
const IV = Buffer.from("Grandstream Inc.", "ascii");

const BLOCK_SIZE = 32;
const KEY_SIZE = 16;

const LOWERCASE = "a".charCodeAt(0);

/**
 * Get the actual hex string for an improperly decoded hex string,
 * replicating Grandstream's buggy hex decoder
 * 
 * @param {string} key
 * @returns {string}
 */
export function gsMisdecodeKey(key) {
    key = key.split("");

    // swap nibbles
    for (let i = 0; i < KEY_SIZE * 2; i += 2) {
        const t = key[i];
        key[i] = key[i + 1];
        key[i + 1] = t;
    }

    // scramble nibbles
    for (let i = 0; i < KEY_SIZE * 2; i += 4) {
        for (let j = 0; j < 3; j++) {
            if (key[i + j + 1].charCodeAt(0) >= LOWERCASE) {
                const nibble = +("0x" + key[i + j]);
                key[i + j] = ((nibble + 2) & 0xF).toString(16);
            }
        }
    }

    return Buffer.from(key.join(""), "hex");
}

export function gsSwapBytes(key) {
    const skey = Buffer.alloc(key.length);
    for (let i = 0; i < KEY_SIZE; i += 2) {
        skey[i] = key[i + 1];
        skey[i + 1] = key[i];
    }
    return skey;
}

/**
 * Implement Grandstream's buggy use of AES
 * 
 * @param {Buffer} buffer
 * @param {boolean} encrypt
 * @param {string} key
 */
export function gsCipher(buffer, encrypt = false, key = gsMisdecodeKey(KEY)) {
    // if the length is not a multiple of the block size, truncate it
    // the trailing incomplete block will be left unencrypted (lol)
    const len = buffer.length - buffer.length % BLOCK_SIZE;
    // encrypt each block
    for (let i = 0; i < len; i += BLOCK_SIZE) {
        // init the cipher
        const cipher = encrypt ?
            crypto.createCipheriv("aes-128-cbc", key, IV) :
            crypto.createDecipheriv("aes-128-cbc", key, IV);
        // we do not want any padding!
        cipher.setAutoPadding(false);
        // get a view of the block we're working on
        const block = buffer.subarray(i, i + BLOCK_SIZE);
        // encrypt it and copy it back over the block
        Buffer.concat([cipher.update(block), cipher.final()]).copy(block);
    }
}