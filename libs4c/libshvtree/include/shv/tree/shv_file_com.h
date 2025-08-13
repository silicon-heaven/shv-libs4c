/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                   <pressste@fel.cvut.cz>
 */

/**
 * @file shv_file_com.h
 * @brief The implementation of file nodes methods.
 */

#pragma once

#include "shv_tree.h"

typedef struct shv_con_ctx shv_con_ctx_t;

/* File type identification enum */
enum shv_file_type
{
    REGULAR = 0,           /* As of July 2025, the only supported file type */
    SHV_FILE_TYPE_COUNT,
};

/* Stat method keys identification enum */
enum shv_file_node_keys
{
    FN_TYPE = 0,
    FN_SIZE,
    FN_PAGESIZE,
    FN_ACCESSTIME,
    FN_MODTIME,
    FN_MAXWRITE,
    SHV_FILE_NODE_KEYS_COUNT
};

/**
 * @brief Packs the file's attributes and sends it
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 */
void shv_file_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Reads the data from the file and sends them
 *
 * @param shv_ctx
 * @param rid
 * @param item
 */
void shv_file_send_read_data(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Unpacks the incoming data and writes them to a file
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 * @return 0 in case of success, -1 in case of garbled data
 */
int shv_file_process_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Unpacks the incoming data of the read method
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 * @return int 
 */
int shv_file_process_read(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Unpacks the desired CRC computation method and computes the CRC
 *
 * @param shv_ctx
 * @param rid
 * @param item
 * @return int
 */
int shv_file_process_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief A wrapper of `shv_file_process_write` to be used
 *        as the file node's write method
 */
extern const shv_method_des_t shv_dmap_item_file_node_write;

/**
 * @brief A wrapper of `shv_file_process_crc32` to be used
 *        as the file node's crc method
 */
extern const shv_method_des_t shv_dmap_item_file_node_crc;

/**
 * @brief A wrapper to be used as the file node's size method
 */
extern const shv_method_des_t shv_dmap_item_file_node_size;

/**
 * @brief A wrapper of `shv_file_send_stat` to be used
 *        as the file node's stat method
 */
extern const shv_method_des_t shv_dmap_item_file_node_stat;
