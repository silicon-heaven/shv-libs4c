#pragma once

#include <stdint.h>
#include <termios.h>
#include <stddef.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#define SHV_FILE_POSIX_BITFLAG_OPENED ((uint32_t)(1 << 0)) /* File already opened flag */

typedef struct shv_dotdevice_node shv_dotdevice_node_t;
typedef struct shv_file_node shv_file_node_t;
struct shv_connection;

struct shv_file_node_fctx
{
    int fd;         /* A descriptor to access the file */
    uint32_t flags; /* A set of bitflags used internally in the platform implementation */
};

struct shv_tlayer_serial_ctx
{
    const char *file_name;      /* A POSIX serial file name */
    int fd;                     /* A descriptor to access the serial port */
    struct termios term_backup; /* Backup of struct termios (in case we need it...) */
    uint32_t flags;             /* A set of bitflags used internally */
    struct pollfd pfds[2];      /* To signal data ready to be read */
};

struct shv_tlayer_tcpip_ctx
{
    int sockfd;                 /* A descriptor to access the socket */
    struct pollfd pfds[2];      /* To signal data ready to be read */
};

struct shv_tlayer_canbus_ctx
{

};

struct shv_thrd_ctx
{
    pthread_t id;
    int thrd_ret;
    int fildes[2]; /* Create a virtual pipe whose end will be polled by poll in dataready */
};

/**
 * @brief POSIX shv_file_node_opener implementation
 *
 * @param item
 * @return int
 */
int shv_file_node_posix_opener(shv_file_node_t *item);

/**
 * @brief POSIX shv_file_node_getsize implementation
 *
 * @param item
 * @return int
 */
int shv_file_node_posix_getsize(shv_file_node_t *item);

/**
 * @brief POSIX shv_file_node_writer implementation
 *
 * @param item
 * @param buf
 * @param count
 * @return int
 */
int shv_file_node_posix_writer(shv_file_node_t *item, void *buf, size_t count);

/**
 * @brief POSIX shv_file_node_seeker implementation
 *
 * @param item
 * @param offset
 * @return int
 */
int shv_file_node_posix_seeker(shv_file_node_t *item, int offset);

/**
 * @brief POSIX shv_file_node_reader implementation
 *
 * @param item
 * @param buf
 * @param count
 * @return int
 */
int shv_file_node_posix_reader(shv_file_node_t *item, void *buf, size_t count);

/**
 * @brief POSIX shv_file_node_crc32 implementation
 *
 * @param item
 * @param start
 * @param size
 * @param result
 * @return int
 */
int shv_file_node_posix_crc32(shv_file_node_t *item, int start, size_t size, uint32_t *result);

/**
 * @brief POSIX shv_dotdevice_node_uptime implementation
 * @return int
 */
int shv_dotdevice_node_posix_uptime(void);

/**
 * @brief POSIX shv_dotdevice_node_reset implementation
 * @return int
 */
int shv_dotdevice_node_posix_reset(void);

/**
 * @brief POSIX shv_tcpip_init implementation
 *
 * @param connection
 * @return int
 */
int shv_tcpip_posix_init(struct shv_connection *connection);

/**
 * @brief POSIX shv_tcpip_read implementation
 *
 * @param connection
 * @param buf
 * @param len
 * @return int
 */
int shv_tcpip_posix_read(struct shv_connection *connection, void *buf, size_t len);

/**
 * @brief POSIX shv_tcpip_write implementation
 *
 * @param connection
 * @param buf
 * @param len
 * @return int
 */
int shv_tcpip_posix_write(struct shv_connection *connection, void *buf, size_t len);

/**
 * @brief POSIX shv_tcpip_close implementation
 *
 * @param tctx
 * @return int
 */
int shv_tcpip_posix_close(struct shv_connection *connection);

/**
 * @brief POSIX shv_tcpip_dataready implementation
 *
 * @param tctx
 * @param timeout
 * @return int
 */
int shv_tcpip_posix_dataready(struct shv_connection *connection, int timeout);
