/*
  COPYRIGHT (C) 2022  Michal Lenc (michallenc@seznam.cz)

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

GAVL_CUST_NODE_INT_IMP(shv_node_list_gavl, shv_node_list_t, shv_node_t,
                       shv_node_list_key_t, list.gavl.root, gavl_node,
                       name, shv_node_list_comp_func)
GSA_CUST_IMP(shv_node_list_gsa, shv_node_list_t, shv_node_t,
                       shv_node_list_key_t, list.gsa.root,
                       name, shv_node_list_comp_func, 0)

GSA_CUST_IMP(shv_dmap, shv_dmap_t, shv_method_des_t, shv_method_des_key_t,
	           methods, name, shv_method_des_comp_func, 0)

/**
 * @brief Basic SHV node destructor
 *
 * @param node
 */
static void shv_node_destructor(shv_node_t *node)
{
  free(node);
}

/**
 * @brief Destructor for shv_node_typed_val_t
 *
 * @param node
 */
static void shv_typed_val_node_destructor(shv_node_t *node)
{
  shv_node_typed_val_t *typed_node = UL_CONTAINEROF(node, shv_node_typed_val_t, shv_node);
  free(typed_node);
}

/**
 * @brief File node destructor
 *
 * @param node
 */
static void shv_file_node_destructor(shv_node_t *node)
{
  shv_file_node_t *file_node = UL_CONTAINEROF(node, shv_file_node_t, shv_node);
  free(file_node->fctx);
  shv_node_destructor(&file_node->shv_node);
}

/****************************************************************************
 * Name: shv_node_find
 *
 * Description:
 *   Find node based on a path.
 *
 ****************************************************************************/

shv_node_t *shv_node_find(shv_node_t *node, const char * path)
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

void shv_node_list_it_init(shv_node_list_t *list, shv_node_list_it_t *it)
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

shv_node_t *shv_node_list_it_next(shv_node_list_it_t *it)
{
  shv_node_t *node;

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

static const char *shv_node_list_names_get_next(shv_str_list_it_t *it,
                                                int reset_to_first)
{
  shv_node_list_names_it_t *names_it;
  shv_node_t *node;

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

void shv_node_list_names_it_init(shv_node_list_t *list,
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

void shv_tree_add_child(shv_node_t *node, shv_node_t *child)
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
 *   Initialize the shv_node_t node.
 *
 ****************************************************************************/

void shv_tree_node_init(shv_node_t *item, const char *child_name,
                        const shv_dmap_t *dir, int mode)
{
  item->name = child_name;
  item->dir = UL_CAST_UNQ1(shv_dmap_t *, dir);

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

shv_node_t *shv_tree_node_new(const char *child_name,
                              const shv_dmap_t *dir, int mode)
{
    shv_node_t *item = calloc(1, sizeof(shv_node_t));
    if (item == NULL) {
        perror("node calloc");
        return NULL;
    }
    shv_tree_node_init(item, child_name, dir, mode);
    item->vtable.destructor = shv_node_destructor;
    return item;
}

shv_node_typed_val_t *shv_tree_node_typed_val_new(const char *child_name,
                                                  const shv_dmap_t *dir,
                                                  int mode)
{
    shv_node_typed_val_t *item = calloc(1, sizeof(shv_node_typed_val_t));
    if (item == NULL) {
        printf("typed_val node calloc");
        return NULL;
    }
    shv_tree_node_init(&item->shv_node, child_name, dir, mode);
    item->shv_node.vtable.destructor = shv_typed_val_node_destructor;
    return item;
}

shv_file_node_t *shv_tree_file_node_new(const char *child_name, const shv_dmap_t *dir, int mode)
{
    shv_file_node_t *item = calloc(1, sizeof(shv_file_node_t));
    if (item == NULL) {
        perror("file node calloc");
        return NULL;
    }
    /* Allocate default file context */
    item->fctx = calloc(1, sizeof(struct shv_file_node_fctx));
    if (item->fctx == NULL) {
        perror("file node ctx calloc");
        return NULL;
    }
    /* Initialize with default ops */
#if defined (CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    item->fops.opener  = shv_file_node_posix_opener;
    item->fops.getsize = shv_file_node_posix_getsize;
    item->fops.writer  = shv_file_node_posix_writer;
    item->fops.seeker  = shv_file_node_posix_seeker;
    item->fops.crc32   = shv_file_node_posix_crc32;
#endif
    shv_tree_node_init(&item->shv_node, child_name, dir, mode);
    item->shv_node.vtable.destructor = shv_file_node_destructor;
    return item;
}

/****************************************************************************
 * Name: shv_tree_destroy
 *
 * Description:
 *  Destroy the whole SHV tree (from given parent node).
 *
 ****************************************************************************/

void shv_tree_destroy(shv_node_t *parent)
{
    shv_node_t *child;

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

int shv_node_process(shv_con_ctx_t *shv_ctx, int rid, const char *met,
                     const char *path)
{
    /* If the node or method names are too long, the printed lengths is limited */
    char error_msg[80];

    /* Find the node */
    shv_node_t *item = shv_node_find(shv_ctx->root, path);
    if (item == NULL) {
        shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
        snprintf(error_msg, sizeof(error_msg), "Node '%.40s' does not exist.", path);
        shv_send_error(shv_ctx, rid, SHV_RE_METHOD_CALL_EXCEPTION, error_msg);
        return 0;
    }

    /* Call coresponding method */
    const shv_method_des_t *met_des = shv_dmap_find(item->dir, &met);
    if (met_des == NULL) {
        shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
        snprintf(error_msg, sizeof(error_msg), "Method '%.40s' does not exist.", met);
        shv_send_error(shv_ctx, rid, SHV_RE_METHOD_CALL_EXCEPTION, error_msg);
        return 0;
    }

    met_des->method(shv_ctx, item, rid);
    return 1;
}
