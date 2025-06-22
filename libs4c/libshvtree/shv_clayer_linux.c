#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <zlib.h>

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_clayer_posix.h>

#define CHUNK_SIZE ((size_t)64)

int shv_file_node_crc32(shv_file_node_t *item, int start, size_t size, uint32_t *result)
{
    unsigned char buffer[CHUNK_SIZE];
    
    if (item == NULL || start < 0 || result == NULL || (start + size) >= item->file_size) {
        return -1;
    }

    if (check_opened_file(&item->fctx, item->name) < 0) {
        return -1;
    }

    if (lseek(item->fctx.fd, start, SEEK_SET) < 0) {
        return -1;
    }
    
    *result = 0;
    while (size) {
        size_t toread;
        if (size > CHUNK_SIZE) {
            toread = CHUNK_SIZE;
        } else {
            toread = size;
        }
        if (read(item->fctx.fd, buffer, toread) < 0) {
            return -1;
        }
        *result = crc32(*result, buffer, toread);
        size -= toread;
    }
    return 0;
}
