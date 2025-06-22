#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_clayer_posix.h>

int check_opened_file(struct shv_file_node_fctx *fctx, const char *name)
{
    if (!(fctx->flags & BITFLAG_OPENED)) {
        fctx->fd = open(name, O_RDWR);
        if (fctx->fd < 0) {
            return -1;
        }
        fctx->flags |= BITFLAG_OPENED;
    }
    return 0;
}

int shv_file_node_writer(shv_file_node_t *item, void *buf, size_t count)
{
    if (check_opened_file(&item->fctx, item->name) >= 0) {
        return write(item->fctx.fd, buf, count);
    }
    return -1;
}

int shv_file_node_seeker(shv_file_node_t *item, int offset)
{
    if (check_opened_file(&item->fctx, item->name) >= 0) {
        return lseek(item->fctx.fd, (off_t)offset, SEEK_SET);
    }
    return -1;
}
