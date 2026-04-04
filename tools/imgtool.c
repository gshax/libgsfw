#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <gsfw/firmware/family_defs.h>
#include <gsfw/firmware/shared.h>
#include "log.h"
#include "util.h"

void usage(char* prog) {
    fprintf(stderr, "Usage: %s -F <family> [-uHflqn] <file>\n"
                    "       %s -F <family> --patch <body> <file>\n\n", prog, prog);
    fprintf(stderr, "Grandstream partition image utility\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -F, --family <name>\tdevice family (required)\n");
    fprintf(stderr, "  -u, --unpack\t\textract raw image body\n");
    fprintf(stderr, "  -H, --header\t\textract image header\n");
    fprintf(stderr, "  -p, --patch <body>\treplace image body and fix header\n");
    fprintf(stderr, "  -f, --fix\t\tfix header errors\n");
    fprintf(stderr, "  -l, --fix-bits\tset compat bits to 1\n");
    fprintf(stderr, "  -q, --quiet\t\tdo not display image details\n");
    fprintf(stderr, "  -n, --names\t\tprint output filenames to stdout\n");
    fprintf(stderr, "  -h, --help\t\tdisplay help for command\n");
    fprintf(stderr, "\nAvailable families:\n");
    for (int i = 0; i < GS_MAX_FAMILIES; i++) {
        fprintf(stderr, "  %s\n", gs_device_families[i].name);
    }
    exit(EXIT_FAILURE);
}

// derive output stem from input filename: strip directory and extension
static void stem(const char* path, char* out, size_t maxlen) {
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    strncpy(out, base, maxlen - 1);
    out[maxlen - 1] = '\0';
    char* dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}

int main(int argc, char *argv[]) {
    gs_device_families_init();

    int r = EXIT_SUCCESS;

    bool strip = false;
    bool strip_header = false;
    bool fix = false;
    bool fixbits = false;
    bool quiet = false;
    bool printnames = false;
    char* family_name = NULL;
    char* patch_body = NULL;

    static struct option long_opts[] = {
        { "family",   required_argument, NULL, 'F' },
        { "unpack",   no_argument,       NULL, 'u' },
        { "header",   no_argument,       NULL, 'H' },
        { "patch",    required_argument, NULL, 'p' },
        { "fix",      no_argument,       NULL, 'f' },
        { "fix-bits", no_argument,       NULL, 'l' },
        { "quiet",    no_argument,       NULL, 'q' },
        { "names",    no_argument,       NULL, 'n' },
        { "help",     no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "F:uHp:flqnh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'F': family_name = optarg; break;
            case 'u': strip = true; break;
            case 'H': strip_header = true; break;
            case 'p': patch_body = optarg; break;
            case 'f': fix = true; break;
            case 'l': fixbits = true; break;
            case 'q': quiet = true; break;
            case 'n': printnames = true; break;
            case 'h':
            default: usage(argv[0]);
        }
    }

    if (!family_name) {
        fprintf(stderr, "missing required option --family (hint: -h for help)\n");
        exit(EXIT_FAILURE);
    }

    if (optind >= argc) {
        fprintf(stderr, "missing required argument 'file' (hint: -h for help)\n");
        exit(EXIT_FAILURE);
    }

    // look up family by name
    gs_family_def_t* family = NULL;
    for (int i = 0; i < GS_MAX_FAMILIES; i++) {
        if (strncmp(gs_device_families[i].name, family_name, GS_FAMILY_NAME_SIZE) == 0) {
            family = &gs_device_families[i];
            break;
        }
    }
    if (family == NULL) {
        fprintf(stderr, "unknown family '%s' (hint: -h for help)\n", family_name);
        exit(EXIT_FAILURE);
    }

    bool need_rw = fix || fixbits || patch_body;

    // open the image file
    ezmmap_ctx_t ctx;
    if (!ezmmap(argv[optind], need_rw ? ezmmap_rw : ezmmap_ro, 0, &ctx)) {
        exit(EXIT_FAILURE);
    }
    void* image = ctx.ptr;
    size_t file_size = ctx.sb.st_size;

    // SANITY CHECKS

    sanity(file_size < (size_t)family->img_body_start,
        FFORMAT("total file size is invalid (%lu < %u)",
            file_size, family->img_body_start));

    uint32_t magic = *(uint32_t*)image;
    sanity(magic != family->img_magic,
        FFORMAT("file magic does not match family %s (%08x != %08x)\n\n"
            "is the image encrypted?",
            family->name, magic, family->img_magic));

    // dump image info
    if (!quiet) {
        fprintf(stderr, "family:\t\t%s\n\n", family->name);
        if (family->methods.img_infodump) {
            family->methods.img_infodump(image);
        } else {
            LOGV(YLW, "model-specific parser not implemented for %s\n", family->name);
        }
    }

    // checksum validation (skip if patching — will recompute after)
    if (!patch_body && family->methods.img_get_checksum) {
        uint16_t* stored;
        void* body;
        size_t body_size;
        family->methods.img_get_checksum(image, file_size, &stored, &body, &body_size);
        uint16_t computed = gs_sum((uint16_t*)body, body_size);
        bool valid = (*stored == computed);

        if (!quiet) {
            LOGV(valid ? GRN : RED,
                "\nchecksum:\t%04x (expected: %04x)\n", *stored, computed);
        }

        if (!valid) {
            if (fix) {
                *stored = computed;
                if (!quiet) { LOG(YLW, "checksum has been corrected\n"); }
            } else {
                r = EXIT_FAILURE;
                LOG(YLW, "checksum is invalid, run with --fix to fix it...\n");
            }
        }
    } else if (fix && !patch_body && !quiet) {
        LOGV(YLW, "\nchecksum fix not supported for %s\n", family->name);
    }

    // fix rollback bits
    if (fixbits) {
        if (family->methods.img_fix_support_bits) {
            family->methods.img_fix_support_bits(image);
            if (!quiet) { LOG(YLW, "\nsupport bits reset to 0000 0000 0000 0001\n"); }
        } else {
            LOGV(YLW, "\nsupport bits fix not supported for %s\n", family->name);
        }
    }

    // patch body: replace body, fix sizes and checksum
    if (patch_body) {
        ezmmap_ctx_t body_ctx;
        if (!ezmmap(patch_body, ezmmap_ro, 0, &body_ctx)) { exit(EXIT_FAILURE); }

        size_t new_body_size = body_ctx.sb.st_size;
        size_t new_total = family->img_body_start + new_body_size;

        // resize the image file, re-mmap at new size
        ezmunmap(&ctx);
        if (truncate(argv[optind], new_total) != 0) { perror("truncate"); exit(EXIT_FAILURE); }
        if (!ezmmap(argv[optind], ezmmap_rw, new_total, &ctx)) { exit(EXIT_FAILURE); }
        image = ctx.ptr;
        file_size = new_total;

        // write new body
        memcpy((char*)image + family->img_body_start, body_ctx.ptr, new_body_size);
        ezmunmap(&body_ctx);

        // update family-specific size fields
        if (family->methods.img_set_body_size) {
            family->methods.img_set_body_size(image, new_body_size, new_total);
        } else if (!quiet) {
            LOGV(YLW, "body size fields not updated (not supported for %s)\n", family->name);
        }

        // recompute and store checksum
        if (family->methods.img_get_checksum) {
            uint16_t* stored;
            void* body;
            size_t body_size;
            family->methods.img_get_checksum(image, file_size, &stored, &body, &body_size);
            *stored = gs_sum((uint16_t*)body, body_size);
        }

        if (!quiet) { LOG(GRN, "\nbody patched successfully!\n"); }
    }

    // derive output stem from input filename
    char filestem[GS_UPDATE_FILENAME_SIZE];
    stem(argv[optind], filestem, sizeof(filestem));
    // output filenames need room for stem + suffix (e.g. "_body.bin")
    char filename[GS_UPDATE_FILENAME_SIZE + 16];

    // extract header if wanted
    if (strip_header) {
        snprintf(filename, sizeof(filename), "%s_hdr.bin", filestem);

        FILE* output = fopen(filename, "w");
        if (output == NULL) { perror("fopen"); exit(EXIT_FAILURE); }
        size_t wb = fwrite(image, 1, family->img_body_start, output);
        if (wb != (size_t)family->img_body_start) { perror("fwrite"); exit(EXIT_FAILURE); }
        fclose(output);

        if (!quiet) { LOGV(GRN, "\nheader written to %s!\n", filename); }
        if (printnames) { printf("%s", filename); }
    }

    // extract body if wanted
    if (strip) {
        if (!strip_header && !quiet) { fprintf(stderr, "\n"); }

        char* body = (char*)image + family->img_body_start;
        size_t body_size = file_size - family->img_body_start;

        snprintf(filename, sizeof(filename), "%s_body.bin", filestem);

        FILE* output = fopen(filename, "w");
        if (output == NULL) { perror("fopen"); exit(EXIT_FAILURE); }
        size_t wb = fwrite(body, 1, body_size, output);
        if (wb != body_size) { perror("fwrite"); exit(EXIT_FAILURE); }
        fclose(output);

        if (!quiet) { LOGV(GRN, "body written to %s!\n", filename); }
        if (printnames) { printf("%s", filename); }
    }

    ezmunmap(&ctx);

    return r;
}
