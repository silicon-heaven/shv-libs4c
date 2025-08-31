/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_connection.h
 * @brief SHV connection parameters and transport layer definitions
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "shv_clayer_posix.h"

/* Default reconnect period */
#define SHV_DEFAULT_RECONNECT_PERIOD ((int)30)

/* Available transport layers defined by Silicon Heaven. */
enum shv_tlayer_type
{
    SHV_TLAYER_SERIAL = 0,   /* Serial port, CDC/ACM ... */
    SHV_TLAYER_TCPIP,        /* TCP/IP */
    SHV_TLAYER_LOCAL_DOMAIN, /* Local domain */
    SHV_TLAYER_CANBUS        /* CAN Bus*/
};

/* Serial communication frame packing */
enum shv_tlayer_serial_pack_state
{
    STX = 0,
    ESC,
    DATA,
    ETX,
    CRC32 
};

/* CAN Bus communication packing */
enum shv_tlayer_canbus_pack_state
{
    FIRST = 0,
    NOTLAST,
    LAST
};

/* Forward declaration */
struct shv_connection;

/**
 * @brief Platform dependant function. Inits the transport layer
 *        communication.
 * 
 * @param connection
 * @return 0 in case of success, -1 otherwise
 */
typedef int (*shv_tlayer_init)(struct shv_connection *connection);

/**
 * @brief Platform dependant function. Reads at most len bytes to buf
 *        from a given transport layer.
 * 
 * @param connection
 * @param buf 
 * @param len 
 * @return > 0 (read bytes) in case of success, -1 in case of failure,
 *         When 0 is returned, it indicates no available data in near future,
 *         stopping the connection.
 * @attention The function can be blocking.
 */
typedef int (*shv_tlayer_read)(struct shv_connection *connection, void *buf, size_t len);

/**
 * @brief Platform dependant function. Writes at most len bytes from buf
 *        to the transport layer.
 * 
 * @param connection 
 * @param buf 
 * @param len 
 * @return >= 0 (written bytes) in case of success, -1 otherwise
 * @attention The function can be blocking.
 */
typedef int (*shv_tlayer_write)(struct shv_connection *sctx, void *buf, size_t len);

/**
 * @brief Platform dependant function. Terminates the transport layer connection.
 * 
 * @param connection 
 * @return int 
 */
typedef int (*shv_tlayer_close)(struct shv_connection *connection);

/**
 * @brief Platform dependant function. Signalizes whether data are ready to be read.
 *        A timeout in ms can be specified (in case timeout > -1)
 * 
 * @param connection 
 * @param timeout in ms, anything < -1 means infinite waiting
 * @return -1 in case of error, 0 in case the polling timeouted, 1 in case of ready data
 */
typedef int (*shv_tlayer_dataready)(struct shv_connection *connection, int timeout);

/**
 * @brief Defines the parameters of the SHV connection to the broker.
 *
 */
struct shv_connection
{
    const char *broker_user;
    const char *broker_password;
    const char *broker_mount;
    const char *device_id;
    enum shv_tlayer_type tlayer_type;
    int reconnect_period;             /* During initialization, this is set to a default value */
    int reconnect_retries;            /* Everything <= 0 counts as infinite */
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
    struct {
        shv_tlayer_init      init;
        shv_tlayer_read      read;
        shv_tlayer_write     write;
        shv_tlayer_close     close;
        shv_tlayer_dataready dataready;
    } tops; /* Transport layer ops */
};

/**
 * @brief Initialize a shv_connection struct.
 *        Use this struct to specify the connection information, as well
 *        as the transport layer.
 * 
 * @param connection
 * @param tlayer 
 */
void shv_connection_init(struct shv_connection *connection, enum shv_tlayer_type tlayer);

/**
 * @brief Initialize an already shv_connection struct to tcp/ip mode.
 *
 * @param connection
 * @param ip_addr
 * @param port
 * @return int
 */
int shv_connection_tcpip_init(struct shv_connection *connection,
                              const char *ip_addr, uint16_t port);

