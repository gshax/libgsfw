#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

#include "hdr.h"
#include "log.h"
#include "util.h"

void usage(char* prog) {
    fprintf(stderr, "Usage: %s [-uflq] <file>\n\n", prog);
    fprintf(stderr, "HT818 firmware update utility\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -u\tunpack all images from the firmware (without decrypting)\n");
    fprintf(stderr, "  -f\tfix checksum errors\n");
    fprintf(stderr, "  -l\tset compat bits to 1\n");
    fprintf(stderr, "  -q\tdo not display image details\n");
    fprintf(stderr, "  -h\tdisplay help for command\n");
    exit(EXIT_FAILURE);
}

void infodump_hdr(gs_update_hdr_t* header, uint16_t realsum) {
    // print header information
    fprintf(stderr, "oem id:\t\t%02x\n", header->oem_id);
    fprintf(stderr, "support bits:\t%04x %04x %04x %04x\n",
        header->support_bits[0], header->support_bits[1],
        header->support_bits[2], header->support_bits[3]);
    fprintf(stderr, "fw v mask:\t%04x\n", header->v_mask);
    fprintf(stderr, "encryption:\t%04x\n", header->encryption_type);

    // print checksum
    fprintf(stderr, "\nheader size:\t%08x bytes (check: %04x bytes)",
        header->header_size, GS_UPDATE_CHECKSUM_SIZE);
    LOGV((realsum == header->checksum) ? GRN : RED,
        "\nchecksum:\t%04x (expected: %04x)\n\n", header->checksum, realsum);
}

int main(int argc, char *argv[]) {
    int r = EXIT_SUCCESS;

    bool unpack = false;
    bool fix = false;
    bool fixbits = false;
    bool quiet = false;

    char opt;
    while ((opt = getopt(argc, argv, "huflq")) != -1) {
        switch (opt) {
            case 'u': unpack = true; break;
            case 'f': fix = true; break;
            case 'l': fixbits = true; break;
            case 'q': quiet = true; break;
            case 'h':
            default: usage(argv[0]);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "missing required argument 'file' (hint: -h for help)\n");
        exit(EXIT_FAILURE);
    }

    // open the file
    size_t size;
    gs_update_hdr_t* header = ezmmap(argv[optind], &size);

    // SANITY CHECKS

    // validate file size
    sanity(size < GS_UPDATE_START,
        FFORMAT("total file size is invalid (%lu < %u)",
            size, GS_UPDATE_START));

    // validate header->magic
    sanity(header->magic != GS_MAGIC,
        FFORMAT("file header magic is invalid (%x != %x)",
            header->magic, GS_MAGIC));
    
    // validate header->header_size
    sanity(header->header_size != GS_UPDATE_START,
        FFORMAT("header size is invalid (%x != %x)\n\n"
            "this doesn't look like an HT818 firmware update...\n"
            "did you mean to run gs_imgtool?",
            header->header_size, GS_UPDATE_START));

    // calc actual data checksum
    uint16_t sum = gs_sum((uint16_t*)header, GS_UPDATE_CHECKSUM_SIZE);
    bool valid = sum == header->checksum;

    // dump header info
    if (!quiet) {
        infodump_hdr(header, sum);
    }

    // get ptr to body start
    char* body = (char*)header + GS_UPDATE_START;

    if (!quiet) {
        fprintf(stderr, "images:\n");
    }
    size_t total_length = 0;
    for (int i = 0; i < GS_UPDATE_FILES; i++) {
        char* filename = header->filenames[i];

        // reached end of archive?
        if (filename[0] == 0) { break; }

        size_t length = header->sizes[i];
        gs_version_t version = header->versions[i];

        // print image info
        if (!quiet) {
            fprintf(
                stderr, "%s\t%08lx bytes (%u.%u.%u)\n",
                filename, length,
                version.major, version.minor, version.revision
            );
        }

        // increment total length
        total_length += header->sizes[i];

        if (unpack) {
            // validate file size
            sanity(size < GS_UPDATE_START + total_length,
                FFORMAT("image '%s' is beyond end of file (%lu < %lu)",
                    filename, size, GS_UPDATE_START + total_length));

            // write out to file
            FILE* output = fopen(filename, "w");
            if (output == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            size_t wb = fwrite(body, 1, length, output);
            if (wb != length) {
                perror("fwrite");
                exit(EXIT_FAILURE);
            }
            fclose(output);

            // seek to next file
            body += length;
        }
    }

    if (!quiet) {
        fprintf(stderr, "\nbody size:\t%08lx bytes\n", total_length);
    }
    
    // fix rollback bits
    if (fixbits) {
        if (!quiet) { fprintf(stderr, "\n"); }
        header->support_bits[0] = 0;
        header->support_bits[1] = 0;
        header->support_bits[2] = 0;
        header->support_bits[3] = 1;
        LOG(YLW, "support bits reset to 0000 0000 0000 0001\n");
    }

    // correct the checksum if desired
    if (!valid) {
        if (!quiet) { fprintf(stderr, "\n"); }
        if (fix) {
            header->checksum = sum;
            if (!quiet) { LOG(YLW, "checksum has been corrected\n"); }
        }
        else {
            r = EXIT_FAILURE;
            LOG(YLW, "checksum is invalid, run gs_fwtool -f to fix it...\n");
        }
    }

    // if we're unpacking, print a success message
    if (unpack && !quiet) {
        LOG(GRN, "\nunpacked successfully!\n");
    }

    ezmunmap();

    return r;
}
