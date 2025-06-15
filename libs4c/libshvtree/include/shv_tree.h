#ifndef SHV_TREE_H
#define SHV_TREE_H

#include <ulut/ul_gavlcust.h>
#include <ulut/ul_gsacust.h>
#include <shv/chainpack/cchainpack.h>
#include <string.h>
#include <stdint.h>

#include "shv_com.h"

#define SHV_NLIST_MODE_GSA 1
#define SHV_NLIST_MODE_STATIC 2

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

/**
 * @typedef shv_file_node_writer
 * @brief A platform dependant function pointer used to write count bytes from buf to a file whose
 *        attributes are stored in the arg context.
 * @param arg   Pointer to the platform dependant file context
 * @param buf   The source buffer
 * @param count The number of bytes to be written
 * @return 0 in case of success, -1 otherwise
 */
typedef int (*shv_file_node_writer)(void *arg, void *buf, size_t count);

/**
 * @typedef shv_file_node_seeker
 * @brief A platform dependant function pointer used to reposition the file offset.
 * @param arg    Pointer to the platform dependant file context
 * @param offset The absolute file offset
 * @return 0 in case of success, -1 otherwise
 */
typedef int (*shv_file_node_seeker)(void *arg, int offset);

/**
 * @typedef shv_file_node_crc32
 * @brief A function pointer used to calculate CRC32 from start to start+size-1 specified by
 *        the arg argument. The provided function must conform to the algorithm used
 *        in IEEE 802.3. The reason this function is exposed is that the user can make use
 *        of platform dependant CRC libraries (such as zlib) or HW accelerated CRC calculators.
 * @param arg    Pointer to the platform dependant file context
 * @param start  The starting offset in the file
 * @param size   The count of bytes to calculate CRC32 over
 * @param result Pointer to the CRC32 result
 * @return 0 in case of success, -1 otherwise
 */
typedef int (*shv_file_node_crc32)(void *arg, int start, size_t size, uint32_t *result);

typedef struct shv_node_list {
  int mode;                         /* Mode selection (GAVL vs GSA, static vs dynamic) */
  union {
    struct {
      gavl_cust_root_field_t root;  /* GAVL root */
      int count;                    /* Number of root's chuldren */
    } gavl;
    struct {
      gsa_array_field_t root;       /* GSA root */
    } gsa;
  } list;
} shv_node_list_t;

typedef struct shv_dmap {
  gsa_array_field_t methods;  /* GSA array of methods */
} shv_dmap_t;

typedef struct shv_node {
  const char *name;         /* Node name */
  gavl_node_t gavl_node;    /* GAVL instance */
  shv_dmap_t *dir;          /* Pointer to supported methods */
  shv_node_list_t children; /* List of node children */
} shv_node_t;

typedef struct shv_node_typed_val {
  shv_node_t shv_node;          /* Node instance */
  void *val_ptr;                /* Double value */
  char *type_name;              /* Type of the value (int, double...) */
} shv_node_typed_val_t;

typedef struct shv_file_node {
  shv_node_t shv_node;               /* Base shv_node */

  shv_file_node_writer writer;       /* The writer function */
  shv_file_node_seeker seeker;       /* The seeker function */
  shv_file_node_crc32  crcalculator; /* The CRC32 calc function */
  
  /* Stat method attributes */
  int file_type;
  int file_size;
  int file_pagesize;
  
  enum shv_unpack_write_state state; /* Internal unpack write state */
  int crc;                           /* Internal CRC accumulator */
  int file_offset;                   /* Internal current file offset */
  
} shv_file_node_t;

typedef int (* shv_method_t) (shv_con_ctx_t *ctx, shv_node_t * node, int rid);

typedef struct shv_method_des {
  const char *name;       /* Method name */
  const int flags;        /* Method flags */
  const char *param;      /* Parameter type for request */
  const char *result;     /* Result type for responses */
  const int access;       /* Access level */
  shv_method_t method;    /* Pointer to the method functiom */
} shv_method_des_t;

typedef const char *shv_node_list_key_t;
typedef const char *shv_method_des_key_t;

/* Custom tree declarations */
/* GAVL_CUST_NODE_INT_DEC - standard custom tree with internal node */
/* GAVL_FLES_INT_DEC      - tree with enhanced first last access speed  */

GAVL_CUST_NODE_INT_DEC(shv_node_list_gavl, shv_node_list_t, shv_node_t, shv_node_list_key_t,
	list.gavl.root, gavl_node, name, shv_node_list_comp_func)

GSA_CUST_DEC(shv_node_list_gsa, shv_node_list_t, shv_node_t, shv_node_list_key_t,
	list.gsa.root, name, shv_node_list_comp_func)

static inline int
shv_node_list_comp_func(const shv_node_list_key_t *a, const shv_node_list_key_t *b)
{
  return strcmp(*a, *b);
}

static inline int
shv_method_des_comp_func(const shv_method_des_key_t *a, const shv_method_des_key_t *b)
{
  return strcmp(*a, *b);
}

GSA_CUST_DEC(shv_dmap, shv_dmap_t, shv_method_des_t, shv_method_des_key_t,
	methods, name, shv_method_des_comp_func)

typedef struct shv_node_list_it_t {
  shv_node_list_t *node_list;
  union {
    shv_node_t *gavl_next_node;
    int gsa_next_indx;
  } list_it;
} shv_node_list_it_t;

void shv_node_list_it_init(shv_node_list_t *list, shv_node_list_it_t *it);
void shv_node_list_it_reset(shv_node_list_it_t *it);
shv_node_t *shv_node_list_it_next(shv_node_list_it_t *it);

static inline int
shv_node_list_count(shv_node_list_t *node_list)
{
  if (node_list->mode & SHV_NLIST_MODE_GSA)
    {
      return node_list->list.gsa.root.count;
    }
  else
    {
      return node_list->list.gavl.count;
    }
}

typedef struct shv_node_list_names_it_t {
  shv_str_list_it_t str_it;
  shv_node_list_it_t list_it;
} shv_node_list_names_it_t;

void shv_node_list_names_it_init(shv_node_list_t *list, shv_node_list_names_it_t *names_it);

/* Public functions definition */

int shv_node_process(shv_con_ctx_t *shv_ctx, int rid, const char * met, const char * path);
shv_node_t *shv_node_find(shv_node_t *node, const char * path);
void shv_tree_add_child(shv_node_t *node, shv_node_t *child);
shv_node_t *shv_tree_node_new(const char *child_name, const shv_dmap_t *dir, int mode);
shv_node_typed_val_t *shv_tree_node_typed_val_new(const char *child_name, const shv_dmap_t *dir, int mode);
void shv_tree_node_init(shv_node_t *item, const char *child_name, const shv_dmap_t *dir, int mode);
void shv_tree_destroy(shv_node_t *parent);

#endif  /* SHV_TREE_H */
