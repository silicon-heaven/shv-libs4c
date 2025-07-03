#include <stdio.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_connection.h>
#include <shv/tree/shv_clayer_posix.h>

#define CHECK_STR(str) (str == NULL || strnlen(str, 100) == 0)

int shv_posix_check_opened_file(struct shv_file_node_fctx *fctx, const char *name)
{
    if (!(fctx->flags & BITFLAG_OPENED)) {
        fctx->fd = open(name, O_RDWR | O_CREAT, 0644);
        if (fctx->fd < 0) {
            return -1;
        }
        fctx->flags |= BITFLAG_OPENED;
    }
    return 0;
}

int shv_file_node_writer(shv_file_node_t *item, void *buf, size_t count)
{
    if (shv_posix_check_opened_file(&item->fctx, item->name) >= 0) {
        return write(item->fctx.fd, buf, count);
    }
    return -1;
}

int shv_file_node_seeker(shv_file_node_t *item, int offset)
{
    if (shv_posix_check_opened_file(&item->fctx, item->name) >= 0) {
        return lseek(item->fctx.fd, (off_t)offset, SEEK_SET);
    }
    return -1;
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

    sctx->flags |= BITFLAG_OPENED;
    sctx->pfds[0].fd = sctx->fd;
    sctx->pfds[0].events = POLLIN;

    return 0;
error:
    close(sctx->fd);
    return -1;
}

static int serial_read(struct shv_tlayer_serial_ctx *sctx, void *buf, size_t len)
{
    if (sctx == NULL || buf == NULL ||
        !(sctx->flags & BITFLAG_OPENED)) {
        return -1;
    }

    return read(sctx->fd, buf, len);
}

static int serial_write(struct shv_tlayer_serial_ctx *sctx, void *buf, size_t len)
{
    if (sctx == NULL || buf == NULL || !(sctx->flags & BITFLAG_OPENED)) {
        return -1;
    }

    return write(sctx->fd, buf, len);
}

static int serial_close(struct shv_tlayer_serial_ctx *sctx)
{
    if (sctx == NULL) {
        return -1;
    }

    sctx->flags &= ~BITFLAG_OPENED;
    tcsetattr(sctx->fd, TCSANOW, &sctx->term_backup);
    return close(sctx->fd);
}

static int tcpip_init(struct shv_connection *connection)
{
    struct sockaddr_in servaddr;
    uint16_t port;
    const char *shv_broker_ip;

    /* Socket creation */

    connection->tlayer.tcpip.ctx.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connection->tlayer.tcpip.ctx.sockfd == -1) {
        printf("ERROR: Socket creation failed.\n");
        return -2;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    /* Get IP address and PORT from environment variables */

    shv_broker_ip = connection->tlayer.tcpip.ip_addr;
    if (CHECK_STR(shv_broker_ip))
    {
      printf("Unable to get the IP addr.");
      close(connection->tlayer.tcpip.ctx.sockfd);
      return -2;
    }

    port = connection->tlayer.tcpip.port;

    /* Assign server IP and PORT */

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(shv_broker_ip);
    servaddr.sin_port = htons(port);

    /* Connect the client socket to server socket */

    if (connect(connection->tlayer.tcpip.ctx.sockfd,
                (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
    {
      close(connection->tlayer.tcpip.ctx.sockfd);
      return -1;
    }

    connection->tlayer.tcpip.ctx.pfds[0].fd = connection->tlayer.tcpip.ctx.sockfd;
    connection->tlayer.tcpip.ctx.pfds[0].events = POLLIN;

    printf("Connected to the server %s:%d.\n", shv_broker_ip, port);

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
    int ret = poll(tctx->pfds, 1, (timeout * 1000) / 2);
    if (ret <= 0) {
        return ret;
    }

    if (tctx->pfds[0].revents & POLLIN) {
        return 1;
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
