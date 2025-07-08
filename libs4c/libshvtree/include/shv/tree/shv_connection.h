#pragma once

#include <stdint.h>
#include <stddef.h>

#include "shv_clayer_posix.h"

enum shv_tlayer_type
{
    SHV_TLAYER_SERIAL = 0,
    SHV_TLAYER_TCPIP,
    SHV_TLAYER_LOCAL_DOMAIN,
    SHV_TLAYER_CANBUS
};

enum shv_tlayer_serial_pack_state
{
    STX = 0,
    ESC,
    DATA,
    ETX,
    CRC32 
};

enum shv_tlayer_canbus_pack_state
{
    FIRST = 0,
    NOTLAST,
    LAST
};

struct shv_connection
{
    const char *broker_user;
    const char *broker_password;
    const char *broker_mount;
    enum shv_tlayer_type tlayer_type;
    union
    {
        struct
        {
            enum shv_tlayer_serial_pack_state rx_state, tx_state;
            struct shv_tlayer_serial_ctx ctx;
        } serial;
        struct
        {
            const char *ip_addr;
            uint16_t port;
            struct shv_tlayer_tcpip_ctx ctx;
        } tcpip;
        struct
        {
            enum shv_tlayer_canbus_pack_state rx_state, tx_state;
            bool qos;
            struct shv_tlayer_canbus_ctx ctx;
        } canbus;
    } tlayer;
};

/**
 * @brief Platform dependant function. Inits the transport layer
 *        communication.
 * 
 * @param connection
 * @return 0 in case of success, -1 otherwise
 */
int shv_tlayer_init(struct shv_connection *connection);

/**
 * @brief Platform dependant function. Reads at most len bytes to buf
 *        from a given transport layer.
 * 
 * @param connection
 * @param buf 
 * @param len 
 * @return 0 in case of success, -1 otherwise
 * @attention The function can be blocking.
 */
int shv_tlayer_read(struct shv_connection *connection, void *buf, size_t len);

/**
 * @brief Platform dependant function. Writes at most len bytes from buf
 *        to the transport layer.
 * 
 * @param connection 
 * @param buf 
 * @param len 
 * @return 0 in case of success, -1 otherwise
 * @attention The function can be blocking.
 */
int shv_tlayer_write(struct shv_connection *sctx, void *buf, size_t len);

/**
 * @brief Platform dependant function. Terminates the transport layer connection.
 * 
 * @param connection 
 * @return int 
 */
int shv_tlayer_close(struct shv_connection *connection);

/**
 * @brief Platform dependant function. Signalizes whether data are ready to be read.
 *        A timeout in ms can be specified (in case timeout > -1)
 * 
 * @param connection 
 * @param timeout in ms, anything < -1 means infinite waiting
 * @return -1 in case of error, 0 in case the polling timeouted, 1 in case of ready data
 */
int shv_tlayer_dataready(struct shv_connection *connection, int timeout);

/**
 * @brief Initialize a shv_connection struct.
 *        Use this struct to specify the connection information, as well
 *        as the transport layer.
 * 
 * @param connection
 * @param tlayer 
 */
void shv_connection_init(struct shv_connection *connection, enum shv_tlayer_type tlayer);

int shv_connection_tcpip_init(struct shv_connection *connection,
                              const char *ip_addr, uint16_t port);

