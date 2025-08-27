/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_dotdevice_com.h
 * @brief Colection of methods for .device node construction
 */

#include <shv/tree/shv_dotdevice_node.h>
#include <shv/tree/shv_methods.h>
#include <ulut/ul_utdefs.h>
#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
  #include <shv/tree/shv_clayer_posix.h>
#endif
#include <stdio.h>
#include <stdlib.h>

int shv_dotdevice_node_method_name(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotdevice_node_t *devnode = UL_CONTAINEROF(item, shv_dotdevice_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, devnode->name);
    return 0;
}

int shv_dotdevice_node_method_version(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotdevice_node_t *devnode = UL_CONTAINEROF(item, shv_dotdevice_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, devnode->version);
    return 0;
}

int shv_dotdevice_node_method_serial_number(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotdevice_node_t *devnode = UL_CONTAINEROF(item, shv_dotdevice_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, devnode->serial_number);
    return 0;
}

int shv_dotdevice_node_method_reset(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotdevice_node_t *devnode = UL_CONTAINEROF(item, shv_dotdevice_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_int(shv_ctx, rid, 0);
    devnode->devops.reset();
    return 0;
}

int shv_dotdevice_node_method_uptime(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotdevice_node_t *devnode = UL_CONTAINEROF(item, shv_dotdevice_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_int(shv_ctx, rid, devnode->devops.uptime());
    return 0;
}

const shv_method_des_t shv_dmap_item_dotdevice_node_name =
{
    .name = "name",
    .method = shv_dotdevice_node_method_name
};

const shv_method_des_t shv_dmap_item_dotdevice_node_version =
{
    .name = "version",
    .method = shv_dotdevice_node_method_version
};

const shv_method_des_t shv_dmap_item_dotdevice_node_serial_number =
{
    .name = "serialNumber",
    .method = shv_dotdevice_node_method_serial_number
};

const shv_method_des_t shv_dmap_item_dotdevice_node_uptime =
{
    .name = "uptime",
    .method = shv_dotdevice_node_method_uptime
};

const shv_method_des_t shv_dmap_item_dotdevice_node_reset =
{
    .name = "reset",
    .method = shv_dotdevice_node_method_reset
};

static const shv_method_des_t *const shv_dmap_dotdevice_items[] =
{
    &shv_dmap_item_dir,
    &shv_dmap_item_ls,
    &shv_dmap_item_dotdevice_node_name,
    &shv_dmap_item_dotdevice_node_reset,
    &shv_dmap_item_dotdevice_node_serial_number,
    &shv_dmap_item_dotdevice_node_uptime,
    &shv_dmap_item_dotdevice_node_version
};

const shv_dmap_t shv_dotdevice_dmap =
    SHV_CREATE_NODE_DMAP(dotdevice, shv_dmap_dotdevice_items);

shv_dotdevice_node_t *shv_tree_dotdevice_node_new(const shv_dmap_t *dir, int mode)
{
    shv_dotdevice_node_t *item = calloc(1, sizeof(shv_dotdevice_node_t));
    if (item == NULL) {
        perror(".device calloc");
        return NULL;
    }
    shv_tree_node_init(&item->shv_node, ".device", dir, mode);
    /* Instantiate the node with default callbacks */
#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    item->devops.reset = shv_dotdevice_node_posix_reset;
    item->devops.uptime = shv_dotdevice_node_posix_uptime;
#endif
    return item;
}
