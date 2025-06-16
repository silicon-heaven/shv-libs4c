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

/**
 * @brief A platform dependant function used to write count bytes from buf to a file whose
 *        attributes are stored in the arg context.
 * @param item
 * @param buf   The source buffer
 * @param count The number of bytes to be written
 * @return 0 in case of success, -1 otherwise
 */
int shv_file_node_writer(shv_file_node_t *item, void *buf, size_t count);

/**
 * @typedef shv_file_node_seeker
 * @brief A platform dependant function used to reposition the file offset.
 * @param item
 * @param offset The absolute file offset
 * @return 0 in case of success, -1 otherwise
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

#endif /* _SHV_FILE_COM_H */
