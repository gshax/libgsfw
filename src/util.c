#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "log.h"

int ezmmap(char* filename, ezmmap_mode_t mode, size_t size, ezmmap_ctx_t* ctx) {
    ctx->mode = mode;
    ctx->size = size;

    int oflag = 0;
    int prot = 0;
    switch (ctx->mode) {
        case ezmmap_ro:
            oflag = O_RDONLY;
            prot = PROT_READ;
            break;
        case ezmmap_rw:
            oflag = O_RDWR;
            prot = PROT_READ | PROT_WRITE;
            break;
        case ezmmap_create:
            oflag = O_RDWR | O_CREAT | O_TRUNC;
            prot = PROT_READ | PROT_WRITE;
            break;
    }

    int fd = open(filename, oflag, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror(filename);
        return 0;
    }

    if (fstat(fd, &ctx->sb) == -1) {
        perror("fstat");
        close(fd);
        return 0;
    }

    if (size == 0) {
        ctx->size = ctx->sb.st_size;
        if (ctx->sb.st_size == 0) {
            fprintf(stderr, "file is empty\n");
            close(fd);
            return 0;
        }
    }

    if (mode == ezmmap_create) {
        posix_fallocate(fd, 0, ctx->size);
    }

    ctx->ptr = mmap(NULL, ctx->size, prot, MAP_SHARED, fd, 0);
    if (ctx->ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 0;
    }
    
    return 1;
}

int ezmunmap(ezmmap_ctx_t* ctx) {
    if (ctx == NULL || ctx->ptr == NULL) {
        return 0;
    }
    if (munmap(ctx->ptr, ctx->size) == -1) {
        perror("munmap");
        ctx->ptr = NULL;
        return 0;
    }
    ctx->ptr = NULL;
    return 1;
}

int ezmknod(char* path, char type, int major, int minor) {
    mode_t mode;
    switch (type) {
        case 'b':
            mode = __S_IFBLK;
            break;
        case 'c':
        case 'u':
            mode = __S_IFCHR;
            break;
        default:
            LOGV(RED, "dont know how to make a %c node\nthis is a bug\n", type);
            return 0;
    }

    int status = mknod(path, mode, makedev(major, minor));
    if (status < 0) {
        switch (errno) {
            // okay for it to already exist
            case EEXIST:
                return 0;
        }
    }

    return 1;
}

void sanity(int assertion, char* should) {
    if (assertion) {
        LOGV(RED, "sanity check failed: %s\n", should);
        exit(EXIT_FAILURE);
    }
}