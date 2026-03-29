#include "util.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"

static struct stat sb;
static void* ptr;

void* ezmmap(char* filename, size_t* size) {
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (sb.st_size == 0) {
        fprintf(stderr, "file is empty\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ptr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    *size = sb.st_size;
    return ptr;
}

void ezmunmap() {
    if (ptr == NULL) {
        return;
    }
    if (munmap(ptr, sb.st_size) == -1) {
        perror("munmap");
    }
    ptr = NULL;
}

void sanity(int assertion, char* should) {
    if (assertion) {
        LOGV(RED, "sanity check failed: %s\n", should);
        ezmunmap();
        exit(EXIT_FAILURE);
    }
}