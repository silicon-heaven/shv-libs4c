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
    SHV_CON_INIT,     /* Failed to init the connection */
    SHV_PROC_THRD,    /* Unable to create the process thread */
    SHV_TLAYER_INIT,  /* Unable to init the transport layer */
    SHV_RECONNECTS,   /* The maximum number of reconnects to a broker reached */
    SHV_LOGIN,        /* Unable to login to the broker */
    SHV_CCPCP_PACK,   /* Error in the CCPCP pack context */
    SHV_CCPCP_UNPACK, /* Error in the CCPCP unpack context */
};

/* Forward declaration */
struct shv_node;

typedef struct shv_con_ctx {
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
  int shv_len;
  int shv_send;
  int reconnects;
  atomic_bool running;
  struct shv_thrd_ctx thrd_ctx;
  struct shv_node *root;
  struct shv_connection *connection;            /* Transport layer information */
} shv_con_ctx_t;

struct shv_dir_res {
  const char *name;
  int flags;
  const char *param;
  const char *result;
  int access;
};

typedef struct shv_str_list_it_t shv_str_list_it_t;

struct shv_str_list_it_t {
   const char * (*get_next_entry)(shv_str_list_it_t *it, int reset_to_first);
};

shv_con_ctx_t *shv_com_init(struct shv_node *root, struct shv_connection *con_info);

void shv_com_end(shv_con_ctx_t *ctx);
void shv_send_int(shv_con_ctx_t *shv_ctx, int rid, int num);
void shv_send_uint(shv_con_ctx_t *shv_ctx, int rid, unsigned int num);
void shv_send_double(shv_con_ctx_t *shv_ctx, int rid, double num);
void shv_send_str(shv_con_ctx_t *shv_ctx, int rid, const char *str);
void shv_send_str_list(shv_con_ctx_t *shv_ctx, int rid, int num_str, const char **str);
void shv_send_str_list_it(shv_con_ctx_t *shv_ctx, int rid, int num_str, shv_str_list_it_t *str_it);
void shv_send_dir(shv_con_ctx_t *shv_ctx, const struct shv_dir_res *results, int cnt, int rid);
void shv_send_error(shv_con_ctx_t *shv_ctx, int rid, enum shv_response_error_code code,
                    const char *msg);
void shv_send_ping(shv_con_ctx_t *shv_ctx);
void shv_send_empty_response(shv_con_ctx_t *shv_ctx, int rid);

int shv_unpack_data(ccpcp_unpack_context * ctx, int * v, double * d);

/**
 * @brief Platform dependant function. Creates the communication processing thread.
 *
 * @param priority
 * @return int 0 on success, -1 on failure
 */
int shv_create_process_thread(int thrd_prio, shv_con_ctx_t *ctx);

void shv_stop_process_thread(shv_con_ctx_t *shv_ctx);

/**
 * @brief Platform dependant function. Close the communication thread and deinit everything.
 *
 * @param shv_ctx
 */
void shv_com_close(shv_con_ctx_t *shv_ctx);

/**
 * @brief Launched in a separate thread, this function handles the connection to the broker.
 *   Firstly, it tries to the broker every shv_connection.reconnect_period seconds.
 *   If it connects to the broker, it communicates with it.
 *   The maximum number of reconnect retries is specified in shv_connection.reconnect_retries.
 *
 * @param shv_ctx
 * @return 0 on success, -1 on failure
 */
int shv_process(shv_con_ctx_t *shv_ctx);
