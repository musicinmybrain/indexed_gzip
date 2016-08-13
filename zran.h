#ifndef __ZRAN_H__
#define __ZRAN_H__

/*
 * The zran module is an adaptation of the zran example, written by Mark
 * Alder, which ships with the zlib source code. It allows the creation
 * of an index into a compressed file, which is used to improve the speed 
 * of random seek/read access to the uncompressed data.
 *
 * Author: Paul McCarthy <pauldmccarthy@gmail.com>
 */

#include <stdlib.h>
#include <stdint.h>


struct _zran_index;
struct _zran_point;


typedef struct _zran_index zran_index_t;
typedef struct _zran_point zran_point_t;

/* 
 * These values may be passed in as flags to the zran_init function.
 * They are specified as bit-masks, rather than bit locations. 
 */
enum {
  ZRAN_AUTO_BUILD = 1,

};
  

/* 
 * Struct representing the index.
 */
struct _zran_index {

    /*
     * Handle to the compressed file.
     */
    FILE         *fd;

    /*
     * Size of the compressed file. This 
     * is calculated in zran_init.
     */
    size_t        compressed_size;
    
    /* 
     * Spacing size in bytes, relative to the compressed 
     * data stream, between adjacent index points 
     */
    uint32_t      spacing;

    /*
     * Number of bytes of uncompressed data to store
     * for each index point. This must be a minimum
     * of 32768 bytes.
     */
    uint32_t      window_size;

    /*
     * Size, in bytes, of buffer used to store 
     * compressed data read from disk.
     */ 
    uint32_t      readbuf_size;

    /* 
     * Number of index points that have been created.
     */
    uint32_t      npoints;

    /*
     * Number of index points that can be stored.
     */
    uint32_t      size;

    /*
     * List of index points.
     */
    zran_point_t *list;

    /*
     * Most recently requested seek/read 
     * location into the uncompressed data 
     * stream - this is used to keep track 
     * of where the calling code thinks it 
     * is in the (uncompressed) file.
     */
    uint64_t      uncmp_seek_offset;

    /* 
     * Flags passed to zran_init
     */
    uint16_t      flags;
};


/* 
 * Struct representing a single seek point in the index.
 */
struct _zran_point {


    /* 
     * Location of this point in the compressed data 
     * stream. This is the location of the first full 
     * byte of compressed data - if  the compressed 
     * and uncompressed locations are not byte-aligned, 
     * the bits field below specifies the bit offset.
     */
    uint64_t  cmp_offset;
 
    /* 
     * Corresponding location of this point 
     * in the uncompressed data stream.
     */
    uint64_t  uncmp_offset;

    /* 
     * If this point is not byte-aligned, this specifies
     * the number of bits, in the compressed stream,
     * back from cmp_offset, that the uncompressed data
     * starts.
     */
    uint8_t   bits;

    /* 
     * Chunk of uncompressed data preceeding this point.
     * This is required to initialise decompression from
     * this point onwards.
     */
    uint8_t  *data;
};


/*
 * Initialise a zran_index_t struct for use with the given file. 
 *
 * Passing in 0 for the spacing, window_size and readbuf_size arguments
 * will result in the follwoing values being used:
 *
 *    spacing:      1048576
 *    window_size:  32768
 *    readbuf_size: 16384
 *
 * The flags argument is a bit mask used to control the following options:
 *
 *     ZRAN_AUTO_BUILD: Build the index automatically on demand.
 */
int  zran_init(
  zran_index_t *index,        /* The index                          */
  FILE         *fd,           /* Open handle to the compressed file */
  uint32_t      spacing,      /* Distance in bytes between 
                                 index seek points                  */
  uint32_t      window_size,  /* Number of uncompressed bytes 
                                 to store with each point           */
  uint32_t      readbuf_size, /* Number of bytes to read at a time  */
  uint16_t      flags         /* Flags controlling index behaviour  */
);


/* 
 * Frees the memory use by the given index. The zran_index_t struct
 * itself is not freed.
 */
void zran_free(
  zran_index_t *index /* The index */
);


/*
 * (Re-)Builds the index to cover the given range, which must be 
 * specified relative to the compressed data stream. Pass in 0
 * for both offsets to re-build the full index.
 * 
 * Returns 0 on success, non-0 on failure.
 */
int zran_build_index(
  zran_index_t *index, /* The index */
  uint64_t      from,  /* Build the index from this point */
  uint64_t      until  /* Build the index to this point   */
);


/*
 * Seek to the specified offset in the uncompressed data stream. 
 * If the index does not currently cover the offset, and it was 
 * created with the ZRAN_AUTO_BUILD flag, the index is expanded
 * to cover the offset.
 *
 * Seeking from the end of the uncompressed stream is not supported
 * - you may only seek from the beginning of the file, or from the 
 * current seek location. In other words, the whence argument must 
 * be equal to SEEK_SET or SEEK_CUR.
 *
 * Returns:
 *    - 0 for success.
 * 
 *    - < 0 to indicate failure.
 *   
 *    - > 0 to indicate that the index does not cover the requested 
 *        offset (will never happen if ZRAN_AUTO_BUILD is active).
 */
int zran_seek(
  zran_index_t  *index,   /* The index                      */
  off_t          offset,  /* Uncompressed offset to seek to */
  int            whence,  /* Must be SEEK_SET or SEEK_CUR   */
  zran_point_t **point    /* Optional place to store 
                             corresponding zran_point_t     */
);

/*
 * Returns the current seek location in the uncompressed data stream
 * (just returns zran_index_t.uncmp_seek_offset).
 */
long zran_tell(
  zran_index_t *index /* The index */
);

  

/*
 * Read len bytes from the current location in the uncompressed 
 * data stream, storing them in buf. If the index was created with 
 * the ZRAN_AUTO_BUILD flag, it is expanded as needed.
 *
 * Returns:
 *   - Number of bytes read for success.
 *   
 *   - -1 to indicate that the index does not cover the requested region
 *     (will never happen if ZRAN_AUTO_BUILD is active). 
 *
 *   - < -1 to indicate failure.
 */
int zran_read(
  zran_index_t  *index, /* The index                 */
  void          *buf,   /* Buffer to store len bytes */
  size_t         len    /* Number of bytes to read   */
);

#endif /* __ZRAN_H__ */
