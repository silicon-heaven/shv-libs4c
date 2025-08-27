/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_com_common.h
 * @brief Common functions used in SHV, not to be used directly
 */

#pragma once

#include <stddef.h>
#include <shv/chainpack/ccpcp.h>

typedef struct shv_con_ctx shv_con_ctx_t;

/**
 * @brief A handler responsible for the sending of data
 *
 * @param ctx
 * @param size_hint
 */
void shv_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint);

/**
 * @brief A handler responsible for the reception of data, in case
 *        the request needs more
 *
 * @param ctx
 * @return Number of read bytes, -1 in case of failure
 */
size_t shv_underrflow_handler(struct ccpcp_unpack_context * ctx);

/**
 * @brief Packs the head of the message for client reply
 *
 * @param shv_ctx
 * @param rid
 */
void shv_pack_head_reply(shv_con_ctx_t *shv_ctx, int rid);

/**
 * @brief Discard data of arbitrary type (container, string, blob).
 *        Also discards all data in a container, if the unpacking happens in a container.
 *
 * @param shv_ctx
 * @return 0 in case of success, -1 in case of failure
 */
int shv_unpack_discard(shv_con_ctx_t * shv_ctx);
