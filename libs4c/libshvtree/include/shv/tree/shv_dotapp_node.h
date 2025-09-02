/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_dotapp_node.h
 * @brief Collection of library's default methods for .app node construction
 */

#pragma once

#include "shv_tree.h"

/**
 * @brief An application specific function used to return the date time.
 * 
 */
typedef int (*shv_dotapp_node_date)(void);

typedef struct shv_dotapp_node {
    shv_node_t shv_node;           /* Base shv_node */
    struct {
        shv_dotapp_node_date date; /* Date function callback */
    } appops;

    const char *name;              /* Application's name */
    const char *version;           /* Application's version */
} shv_dotapp_node_t;

extern const shv_method_des_t shv_dmap_item_dotapp_shvversionmajor;
extern const shv_method_des_t shv_dmap_item_dotapp_shvversionminor;
extern const shv_method_des_t shv_dmap_item_dotapp_name;
extern const shv_method_des_t shv_dmap_item_dotapp_version;
extern const shv_method_des_t shv_dmap_item_dotapp_ping;
extern const shv_method_des_t shv_dmap_item_dotapp_date;

/**
 * @brief The dotapp method structure.
 *
 */
extern const shv_dmap_t shv_dotapp_dmap;

/**
 * @brief Allocate a new standard .app node
 *
 * @param dir
 * @param mode
 * @return non NULL reference on success, NULL otherwise
 */
shv_dotapp_node_t *shv_tree_dotapp_node_new(const shv_dmap_t *dir, int mode);
