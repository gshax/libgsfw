#pragma once

/*
 * util.h
 * Helpers for command line apps
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

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

typedef enum ezmmap_mode {
    ezmmap_ro,
    ezmmap_rw,
    ezmmap_create
} ezmmap_mode_t;

typedef struct ezmmap_ctx {
    ezmmap_mode_t mode;
    size_t size;
    struct stat sb;
    void* ptr;
} ezmmap_ctx_t;

typedef struct applet {
    char name[0x10];
    int (*main)(int argc, char* argv[]);
} applet_t;

extern int ezmmap(char* filename, ezmmap_mode_t mode, size_t size, ezmmap_ctx_t* ctx);
extern int ezmunmap(ezmmap_ctx_t* ctx);
extern int ezmknod(char* path, char type, int major, int minor);
extern void sanity(int assertion, char* should);
