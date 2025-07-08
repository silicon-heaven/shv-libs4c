#pragma once

#include <stdint.h>
#include <termios.h>
#include <stddef.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#define BITFLAG_OPENED ((uint32_t)(1 << 0)) /* File already opened flag */

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
 * @brief Internal function to check whether the file is opened.
 *        If not, the function opens the file given by "name".
 *
 * @param fctx
 * @param name
 * @return 0 in case of success, -1 otherwise
 */
int shv_posix_check_opened_file(struct shv_file_node_fctx *fctx, const char *name);
