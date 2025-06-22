#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <nuttx/crc32.h>

#include <shv/tree/shv_file_com.h>
#include <shv/tree/shv_clayer_posix.h>

#define CHUNK_SIZE ((size_t)64)

int shv_file_node_crc32(shv_file_node_t *item, int start, size_t size, uint32_t *result)
{
    int ret;
    unsigned char buffer[CHUNK_SIZE];
    
    if (item == NULL || start < 0 || result == NULL) {
        return -1;
    }
    
    if (check_opened_file(&item->fctx, item->name) < 0) {
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
        ret = read(arg->fd, buffer, toread);        
        if (ret < 0) {
            return -1;
        }
        *result = crc32part(buffer, toread, *result);         
        size -= toread;
    }
    return 0;
}
