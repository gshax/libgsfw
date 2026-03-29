#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/param.h>
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

typedef struct image_summary {
    uint16_t checksum;
    size_t file_size;
    size_t body_size;
} image_summary_t;

static char img_names[4][GS_UPDATE_FILENAME_SIZE] = {
    "boot",
    "core",
    "base",
    "prog"
};

void usage(char* prog) {
    fprintf(stderr, "Usage: %s [-uflqn] <file>\n\n", prog);
    fprintf(stderr, "HT818 image utility\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -u\textract raw image\n");
    fprintf(stderr, "  -H\textract image header\n");
    fprintf(stderr, "  -f\tfix header errors\n");
    fprintf(stderr, "  -l\tset compat bits to 1\n");
    fprintf(stderr, "  -q\tdo not display image details\n");
    fprintf(stderr, "  -n\tprint output filenames to stdout\n");
    fprintf(stderr, "  -h\tdisplay help for command\n");
    exit(EXIT_FAILURE);
}

void infodump(ht818_image_hdr_t* header, image_summary_t* summary) {
    gs_version_t version = header->version;
    gs_timestamp_t ts = header->timestamp;

    // image id, use pretty name if possible
    if (GS_IMAGE_ID_KNOWN(header->id)) {
        fprintf(stderr, "image id:\t%02x (%s)\n",
            header->id, img_names[header->id - GS_IMG_BOOT]);
    }
    else {
        fprintf(stderr, "image id:\t%02x\n", header->id);
    }

    // image version/timestamp
    fprintf(stderr, "version:\t%u.%u.%u\n",
        version.major, version.minor, version.revision);
    fprintf(stderr, "build date:\t%04u-%02u-%02u %02u:%02u\n",
        ts.year, ts.month, ts.day, ts.hour, ts.minute);

    // print hardware and oem id
    fprintf(stderr, "\nhw id:\t\t%04x\n", header->hw_id);
    fprintf(stderr, "oem id:\t\t%04x\n", header->oem_id);
    
    // supbits flags
    fprintf(stderr, "support bits:\t%04x %04x %04x %04x\n",
        header->support_bits[0], header->support_bits[1],
        header->support_bits[2], header->support_bits[3]);

    // "compatible images"
    fprintf(stderr, "compat ver 0:\t%u.%u.%u\n",
        header->compat_version[0].major,
        header->compat_version[0].minor,
        header->compat_version[0].revision);
    fprintf(stderr, "compat ver 1:\t%u.%u.%u\n",
        header->compat_version[1].major,
        header->compat_version[1].minor,
        header->compat_version[1].revision);
    
    // additional unknown fields
    fprintf(stderr, "\nfw v mask:\t%04x\n", header->v_mask);
    fprintf(stderr, "pad size:\t%08x\n", header->pad_size);

    // provision counter, if it's >0 (dumped from live device)
    if (header->prov_counter > 0) {
        fprintf(stderr, "\nflash counter:\t%08x\n", header->prov_counter);
    }

    fprintf(stderr, "\nimage length:\t%08x (%s)\n", header->size_image,
        GS_IMAGE_TRUNCATED(header, summary->file_size) ?
            "truncated" : "not truncated");
    fprintf(stderr, "body start:\t%08x\n", header->start);

    // handle invalid sizes
    if (
        GS_IMAGE_TRUNCATED(header, summary->file_size) &&
        (header->size != summary->body_size)
    ) {
        bool too_small = header->size > summary->body_size;
        LOGV(too_small ? RED : YLW, "body length:\t%08x (expected: %08lx)\n",
            header->size, summary->body_size);
    }
    else {
        fprintf(stderr, "body length:\t%08x\n", header->size);
    }

    // print current and expected checksum values
    LOGV(GS_CHECKSUM_VALID(header, summary->checksum) ? GRN : RED,
        "checksum:\t%04x (expected: %04x)\n", header->checksum, summary->checksum);
}

int main(int argc, char *argv[]) {
    int r = EXIT_SUCCESS;

    bool strip = false;
    bool strip_header = false;
    bool fix = false;
    bool fixbits = false;
    bool quiet = false;
    bool printnames = false;

    char opt;
    while ((opt = getopt(argc, argv, "huHflqn")) != -1) {
        switch (opt) {
            case 'u': strip = true; break;
            case 'H': strip_header = true; break;
            case 'f': fix = true; break;
            case 'l': fixbits = true; break;
            case 'q': quiet = true; break;
            case 'n': printnames = true; break;
            case 'h':
            default: usage(argv[0]);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "missing required argument 'file' (hint: -h for help)\n");
        exit(EXIT_FAILURE);
    }

    // open the file
    image_summary_t summary;
    ht818_image_hdr_t* header = ezmmap(argv[optind], &summary.file_size);

    // SANITY CHECKS

    // validate file size
    sanity(summary.file_size < sizeof(ht818_image_hdr_t),
        FFORMAT("total file size is invalid (%lu < %lu)",
            summary.file_size, (long unsigned int)sizeof(ht818_image_hdr_t)));

    // validate header->magic
    sanity(header->magic != GS_MAGIC,
        FFORMAT("file header magic is invalid (%x != %x)\n\n"
            "try running gs_imgcrypt?",
            header->magic, GS_MAGIC));

    // validate header->start
    sanity(header->start >= summary.file_size,
        FFORMAT("header start address is invalid (%x >= %lx)\n\n"
            "this doesn't look like an HT818 partition image...\n"
            "did you mean to run gs_fwtool?",
            header->start, summary.file_size));

    // get pointer to body
    char* body = (char*)header + header->start;
    // calc actual body length
    summary.body_size = summary.file_size - header->start;
    // calc actual data checksum
    // try not to go out of bounds...
    summary.checksum = gs_sum((uint16_t*)body,
        MIN(header->size, summary.body_size));

    // dump image info
    if (!quiet) {
        infodump(header, &summary);
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

    bool bad_checksum = !GS_CHECKSUM_VALID(header, summary.checksum);
    bool bad_size = GS_IMAGE_TRUNCATED(header, summary.file_size) &&
        (header->size != summary.body_size);
    if ((bad_checksum || bad_size) && !quiet) { fprintf(stderr, "\n"); }

    // fix checksum
    if (bad_checksum) {
        if (fix) {
            header->checksum = summary.checksum;
            if (!quiet) { LOG(YLW, "checksum has been corrected\n"); }
        }
        else {
            r = EXIT_FAILURE;
            LOG(YLW, "checksum is invalid, run gs_imgtool -f to fix it...\n");
        }
    }

    // fix size
    if (bad_size) {
        if (fix) {
            header->size = summary.body_size;
            if (!quiet) { LOG(YLW, "body length has been corrected\n"); }
        }
        else {
            r = EXIT_FAILURE;
            LOG(YLW, "body length invalid for truncated image, run gs_imgtool -f to fix it...\n");
        }
    }

    // extract header if wanted
    if (strip_header) {
        // figure out name
        char filename[GS_UPDATE_FILENAME_SIZE];
        if (GS_IMAGE_ID_KNOWN(header->id)) {
            snprintf(filename, GS_UPDATE_FILENAME_SIZE, "ht818%s_hdr.bin",
                img_names[header->id - GS_IMG_BOOT]);
        }
        else {
            snprintf(filename, GS_UPDATE_FILENAME_SIZE, "ht818unk%x_hdr.bin",
                header->id);
        }

        // write out to file
        FILE* output = fopen(filename, "w");
        if (output == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        size_t wb = fwrite(header, 1, header->start, output);
        if (wb != header->start) {
            perror("fwrite");
            exit(EXIT_FAILURE);
        }
        fclose(output);

        if (!quiet) {
            LOGV(GRN, "\nheader written to %s!\n", filename);
        }
        if (printnames) {
            printf("%s", filename);
        }
    }

    // unencapsulate if wanted
    if (strip) {
        if (!strip_header && !quiet) { fprintf(stderr, "\n"); }

        // validate header->start + header->size
        sanity(header->start + header->size > summary.file_size,
            FFORMAT("body length is invalid (%x+%x > %lx)",
                header->start, header->size, summary.file_size));

        // figure out name
        char filename[GS_UPDATE_FILENAME_SIZE];
        if (GS_IMAGE_ID_KNOWN(header->id)) {
            snprintf(filename, GS_UPDATE_FILENAME_SIZE, "ht818%s.img",
                img_names[header->id - GS_IMG_BOOT]);
        }
        else {
            snprintf(filename, GS_UPDATE_FILENAME_SIZE, "ht818unk%x.img",
                header->id);
        }

        // write out to file
        FILE* output = fopen(filename, "w");
        if (output == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        size_t wb = fwrite(body, 1, header->size, output);
        if (wb != header->size) {
            perror("fwrite");
            exit(EXIT_FAILURE);
        }
        fclose(output);

        if (!quiet) {
            LOGV(GRN, "body written to %s!\n", filename);
        }
        if (printnames) {
            printf("%s", filename);
        }
    }

    ezmunmap();

    return r;
}
