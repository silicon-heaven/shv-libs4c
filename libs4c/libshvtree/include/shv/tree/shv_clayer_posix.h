#ifndef SHV_CLAYER_POSIX_H
#define SHV_CLAYER_POSIX_H

#include <stdint.h>

#define BITFLAG_OPENED ((uint32_t)(1 << 0)) /* File already opened flag */

struct shv_file_node_fctx
{
    int fd;         /* A descriptor to access the file */
    uint32_t flags; /* A set of bitflags used internally in the platform implementation */
};

int check_opened_file(struct shv_file_node_fctx *fctx, const char *name);

#endif /* SHV_CLAYER_POSIX_H */
