#include <stddef.h>
#include <unistd.h>

#include "shv_tree.h"
#include "shv_clayer_posix.h"

int shv_file_node_writer(shv_file_node_t *item, void *buf, size_t count)
{
    return write(item->fctx.fd, buf, count);
}

int shv_file_node_seeker(shv_file_node_t *item, int offset)
{
    if (offset >= item->file_size) {
        return -1;
    }
    return lseek(item->fctx.fd, (off_t)offset, SEEK_SET);
}
