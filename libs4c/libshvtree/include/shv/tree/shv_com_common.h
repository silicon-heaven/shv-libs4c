#ifndef SHV_OUFLOW_HANDLER_H
#define SHV_OUFLOW_HANDLER_H

#include <stddef.h>
#include <shv/chainpack/ccpcp.h>
#include "shv_com.h"

extern int shv_write_err;

void shv_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint);
size_t shv_underrflow_handler(struct ccpcp_unpack_context * ctx);
void shv_pack_head_reply(shv_con_ctx_t *shv_ctx, int rid);
int shv_unpack_skip(shv_con_ctx_t * shv_ctx);
int shv_unpack_discard(shv_con_ctx_t * shv_ctx);


#endif
