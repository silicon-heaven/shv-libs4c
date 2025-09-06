/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copytight (c) Michal Lenc 2022-2025 <michallenc@seznam.cz>
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_tree.c
 * @brief Definitions for SHV tree building
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_com.h>
#include <shv/tree/shv_com_common.h>
#if defined (CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    #include <shv/tree/shv_clayer_posix.h>
#endif
#include <ulut/ul_utdefs.h>


/* Custom tree implementation */

GAVL_CUST_NODE_INT_IMP(shv_node_list_gavl, struct shv_node_list, struct shv_node,
                       shv_node_list_key_t, list.gavl.root, gavl_node,
                       name, shv_node_list_comp_func)
GSA_CUST_IMP(shv_node_list_gsa, struct shv_node_list, struct shv_node,
                       shv_node_list_key_t, list.gsa.root,
                       name, shv_node_list_comp_func, 0)

GSA_CUST_IMP(shv_dmap, struct shv_dmap, struct shv_method_des, shv_method_des_key_t,
	           methods, name, shv_method_des_comp_func, 0)

/**
 * @brief Basic SHV node destructor
 *
 * @param node
 */
static void shv_node_destructor(struct shv_node *node)
{
  free(node);
}

/**
 * @brief Destructor for struct shv_node_typed_val
 *
 * @param node
 */
static void shv_typed_val_node_destructor(struct shv_node *node)
{
  struct shv_node_typed_val *typed_node = UL_CONTAINEROF(node, struct shv_node_typed_val, shv_node);
  free(typed_node);
}

/****************************************************************************
 * Name: shv_node_find
 *
 * Description:
 *   Find node based on a path.
 *
 ****************************************************************************/

struct shv_node *shv_node_find(struct shv_node *node, const char * path)
{
  if (strlen(path) == 0)
    {
      return node;
    }

  char *p = strdup(path);
  char *r = p;
  char *s;
  char sentinel = 0;

  do
    {
      s = strchr(r, '/');
      const char *t = r;
      if (s == NULL)
        {
          r = &sentinel;
        }
      else
        {
          *s = '\0';
          r = s + 1;
        }
      if (node->children.mode & SHV_NLIST_MODE_GSA)
        {
          node = shv_node_list_gsa_find(&node->children, &t);
        }
      else
        {
          node = shv_node_list_gavl_find(&node->children, &t);
        }
    } while ((node != NULL) && (*r));

  free(p);

  return node;
}

/****************************************************************************
 * Name: shv_node_list_it_reset
 *
 * Description:
 *   Reset position to the first node in the shv_node_list.
 *
 ****************************************************************************/

void shv_node_list_it_reset(shv_node_list_it_t *it)
{
  if (it->node_list->mode & SHV_NLIST_MODE_GSA)
    {
      it->list_it.gsa_next_indx = shv_node_list_gsa_first_indx(it->node_list);
    }
  else
    {
      it->list_it.gavl_next_node = shv_node_list_gavl_first(it->node_list);
    }
}

/****************************************************************************
 * Name: shv_node_list_it_init
 *
 * Description:
 *   Setup iterator for consecutive access to the list nodes.
 *
 ****************************************************************************/

void shv_node_list_it_init(struct shv_node_list *list, shv_node_list_it_t *it)
{
  it->node_list = list;
  shv_node_list_it_reset(it);
}

/****************************************************************************
 * Name: shv_node_list_it_next
 *
 * Description:
 *   Get next node from the shv_node_list according to iterator position.
 *
 ****************************************************************************/

struct shv_node *shv_node_list_it_next(shv_node_list_it_t *it)
{
  struct shv_node *node;

  if (it->node_list->mode & SHV_NLIST_MODE_GSA)
    {
       node = shv_node_list_gsa_at(it->node_list, it->list_it.gsa_next_indx);
       it->list_it.gsa_next_indx++;
    }
  else
    {
      node = it->list_it.gavl_next_node;
      it->list_it.gavl_next_node = shv_node_list_gavl_next(it->node_list, node);
    }
  return node;
}

/****************************************************************************
 * Name: shv_node_list_names_get_next
 *
 * Description:
 *   Helper function for the names as string list iterator.
 *
 ****************************************************************************/

static const char *shv_node_list_names_get_next(struct shv_str_list_it *it,
                                                int reset_to_first)
{
  shv_node_list_names_it_t *names_it;
  struct shv_node *node;

  names_it = UL_CONTAINEROF(it, shv_node_list_names_it_t, str_it);

  if (reset_to_first)
    {
      shv_node_list_it_reset(&names_it->list_it);
    }

  node = shv_node_list_it_next(&names_it->list_it);

  if (node != NULL)
    {
      return node->name;
    }
  else
    {
      return NULL;
    }
}

/****************************************************************************
 * Name: shv_node_list_names_it_init
 *
 * Description:
 *   Setup iterator for consecutive access to the node list children names.
 *
 ****************************************************************************/

void shv_node_list_names_it_init(struct shv_node_list *list,
                                 shv_node_list_names_it_t *names_it)
{
  shv_node_list_it_init(list, &names_it->list_it);
  names_it->str_it.get_next_entry = shv_node_list_names_get_next;
}

/****************************************************************************
 * Name: shv_tree_add_child
 *
 * Description:
 *   Adds an item to SHV tree.
 *
 ****************************************************************************/

void shv_tree_add_child(struct shv_node *node, struct shv_node *child)
{
  if (node->children.mode & SHV_NLIST_MODE_GSA)
    {
      shv_node_list_gsa_insert(&node->children, child);
    }
  else
    {
      node->children.list.gavl.count += 1;

      shv_node_list_gavl_insert(&node->children, child);
    }
}

/****************************************************************************
 * Name: shv_tree_node_init
 *
 * Description:
 *   Initialize the struct shv_node node.
 *
 ****************************************************************************/

void shv_tree_node_init(struct shv_node *item, const char *child_name,
                        const struct shv_dmap *dir, int mode)
{
  item->name = child_name;
  item->dir = UL_CAST_UNQ1(struct shv_dmap *, dir);

  item->children.mode = mode;

  if (mode & SHV_NLIST_MODE_GSA)
    {
     shv_node_list_gsa_init_array_field(&item->children);
    }
  else
    {
      item->children.list.gavl.count = 0;
      shv_node_list_gavl_init_root_field(&item->children);
    }
}

struct shv_node *shv_tree_node_new(const char *child_name,
                              const struct shv_dmap *dir, int mode)
{
    struct shv_node *item = calloc(1, sizeof(struct shv_node));
    if (item == NULL) {
        perror("node calloc");
        return NULL;
    }
    shv_tree_node_init(item, child_name, dir, mode);
    item->vtable.destructor = shv_node_destructor;
    return item;
}

struct shv_node_typed_val *shv_tree_node_typed_val_new(const char *child_name,
                                                  const struct shv_dmap *dir,
                                                  int mode)
{
    struct shv_node_typed_val *item = calloc(1, sizeof(struct shv_node_typed_val));
    if (item == NULL) {
        printf("typed_val node calloc");
        return NULL;
    }
    shv_tree_node_init(&item->shv_node, child_name, dir, mode);
    item->shv_node.vtable.destructor = shv_typed_val_node_destructor;
    return item;
}

/****************************************************************************
 * Name: shv_tree_destroy
 *
 * Description:
 *  Destroy the whole SHV tree (from given parent node).
 *
 ****************************************************************************/

void shv_tree_destroy(struct shv_node *parent)
{
    struct shv_node *child;

    if (parent->children.mode & SHV_NLIST_MODE_GSA) {
        gsa_cust_for_each_cut(shv_node_list_gsa, &parent->children, child) {
            shv_tree_destroy(child);
        }
    } else {
        gavl_cust_for_each_cut(shv_node_list_gavl, &parent->children, child) {
            shv_tree_destroy(child);
        }
    }

    if ((parent->children.mode & SHV_NLIST_MODE_STATIC) == 0) {
        /* The deallocation must be done according to the node's type */
        parent->vtable.destructor(parent);
    }
}

/****************************************************************************
 * Name: shv_node_process
 *
 * Description:
 *   Find node based on a path.
 *
 ****************************************************************************/

int shv_node_process(struct shv_con_ctx *shv_ctx, int rid, const char *met,
                     const char *path)
{
    /* If the node or method names are too long, the printed lengths is limited */
    char error_msg[80];

    /* Find the node */
    struct shv_node *item = shv_node_find(shv_ctx->root, path);
    if (item == NULL) {
        shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
        snprintf(error_msg, sizeof(error_msg), "Node '%.40s' does not exist.", path);
        shv_send_error(shv_ctx, rid, SHV_RE_METHOD_CALL_EXCEPTION, error_msg);
        return 0;
    }

    /* Call coresponding method */
    const struct shv_method_des *met_des = shv_dmap_find(item->dir, &met);
    if (met_des == NULL) {
        shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
        snprintf(error_msg, sizeof(error_msg), "Method '%.40s' does not exist.", met);
        shv_send_error(shv_ctx, rid, SHV_RE_METHOD_CALL_EXCEPTION, error_msg);
        return 0;
    }

    met_des->method(shv_ctx, item, rid);
    return 1;
}
