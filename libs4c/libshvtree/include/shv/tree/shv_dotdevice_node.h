/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_dotdevice_com.h
 * @brief Collection of methods for .device node construction
 */

#pragma once

#include "shv_tree.h"

/* Forward declaration */
typedef struct shv_dotdevice_node shv_dotdevice_node_t;

/**
 * @brief A platform dependant function used to track time.
 * @return number of runtime seconds, -1 otherwise
 */
typedef int (*shv_dotdevice_node_uptime)(void);

/**
 * @brief A platform dependant function used to reset the device.
 * @return should normally be 0 but we don't expect this to return really
 */
typedef int (*shv_dotdevice_node_reset)(void);

typedef struct shv_dotdevice_node {
    struct shv_node shv_node;             /* Base shv_node */
    struct {
        shv_dotdevice_node_reset  reset;  /* Platform dependant reset function */
        shv_dotdevice_node_uptime uptime; /* Platform dependant uptime function */
    } devops;

    const char *name;                     /* */
    const char *serial_number;
    const char *version;
} shv_dotdevice_node_t;

extern const shv_method_des_t shv_dmap_item_dotdevice_node_name;
extern const shv_method_des_t shv_dmap_item_dotdevice_node_version;
extern const shv_method_des_t shv_dmap_item_dotdevice_node_serial_number;
extern const shv_method_des_t shv_dmap_item_dotdevice_node_uptime;
extern const shv_method_des_t shv_dmap_item_dotdevice_node_reset;

/**
 * @brief The dotdevice method structure.
 * 
 */
extern const struct shv_dmap shv_dotdevice_dmap;

/**
 * @brief Allocate a new standard .dotdevice node
 * 
 * @param dir 
 * @param mode 
 * @return non NULL reference on success, NULL otherwise
 */
shv_dotdevice_node_t *shv_tree_dotdevice_node_new(const struct shv_dmap *dir, int mode);
