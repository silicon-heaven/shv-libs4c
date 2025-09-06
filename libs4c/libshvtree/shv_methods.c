/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Michal Lenc 2022-2025 <michallenc@seznam.cz>
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_methods.c
 * @brief The definitions of core SHV methods
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <shv/tree/shv_methods.h>
#include <ulut/ul_utdefs.h>

/* Method descriptors - general methods "ls" and "dir" */

const shv_method_des_t shv_dmap_item_ls = {
  .name = "ls",
  .access = SHV_ACCESS_BROWSE,
  .method = shv_ls
};
const shv_method_des_t shv_dmap_item_dir = {
  .name = "dir",
  .access = SHV_ACCESS_BROWSE,
  .method = shv_dir
};

/* Method descriptors - methods for parameters */

const shv_method_des_t shv_dmap_item_type = {
  .name = "typeName",
  .flags = SHV_METHOD_GETTER,
  .result = "s",
  .access = SHV_ACCESS_READ,
  .method = shv_type
};

/* Method descriptors - methods for double values: params and inputs/outputs */

const shv_method_des_t shv_double_dmap_item_get = {
  .name = "get",
  .flags = SHV_METHOD_GETTER,
  .result = "d",
  .access = SHV_ACCESS_READ,
  .method = shv_double_get
};
const shv_method_des_t shv_double_dmap_item_set = {
  .name = "set",
  .flags = SHV_METHOD_SETTER,
  .param = "d|f",
  .access = SHV_ACCESS_WRITE,
  .method = shv_double_set
};

const shv_method_des_t * const shv_double_dmap_items[] = {
  &shv_dmap_item_dir,
  &shv_double_dmap_item_get,
  &shv_dmap_item_ls,
  &shv_double_dmap_item_set,
  &shv_dmap_item_type,
};

const shv_method_des_t * const shv_double_read_only_dmap_items[] = {
  &shv_dmap_item_dir,
  &shv_double_dmap_item_get,
  &shv_dmap_item_ls,
  &shv_dmap_item_type,
};

const shv_method_des_t * const shv_dir_ls_dmap_items[] = {
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
};

const shv_method_des_t * const shv_root_dmap_items[] = {
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
};

const struct shv_dmap shv_double_dmap = {.methods = {.items = (void **)shv_double_dmap_items,
                                                .count = sizeof(shv_double_dmap_items)/sizeof(shv_double_dmap_items[0]),
                                                .alloc_count = 0,
                                               }};
const struct shv_dmap shv_double_read_only_dmap = {.methods = {.items = (void **)shv_double_read_only_dmap_items,
                                              .count = sizeof(shv_double_read_only_dmap_items)/sizeof(shv_double_read_only_dmap_items[0]),
                                              .alloc_count = 0,
                                              }};
const struct shv_dmap shv_dir_ls_dmap = {.methods = {.items = (void **)shv_dir_ls_dmap_items,
                                                .count = sizeof(shv_dir_ls_dmap_items)/sizeof(shv_dir_ls_dmap_items[0]),
                                                .alloc_count = 0,
                                               }};

const struct shv_dmap shv_root_dmap = {.methods = {.items = (void **)shv_root_dmap_items,
                                              .count = sizeof(shv_root_dmap_items)/sizeof(shv_root_dmap_items[0]),
                                              .alloc_count = 0,
                                             }};

/****************************************************************************
 * Name: shv_ls
 *
 * Description:
 *   Method "ls". This methods returns the names of the node's children.
 *
 ****************************************************************************/

int shv_ls(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid)
{
  int count;
  shv_node_list_names_it_t names_it;

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);

  /* Get item's children count */

  count = shv_node_list_count(&item->children);

  /* Find each child */

  shv_node_list_names_it_init(&item->children, &names_it);

  /* And send it */

  shv_send_str_list_it(shv_ctx, rid, count, &names_it.str_it);

  return 0;
}

/****************************************************************************
 * Name: shv_dir
 *
 * Description:
 *   Method "dir". This method returns the methods supported by the node.
 *
 ****************************************************************************/

int shv_dir(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid)
{
  int met_num;
  int i;

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);

  /* Is the node a parameter? */

  met_num = item->dir->methods.count;
  struct shv_dir_res responses[met_num];
  memset(responses, 0, sizeof *responses * met_num);

  for (i = 0; i < met_num; i++)
    {
      responses[i].name = shv_dmap_at(item->dir, i)->name;
      responses[i].flags = shv_dmap_at(item->dir, i)->flags;
      responses[i].param = shv_dmap_at(item->dir, i)->param;
      responses[i].result = shv_dmap_at(item->dir, i)->result;
      responses[i].access = shv_dmap_at(item->dir, i)->access;
    }

  shv_send_dir(shv_ctx, responses, met_num, rid);

  return 0;
}

/****************************************************************************
 * Name: shv_type
 *
 * Description:
 *   Returns type name of the parameter (double, int, etc...)
 *
 ****************************************************************************/

int shv_type(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid)
{
  const char *str;

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);

  struct shv_node_typed_val *item_node = UL_CONTAINEROF(item, struct shv_node_typed_val,
                                                   shv_node);

  str = item_node->type_name;

  shv_send_str(shv_ctx, rid, str);

  return 0;
}

/****************************************************************************
 * Name: shv_set
 *
 * Description:
 *   Method "set".
 *
 ****************************************************************************/

int shv_double_set(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid)
{
  double shv_received;

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, &shv_received);

  struct shv_node_typed_val *item_node = UL_CONTAINEROF(item, struct shv_node_typed_val,
                                                   shv_node);

  *(double *)item_node->val_ptr = shv_received;

  shv_send_double(shv_ctx, rid, shv_received);

  return 0;
}


/****************************************************************************
 * Name: shv_get
 *
 * Description:
 *   Method "get".
 *
 ****************************************************************************/

int shv_double_get(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid)
{
  double shv_send;

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);

  struct shv_node_typed_val *item_node = UL_CONTAINEROF(item, struct shv_node_typed_val,
                                                   shv_node);

  shv_send = *(double *)item_node->val_ptr;

  shv_send_double(shv_ctx, rid, shv_send);

  return 0;
}
