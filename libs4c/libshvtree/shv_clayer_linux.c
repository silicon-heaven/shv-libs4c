#include <stddef.h>
#include <stdint.h>
#include <zlib.h>

#include "shv_tree.h"
#include "shv_clayer_posix.h"

#define CHUNK_SIZE ((size_t)64)

int shv_file_node_crc32(shv_file_node_t *item, int start, size_t size, uint32_t *result)
{
    int ret;
    unsigned char buffer[CHUNK_SIZE];
    
    if (item == NULL || start < 0 || result == NULL || (start + size) >= item->file_size) {
        return -1;
    }
    ret = lseek(item->fctx.fd, start, SEEK_SET);
    if (ret < 0) {
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
        ret = read(item->fctx.fd, buffer, toread);
        if (ret < 0) {
            return -1;
        }
        *result = crc32(*result, buffer, toread);
        size -= toread;
    }
    return 0;
}
