/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                   <pressste@fel.cvut.cz>
 */

/**
 * @file shv_connection.c
 * @brief The implementation of SHV Transport Layer methods.
 */

#include <stdlib.h>
#include <string.h>
#include <shv/tree/shv_connection.h>

void shv_connection_init(struct shv_connection *connection, enum shv_tlayer_type tlayer)
{
    memset(connection, 0, sizeof(struct shv_connection));
    connection->tlayer_type = tlayer;
    connection->reconnect_period = SHV_DEFAULT_RECONNECT_PERIOD;
    /* Infinite */
    connection->reconnect_retries = 0;
}

int shv_connection_tcpip_init(struct shv_connection *connection,
                              const char *ip_addr, uint16_t port)
{
    if (connection->tlayer_type != SHV_TLAYER_TCPIP) {
        return -1;
    }
    
    connection->tlayer.tcpip.ip_addr = ip_addr;
    connection->tlayer.tcpip.port    = port;
    return 0;
}
