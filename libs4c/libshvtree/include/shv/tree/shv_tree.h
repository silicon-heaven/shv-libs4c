/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copytight (c) Michal Lenc 2022-2025 <michallenc@seznam.cz>
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_tree.h
 * @brief Definitions for SHV tree building
 */

#pragma once

#include <ulut/ul_gavlcust.h>
#include <ulut/ul_gsacust.h>
#include <shv/chainpack/cchainpack.h>
#include <string.h>
#include <stdint.h>

#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    #include "shv_clayer_posix.h"
#endif
#include "shv_com.h"

#define SHV_NLIST_MODE_GAVL   0
#define SHV_NLIST_MODE_GSA    1
#define SHV_NLIST_MODE_STATIC 2

/* A helper macro that defines the node's methods */
#define SHV_CREATE_NODE_DMAP(node, list_of_addrs)\
    {\
        .methods =\
        {\
            .items = (void **)list_of_addrs,\
            .count = sizeof(list_of_addrs) / sizeof(list_of_addrs[0]),\
            .alloc_count = 0\
        }\
    }

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

typedef struct shv_node shv_node_t;
typedef struct shv_node {
  struct {
    void (*destructor)(shv_node_t *this);
  } vtable;                 /* Node vtable */
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

struct shv_con_ctx;
typedef int (* shv_method_t) (struct shv_con_ctx *ctx, shv_node_t * node, int rid);

typedef struct shv_method_des {
  const char *name;       /* Method name */
  const int flags;        /* Method flags */
  const char *param;      /* Parameter type for request */
  const char *result;     /* Result type for responses */
  const int access;       /* Access level */
  shv_method_t method;    /* Pointer to the method function */
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

typedef struct shv_node_list_it
{
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
  struct shv_str_list_it str_it;
  shv_node_list_it_t list_it;
} shv_node_list_names_it_t;

void shv_node_list_names_it_init(shv_node_list_t *list, shv_node_list_names_it_t *names_it);

/* Public functions definition */

int shv_node_process(struct shv_con_ctx *shv_ctx, int rid, const char * met, const char * path);
shv_node_t *shv_node_find(shv_node_t *node, const char * path);
void shv_tree_add_child(shv_node_t *node, shv_node_t *child);
void shv_tree_node_init(shv_node_t *item, const char *child_name, const shv_dmap_t *dir, int mode);

/**
 * @brief Destroy the whole SHV tree, given the parent node
 *
 * @param parent
 */
void shv_tree_destroy(shv_node_t *parent);

/**
 * @brief Allocates and initializes a simple node
 *
 * @param child_name
 * @param dir
 * @param mode
 * @return A nonNULL pointer on success, NULL otherwise
 */
shv_node_t *shv_tree_node_new(const char *child_name, const shv_dmap_t *dir, int mode);

/**
 * @brief Initialize the shv_node_typed_val_t node
 *
 * @param child_name
 * @param dir
 * @param mode
 * @return A nonNULL pointer on success, NULL otherwise
 */
shv_node_typed_val_t *shv_tree_node_typed_val_new(const char *child_name, const shv_dmap_t *dir, int mode);
