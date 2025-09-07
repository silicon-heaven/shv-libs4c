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

struct shv_node_list
{
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
};

struct shv_dmap {
  gsa_array_field_t methods;  /* GSA array of methods */
};

struct shv_node;
struct shv_node
{
  struct {
    void (*destructor)(struct shv_node *this);
  } vtable;                      /* Node vtable */
  const char *name;              /* Node name */
  gavl_node_t gavl_node;         /* GAVL instance */
  struct shv_dmap *dir;          /* Pointer to supported methods */
  struct shv_node_list children; /* List of node children */
};

struct shv_node_typed_val
{
  struct shv_node shv_node;     /* Node instance */
  void *val_ptr;                /* Double value */
  char *type_name;              /* Type of the value (int, double...) */
};

struct shv_con_ctx;
typedef int (* shv_method_t) (struct shv_con_ctx *ctx, struct shv_node * node, int rid);

struct shv_method_des
{
  const char *name;       /* Method name */
  const int flags;        /* Method flags */
  const char *param;      /* Parameter type for request */
  const char *result;     /* Result type for responses */
  const int access;       /* Access level */
  shv_method_t method;    /* Pointer to the method function */
};

typedef const char *shv_node_list_key_t;
typedef const char *shv_method_des_key_t;

/* Custom tree declarations */
/* GAVL_CUST_NODE_INT_DEC - standard custom tree with internal node */
/* GAVL_FLES_INT_DEC      - tree with enhanced first last access speed  */

GAVL_CUST_NODE_INT_DEC(shv_node_list_gavl, struct shv_node_list, struct shv_node, shv_node_list_key_t,
	list.gavl.root, gavl_node, name, shv_node_list_comp_func)

GSA_CUST_DEC(shv_node_list_gsa, struct shv_node_list, struct shv_node, shv_node_list_key_t,
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

GSA_CUST_DEC(shv_dmap, struct shv_dmap, struct shv_method_des, shv_method_des_key_t,
	methods, name, shv_method_des_comp_func)

struct shv_node_list_it
{
  struct shv_node_list *node_list;
  union {
    struct shv_node *gavl_next_node;
    int gsa_next_indx;
  } list_it;
};

void shv_node_list_it_init(struct shv_node_list *list, struct shv_node_list_it *it);
void shv_node_list_it_reset(struct shv_node_list_it *it);
struct shv_node *shv_node_list_it_next(struct shv_node_list_it *it);

static inline int
shv_node_list_count(struct shv_node_list *node_list)
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

struct shv_node_list_names_it
{
  struct shv_str_list_it str_it;
  struct shv_node_list_it list_it;
};

void shv_node_list_names_it_init(struct shv_node_list *list, struct shv_node_list_names_it *names_it);

/* Public functions definition */

int shv_node_process(struct shv_con_ctx *shv_ctx, int rid, const char * met, const char * path);
struct shv_node *shv_node_find(struct shv_node *node, const char * path);
void shv_tree_add_child(struct shv_node *node, struct shv_node *child);
void shv_tree_node_init(struct shv_node *item, const char *child_name, const struct shv_dmap *dir, int mode);

/**
 * @brief Destroy the whole SHV tree, given the parent node
 *
 * @param parent
 */
void shv_tree_destroy(struct shv_node *parent);

/**
 * @brief Allocates and initializes a simple node
 *
 * @param child_name
 * @param dir
 * @param mode
 * @return A nonNULL pointer on success, NULL otherwise
 */
struct shv_node *shv_tree_node_new(const char *child_name, const struct shv_dmap *dir, int mode);

/**
 * @brief Initialize the struct shv_node_typed_val node
 *
 * @param child_name
 * @param dir
 * @param mode
 * @return A nonNULL pointer on success, NULL otherwise
 */
struct shv_node_typed_val *shv_tree_node_typed_val_new(const char *child_name, const struct shv_dmap *dir, int mode);
