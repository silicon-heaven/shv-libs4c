/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                   <pressste@fel.cvut.cz>
 */

/**
 * @file shv_file_com.c
 * @brief The implementation of file nodes methods.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#include <shv/chainpack/ccpcp.h>
#include <shv/chainpack/cchainpack.h>
#include <shv/tree/shv_file_com.h>
#include <shv/tree/shv_com.h>
#include <shv/tree/shv_com_common.h>
#include <shv/tree/shv_tree.h>
#include <ulut/ul_utdefs.h>

/* The Write method unpack state */
enum shv_unpack_write_state
{
  IMAP_START = 0, /* Wait for the IMAP start */
  REQUEST_1,      /* Wait for the 1 key */
  LIST_START,     /* Wait for the List start */
  OFFSET,         /* Read the offset */
  BLOB,           /* Read the blob */
  LIST_STOP,      /* Wait for the List end */
  IMAP_STOP       /* Wait for the IMAP end */
};

/* The Crc method unpack state */
enum shv_unpack_crc_state
{
  C_IMAP_START,
  C_IMAP_END,
  C_REQUEST_1,
  C_LIST_START,
  C_OFFSET,
  C_LIST_END,
  C_SIZE
};

void shv_file_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    /* SHV only supports regular files, as of July 2025 */ 
    if (item->file_type != REGULAR) {
        shv_send_error(shv_ctx, rid, SHV_RE_INVALID_PARAMS, NULL);
        return; 
    }

    ccpcp_pack_context_init(&shv_ctx->pack_ctx,shv_ctx->shv_data, SHV_BUF_LEN,
                            shv_overflow_handler);

    for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
        if (shv_ctx->shv_send) {
            cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
        }

        shv_ctx->shv_len = 0;
        cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);
        shv_pack_head_reply(shv_ctx, rid);

        /* An Imap in a Imap */
        cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);
        cchainpack_pack_int(&shv_ctx->pack_ctx, 2);
        cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);

        /* The first key (file type) */
        cchainpack_pack_int(&shv_ctx->pack_ctx, FN_TYPE);
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_type);

        /* The second key (file size) */
        cchainpack_pack_int(&shv_ctx->pack_ctx, FN_SIZE);
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_maxsize);

        /* The third key (page size) */
        cchainpack_pack_int(&shv_ctx->pack_ctx, FN_PAGESIZE);
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_pagesize);

        /* The sixth key (max send size), limit this by the pagesize multiples */
        cchainpack_pack_int(&shv_ctx->pack_ctx, FN_MAXWRITE);
        /* 4 * PG_SIZE is reasonable */
        cchainpack_pack_int(&shv_ctx->pack_ctx, 4 * item->file_pagesize);

        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
    }
}

void shv_file_send_size(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    ccpcp_pack_context_init(&shv_ctx->pack_ctx,shv_ctx->shv_data, SHV_BUF_LEN,
                            shv_overflow_handler);

    for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
        if (shv_ctx->shv_send) {
            cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
        }

        shv_ctx->shv_len = 0;
        cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);
        shv_pack_head_reply(shv_ctx, rid);

        cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);
        /* Reply */
        cchainpack_pack_int(&shv_ctx->pack_ctx, 2);
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_maxsize);

        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
    }
}

int shv_file_process_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    int ret;
    ccpcp_unpack_context *ctx = &shv_ctx->unpack_ctx;
    item->platform_error = false;
    struct shv_file_node_fctx *fctx = (struct shv_file_node_fctx *) item->fctx;

    do {
        cchainpack_unpack_next(ctx);
        if (ctx->err_no != CCPCP_RC_OK) {
            return -1;
        }

        switch (item->state) {
        case IMAP_START: {
            // the start of imap, proceed next
            if (ctx->item.type == CCPCP_ITEM_IMAP) {
                item->state = REQUEST_1;
            }
            break;
        }
        case REQUEST_1: {
            // wait for UInt or Int (namely number 1)
            if (ctx->item.type == CCPCP_ITEM_INT) {
                if (ctx->item.as.Int == 1) {
                    item->state = LIST_START;
                } else {
                    // something different received
                    shv_unpack_discard(shv_ctx);
                    ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                    item->state = IMAP_START;
                }
            } else if (ctx->item.type == CCPCP_ITEM_UINT) {
                if (ctx->item.as.UInt == 1) {
                    item->state = LIST_START;
                } else {
                    // something different received
                    shv_unpack_discard(shv_ctx);
                    ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                    item->state = IMAP_START;
                }
            } else {
                // Int or UInt expected!
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                item->state = IMAP_START;
            }
            break;
        }
        case LIST_START: {
            if (ctx->item.type == CCPCP_ITEM_LIST) {
                item->state = OFFSET;
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                item->state = IMAP_START;
            }
            break;
        }
        case OFFSET: {
            if (ctx->item.type == CCPCP_ITEM_INT) {
                /* save the loaded offset into the struct */
                item->file_offset = ctx->item.as.Int;
                item->state = BLOB;
                if (!item->platform_error) {
                    ret = item->fops.seeker(item, item->file_offset);
                    if (ret < 0) {
                        item->platform_error = true;
                    }
                }
            } else { 
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                item->state = IMAP_START;
            }
            break;
        }
        case BLOB: {
            if (ctx->item.type == CCPCP_ITEM_BLOB) {
                /* If the writer returns zero, it can mean the offset was set beyond
                 * the maximum size (or equal to it). Thus writer will return zero,
                 * it won't write anything but every other writes should be parsed correctly.
                 * Yes, triggering an error is a solution too but the expected usage
                 * is to get the file's attributes beforehand and work with that.
                 */
                if (!item->platform_error) {
                    ret = item->fops.writer(item, ctx->item.as.String.chunk_start,
                                       ctx->item.as.String.chunk_size);
                    if (ret < 0) {
                        item->platform_error = true;
                    }
                }
                if (ctx->item.as.String.last_chunk) {
                    /* We received the last chunk of the string, we can proceed further */
                    item->state = LIST_STOP;
                }
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
                item->state = IMAP_START;
            }
            break;
        }
        case LIST_STOP: {
            if (ctx->item.type == CCPCP_ITEM_CONTAINER_END) {
                item->state = IMAP_STOP;
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
                item->state = IMAP_START;
            }
            break;
        }
        case IMAP_STOP: {
            if (ctx->item.type != CCPCP_ITEM_CONTAINER_END) {
                // something horrible happened
                ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
                item->state = IMAP_START;
            } else {
                // restore state and return
                item->state = IMAP_START;
                return 0;
            }
            break;
        }
        default:
            break;
        }
    } while (ctx->err_no == CCPCP_RC_OK);

    if (ctx->err_no != CCPCP_RC_OK) {
        return -1;
    }

    return 0;
}

/*
 * The CRC parsing procedure is not that straightforward.
 * We know that the request is always in the form of an Imap. But we must perform CRC calculation
 * based on what's inside the Imap. If the Imap is empty, the CRC is calculated over the whole map.
 * If only the first number is passed (offset), CRC is calculated until the end of the file.
 * If both numbers are passed (offset and size), CRC is calcaulted over size bytes.
 */
int shv_file_process_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    int ret;
    int parse_result = -1;
    size_t size;
    int start;
    ccpcp_unpack_context *ctx = &shv_ctx->unpack_ctx;

    do {
        /* The parsed result is ready */
        if (parse_result >= 0) {
            break;
        }

        cchainpack_unpack_next(ctx);
        if (ctx->err_no != CCPCP_RC_OK) {
            return -1;
        }

        switch (item->crcstate) {
        case C_IMAP_START:
            if (ctx->item.type == CCPCP_ITEM_IMAP) {
                item->crcstate = C_IMAP_END;
                /* Initialize these values so it can be decided during the automata traversal */
                item->crc_offset = -1;
                item->crc_size = -1;
            }
            break;
        case C_IMAP_END:
            if (ctx->item.type == CCPCP_ITEM_CONTAINER_END) {
                item->crcstate = C_IMAP_START;
                /* decide on what was parsed */
                if (item->crc_offset == -1) {
                    /* Not even offset was passed, calculate over the whole file */
                    parse_result = 0;
                } else {
                    if (item->crc_size == -1) {
                        /* Only the offset was passed */
                        parse_result = 1;
                    } else {
                        /* Both number were passed */
                        parse_result = 2;
                    }
                }
                break;
            } else {
                item->crcstate = C_REQUEST_1;
            }
        case C_REQUEST_1:
            if (ctx->item.type == CCPCP_ITEM_INT) {
                if (ctx->item.as.Int == 1) {
                    item->crcstate = C_LIST_START;
                } else {
                    shv_unpack_discard(shv_ctx);
                    ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                }
            } else if (ctx->item.type == CCPCP_ITEM_UINT) {
                if (ctx->item.as.UInt == 1) {
                    item->crcstate = C_LIST_START;
                } else {
                    shv_unpack_discard(shv_ctx);
                    ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                }
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
            }
            break;
        case C_LIST_START:
            if (ctx->item.type == CCPCP_ITEM_LIST) {
                item->crcstate = C_OFFSET;
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
            }
            break;
        case C_OFFSET:
            if (ctx->item.type == CCPCP_ITEM_INT) {
                item->crc_offset = ctx->item.as.Int;
                item->crcstate = C_SIZE;
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
            }
            break;
        case C_LIST_END:
            if (ctx->item.type == CCPCP_ITEM_CONTAINER_END) {
                item->crcstate = C_IMAP_END;
            }
            break;
        case C_SIZE:
            // this marks the end of list parsing
            if (ctx->item.type == CCPCP_ITEM_CONTAINER_END) {
                item->crcstate = C_IMAP_END;
            } else if (ctx->item.type == CCPCP_ITEM_INT) {
                item->crc_size = ctx->item.as.Int;
                item->crcstate = C_LIST_END;
            } else {
                shv_unpack_discard(shv_ctx);
                ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
            }
            break;
        }
    } while (ctx->err_no == CCPCP_RC_OK);
    
    /* In case of 0 and 1, the file can be much smaller than file_maxsize.
     * But suppose the crc32 calculator does not go beyond the actual file size.
     */
    if (parse_result == 0) {
        start = 0;
        size = item->file_maxsize;
    } else if (parse_result == 1) {
        start = item->crc_offset;
        size = item->file_maxsize - item->crc_offset;
    } else if (parse_result == 2) {
        start = item->crc_offset;
        size = item->crc_size;
    }
    if (parse_result >= 0) {
        if (item->fops.crc32(item, start, size, &item->crc) < 0) {
            item->platform_error = true;
        } else {
            item->platform_error = false;
        }
        return 0;
    }
    return -1;
}

int shv_file_node_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    int ret = 0;
    shv_file_node_t *file_node = UL_CONTAINEROF(item, shv_file_node_t, shv_node);
    ret = shv_file_process_write(shv_ctx, rid, file_node);
    if (ret < 0) {
        shv_send_error(shv_ctx, rid, SHV_RE_INVALID_PARAMS, "Garbled data");
    } else if (file_node->platform_error) {
        /* It is an error, but not protocol wise. Just inform the other end. */
        shv_send_error(shv_ctx, rid, SHV_RE_PLATFORM_ERROR, "I/O Error");
    } else {
        shv_send_empty_response(shv_ctx, rid);
    }
    return 0;
}

int shv_file_node_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    int ret;
    shv_file_node_t *file_node = UL_CONTAINEROF(item, shv_file_node_t, shv_node);
    ret = shv_file_process_crc(shv_ctx, rid, file_node);
    if (ret < 0) {
        shv_send_error(shv_ctx, rid, SHV_RE_INVALID_PARAMS, "Garbled data");
    } else if (file_node->platform_error) {
        /* It is an error, but not protocol wise. Just inform the other end. */
        shv_send_error(shv_ctx, rid, SHV_RE_PLATFORM_ERROR, "I/O Error");
    } else {
        shv_send_uint(shv_ctx, rid, file_node->crc);
    }
    return ret;
}

int shv_file_node_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_file_node_t *file_node = UL_CONTAINEROF(item, shv_file_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_uint(shv_ctx, rid, file_node->file_maxsize);
    return 0;
}

int shv_file_node_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_file_node_t *file_node = UL_CONTAINEROF(item, shv_file_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_file_send_stat(shv_ctx, rid, file_node);
    return 0;
}

const shv_method_des_t shv_dmap_item_file_node_crc =
{
    .name = "crc",
    .method = shv_file_node_crc
};

const shv_method_des_t shv_dmap_item_file_node_write =
{
    .name = "write",
    .method = shv_file_node_write
};

const shv_method_des_t shv_dmap_item_file_node_stat =
{
    .name = "stat",
    .method = shv_file_node_stat
};

const shv_method_des_t shv_dmap_item_file_node_size =
{
    .name = "size",
    .method = shv_file_node_size
};
