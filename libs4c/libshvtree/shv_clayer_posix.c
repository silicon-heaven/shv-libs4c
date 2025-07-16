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
#include <shv/tree/shv_file_com.h>

#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX)
    #include <zlib.h>
#elif defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    #include <nuttx/crc32.h>
    #include <nxboot.h>
    #include <sys/ioctl.h>
    #include <nuttx/mtd/mtd.h>
#endif

/* The whole file is common for Linux and NuttX but NuttX does not use zlib's CRC */

#define CHECK_STR(str) (str == NULL || strnlen(str, 100) == 0)
#define RETLZ_ERROR(__err_label) if (ret < 0) goto __err_label
#define CHUNK_SIZE ((size_t)64)

static int shv_posix_check_opened_file(shv_file_node_t *item)
{
    if (!(item->fctx.flags & BITFLAG_OPENED)) {
#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX)
        item->fctx.fd = open(item->name, O_RDWR | O_CREAT, 0644);

        if (item->fctx.fd < 0) {
            return -1;
        }
#elif defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
        if (strncmp(item->name, "*NXBOOT_MTD*", 12) == 0) {
            item->fctx.fd = nxboot_open_update_partition();
        } else {
            item->fctx.fd = open(item->name, O_RDWR | O_CREAT, 0644);
        }

        if (item->fctx.fd < 0) {
            return -1;
        }
#endif
        item->fctx.flags |= BITFLAG_OPENED;
    }
    return 0;
}

static int shv_posix_get_size(shv_file_node_t *item)
{
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_NUTTX
    /* Since NXBoot has a different API, suppose the file is zero bytes big.
     * This overcomes following conditions.
     */
    if (strncmp(item->name, "*NXBOOT_MTD*", 12) == 0) {
        return 0;
    }
#endif
    struct stat st;
    if (stat(item->name, &st) < 0) {
        return -1;
    }
    return st.st_size;
}

int shv_file_node_writer(shv_file_node_t *item, void *buf, size_t count)
{
    int fsize;

    if (shv_posix_check_opened_file(item) >= 0) {
        /* First, check the current file size */
        fsize = shv_posix_get_size(item);
        if (fsize < 0)  {
            return -1;
        }
        /* Update count if needed. Be an ostrich and suppose st_size > 0 */
        if (fsize >= item->file_maxsize) {
            return 0;
        } else if (fsize + count > item->file_maxsize) {
            count = item->file_maxsize - fsize;
        }

        return write(item->fctx.fd, buf, count);
    }
    return -1;
}

int shv_file_node_seeker(shv_file_node_t *item, int offset)
{
    int fsize;

    if (shv_posix_check_opened_file(item) >= 0) {
        /* First, check boundaries */
        fsize = shv_posix_get_size(item);
        if (fsize < 0) {
            return -1;
        }

        /* Cap it at the maximum file boundary. */
        if (fsize + offset >= item->file_maxsize) {
            offset = item->file_maxsize;
        }
        return lseek(item->fctx.fd, (off_t)offset, SEEK_SET);
    }
    return -1;
}

int shv_file_node_crc32(shv_file_node_t *item, int start, size_t size, uint32_t *result)
{
    /* The calculation between Linux and NuttX differs a bit. While both implementations
     * use the IEEE 802.3, the process is a bit different.
     * Linux: init=0, use zlib
     * NuttX: init=0xFFFFFFFF, the result must be negated, use internal CRC table.
     */

    unsigned char buffer[CHUNK_SIZE];

    /* Sanity check. Don't allow computation beyond the file's maximum size. */
    if (item == NULL || start < 0 || result == NULL || (start + size) >= item->file_maxsize) {
        return -1;
    }

    /* In this case, it is better to sync the data to assure all data was
       stored into the physical medium. Also, close the file.
       Do this only if the file is opened. */
    if (item->fctx.flags & BITFLAG_OPENED) {
        fsync(item->fctx.fd);
        close(item->fctx.fd);
        item->fctx.flags &= ~BITFLAG_OPENED;
    }

    if (shv_posix_check_opened_file(item) < 0) {
        return -1;
    }

    /* This is a valid seek, as it inside the file's range */
    if (lseek(item->fctx.fd, start, SEEK_SET) < 0) {
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
        bytes_read = read(item->fctx.fd, buffer, toread);
        if (bytes_read < 0) {
            return -1;
        } else if (bytes_read == 0) {
            /* The boundary was reached and thus this is the final read */
            close(item->fctx.fd);
            item->fctx.flags &= ~BITFLAG_OPENED;
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
    close(item->fctx.fd);
    item->fctx.flags &= ~BITFLAG_OPENED;
    printf("CRC calc OK!\n");
    return 0;
}

static int serial_init(struct shv_tlayer_serial_ctx *sctx)
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

static int serial_read(struct shv_tlayer_serial_ctx *sctx, void *buf, size_t len)
{
    if (sctx == NULL || buf == NULL) {
        return -1;
    }

    return read(sctx->fd, buf, len);
}

static int serial_write(struct shv_tlayer_serial_ctx *sctx, void *buf, size_t len)
{
    if (sctx == NULL || buf == NULL) {
        return -1;
    }

    return write(sctx->fd, buf, len);
}

static int serial_close(struct shv_tlayer_serial_ctx *sctx)
{
    if (sctx == NULL) {
        return -1;
    }

    tcsetattr(sctx->fd, TCSANOW, &sctx->term_backup);
    return close(sctx->fd);
}

static int tcpip_init(struct shv_connection *connection)
{
    struct sockaddr_in servaddr;

    /* Socket creation */

    connection->tlayer.tcpip.ctx.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connection->tlayer.tcpip.ctx.sockfd == -1) {
        printf("ERROR: Socket creation failed.\n");
        return -2;
    }

    /* Get IP address and PORT from environment variables */

    if (CHECK_STR(connection->tlayer.tcpip.ip_addr)) {
        printf("Unable to get the IP addr.");
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

static int tcpip_read(struct shv_connection *connection, void *buf, size_t len)
{
    return read(connection->tlayer.tcpip.ctx.sockfd, buf, len);
}

static int tcpip_write(struct shv_connection *connection, void *buf, size_t len)
{
    return write(connection->tlayer.tcpip.ctx.sockfd, buf, len);
}

static int tcpip_close(struct shv_tlayer_tcpip_ctx *tctx)
{
    int ret;

    ret = close(tctx->sockfd);
    if (ret < 0) {
      printf("ERROR: tcp_terminate() cannot close connection to the server, \
              errno = %d\n", errno);
    } else if (ret == 0) {
      printf("Client successfully disconnected.\n");
    }

    return ret;
}

static int serial_dataready(struct shv_tlayer_serial_ctx *sctx, int timeout)
{
    return -1;
}

static int tcpip_dataready(struct shv_tlayer_tcpip_ctx *tctx, int timeout)
{
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
        printf("ERROR: error on sock's fd, errno=%d, getsockopterr=%d\n", errno, dest);
    }
    return -1;
}

int shv_tlayer_init(struct shv_connection *connection)
{
    switch (connection->tlayer_type) {
    case SHV_TLAYER_TCPIP:
        return tcpip_init(connection);
    case SHV_TLAYER_SERIAL:
        return serial_init(&connection->tlayer.serial.ctx);
    default:
        return -2;
    }
}

int shv_tlayer_read(struct shv_connection *connection, void *buf, size_t len)
{
    switch (connection->tlayer_type) {
    case SHV_TLAYER_TCPIP:
        return tcpip_read(connection, buf, len);
    case SHV_TLAYER_SERIAL:
        return serial_read(&connection->tlayer.serial.ctx, buf, len);
    default:
        return -1;
    }
}

int shv_tlayer_write(struct shv_connection *connection, void *buf, size_t len)
{
    switch (connection->tlayer_type) {
    case SHV_TLAYER_TCPIP:
        return tcpip_write(connection, buf, len);
    case SHV_TLAYER_SERIAL:
        return serial_write(&connection->tlayer.serial.ctx, buf, len);
    default:
        return -1;
    }
}

int shv_tlayer_close(struct shv_connection *connection)
{
    switch (connection->tlayer_type) {
    case SHV_TLAYER_TCPIP:
        return tcpip_close(&connection->tlayer.tcpip.ctx);
    case SHV_TLAYER_SERIAL:
        return serial_close(&connection->tlayer.serial.ctx);
    default:
        return -1;
    }
}

int shv_tlayer_dataready(struct shv_connection *connection, int timeout)
{
    switch (connection->tlayer_type) {
    case SHV_TLAYER_TCPIP:
        return tcpip_dataready(&connection->tlayer.tcpip.ctx, timeout);
    case SHV_TLAYER_SERIAL:
        return serial_dataready(&connection->tlayer.serial.ctx, timeout);
    default:
        return -1;
    }
}

static void *__shv_process(void *arg)
{
    shv_con_ctx_t *shv_ctx = (shv_con_ctx_t *)arg;
    shv_ctx->thrd_ctx.thrd_ret = shv_process(shv_ctx);
    return NULL;
}

int shv_create_process_thread(int thrd_prio, shv_con_ctx_t *ctx)
{
    int ret;
    int policy;
    pthread_attr_t attr;
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
    ret = pthread_create(&ctx->thrd_ctx.id, NULL, __shv_process, (void *)ctx);
    RETLZ_ERROR(error);
    return ret;

error:
    ctx->err_no = SHV_PROC_THRD;
    return -1;
}

void shv_stop_process_thread(shv_con_ctx_t *shv_ctx)
{
    /* Write to the pipe to simulate the instant timeout */
    write(shv_ctx->thrd_ctx.fildes[1], "x", 1);

    /* Wait for it to join */
    pthread_join(shv_ctx->thrd_ctx.id, NULL);

    close(shv_ctx->thrd_ctx.fildes[0]);
    close(shv_ctx->thrd_ctx.fildes[1]);
}
