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

struct shv_dotapp_node
{
    struct shv_node shv_node;      /* Base shv_node */
    struct {
        shv_dotapp_node_date date; /* Date function callback */
    } appops;

    const char *name;              /* Application's name */
    const char *version;           /* Application's version */
};

extern const struct shv_method_des shv_dmap_item_dotapp_shvversionmajor;
extern const struct shv_method_des shv_dmap_item_dotapp_shvversionminor;
extern const struct shv_method_des shv_dmap_item_dotapp_name;
extern const struct shv_method_des shv_dmap_item_dotapp_version;
extern const struct shv_method_des shv_dmap_item_dotapp_ping;
extern const struct shv_method_des shv_dmap_item_dotapp_date;

/**
 * @brief The dotapp method structure.
 *
 */
extern const struct shv_dmap shv_dotapp_dmap;

/**
 * @brief Allocate a new standard .app node
 *
 * @param dir
 * @param mode
 * @return non NULL reference on success, NULL otherwise
 */
struct shv_dotapp_node *shv_tree_dotapp_node_new(const struct shv_dmap *dir, int mode);
