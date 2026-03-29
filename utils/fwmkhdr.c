#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include "hdr.h"
#include "log.h"
#include "util.h"

#define PACK_FILES 4

static char img_names[PACK_FILES][GS_UPDATE_FILENAME_SIZE] = {
    "ht818boot.bin",
    "ht818core.bin",
    "ht818base.bin",
    "ht818prog.bin"
};

void usage(char* prog) {
    fprintf(stderr, "Usage: %s [-Hqn]\n\n", prog);
    fprintf(stderr, "HT818 firmware header builder\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -H\tgenerate header from decrypted images\n");
    fprintf(stderr, "  -q\tdo not display image details\n");
    fprintf(stderr, "  -n\tprint output filename to stdout\n");
    fprintf(stderr, "  -h\tdisplay help for command\n");
    exit(EXIT_FAILURE);
}

void infodump_hdr(gs_update_hdr_t* header, uint16_t realsum) {
    // print header information
    fprintf(stderr, "\noem id:\t\t%02x\n", header->oem_id);
    fprintf(stderr, "support bits:\t%04x %04x %04x %04x\n",
        header->support_bits[0], header->support_bits[1],
        header->support_bits[2], header->support_bits[3]);
    fprintf(stderr, "fw v mask:\t%04x\n", header->v_mask);
    fprintf(stderr, "encryption:\t%04x\n", header->encryption_type);

    // print checksum
    fprintf(stderr, "\nheader size:\t%08x bytes (check: %04x bytes)",
        header->header_size, GS_UPDATE_CHECKSUM_SIZE);
    fprintf(stderr, "\nchecksum:\t%x\n", header->checksum);
}

int main(int argc, char *argv[]) {
    int r = EXIT_SUCCESS;

    bool genhdr = false;
    bool quiet = false;
    bool printnames = false;

    char opt;
    while ((opt = getopt(argc, argv, "hHqn")) != -1) {
        switch (opt) {
            case 'H': genhdr = true; break;
            case 'q': quiet = true; break;
            case 'n': printnames = true; break;
            case 'h':
            default: usage(argv[0]);
        }
    }

    // allocate the header
    gs_update_hdr_t* header = malloc(GS_UPDATE_START);
    memset(header, 0, GS_UPDATE_START);
    header->magic = GS_MAGIC;
    header->header_size = GS_UPDATE_START;

    // iterate over images
    for (int i = 0; i < PACK_FILES; i++) {
        char* filename = img_names[i];

        // open image
        size_t size;
        ht818_image_hdr_t* img_hdr = ezmmap(filename, &size);

        // validate file size
        sanity(size < sizeof(ht818_image_hdr_t),
            FFORMAT("%s: total file size is invalid (%lu < %lu)",
                filename, size, (long unsigned int)sizeof(ht818_image_hdr_t)));

        // validate header->magic
        sanity(img_hdr->magic != GS_MAGIC,
            FFORMAT("%s: file header magic is invalid (%x != %x)\n\n"
                "try running gs_imgcrypt?",
                filename, img_hdr->magic, GS_MAGIC));

        // validate header->start
        sanity(img_hdr->start >= size,
            FFORMAT("%s: header start address is invalid (%x >= %lx)\n\n"
                "this doesn't look like an HT818 partition image...\n"
                "did you mean to run gs_fwtool?",
                filename, img_hdr->start, size));

        // validate header->start + header->size
        sanity(img_hdr->start + img_hdr->size > size,
            FFORMAT("%s: header length is invalid (%x+%x > %lx)",
                filename, img_hdr->start, img_hdr->size, size));

        if (!quiet) {
            fprintf(
                stderr, "%s\t%08lx bytes (%u.%u.%u)\n",
                filename, size,
                img_hdr->version.major,
                img_hdr->version.minor,
                img_hdr->version.revision
            );
        }

        // fill in header
        strcpy(header->filenames[i], filename);
        header->sizes[i] = size;
        memcpy(&header->versions[i], &img_hdr->version,
            sizeof(img_hdr->version));

        // is this the first file?
        if (i == 0) {
            // on first file, fill in additional information in the update header
            memcpy(&header->support_bits, &img_hdr->support_bits,
                sizeof(img_hdr->support_bits));
            header->v_mask = img_hdr->v_mask;
            header->oem_id = img_hdr->oem_id;
        }
        else {
            // on all other files, consistency check...
            sanity(
                memcmp(&header->support_bits, &img_hdr->support_bits,
                    sizeof(img_hdr->support_bits)) != 0,
                FFORMAT("fw v mask is inconsistent for %s\n\n"
                    "your images are mismatched!\n",
                    filename));
            sanity(header->v_mask != img_hdr->v_mask,
                FFORMAT("fw v mask is inconsistent for %s (%x != %x)\n\n"
                    "your images are mismatched!\n",
                    filename, header->v_mask, img_hdr->v_mask));
            sanity(header->oem_id != img_hdr->oem_id,
                FFORMAT("oem id is inconsistent for %s (%x != %x)\n\n"
                    "your images are mismatched!\n",
                    filename, header->oem_id, img_hdr->oem_id));
        }

        ezmunmap();
    }

    // generate checksum
    header->checksum = gs_sum((uint16_t*)header, GS_UPDATE_CHECKSUM_SIZE);

    // write it to disk
    if (genhdr) {
        // dump header info
        if (!quiet) {
            infodump_hdr(header, header->checksum);
        }

        FILE* output = fopen("ht818fw_hdr.bin", "w");
        if (output == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        fwrite(header, 1, GS_UPDATE_START, output);
        fclose(output);

        if (!quiet) {
            LOG(GRN, "\nheader generated successfully!\n");
            LOG(YLW, "encrypt your images before concatenation\n"
                "expected order: header, boot, core, base, prog\n");
        }
        if (printnames) {
            printf("%s", "ht818fw_hdr.bin");
        }
    }

    return r;
}
