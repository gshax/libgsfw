#!/usr/bin/env node

import fs from "fs/promises";

const args = process.argv.slice(2);
if (args.length <= 0) {
    console.error("no image file provided!");
    process.exit(1);
}

const UINT_SIZE = 4;

// pointer bias
const DVF_UBOOT_LOAD_ADDR = 0x41000000;

// return 0;
const NO_OP = [
    // mov r0, #0
    0xe3a00000,
    // bx lr
    0xe12fff1e
];

// check for presence of uart_locked
const SANITY = "uart_locked";
// string we use to find board_init_r
const NEEDLE = "[addr [arg ...]]";
// board_init_r[21] -> ht8xx_serial_lock
const HT8XX_SERIAL_LOCK = 21;

const uboot = await fs.readFile(args[0]);

// sanity check: validate that this uboot has uart_locked
if (uboot.indexOf(SANITY) === -1) {
    console.error("did not find uart_locked string");
    process.exit(1);
}

// find start of board_init_r
const board_init_r_end = uboot.indexOf(NEEDLE, "ascii");
let board_init_r = uboot.lastIndexOf("00000000", board_init_r_end, "hex");
board_init_r += UINT_SIZE;
console.debug("board_init_r =", board_init_r.toString(16));

// find start of ht8xx_serial_lock
let ht8xx_serial_lock = uboot.readUint32LE(board_init_r + HT8XX_SERIAL_LOCK * UINT_SIZE);
ht8xx_serial_lock -= DVF_UBOOT_LOAD_ADDR;
console.debug("ht8xx_serial_lock =", ht8xx_serial_lock.toString(16));

// patch it!
const inst = uboot.readUint32LE(ht8xx_serial_lock);
if (inst === NO_OP[0]) {
    console.error("already patched!");
    process.exit(1);
}
for (let i = 0; i < NO_OP.length; i++) {
    uboot.writeUint32LE(NO_OP[i], ht8xx_serial_lock + i * UINT_SIZE);
}

// write it back out
await fs.writeFile(args[0], uboot);
console.debug("patched successfully!");
