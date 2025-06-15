/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                   <pressste@fel.cvut.cz>
 */

/**
 * @file shv_file_com.h
 * @brief The implementation of file nodes methods.
 */

#ifndef _SHV_FILE_COM_H
#define _SHV_FILE_COM_H

#include "shv_com.h"
#include "shv_tree.h"

/* File type identification enum */
enum shv_file_type
{
  REGULAR = 0,         /* As of July 2025, the only supported file type */
  SHV_FILE_TYPE_COUNT
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
 * @brief File stat method`
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 */
void shv_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief File size method - contrary to stat, only the size param is sent
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 */
void shv_send_size(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Crc method - compute crc over the specified range
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 */
void shv_send_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Unpacks the incoming data and writes them to a file
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 * @return int 
 */
int shv_process_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Sends write confirmation to the broker
   * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 */
void shv_confirm_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief Unpacks the desired CRC computation method and computes the CRC
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 * @return int 
 */
int shv_process_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

#endif /* _SHV_FILE_COM_H */
