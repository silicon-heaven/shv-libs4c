/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_clayer_posix.c
 * @brief POSIX compatibility layer for SHV
 */

#include <stdio.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_connection.h>
#include <shv/tree/shv_clayer_posix.h>
#include <shv/tree/shv_com.h>
#include <shv/tree/shv_file_node.h>

#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX)
    #include <zlib.h>
    #include <linux/reboot.h>
    #include <sys/reboot.h>
    #include <sys/syscall.h>
    #include <time.h>
#elif defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    #include <nuttx/config.h>
    #include <nuttx/crc32.h>
    #include <sys/boardctl.h>
#endif

/* The whole file is common for Linux and NuttX but NuttX does not use zlib's CRC */

#define CHECK_STR(str) (str == NULL || strnlen(str, 100) == 0)
#define RETLZ_ERROR(__err_label) if (ret < 0) goto __err_label
#define CHUNK_SIZE ((size_t)64)

int shv_file_node_posix_opener(struct shv_file_node *item)
{
    struct shv_file_node_fctx *fctx = (struct shv_file_node_fctx*) item->fctx;
    if (!(fctx->flags & SHV_FILE_POSIX_BITFLAG_OPENED)) {
        fctx->fd = open(item->name, O_RDWR | O_CREAT, 0644);

        if (fctx->fd < 0) {
            return -1;
        }
        fctx->flags |= SHV_FILE_POSIX_BITFLAG_OPENED;
    }
    return 0;
}

int shv_file_node_posix_getsize(struct shv_file_node *item)
{
    struct stat st;
    if (stat(item->name, &st) < 0) {
        return -1;
    }
    return st.st_size;
}

int shv_file_node_posix_writer(struct shv_file_node *item, void *buf, size_t count)
{
    struct shv_file_node_fctx *fctx = (struct shv_file_node_fctx*) item->fctx;

    if (item->fops.opener(item) >= 0) {
        /* Update count if needed. Be an ostrich and suppose st_size > 0 */
        if (item->file_offset >= item->file_maxsize) {
            return 0;
        } else if (item->file_offset + count > item->file_maxsize) {
            count = item->file_maxsize - item->file_offset;
        }

        return write(fctx->fd, buf, count);
    }
    return -1;
}

int shv_file_node_posix_seeker(struct shv_file_node *item, int offset)
{
    struct shv_file_node_fctx *fctx = (struct shv_file_node_fctx*) item->fctx;

    if (item->fops.opener(item) >= 0) {
        /* Cap it at the maximum file boundary. */
        if (offset >= item->file_maxsize) {
            offset = item->file_maxsize;
        }
        return lseek(fctx->fd, (off_t)offset, SEEK_SET);
    }
    return -1;
}

int shv_file_node_posix_crc32(struct shv_file_node *item, int start, size_t size, uint32_t *result)
{
    /* The calculation between Linux and NuttX differs a bit. While both implementations
     * use the IEEE 802.3, the process is a bit different.
     * Linux: init=0, use zlib
     * NuttX: init=0xFFFFFFFF, the result must be negated, use internal CRC table.
     */

    unsigned char buffer[CHUNK_SIZE];
    struct shv_file_node_fctx *fctx = (struct shv_file_node_fctx*) item->fctx;

    /* Sanity check. Don't allow computation beyond the file's maximum size. */
    if (item == NULL || start < 0 || result == NULL || (start + size) >= item->file_maxsize) {
        return -1;
    }

    /* In this case, it is better to sync the data to assure all data was
       stored into the physical medium. Also, close the file.
       Do this only if the file is opened. */
    if (fctx->flags & SHV_FILE_POSIX_BITFLAG_OPENED) {
        fsync(fctx->fd);
        close(fctx->fd);
        fctx->flags &= ~SHV_FILE_POSIX_BITFLAG_OPENED;
    }

    if (item->fops.opener(item) < 0) {
        return -1;
    }

    /* This is a valid seek, as it inside the file's range */
    if (lseek(fctx->fd, start, SEEK_SET) < 0) {
        return -1;
    }

    /* This algorithm handles two scenarios:
     * 1) The CRC is computed over all specified bytes which are in the file's range.
     * 2) If the range goes beyond the file's actual size (but not the maxsize),
     *    the CRC is computed only over the "correct" bytes (as read starts returning 0).
     */
#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX)
    *result = 0;
#elif defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    *result = 0xFFFFFFFF;
#endif
    ssize_t bytes_read;
    while (size) {
        size_t toread;
        if (size > CHUNK_SIZE) {
            toread = CHUNK_SIZE;
        } else {
            toread = size;
        }
        bytes_read = read(fctx->fd, buffer, toread);
        if (bytes_read < 0) {
            return -1;
        } else if (bytes_read == 0) {
            /* The boundary was reached and thus this is the final read */
            close(fctx->fd);
            fctx->flags &= ~SHV_FILE_POSIX_BITFLAG_OPENED;
            return 0;
        }
#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX)
        *result = crc32(*result, buffer, toread);
#elif defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
        *result = crc32part((uint8_t *)buffer, toread, *result);
#endif
        size -= toread;
    }
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_NUTTX
    /* Negate the whole thing. */
    *result = ~*result;
#endif
    /* Close the file, the expected use it to obtain the CRC after writing or reading */
    close(fctx->fd);
    fctx->flags &= ~SHV_FILE_POSIX_BITFLAG_OPENED;
    return 0;
}

int shv_dotdevice_node_posix_uptime(void)
{
    struct timespec ret;
    if (clock_gettime(CLOCK_MONOTONIC, &ret) < 0) {
        return -1;
    }
    return ret.tv_sec;
}

int shv_dotdevice_node_posix_reset(void)
{
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_LINUX
    sync();
    /* This is dangerous for testing, commented. */
    if (reboot(LINUX_REBOOT_CMD_RESTART) < 0) {
        return -1;
    }
#endif
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_NUTTX
#if defined(CONFIG_BOARDCTL_RESET) || defined(CONFIG_BOARDCTL_RESET_CAUSE)
    if (boardctl(BOARDIOC_RESET, BOARDIOC_RESETCAUSE_CPU_SOFT) < 0) {
        return -1;
    }
#endif
#endif
    /* Shouldn't get here, but still... */
    return 0;
}

int shv_serial_posix_init(struct shv_tlayer_serial_ctx *sctx)
{
    if (sctx == NULL) {
        return -1;
    }

    struct termios term;

    /* Try to open the serial port at sctx->file_name */
    sctx->fd = open(sctx->file_name, O_RDWR | O_NOCTTY);
    if (sctx->fd < 0) {
        return -1;
    }

    /* Check whether the device is serial-like */
    if (isatty(sctx->fd) == 0) {
        goto error;
    }

    if (tcgetattr(sctx->fd, &sctx->term_backup) < 0) {
        goto error;
    }
    term = sctx->term_backup;
    cfmakeraw(&term);
    if (tcsetattr(sctx->fd, TCSANOW, &term) < 0) {
        goto error;
    }

    sctx->pfds[0].fd = sctx->fd;
    sctx->pfds[0].events = POLLIN;

    return 0;
error:
    close(sctx->fd);
    return -1;
}

int shv_serial_posix_read(struct shv_tlayer_serial_ctx *sctx, void *buf, size_t len)
{
    if (sctx == NULL || buf == NULL) {
        return -1;
    }

    return read(sctx->fd, buf, len);
}

int shv_serial_posix_write(struct shv_tlayer_serial_ctx *sctx, void *buf, size_t len)
{
    if (sctx == NULL || buf == NULL) {
        return -1;
    }

    return write(sctx->fd, buf, len);
}

int shv_serial_posix_close(struct shv_tlayer_serial_ctx *sctx)
{
    if (sctx == NULL) {
        return -1;
    }

    tcsetattr(sctx->fd, TCSANOW, &sctx->term_backup);
    return close(sctx->fd);
}

int shv_serial_posix_dataready(struct shv_tlayer_serial_ctx *sctx, int timeout)
{
    /* The packed data (including byte stuffing, ETX, STX, CRC32) must be unpacked first.
     * We can do it here. Simulate that multiple unpacking reads behave as one blocking
     * function. Until the packet has been processed by multiple chunks and its CRC32
     * has been checked, there will be no data in shv_rd_data.
     */
    return -1;
}

int shv_tcpip_posix_init(struct shv_connection *connection)
{
    struct sockaddr_in servaddr;

    /* Socket creation */

    connection->tlayer.tcpip.ctx.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connection->tlayer.tcpip.ctx.sockfd == -1) {
        fprintf(stderr, "ERROR: Socket creation failed.\n");
        return -2;
    }

    /* Get IP address and PORT from environment variables */

    if (CHECK_STR(connection->tlayer.tcpip.ip_addr)) {
        fprintf(stderr, "Unable to get the IP addr.");
        close(connection->tlayer.tcpip.ctx.sockfd);
        return -2;
    }

    /* Assign server IP and PORT */

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(connection->tlayer.tcpip.ip_addr);
    servaddr.sin_port = htons(connection->tlayer.tcpip.port);

    /* Connect the client socket to server socket */

    if (connect(connection->tlayer.tcpip.ctx.sockfd,
                (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        close(connection->tlayer.tcpip.ctx.sockfd);
        if (errno == ECONNREFUSED ||
            errno == ENETUNREACH ||
            errno == ETIMEDOUT ||
            errno == ECONNRESET ||
            errno == EHOSTUNREACH ||
            errno == ENETDOWN) {
            return -1;
        }
        return -2;
    }

    connection->tlayer.tcpip.ctx.pfds[0].fd = connection->tlayer.tcpip.ctx.sockfd;
    connection->tlayer.tcpip.ctx.pfds[0].events = POLLIN;

    printf("Connected to the server %s:%d.\n", connection->tlayer.tcpip.ip_addr,
                                               connection->tlayer.tcpip.port);

    return 0;
}

int shv_tcpip_posix_read(struct shv_connection *connection, void *buf, size_t len)
{
    return read(connection->tlayer.tcpip.ctx.sockfd, buf, len);
}

int shv_tcpip_posix_write(struct shv_connection *connection, void *buf, size_t len)
{
    return write(connection->tlayer.tcpip.ctx.sockfd, buf, len);
}

int shv_tcpip_posix_close(struct shv_connection *connection)
{
    int ret;

    ret = close(connection->tlayer.tcpip.ctx.sockfd);
    if (ret < 0) {
        fprintf(stderr, "ERROR: tcp_terminate() cannot close connection to the server, \
              errno = %d\n", errno);
    } else if (ret == 0) {
        fprintf(stderr, "Client successfully disconnected.\n");
    }

    return ret;
}

int shv_tcpip_posix_dataready(struct shv_connection *connection, int timeout)
{
    struct shv_tlayer_tcpip_ctx *tctx = &connection->tlayer.tcpip.ctx;
    int ret = poll(tctx->pfds, 2, timeout);
    if (ret <= 0) {
        return ret;
    }

    /* The pipe signalling termination has data ready - simulate this as a timeout */
    if (tctx->pfds[1].revents & POLLIN) {
        return 0;
    }

    if (tctx->pfds[0].revents & POLLIN) {
        return 1;
    }
    if (tctx->pfds[0].revents & POLLHUP || tctx->pfds[0].revents & POLLERR) {
        int dest;
        socklen_t len = sizeof(dest);
        getsockopt(tctx->sockfd, SOL_SOCKET, SO_ERROR, &dest, &len);
        fprintf(stderr, "ERROR: error on sock's fd, errno=%d, getsockopterr=%d\n", errno, dest);
    }
    return -1;
}

static void *__shv_process(void *arg)
{
    struct shv_con_ctx *shv_ctx = (struct shv_con_ctx *)arg;
    shv_ctx->thrd_ctx.thrd_ret = shv_process(shv_ctx);
    /* Report a hard error */
    if (shv_ctx->thrd_ctx.thrd_ret == -1 && shv_ctx->at_signlr != NULL) {
        shv_ctx->at_signlr(shv_ctx, SHV_ATTENTION_ERROR);
    }
    return NULL;
}

int shv_create_process_thread(int thrd_prio, struct shv_con_ctx *ctx)
{
    int ret;
    int policy;
    pthread_attr_t attr;
    pthread_attr_t *pattr;
    struct sched_param schparam;

    /* Create the pipe to the dataready function. The poll function is used but we expect
     * very large timeouts to be used in the SHV application.
     */
    if (pipe(ctx->thrd_ctx.fildes) < 0) {
        return -1;
    }

    /* Do a bit of hacking - in this case, the connection is specified,
     * so we should have no problem assigning to pfds[1].
     */
    switch (ctx->connection->tlayer_type) {
    case SHV_TLAYER_TCPIP:
        ctx->connection->tlayer.tcpip.ctx.pfds[1].fd = ctx->thrd_ctx.fildes[0];
        ctx->connection->tlayer.tcpip.ctx.pfds[1].events = POLLIN;
        break;
    case SHV_TLAYER_SERIAL:
        ctx->connection->tlayer.serial.ctx.pfds[1].fd = ctx->thrd_ctx.fildes[0];
        ctx->connection->tlayer.serial.ctx.pfds[1].events = POLLIN;
        break;
    default:
        return -2;
    }

    if (thrd_prio != -1) {
        ret = pthread_attr_init(&attr);
        RETLZ_ERROR(error);
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        RETLZ_ERROR(error);
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        RETLZ_ERROR(error);
        ret = pthread_getschedparam(pthread_self(), &policy, &schparam);
        RETLZ_ERROR(error);
        schparam.sched_priority = thrd_prio;
        ret = pthread_attr_setschedparam(&attr, &schparam);
        RETLZ_ERROR(error);
        pattr = &attr;
    } else {
        pattr = NULL;
    }
    ret = pthread_create(&ctx->thrd_ctx.id, pattr, __shv_process, (void *)ctx);
    RETLZ_ERROR(error);
    return ret;

error:
    ctx->err_no = SHV_PROC_THRD;
    return -1;
}

void shv_stop_process_thread(struct shv_con_ctx *shv_ctx)
{
    /* Write to the pipe to simulate the instant timeout */
    /* The write is enclosed in {} to suppress warn_unused_result warning */
    { write(shv_ctx->thrd_ctx.fildes[1], "x", 1); }

    /* Wait for it to join */
    pthread_join(shv_ctx->thrd_ctx.id, NULL);

    close(shv_ctx->thrd_ctx.fildes[0]);
    close(shv_ctx->thrd_ctx.fildes[1]);
}
