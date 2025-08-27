/* SPDX-License-Identifier: LGPL-2.1-or-later OR BSD-2-Clause OR Apache-2.0
 *
 * Copyright (c) Stepan Pressl 2025 <pressl.stepan@gmail.com>
 *                                  <pressste@fel.cvut.cz>
 */

/**
 * @file shv_file_node.h
 * @brief The implementation of file node and its methods
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

typedef struct shv_file_node shv_file_node_t;

/**
 * @brief A platform dependant function used to open the file
 * @param item
 * @return 0 in case of success, -1 otherwise
 */
typedef int (*shv_file_node_opener)(shv_file_node_t *item);

/**
 * @brief A platform dependant function used to get file's current size
 * @param item
 * @return file's size in case of success, -1 otherwise
 */
typedef int (*shv_file_node_getsize)(shv_file_node_t *item);

/**
 * @brief A platform dependant function used to write count bytes from buf to a file
 * @param item
 * @param buf   The source buffer
 * @param count The number of bytes to be written
 * @warning The function must assure it does not write beyond the bounds of file_maxsize.
 * @return written bytes in case of success, -1 otherwise
 */
typedef int (*shv_file_node_writer)(shv_file_node_t *item, void *buf, size_t count);

/**
 * @brief A platform dependant function used to read count bytes from a file to buf
 * @param item
 * @param buf   The destination buffer
 * @param count The number of bytes to be read
 * @warning The function must assure it does not read outside the bounds of file_maxsize.
 * @return read bytes in case of success, -1 otherwise
 */
typedef int (*shv_file_node_reader)(shv_file_node_t *item, void *buf, size_t count);

/**
 * @typedef shv_file_node_seeker
 * @brief A platform dependant function used to reposition the file offset.
 * @param item
 * @param offset The absolute file offset
 * @warning The function must assure it does not seek beyond the bounds of file_maxsize.
 * @return the new offset in case of success, -1 otherwise
 */
typedef int (*shv_file_node_seeker)(shv_file_node_t *item, int offset);

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
typedef int (*shv_file_node_crc32)(shv_file_node_t *item, int start, size_t size,
                                   uint32_t *result);

typedef struct shv_file_node {
  shv_node_t shv_node;                /* Base shv_node */
  const char *name;                   /* File name, system-wise */
  void *fctx;                         /* Platform dependant file context, can be overriden */
  struct {
    shv_file_node_opener  opener;
    shv_file_node_getsize getsize;
    shv_file_node_writer  writer;
    shv_file_node_reader  reader;
    shv_file_node_seeker  seeker;
    shv_file_node_crc32   crc32;
  } fops;

  /* Stat method attributes */
  int file_type;                      /* Defined in shv_file_type */
  int file_maxsize;                   /* The implementation allows the file to grow,
                                         but not beyond the absolute maximum size */
  int file_pagesize;                  /* Page size on a given filesystem/physical memory
                                         for efficient write accesses */

  unsigned int state;                 /* Internal unpack write state */
  int file_offset;                    /* Internal current file offset */

  unsigned int crcstate;              /* Internal unpack crc state */
  uint32_t crc;                       /* Internal CRC accumulator */
  int crc_offset;                     /* Internal file CRC compute region */
  int crc_size;                       /* Internal file CRC compute region */

  bool platform_error;                /* A flag to indicate that something bad in the platform
                                         has happened. It does not indicate faulty data,
                                         it only indicates that the unpack should
                                         take this into consideration. */
} shv_file_node_t;

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

/**
 * @brief The file node method structure.
 *
 */
extern const shv_dmap_t shv_file_node_dmap;

/**
 * @brief Allocates and initializes a file node
 *
 * @param child_name
 * @param dir
 * @param mode
 * @return A nonNULL pointer on success, NULL otherwise
 */
shv_file_node_t *shv_tree_file_node_new(const char *child_name, const shv_dmap_t *dir, int mode);
