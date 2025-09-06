/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Michal Lenc 2022-2025 <michallenc@seznam.cz>
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_methods.h
 * @brief The definitions of core SHV methods
 */

#pragma once

#include <ulut/ul_gavlcust.h>
#include <ulut/ul_gsacust.h>
#include <shv/chainpack/cchainpack.h>
#include <string.h>

#include "shv_com.h"
#include "shv_tree.h"

#define SHV_METHOD_GETTER (1 << 1)
#define SHV_METHOD_SETTER (1 << 2)

#define SHV_ACCESS_BROWSE  ((int)1)
#define SHV_ACCESS_READ    ((int)8)
#define SHV_ACCESS_WRITE   ((int)16)
#define SHV_ACCESS_COMMAND ((int)24)

extern const shv_method_des_t shv_dmap_item_ls;
extern const shv_method_des_t shv_dmap_item_dir;

extern const struct shv_dmap shv_double_dmap;
extern const struct shv_dmap shv_double_read_only_dmap;
extern const struct shv_dmap shv_dir_ls_dmap;
extern const struct shv_dmap shv_root_dmap;

int shv_ls(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid);
int shv_dir(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid);
int shv_type(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid);
int shv_double_get(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid);
int shv_double_set(struct shv_con_ctx * shv_ctx, struct shv_node* item, int rid);
