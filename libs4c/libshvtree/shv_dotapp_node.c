/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_dotapp_node.c
 * @brief Collection of library's default methods for .app node construction
 */


#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_dotapp_node.h>
#include <shv/tree/shv_methods.h>
#include <ulut/ul_utdefs.h>

#include <stdio.h>
#include <stdlib.h>

int shv_dotapp_node_method_shvversionmajor(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_int(shv_ctx, rid, 3);
    return 0;
}

int shv_dotapp_node_method_shvversionminor(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_int(shv_ctx, rid, 0);
    return 0;
}

int shv_dotapp_node_method_name(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotapp_node_t *appnode = UL_CONTAINEROF(item, shv_dotapp_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, appnode->name);
    return 0;
}

int shv_dotapp_node_method_version(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_dotapp_node_t *appnode = UL_CONTAINEROF(item, shv_dotapp_node_t, shv_node);
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_str(shv_ctx, rid, appnode->version);
    return 0;
}

int shv_dotapp_node_method_ping(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_empty_response(shv_ctx, rid);
    return 0;
}

int shv_dotapp_node_method_date(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
    shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
    shv_send_error(shv_ctx, rid, SHV_RE_NOT_IMPLEMENTED, "shv-libs4c missing send date impl");
    return 0;
}

const shv_method_des_t shv_dmap_item_dotapp_shvversionmajor =
{
    .name = "shvVersionMajor",
    .flags = SHV_METHOD_GETTER,
    .result = "i",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotapp_node_method_shvversionmajor
};

const shv_method_des_t shv_dmap_item_dotapp_shvversionminor =
{
    .name = "shvVersionMinor",
    .flags = SHV_METHOD_GETTER,
    .result = "i",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotapp_node_method_shvversionminor
};

const shv_method_des_t shv_dmap_item_dotapp_name =
{
    .name = "name",
    .flags = SHV_METHOD_GETTER,
    .result = "s",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotapp_node_method_name
};

const shv_method_des_t shv_dmap_item_dotapp_version =
{
    .name = "version",
    .flags = SHV_METHOD_GETTER,
    .result = "s",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotapp_node_method_version
};

const shv_method_des_t shv_dmap_item_dotapp_ping =
{
    .name = "ping",
    .flags = 0,
    .result = "",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotapp_node_method_ping
};

const shv_method_des_t shv_dmap_item_dotapp_date =
{
    .name = "date",
    .flags = 0,
    .result = "t",
    .access = SHV_ACCESS_BROWSE,
    .method = shv_dotapp_node_method_date
};

static const shv_method_des_t *const shv_dmap_dotdevice_items[] =
{
    &shv_dmap_item_dotapp_date,
    &shv_dmap_item_dir,
    &shv_dmap_item_ls,
    &shv_dmap_item_dotapp_name,
    &shv_dmap_item_dotapp_ping,
    &shv_dmap_item_dotapp_shvversionmajor,
    &shv_dmap_item_dotapp_shvversionminor,
    &shv_dmap_item_dotapp_version
};

const shv_dmap_t shv_dotapp_dmap =
    SHV_CREATE_NODE_DMAP(dotapp, shv_dmap_dotdevice_items);

shv_dotapp_node_t *shv_tree_dotapp_node_new(const shv_dmap_t *dir, int mode)
{
    shv_dotapp_node_t *item = calloc(1, sizeof(shv_dotapp_node_t));
    if (item == NULL) {
        perror(".app calloc");
        return NULL;
    }
    shv_tree_node_init(&item->shv_node, ".app", dir, mode);
    item->name = "";
    item->version = "";
    return item;
}
