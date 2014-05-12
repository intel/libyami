/*
 *  bitwriter.h - bitstream writer
 *
 *  Copyright (C) 2013 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef BIT_WRITER_H
#define BIT_WRITER_H

#include <string.h>
#include <stdlib.h>
/*FIXME: review this*/
#define G_LIKELY(a) (a)

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>

#define BIT_WRITER_DATA(writer)     ((writer)->data)
#define BIT_WRITER_BIT_SIZE(writer) ((writer)->bit_size)
#define BIT_WRITER(writer)          ((BitWriter *) (writer))

  typedef struct _BitWriter BitWriter;

/**
 * BitWriter:
 * @data: Allocated @data for bit writer to write
 * @bit_size: Size of written @data in bits
 *
 * Private:
 * @bit_capacity: Capacity of the allocated @data
 * @auto_grow: @data space can auto grow
 *
 * A bit writer instance.
 */
  struct _BitWriter
  {
    uint8_t *data;
    uint32_t bit_size;

    /*< private > */
    uint32_t bit_capacity;
    bool auto_grow;
  };

  BitWriter *bit_writer_new (uint32_t reserved_bits);

  BitWriter *bit_writer_new_fill (uint8_t * data, uint32_t bits);

  void bit_writer_free (BitWriter * writer, bool free_data);

  void bit_writer_init (BitWriter * bitwriter, uint32_t reserved_bits);

  void bit_writer_init_fill (BitWriter * bitwriter, uint8_t * data, uint32_t bits);

  void bit_writer_clear (BitWriter * bitwriter, bool free_data);

  uint bit_writer_get_size (BitWriter * bitwriter);

  uint8_t *bit_writer_get_data (BitWriter * bitwriter);

  bool bit_writer_set_pos (BitWriter * bitwriter, uint32_t pos);

  uint bit_writer_get_space (BitWriter * bitwriter);

    bool
      bit_writer_put_bits_uint8 (BitWriter * bitwriter,
      uint8_t value, uint32_t nbits);

    bool
      bit_writer_put_bits_uint16 (BitWriter * bitwriter,
      uint16_t value, uint32_t nbits);

    bool
      bit_writer_put_bits_uint32 (BitWriter * bitwriter,
      uint32_t value, uint32_t nbits);

    bool
      bit_writer_put_bits_uint64 (BitWriter * bitwriter,
      uint64_t value, uint32_t nbits);

    bool
      bit_writer_put_bytes (BitWriter * bitwriter, const uint8_t * data,
      uint32_t nbytes);

    bool bit_writer_align_bytes (BitWriter * bitwriter, uint8_t trailing_bit);

  static const uint8_t _bit_writer_bit_filling_mask[9] = {
    0x00, 0x01, 0x03, 0x07,
    0x0F, 0x1F, 0x3F, 0x7F,
    0xFF
  };

/* Aligned to 256 bytes */
#define __BITS_WRITER_ALIGNMENT_MASK 2047
#define __BITS_WRITER_ALIGNED(bitsize)                   \
    (((bitsize) + __BITS_WRITER_ALIGNMENT_MASK)&(~__BITS_WRITER_ALIGNMENT_MASK))

  static inline bool
      _bit_writer_check_space (BitWriter * bitwriter, uint32_t bits)
  {
    uint32_t new_bit_size = bits + bitwriter->bit_size;
    uint32_t clear_pos;

    assert (bitwriter->bit_size <= bitwriter->bit_capacity);
    if (new_bit_size <= bitwriter->bit_capacity)
      return true;

    if (!bitwriter->auto_grow)
      return false;

    /* auto grow space */
    new_bit_size = __BITS_WRITER_ALIGNED (new_bit_size);
    assert (new_bit_size
        && ((new_bit_size & __BITS_WRITER_ALIGNMENT_MASK) == 0));
    clear_pos = ((bitwriter->bit_size + 7) >> 3);
    bitwriter->data = realloc (bitwriter->data, (new_bit_size >> 3));
    memset (bitwriter->data + clear_pos, 0, (new_bit_size >> 3) - clear_pos);
    bitwriter->bit_capacity = new_bit_size;
    return true;
  }

#undef __BITS_WRITER_ALIGNMENT_MASK
#undef __BITS_WRITER_ALIGNED

#define __BIT_WRITER_WRITE_BITS_UNCHECKED(bits) \
static inline void \
bit_writer_put_bits_uint##bits##_unchecked( \
    BitWriter *bitwriter, \
    uint##bits##_t value, \
    uint32_t nbits \
) \
{ \
    uint32_t byte_pos, bit_offset; \
    uint8_t  *cur_byte; \
    uint32_t fill_bits; \
    \
    byte_pos = (bitwriter->bit_size >> 3); \
    bit_offset = (bitwriter->bit_size & 0x07); \
    cur_byte = bitwriter->data + byte_pos; \
    assert (nbits <= bits); \
    assert( bit_offset < 8 && \
            bitwriter->bit_size <= bitwriter->bit_capacity); \
    \
    while (nbits) { \
        fill_bits = ((8 - bit_offset) < nbits ? (8 - bit_offset) : nbits); \
        nbits -= fill_bits; \
        bitwriter->bit_size += fill_bits; \
        \
        *cur_byte |= (((value >> nbits) & _bit_writer_bit_filling_mask[fill_bits]) \
                      << (8 - bit_offset - fill_bits)); \
        ++cur_byte; \
        bit_offset = 0; \
    } \
    assert(cur_byte <= \
           (bitwriter->data + (bitwriter->bit_capacity >> 3))); \
}

  __BIT_WRITER_WRITE_BITS_UNCHECKED (8)
      __BIT_WRITER_WRITE_BITS_UNCHECKED (16)
      __BIT_WRITER_WRITE_BITS_UNCHECKED (32)
      __BIT_WRITER_WRITE_BITS_UNCHECKED (64)
#undef __BIT_WRITER_WRITE_BITS_UNCHECKED
  static inline uint bit_writer_get_size_unchecked (BitWriter * bitwriter)
  {
    return BIT_WRITER_BIT_SIZE (bitwriter);
  }

  static inline uint8_t *bit_writer_get_data_unchecked (BitWriter * bitwriter)
  {
    return BIT_WRITER_DATA (bitwriter);
  }

  static inline bool
      bit_writer_set_pos_unchecked (BitWriter * bitwriter, uint32_t pos)
  {
    BIT_WRITER_BIT_SIZE (bitwriter) = pos;
    return true;
  }

  static inline uint bit_writer_get_space_unchecked (BitWriter * bitwriter)
  {
    return bitwriter->bit_capacity - bitwriter->bit_size;
  }

  static inline void
      bit_writer_put_bytes_unchecked (BitWriter * bitwriter,
      const uint8_t * data, uint32_t nbytes)
  {
    if ((bitwriter->bit_size & 0x07) == 0) {
      memcpy (&bitwriter->data[bitwriter->bit_size >> 3], data, nbytes);
      bitwriter->bit_size += (nbytes << 3);
    } else {
      assert (0);
      while (nbytes) {
        bit_writer_put_bits_uint8_unchecked (bitwriter, *data, 8);
        --nbytes;
        ++data;
      }
    }
  }

  static inline void
      bit_writer_align_bytes_unchecked (BitWriter * bitwriter,
      uint8_t trailing_bit)
  {
    uint32_t bit_offset, bit_left;
    uint8_t value = 0;

    bit_offset = (bitwriter->bit_size & 0x07);
    if (!bit_offset)
      return;

    bit_left = 8 - bit_offset;
    if (trailing_bit)
      value = _bit_writer_bit_filling_mask[bit_left];
    return bit_writer_put_bits_uint8_unchecked (bitwriter, value, bit_left);
  }

#define __BIT_WRITER_WRITE_BITS_INLINE(bits) \
static inline bool \
_bit_writer_put_bits_uint##bits##_inline( \
    BitWriter *bitwriter, \
    uint##bits##_t value, \
    uint32_t nbits \
) \
{ \
    RETURN_VAL_IF_FAIL(bitwriter != NULL, false); \
    RETURN_VAL_IF_FAIL(nbits != 0, false); \
    RETURN_VAL_IF_FAIL(nbits <= bits, false); \
    \
    if (!_bit_writer_check_space(bitwriter, nbits)) \
        return false; \
    bit_writer_put_bits_uint##bits##_unchecked(bitwriter, value, nbits); \
    return true; \
}

  __BIT_WRITER_WRITE_BITS_INLINE (8)
      __BIT_WRITER_WRITE_BITS_INLINE (16)
      __BIT_WRITER_WRITE_BITS_INLINE (32)
      __BIT_WRITER_WRITE_BITS_INLINE (64)
#undef __BIT_WRITER_WRITE_BITS_INLINE
  static inline uint _bit_writer_get_size_inline (BitWriter * bitwriter)
  {
    RETURN_VAL_IF_FAIL (bitwriter != NULL, 0);

    return bit_writer_get_size_unchecked (bitwriter);
  }

  static inline uint8_t *_bit_writer_get_data_inline (BitWriter * bitwriter)
  {
    RETURN_VAL_IF_FAIL (bitwriter != NULL, NULL);

    return bit_writer_get_data_unchecked (bitwriter);
  }

  static inline bool
      _bit_writer_set_pos_inline (BitWriter * bitwriter, uint32_t pos)
  {
    RETURN_VAL_IF_FAIL (bitwriter != NULL, false);
    RETURN_VAL_IF_FAIL (pos <= bitwriter->bit_capacity, false);

    return bit_writer_set_pos_unchecked (bitwriter, pos);
  }

  static inline uint _bit_writer_get_space_inline (BitWriter * bitwriter)
  {
    RETURN_VAL_IF_FAIL (bitwriter != NULL, 0);
    RETURN_VAL_IF_FAIL (bitwriter->bit_size < bitwriter->bit_capacity, 0);

    return bit_writer_get_space_unchecked (bitwriter);
  }

  static inline bool
      _bit_writer_put_bytes_inline (BitWriter * bitwriter,
      const uint8_t * data, uint32_t nbytes)
  {
    RETURN_VAL_IF_FAIL (bitwriter != NULL, false);
    RETURN_VAL_IF_FAIL (data != NULL, false);
    RETURN_VAL_IF_FAIL (nbytes, false);

    if (!_bit_writer_check_space (bitwriter, nbytes * 8))
      return false;

    bit_writer_put_bytes_unchecked (bitwriter, data, nbytes);
    return true;
  }

  static inline bool
      _bit_writer_align_bytes_inline (BitWriter * bitwriter, uint8_t trailing_bit)
  {
    RETURN_VAL_IF_FAIL (bitwriter != NULL, false);
    RETURN_VAL_IF_FAIL ((trailing_bit == 0 || trailing_bit == 1), false);
    RETURN_VAL_IF_FAIL (((bitwriter->bit_size + 7) & (~7)) <=
        bitwriter->bit_capacity, false);

    bit_writer_align_bytes_unchecked (bitwriter, trailing_bit);
    return true;
  }

#ifndef BIT_WRITER_DISABLE_INLINES
#define bit_writer_get_size(bitwriter) \
    _bit_writer_get_size_inline(bitwriter)
#define bit_writer_get_data(bitwriter) \
    _bit_writer_get_data_inline(bitwriter)
#define bit_writer_set_pos(bitwriter, pos) \
    G_LIKELY (_bit_writer_set_pos_inline (bitwriter, pos))
#define bit_writer_get_space(bitwriter) \
    _bit_writer_get_space_inline(bitwriter)

#define bit_writer_put_bits_uint8(bitwriter, value, nbits) \
    G_LIKELY (_bit_writer_put_bits_uint8_inline (bitwriter, value, nbits))
#define bit_writer_put_bits_uint16(bitwriter, value, nbits) \
    G_LIKELY (_bit_writer_put_bits_uint16_inline (bitwriter, value, nbits))
#define bit_writer_put_bits_uint32(bitwriter, value, nbits) \
    G_LIKELY (_bit_writer_put_bits_uint32_inline (bitwriter, value, nbits))
#define bit_writer_put_bits_uint64(bitwriter, value, nbits) \
    G_LIKELY (_bit_writer_put_bits_uint64_inline (bitwriter, value, nbits))

#define bit_writer_put_bytes(bitwriter, data, nbytes) \
    G_LIKELY (_bit_writer_put_bytes_inline (bitwriter, data, nbytes))

#define bit_writer_align_bytes(bitwriter, trailing_bit) \
    G_LIKELY (_bit_writer_align_bytes_inline(bitwriter, trailing_bit))
#endif

#ifdef __cplusplus
}
#endif

#endif /* BIT_WRITER_H */
