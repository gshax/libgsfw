#pragma once

/*
 * log.h
 * Very stupid logging utilities
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

#include <stdio.h>

#define GRN 32
#define YLW 33
#define RED 31

#define LOG(color, template) fprintf(stderr, "\x1b[%um" template "\x1b[0m", color)
#define LOGV(color, template, ...) fprintf(stderr, "\x1b[%um" template "\x1b[0m", color, __VA_ARGS__)

#define MAX_FFORMAT 256
static char __errbuff[MAX_FFORMAT];
#define FFORMAT(template, ...) (snprintf(__errbuff, MAX_FFORMAT, template, __VA_ARGS__) > 0 ? __errbuff : "(FFORMAT failed)")
