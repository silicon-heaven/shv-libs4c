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

#include "shv_com.h"
#include "shv_tree.h"

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
 * @brief File stat method`
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 */
void shv_file_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

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
 * @brief Unpacks the desired CRC computation method and computes the CRC
 * 
 * @param shv_ctx 
 * @param rid 
 * @param item 
 * @return int 
 */
int shv_file_process_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

/**
 * @brief A platform dependant function used to write count bytes from buf to a file
 * @param item
 * @param buf   The source buffer
 * @param count The number of bytes to be written
 * @warning The function must assure it does not write beyond the bounds of file_maxsize.
 * @return written bytes in case of success, -1 otherwise
 */
int shv_file_node_writer(shv_file_node_t *item, void *buf, size_t count);

/**
 * @typedef shv_file_node_seeker
 * @brief A platform dependant function used to reposition the file offset.
 * @param item
 * @param offset The absolute file offset
 * @warning The function must assure it does not seek beyond the bounds of file_maxsize.
 * @return the new offset in case of success, -1 otherwise
 */
int shv_file_node_seeker(shv_file_node_t *item, int offset);

/**
 * @brief A function used to calculate CRC32 from start to start+size-1 specified by
 *        the arg argument. The provided function must conform to the algorithm used
 *        in IEEE 802.3. The reason this function is exposed is that the user can make use
 *        of platform dependant CRC libraries (such as zlib) or HW accelerated CRC calculators.
 * @param item
 * @param start  The starting offset in the file
 * @param size   The count of bytes to calculate CRC32 over
 * @param result Pointer to the CRC32 result
 * @return 0 in case of success, -1 otherwise
 */
int shv_file_node_crc32(shv_file_node_t *item, int start, size_t size, uint32_t *result);


/**
 * @brief The file write method implementation
 *
 * @param shv_ctx
 * @param item
 * @param rid
 * @return int
 */
int shv_file_node_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

/**
 * @brief The file crc method implementation
 *
 * @param shv_ctx
 * @param item
 * @param rid
 * @return int
 */
int shv_file_node_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

/**
 * @brief
 *
 * @param shv_ctx
 * @param item
 * @param rid
 * @return int
 */
int shv_file_node_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

/**
 * @brief
 *
 * @param shv_ctx
 * @param item
 * @param rid
 * @return int
 */
int shv_file_node_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

extern const shv_method_des_t shv_dmap_item_file_node_write;
extern const shv_method_des_t shv_dmap_item_file_node_crc;
extern const shv_method_des_t shv_dmap_item_file_node_size;
extern const shv_method_des_t shv_dmap_item_file_node_stat;
