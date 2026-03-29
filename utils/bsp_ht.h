#pragma once

/*
 * bsp_ht.h
 * Definitions for interfacing with bsp_ht.ko
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
 *
 * Order of operations:
 *  0.  Make a character device file with BSP_MAJOR as the major number and
 *      open it
 *  1.  Invoke HT_BSP_INIT to initialize the BSP driver and get the
 *      model-specific FXS/FXO loadout
 *  2.  Invoke HT_BSP_RESET_ASSERT to assert the SLIC reset GPIO pin
 *  3.  Wait a bit... a small amount will probably do, but gs_ata waits an
 *      entire second!
 *  4.  Invoke HT_BSP_RESET_CLEAR to clear the reset pin
 *  5.  You can now use the information provided by HT_BSP_INIT to create
 *      the TAPI device files for /dev/fxsXX
 *
 * Why steps 2-4 are done in userspace, as opposed to being folded into
 * HT_BSP_INIT or even a separate HT_BSP_RESET ioctl, I cannot tell you, or
 * even begin to imagine.
 */

#define DEV_BSP "/dev/slic_bsp"
#define DEV_FXS "/dev/fxs%02i"
#define DEV_VOICE "/dev/voice%02i"
#define DEV_SHAREDMEM "/dev/sharedmem"

#define HT_BSP_MAJOR 122
#define TAPI_FXS_MAJOR 121
#define COMA_VOICE_MAJOR 245
#define COMA_SHMEM_MAJOR 249

// just prints the version...
#define HT_BSP_PRINT_VERSION 0x0
// initialize BSP and get FXS/FXO loadout
// takes a ht_bsp_init_result_t*
#define HT_BSP_INIT 0x1
// does nothing at all; might be a debug feature ifdef'd out...
#define HT_BSP_NOOP 0x2
// assert the SLIC reset pin
#define HT_BSP_RESET_ASSERT 0x3
// unassert the SLIC reset pin
#define HT_BSP_RESET_CLEAR 0x4

typedef struct ht_bsp_init_result {
    // kernel pointer (!?) to model name
    char* model;
    // number of SLICs available
    int slic_count;
    // number of channels per SLIC
    int slic_channels;
    // number of DAAs available
    int daa_count;
    // number of channels per DAA
    int daa_channels;
    // max ringer REN
    int ren;
    int unk0;
    int unk1;
} ht_bsp_init_result_t;
