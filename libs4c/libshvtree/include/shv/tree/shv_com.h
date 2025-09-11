/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Michal Lenc 2022-2025 <michallenc@seznam.cz>
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_com.h
 * @brief Main SHV communication and main SHV functions
 */

#pragma once

#include <stdint.h>
#include <stdatomic.h>
#include <shv/chainpack/cchainpack.h>

#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
  #include "shv_clayer_posix.h"
#endif
#include "shv_connection.h"

#define SHV_BUF_LEN  1024
#define SHV_MET_LEN  64
#define SHV_PATH_LEN 256

#define TAG_ERROR       8
#define TAG_REQUEST_ID  8
#define TAG_SHV_PATH    9
#define TAG_METHOD      10
#define TAG_CALLER_IDS  11

/**
 * @brief SHV Method response error codes enum.
 *
 */
enum shv_response_error_code
{
    SHV_RE_METHOD_NOT_FOUND = 2,
    SHV_RE_INVALID_PARAMS = 3,
    SHV_RE_PLATFORM_ERROR = 6,        /* An error occured when performing platform specific op */
    SHV_RE_FILE_MAXSIZE = 7,          /* A file operation went beyond the maximum size */
    SHV_RE_METHOD_CALL_EXCEPTION = 8,
    SHV_RE_LOGIN_REQUIRED = 10,
    SHV_RE_USER_ID_REQUIRED = 11,
    SHV_RE_NOT_IMPLEMENTED = 12,
    SHV_RE_TRY_AGAIN_LATER = 13,
    SHV_RE_REQUEST_INVALID = 14
};

/**
 * @brief SHV Connection error reporting enum.
 *
 */
enum shv_con_errno
{
    SHV_NO_ERROR = 0,
    SHV_PROC_THRD,     /* Unable to create the process thread */
    SHV_TLAYER_INIT,   /* Unable to init the transport layer */
    SHV_TLAYER_READ,   /* Failure reading from the transport layer */
    SHV_RECONNECTS,    /* The maximum number of reconnects to a broker reached */
    SHV_LOGIN,         /* Unable to login to the broker */
    SHV_CCPCP_PACK,    /* Error in the CCPCP pack context */
    SHV_CCPCP_UNPACK,  /* Error in the CCPCP unpack context */
    SHV_ERRNOS_COUNT
};

static const char *shv_con_errno_strs[] =
{
    [SHV_NO_ERROR] = "",
    [SHV_PROC_THRD] = "Process thread creation fail",
    [SHV_TLAYER_INIT] = "Tlayer init fail",
    [SHV_TLAYER_READ] = "Read from tlayer fail",
    [SHV_RECONNECTS] = "Too many reconnects",
    [SHV_LOGIN] = "Login to broker fail",
    [SHV_CCPCP_PACK] =  "Error in chainpack packing",
    [SHV_CCPCP_UNPACK] = "Error in chainpack unpacking"
};

/**
 * @brief SHV Attention reason enum used in `shv_attention_signaller`
 *        used to report SHV events to the application
 */
enum shv_attention_reason
{
    SHV_ATTENTION_ERROR,        /* A nonrecoverable error occured, you should inspect
                                   `err_no` in struct shv_con_ctx */
    SHV_ATTENTION_CONNECTED,    /* The thread succesfully connected to a broker */
    SHV_ATTENTION_DISCONNECTED, /* The connection was closed by the remote host */
    SHV_ATTENTION_COUNT
};

/* Forward declaration */
struct shv_con_ctx;

/**
 * @brief An attention signaller used to signal the application
 *        of some events
 */
typedef void (*shv_attention_signaller)(struct shv_con_ctx *shv_ctx,
                                        enum shv_attention_reason r);

/* Forward declaration */
struct shv_node;

/**
 * @brief Main SHV Communication context.
 *
 */
struct shv_con_ctx
{
    int stream_fd;
    int timeout;
    int rid;
    int cid_cnt;
    int cid_capacity;
    int *cid_ptr;
    enum shv_con_errno err_no;
    struct ccpcp_pack_context pack_ctx;
    struct ccpcp_unpack_context unpack_ctx;
    char shv_data[SHV_BUF_LEN];
    char shv_rd_data[SHV_BUF_LEN];
    int write_err;
    int shv_len;
    int shv_send;
    int reconnects;
    atomic_bool running;
    struct shv_thrd_ctx thrd_ctx;
    struct shv_node *root;
    struct shv_connection *connection;            /* Transport layer information */
    shv_attention_signaller at_signlr;            /* A user defined attention signaller callback */
};

struct shv_dir_res {
  const char *name;
  int flags;
  const char *param;
  const char *result;
  int access;
};

/* Forward declaration */
struct shv_str_list_it;

struct shv_str_list_it {
   const char * (*get_next_entry)(struct shv_str_list_it *it, int reset_to_first);
};

/**
 * @brief Get str of errno contained in ctx
 *
 * @param ctx
 * @return const char*
 */
static inline const char *shv_errno_str(struct shv_con_ctx *ctx)
{
    return shv_con_errno_strs[ctx->err_no];
}

void shv_send_int(struct shv_con_ctx *shv_ctx, int rid, int num);
void shv_send_uint(struct shv_con_ctx *shv_ctx, int rid, unsigned int num);
void shv_send_double(struct shv_con_ctx *shv_ctx, int rid, double num);
void shv_send_str(struct shv_con_ctx *shv_ctx, int rid, const char *str);
void shv_send_str_list(struct shv_con_ctx *shv_ctx, int rid, int num_str, const char **str);
void shv_send_str_list_it(struct shv_con_ctx *shv_ctx, int rid, int num_str, struct shv_str_list_it *str_it);
void shv_send_dir(struct shv_con_ctx *shv_ctx, const struct shv_dir_res *results, int cnt, int rid);
void shv_send_error(struct shv_con_ctx *shv_ctx, int rid, enum shv_response_error_code code,
                    const char *msg);
void shv_send_ping(struct shv_con_ctx *shv_ctx);
void shv_send_empty_response(struct shv_con_ctx *shv_ctx, int rid);

int shv_unpack_data(ccpcp_unpack_context * ctx, int * v, double * d);

/**
 * @brief Platform dependant function. Creates the communication processing thread.
 *
 * @param thrd_prio an int number that specifies the priority on the platform level,
 *                  if you wish the OS to default to normal thread priorities and policy,
 *                  set thrd_prio to -1
 * @return int 0 on success, -1 on failure
 */
int shv_create_process_thread(int thrd_prio, struct shv_con_ctx *ctx);

/**
 * @brief Platform dependant function. Stops the communication processing thread.
 *
 * @param shv_ctx
 */
void shv_stop_process_thread(struct shv_con_ctx *shv_ctx);

/**
 * @brief Allocate and initialize a shv_com_ctx_t struct.
 *
 * @param root
 * @param con_info
 * @param at_signlr
 * @return nonNULL pointer on success, NULL on failure
 */
struct shv_con_ctx *shv_com_init(struct shv_node *root, struct shv_connection *con_info,
                            shv_attention_signaller at_signlr);

/**
 * @brief Close the communication thread, deinitialize everything and deallocate shv_ctx.
 *
 * @param shv_ctx
 */
void shv_com_destroy(struct shv_con_ctx *shv_ctx);

/**
 * @brief Launched in a separate thread, this function handles the connection to the broker.
 *        Firstly, it tries to connect to the broker every shv_connection.reconnect_period seconds.
 *        If it connects to the broker, it communicates with it.
 *        The maximum number of reconnect retries is specified in shv_connection.reconnect_retries.
 *
 * @param shv_ctx
 * @return 0 on success, -1 on failure
 */
int shv_process(struct shv_con_ctx *shv_ctx);
