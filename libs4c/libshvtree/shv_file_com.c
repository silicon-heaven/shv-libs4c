/*
  COPYRIGHT (C) 2025 Stepan Pressl 
    <pressl.stepan@gmail.com>
    <pressste@fel.cvut.cz>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#include "ccpcp.h"
#include "include/shv_file_com.h"
#include "shv_com.h"
#include "shv_tree.h"
#include "shv_file_com.h"
#include "cchainpack.h"

void shv_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    /* SHV only supports regular files, as of July 2025 */ 
    if (item->file_type != REGULAR) {
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
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_size);

        /* The third key (page size) */
        cchainpack_pack_int(&shv_ctx->pack_ctx, FN_PAGESIZE);
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_pagesize);

        /* The sixth key (max send size), limit this by the pagesize multiples */
        cchainpack_pack_int(&shv_ctx->pack_ctx, FN_MAXWRITE);
        /* 8 * PG_SIZE is reasonable */
        cchainpack_pack_int(&shv_ctx->pack_ctx, 8*item->file_pagesize);

        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
    }
}

void shv_send_size(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
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
        cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_size);

        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
    }
}


int shv_process_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    int ret;
    ccpcp_unpack_context *ctx = &shv_ctx->unpack_ctx;

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
                ret = shv_file_node_seeker(item, item->file_offset);
                if (ret < 0) {
                    // how to handle errors?
                    ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                    item->state = IMAP_START;
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
                item->received_bytes += ctx->item.as.String.chunk_size;
                /* Check overflow */
                if (item->received_bytes <= item->file_size) {
                    ret = shv_file_node_writer(item, ctx->item.as.String.chunk_start,
                                               ctx->item.as.String.chunk_size);
                    if (ret < 0) {
                        shv_unpack_discard(shv_ctx);
                        ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
                        item->state = IMAP_START;
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

void shv_confirm_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
    for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
        if (shv_ctx->shv_send) {
            cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
        }

        shv_ctx->shv_len = 0;
        cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);

        shv_pack_head_reply(shv_ctx, rid);

        /* Empty IMap */
        cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);
        cchainpack_pack_container_end(&shv_ctx->pack_ctx);

        shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
    }
}

/*
 * The CRC parsing procedure is not that straightforward.
 * We know that the request is always in the form of an Imap. But we must perform CRC calculation
 * based on what's inside the Imap. If the Imap is empty, the CRC is calculated over the whole map.
 * If only the first number is passed (offset), CRC is calculated until the end of the file.
 * If both numbers are passed (offset and size), CRC is calcaulted over size bytes.
 */
int shv_process_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
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
    
    if (parse_result == 0) {
        start = 0;
        size = file->file_size;
    } else if (parse_result == 1) {
        start = file->crc_offset;
        size = file->file_size - file->crc_offset; 
    } else if (parse_result == 2) {
        start = file->crc_offset;
        size = file->crc_size;
    }
    if (parse_result >= 0) {
        return shv_file_node_crc32(item->fctx, start, size, &file->crc);
    }
    return 0;
}

void shv_confirm_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
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

        /* Send reply with the crc as result (imap with the "2" key) */
        cchainpack_pack_int(&shv_ctx->pack_ctx, 2);
        cchainpack_pack_uint(&shv_ctx->pack_ctx, item->crc);

        cchainpack_pack_container_end(&shv_ctx->pack_ctx);
        shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
    }
}
