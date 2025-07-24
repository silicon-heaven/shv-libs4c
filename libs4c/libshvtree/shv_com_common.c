#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <shv/tree/shv_com.h>
#include <shv/tree/shv_com_common.h>
#include <shv/tree/shv_connection.h>
#include <ulut/ul_utdefs.h>

/****************************************************************************
 * Name: shv_overflow_handler
 *
 * Description:
 *  Overflow handler responsible for data send.
 *
 ****************************************************************************/

void shv_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint)
{

  shv_con_ctx_t *shv_ctx = UL_CONTAINEROF(ctx, shv_con_ctx_t, pack_ctx);
  size_t to_send = ctx->current - ctx->start;
  char * ptr_data = shv_ctx->shv_data;
  int ret = 0;

  if (shv_ctx->shv_send)
    {
      while ((shv_ctx->write_err == 0) && (to_send > 0))
        {
          ret = shv_tlayer_write(shv_ctx->connection, ptr_data, to_send);
          if (ret <= 0)
            {
              printf("ERROR: Write error, ret = %d\n", ret);
              if (ret == -1)
                {
                  shv_ctx->write_err = 1;
                  printf("ERROR: Write error, errno = %d\n", errno);
                }
              break;
            }

          to_send -= ret;
          ptr_data += ret;
        }
    }

  shv_ctx->shv_len += ctx->current-ctx->start;
  ctx->current = ctx->start;
  ctx->end = ctx->start + SHV_BUF_LEN;
}

/****************************************************************************
 * Name: shv_underrflow_handler
 *
 * Description:
 *  Underflow handler responsible for data send.
 *
 ****************************************************************************/

size_t shv_underrflow_handler(struct ccpcp_unpack_context * ctx)
{
  int i;

  shv_con_ctx_t *shv_ctx = UL_CONTAINEROF(ctx, shv_con_ctx_t, unpack_ctx);

  i = shv_tlayer_read(shv_ctx->connection, shv_ctx->shv_rd_data, sizeof(shv_ctx->shv_rd_data));
  if (i > 0)
    {
      ctx->start = shv_ctx->shv_rd_data;
      ctx->current = ctx->start;
      ctx->end = ctx->start + i;
    }

  return i;
}

/****************************************************************************
 * Name: shv_pack_head_reply
 *
 * Description:
 *  Packs the head of the message for client reply.
 *
 ****************************************************************************/

void shv_pack_head_reply(shv_con_ctx_t *shv_ctx, int rid)
{
  cchainpack_pack_meta_begin(&shv_ctx->pack_ctx);

  cchainpack_pack_int(&shv_ctx->pack_ctx, 1);
  cchainpack_pack_int(&shv_ctx->pack_ctx, 1);

  cchainpack_pack_int(&shv_ctx->pack_ctx, TAG_REQUEST_ID);
  cchainpack_pack_int(&shv_ctx->pack_ctx, rid);

  if (shv_ctx->cid_cnt == 1)
    {
      cchainpack_pack_int(&shv_ctx->pack_ctx, TAG_CALLER_IDS);
      cchainpack_pack_int(&shv_ctx->pack_ctx, shv_ctx->cid_ptr[0]);
    }
  else
    {
      cchainpack_pack_list_begin(&shv_ctx->pack_ctx);
      for (int i = 0; i < shv_ctx->cid_cnt; i++)
        {
          cchainpack_pack_int(&shv_ctx->pack_ctx, shv_ctx->cid_ptr[i]);
        }

      cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    }

  cchainpack_pack_container_end(&shv_ctx->pack_ctx);
}

/****************************************************************************
 * Name: shv_unpack_skip
 *
 * Description:
 *   Skip data inside container
 *
 ****************************************************************************/

int shv_unpack_skip(shv_con_ctx_t * shv_ctx)
{
  struct ccpcp_unpack_context *ctx = &shv_ctx->unpack_ctx;
  int level = 1;

  do
    {
      cchainpack_unpack_next(ctx);
      if (ctx->err_no != CCPCP_RC_OK) return -1;

      if ((ctx->item.type == CCPCP_ITEM_META) ||
          (ctx->item.type == CCPCP_ITEM_LIST) ||
          (ctx->item.type == CCPCP_ITEM_MAP) ||
          (ctx->item.type == CCPCP_ITEM_IMAP))
        {
          level++;
        }
      else if (ctx->item.type == CCPCP_ITEM_CONTAINER_END)
        {
          level--;
        }
    }
  while (level);
}


/****************************************************************************
 * Name: shv_unpack_discard
 *
 * Description:
 *   Discard current data of arbitrary type
 *
 ****************************************************************************/

int shv_unpack_discard(shv_con_ctx_t * shv_ctx)
{
  struct ccpcp_unpack_context *ctx = &shv_ctx->unpack_ctx;

  if ((ctx->item.type == CCPCP_ITEM_META) ||
      (ctx->item.type == CCPCP_ITEM_LIST) ||
      (ctx->item.type == CCPCP_ITEM_MAP) ||
      (ctx->item.type == CCPCP_ITEM_IMAP))
    {
      shv_unpack_skip(shv_ctx);
    }
  else
    {
      if (((ctx->item.type == CCPCP_ITEM_BLOB) ||
          (ctx->item.type == CCPCP_ITEM_STRING)))
        {
          while (ctx->item.as.String.last_chunk != 0)
            {
              cchainpack_unpack_next(ctx);
              if (ctx->err_no != CCPCP_RC_OK) return -1;
            }
        }
    }
  
  return 0;
}
