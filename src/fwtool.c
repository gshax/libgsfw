#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <libgen.h>

#include "firmware/family_defs.h"
#include "firmware/shared.h"
#include "log.h"
#include "util.h"

void usage(char* prog) {
    fprintf(stderr, "Usage: %s [-udflq] <file>\n", prog);
    fprintf(stderr, "       %s -p -F <family> <output> <img> [<img>...]\n\n", prog);
    fprintf(stderr, "Grandstream firmware update utility\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -u, --unpack\t\tunpack all images\n");
    fprintf(stderr, "  -d, --decrypt\t\tdecrypt all images (implies --unpack)\n");
    fprintf(stderr, "  -p, --pack\t\tpack and encrypt images into a firmware update\n");
    fprintf(stderr, "  -F, --family <name>\tdevice family (required for --pack)\n");
    fprintf(stderr, "  -f, --fix\t\tfix checksum errors\n");
    fprintf(stderr, "  -l, --fix-bits\tset compat bits to 1\n");
    fprintf(stderr, "  -q, --quiet\t\tdo not display details\n");
    fprintf(stderr, "      --families\tlist supported device families\n");
    fprintf(stderr, "  -h, --help\t\tdisplay help for command\n");
    exit(EXIT_FAILURE);
}

void list_families() {
    char capstr[32];
    fprintf(stderr, "Capabilities:\n"
        " U... = Unpack firmware updates\n"
        " .P.. = Rebuild firmware updates\n"
        " ..D. = Decrypt firmware images\n"
        " ...E = Patch firmware images\n"
        " -----\n"
    );
    for (int i = 0; i < GS_MAX_FAMILIES; i++) {
        gs_family_def_t* family = &gs_device_families[i];
        gs_family_capability_string(family, capstr, sizeof(capstr));
        fprintf(stderr, " %s %-24s\t", capstr, family->name);

        int count = 0;
        for (int j = 0; j < GS_FAMILY_MEMBERS; j++) {
            if (!family->members[j][0]) { break; }
            count++;
        }
        switch (count) {
            case 0: break;
            case 1:
                fprintf(stderr, "%s", family->members[0]);
                break;
            case 2:
                fprintf(stderr, "%s and %s", family->members[0], family->members[1]);
                break;
            default: {
                int j = 0;
                for (; j < count - 1; j++) {
                    fprintf(stderr, "%s, ", family->members[j]);
                }
                fprintf(stderr, "and %s", family->members[j]);
                break;
            }
        }
        fprintf(stderr, "\n");
    }
    exit(EXIT_SUCCESS);
}

static gs_family_def_t* find_family(const char* name) {
    for (int i = 0; i < GS_MAX_FAMILIES; i++) {
        if (strncmp(gs_device_families[i].name, name, GS_FAMILY_NAME_SIZE) == 0) {
            return &gs_device_families[i];
        }
    }
    return NULL;
}

static int do_pack(gs_family_def_t* family, bool quiet, int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "pack requires an output file and at least one image\n");
        return EXIT_FAILURE;
    }

    if (!family->methods.fw_build_header) {
        fprintf(stderr, "pack not supported for family %s\n", family->name);
        return EXIT_FAILURE;
    }

    char* outpath = argv[0];
    int nimg = argc - 1;
    char** imgpaths = argv + 1;

    // pass 1: open each image, collect metadata, sum sizes
    gs_filename_t filenames[GS_MAX_FAMILIES * 4] = {0};
    uint32_t sizes[GS_MAX_FAMILIES * 4] = {0};
    gs_version_t versions[GS_MAX_FAMILIES * 4] = {0};

    // stash a copy of the first image header for fw_build_header
    char first_hdr_buf[4096] = {0};
    size_t total_img_size = 0;

    for (int i = 0; i < nimg; i++) {
        ezmmap_ctx_t ctx;
        if (!ezmmap(imgpaths[i], ezmmap_ro, 0, &ctx)) { return EXIT_FAILURE; }

        uint32_t magic = *(uint32_t*)ctx.ptr;
        if (magic != family->img_magic) {
            fprintf(stderr, "%s: magic does not match family %s (%08x != %08x)\n\n"
                "is the image encrypted?\n",
                imgpaths[i], family->name, magic, family->img_magic);
            ezmunmap(&ctx);
            return EXIT_FAILURE;
        }

        // record filename (basename only), size, version from header
        char pathcopy[4096];
        strncpy(pathcopy, imgpaths[i], sizeof(pathcopy) - 1);
        strncpy(filenames[i], basename(pathcopy), GS_UPDATE_FILENAME_SIZE - 1);
        sizes[i] = ctx.sb.st_size;

        // extract version from image header — it's at the same offset for all ht8xx families
        // (gs_version_t immediately follows the magic uint32)
        gs_version_t* ver = (gs_version_t*)((char*)ctx.ptr + sizeof(uint32_t));
        versions[i] = *ver;

        if (!quiet) {
            fprintf(stderr, "%s\t%08x bytes (%u.%u.%u)\n",
                filenames[i], sizes[i],
                ver->major, ver->minor, ver->revision);
        }

        if (i == 0 && family->img_body_start <= (size_t)ctx.sb.st_size) {
            size_t copy_sz = family->img_body_start;
            if (copy_sz > sizeof(first_hdr_buf)) { copy_sz = sizeof(first_hdr_buf); }
            memcpy(first_hdr_buf, ctx.ptr, copy_sz);
        }

        total_img_size += ctx.sb.st_size;
        ezmunmap(&ctx);
    }

    size_t out_size = family->fw_body_start + total_img_size;

    // create output file
    ezmmap_ctx_t out;
    if (!ezmmap(outpath, ezmmap_create, out_size, &out)) { return EXIT_FAILURE; }

    // pass 2: copy + encrypt each image into output
    size_t offset = family->fw_body_start;
    for (int i = 0; i < nimg; i++) {
        ezmmap_ctx_t ctx;
        if (!ezmmap(imgpaths[i], ezmmap_ro, 0, &ctx)) {
            ezmunmap(&out);
            return EXIT_FAILURE;
        }

        char* dst = (char*)out.ptr + offset;
        memcpy(dst, ctx.ptr, ctx.sb.st_size);
        gs_family_img_encrypt(family, dst, ctx.sb.st_size, NULL);
        offset += ctx.sb.st_size;

        ezmunmap(&ctx);
    }

    // build firmware header
    gs_update_directory_t dir;
    dir.filenames = filenames;
    dir.sizes = sizes;
    dir.versions = versions;
    dir.body = (char*)out.ptr + family->fw_body_start;

    family->methods.fw_build_header(out.ptr, &dir, first_hdr_buf);

    // compute and store checksum
    if (family->fw_check_size > 0 && family->methods.fw_get_checksum) {
        void* hdr = out.ptr + family->fw_start;
        uint16_t* stored = NULL;
        family->methods.fw_get_checksum(hdr, &stored);
        if (stored) {
            *stored = gs_sum((uint16_t*)hdr, family->fw_check_size);
        }
    }

    ezmunmap(&out);

    if (!quiet) {
        LOG(GRN, "\npacked successfully!\n");
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    gs_device_families_init();

    int r = EXIT_SUCCESS;

    bool unpack = false;
    bool decrypt = false;
    bool pack = false;
    bool fix = false;
    bool fixbits = false;
    bool quiet = false;
    char* family_name = NULL;

    static struct option long_opts[] = {
        { "unpack",    no_argument,       NULL, 'u' },
        { "decrypt",   no_argument,       NULL, 'd' },
        { "pack",      no_argument,       NULL, 'p' },
        { "family",    required_argument, NULL, 'F' },
        { "fix",       no_argument,       NULL, 'f' },
        { "fix-bits",  no_argument,       NULL, 'l' },
        { "quiet",     no_argument,       NULL, 'q' },
        { "families",  no_argument,       NULL,  1  },
        { "help",      no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "udpF:flqh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': decrypt = true;
            /* fallthrough */
            case 'u': unpack = true; break;
            case 'p': pack = true; break;
            case 'F': family_name = optarg; break;
            case 'f': fix = true; break;
            case 'l': fixbits = true; break;
            case 'q': quiet = true; break;
            case  1 : list_families();
            case 'h':
            default: usage(argv[0]);
        }
    }

    // pack is a separate mode with its own argument layout
    if (pack) {
        if (!family_name) {
            fprintf(stderr, "--family is required for --pack (hint: -h for help)\n");
            return EXIT_FAILURE;
        }
        gs_family_def_t* family = find_family(family_name);
        if (!family) {
            fprintf(stderr, "unknown family '%s' (hint: -h for help)\n", family_name);
            return EXIT_FAILURE;
        }
        return do_pack(family, quiet, argc - optind, argv + optind);
    }

    if (optind >= argc) {
        fprintf(stderr, "missing required argument 'file' (hint: -h for help)\n");
        exit(EXIT_FAILURE);
    }

    // open the file
    ezmmap_ctx_t mapped_fw;
    if (!ezmmap(argv[optind], (fix || fixbits) ? ezmmap_rw : ezmmap_ro, 0, &mapped_fw)) {
        exit(EXIT_FAILURE);
    }
    void* file = mapped_fw.ptr;
    size_t size = mapped_fw.sb.st_size;

    // attempt to determine format variant
    gs_family_def_t* family = gs_family_fw_fingerprint(file);
    sanity(family == NULL, "could not determine device family");
    if (!quiet) {
        fprintf(stderr, "format variant:\t%s\n\n", family->name);
    }

    // validate file size
    sanity(size < (size_t)family->fw_body_start,
        FFORMAT("total file size is invalid (%lu < %u)",
            size, family->fw_body_start));

    // dump header info
    if (!quiet) {
        if (family->methods.fw_infodump) {
            family->methods.fw_infodump(file + family->fw_start);
        }
        else {
            LOGV(YLW, "model-specific parser not implemented for %s\n\n", family->name);
        }
    }

    gs_update_directory_t directory;
    sanity(gs_family_fw_build_directory(family, file, &directory),
        "could not build file directory");

    char* body = directory.body;

    if (!quiet) {
        fprintf(stderr, "images:\n");
    }
    size_t total_length = 0;
    for (int i = 0; i < family->fw_file_slots; i++) {
        char* filename = directory.filenames[i];

        if (filename[0] == 0) { break; }

        size_t length = directory.sizes[i];
        gs_version_t version = directory.versions[i];

        if (!quiet) {
            fprintf(stderr, "%s\t%08lx bytes (%u.%u.%u)\n",
                filename, length,
                version.major, version.minor, version.revision);
        }

        total_length += directory.sizes[i];

        if (unpack) {
            sanity(size < family->fw_body_start + total_length,
                FFORMAT("image '%s' is beyond end of file (%lu < %lu)",
                    filename, size, family->fw_body_start + total_length));

            ezmmap_ctx_t mapped_img;
            if (!ezmmap(filename, ezmmap_create, length, &mapped_img)) {
                fprintf(stderr, "could not open %s for writing\n", filename);
            }
            memcpy(mapped_img.ptr, body, length);

            if (decrypt) {
                if (gs_family_img_decrypt(family, mapped_img.ptr, mapped_img.size, NULL)) {
                    LOGV(YLW, "warning: could not decrypt '%s' (unknown key?)\n", filename);
                }
            }

            ezmunmap(&mapped_img);
            body += length;
        }
    }

    if (!quiet) {
        fprintf(stderr, "\nbody size:\t%08lx bytes\n", total_length);
    }

    // fix rollback bits (before checksum, so -f -l together produces correct checksum)
    if (fixbits) {
        if (family->methods.fw_fix_support_bits) {
            family->methods.fw_fix_support_bits(file + family->fw_start);
            if (!quiet) { LOG(YLW, "\nsupport bits have been reset\n"); }
        }
        else {
            LOGV(YLW, "\nsupport bits fixing not supported for %s\n", family->name);
        }
    }

    // validate and optionally correct the checksum
    if (family->fw_check_size > 0 && family->methods.fw_get_checksum) {
        void* header = file + family->fw_start;
        uint16_t* stored = NULL;
        family->methods.fw_get_checksum(header, &stored);

        if (stored) {
            uint16_t sum = gs_sum((uint16_t*)header, family->fw_check_size);
            bool valid = (sum == *stored);

            if (!quiet) {
                LOGV(valid ? GRN : RED,
                    "\nchecksum:\t%04x (expected: %04x)\n", *stored, sum);
            }
            if (!valid) {
                if (fix) {
                    *stored = sum;
                    if (!quiet) { LOG(YLW, "checksum has been corrected\n"); }
                }
                else {
                    r = EXIT_FAILURE;
                    LOG(YLW, "checksum is invalid, run with -f to fix...\n");
                }
            }
        }
    }
    else if (fix && !quiet) {
        LOGV(YLW, "\nchecksum validation not supported for %s\n", family->name);
    }

    if (unpack && !quiet) {
        LOG(GRN, "\nunpacked successfully!\n");
    }

    ezmunmap(&mapped_fw);

    return r;
}
