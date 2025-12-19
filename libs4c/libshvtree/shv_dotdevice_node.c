/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_dotdevice_com.c
 * @brief Collection of methods for .device node construction
 */

#include <shv/tree/shv_dotdevice_node.h>
#include <shv/tree/shv_methods.h>
#include <ulut/ul_utdefs.h>
#if defined(CONFIG_SHV_LIBS4C_PLATFORM_LINUX) || defined(CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
    #include <shv/tree/shv_clayer_posix.h>
#endif
#include <stdio.h>
#include <stdlib.h>

/* The reset defines may not even exist! */
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_NUTTX
    #include <nuttx/config.h>
#endif

static void shv_dotdevice_node_destructor(struct shv_node *node)
{
    struct shv_dotdevice_node *devnode = UL_CONTAINEROF(node, struct shv_dotdevice_node, shv_node);
    free(devnode);
}

int shv_dotdevice_node_method_name(struct shv_con_ctx *shv_ctx, struct shv_node *item, int rid)
{
    struct shv_dotdevice_node *devnode = UL_CONTAINEROF(item, struct shv_dotdevice_node, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, devnode->name);
    return 0;
}

int shv_dotdevice_node_method_version(struct shv_con_ctx *shv_ctx, struct shv_node *item, int rid)
{
    struct shv_dotdevice_node *devnode = UL_CONTAINEROF(item, struct shv_dotdevice_node, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, devnode->version);
    return 0;
}

int shv_dotdevice_node_method_serial_number(struct shv_con_ctx *shv_ctx, struct shv_node *item, int rid)
{
    struct shv_dotdevice_node *devnode = UL_CONTAINEROF(item, struct shv_dotdevice_node, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, devnode->serial_number);
    return 0;
}

int shv_dotdevice_node_method_reset(struct shv_con_ctx *shv_ctx, struct shv_node *item, int rid)
{
    struct shv_dotdevice_node *devnode = UL_CONTAINEROF(item, struct shv_dotdevice_node, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_int(shv_ctx, rid, 0);
    shv_com_connection_close(shv_ctx);
    /* Let the response bubble through the network stack */
    usleep(2 * 1000 * 1000);
    if (devnode->devops.reset) {
        devnode->devops.reset();
    } else {
        shv_send_error(shv_ctx, rid, SHV_RE_NOT_IMPLEMENTED, "NIMPL");
        return 0;
    }
    return -1; /* How the hell did you get here?!! */
}

int shv_dotdevice_node_method_uptime(struct shv_con_ctx *shv_ctx, struct shv_node *item, int rid)
{
    struct shv_dotdevice_node *devnode = UL_CONTAINEROF(item, struct shv_dotdevice_node, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_int(shv_ctx, rid, devnode->devops.uptime());
    return 0;
}

const struct shv_method_des shv_dmap_item_dotdevice_node_name =
{
    .name = "name",
    .flags = SHV_METHOD_GETTER,
    .result = "s",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotdevice_node_method_name
};

const struct shv_method_des shv_dmap_item_dotdevice_node_version =
{
    .name = "version",
    .flags = SHV_METHOD_GETTER,
    .result = "s",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotdevice_node_method_version
};

const struct shv_method_des shv_dmap_item_dotdevice_node_serial_number =
{
    .name = "serialNumber",
    .flags = SHV_METHOD_GETTER,
    .result = "s|n",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotdevice_node_method_serial_number
};

const struct shv_method_des shv_dmap_item_dotdevice_node_uptime =
{
    .name = "uptime",
    .flags = SHV_METHOD_GETTER,
    .result = "u|n",
    .access = SHV_ACCESS_READ,
    .method = shv_dotdevice_node_method_uptime
};

const struct shv_method_des shv_dmap_item_dotdevice_node_reset =
{
    .name = "reset",
    .flags = 0,
    .result = "",
    .access = SHV_ACCESS_COMMAND,
    .method = shv_dotdevice_node_method_reset
};

static const struct shv_method_des *const shv_dmap_dotdevice_items[] =
{
    &shv_dmap_item_dir,
    &shv_dmap_item_ls,
    &shv_dmap_item_dotdevice_node_name,
    &shv_dmap_item_dotdevice_node_reset,
    &shv_dmap_item_dotdevice_node_serial_number,
    &shv_dmap_item_dotdevice_node_uptime,
    &shv_dmap_item_dotdevice_node_version
};

const struct shv_dmap shv_dotdevice_dmap =
    SHV_CREATE_NODE_DMAP(dotdevice, shv_dmap_dotdevice_items);

struct shv_dotdevice_node *shv_tree_dotdevice_node_new(const struct shv_dmap *dir, int mode)
{
    struct shv_dotdevice_node *item = calloc(1, sizeof(struct shv_dotdevice_node));
    if (item == NULL) {
        perror(".device calloc");
        return NULL;
    }
    shv_tree_node_init(&item->shv_node, ".device", dir, mode);
    /* Instantiate the node with default callbacks */
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_LINUX
    item->devops.reset = shv_dotdevice_node_posix_reset;
    item->devops.uptime = shv_dotdevice_node_posix_uptime;
#endif
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_NUTTX
    /* The reset method may not be implemented, if NuttX's defines are missing */
    item->devops.uptime = shv_dotdevice_node_posix_uptime;
#if defined(CONFIG_BOARDCTL_RESET) || defined(CONFIG_BOARDCTL_RESET_CAUSE)
    item->devops.reset = shv_dotdevice_node_posix_reset;
#endif
#endif
    item->shv_node.vtable.destructor = shv_dotdevice_node_destructor;
    return item;
}
