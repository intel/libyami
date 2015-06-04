/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2002 Thomas Vander Stichele <thomas@apestaart.org>
 *
 * gstutils.h: Header for various utility functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __GST_UTILS_H__
#define __GST_UTILS_H__

/* Define PUT and GET functions for unaligned memory */
#define _GST_GET(__data, __idx, __size, __shift) \
    (((uint##__size##_t) (((const uint8_t *) (__data))[__idx])) << (__shift))

#define _GST_PUT(__data, __idx, __size, __shift, __num) \
    (((uint8_t *) (__data))[__idx] = (((uint##__size##_t) (__num)) >> (__shift)) & 0xff)

/**
 * GST_READ_UINT64_BE:
 * @data: memory location
 *
 * Read a 64 bit unsigned integer value in big endian format from the memory buffer.
 */

/**
 * GST_READ_UINT64_LE:
 * @data: memory location
 *
 * Read a 64 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT64_BE(data)	(_GST_GET (data, 0, 64, 56) | \
					 _GST_GET (data, 1, 64, 48) | \
					 _GST_GET (data, 2, 64, 40) | \
					 _GST_GET (data, 3, 64, 32) | \
					 _GST_GET (data, 4, 64, 24) | \
					 _GST_GET (data, 5, 64, 16) | \
					 _GST_GET (data, 6, 64,  8) | \
					 _GST_GET (data, 7, 64,  0))

#define GST_READ_UINT64_LE(data)	(_GST_GET (data, 7, 64, 56) | \
					 _GST_GET (data, 6, 64, 48) | \
					 _GST_GET (data, 5, 64, 40) | \
					 _GST_GET (data, 4, 64, 32) | \
					 _GST_GET (data, 3, 64, 24) | \
					 _GST_GET (data, 2, 64, 16) | \
					 _GST_GET (data, 1, 64,  8) | \
					 _GST_GET (data, 0, 64,  0))

/**
 * GST_READ_UINT32_BE:
 * @data: memory location
 *
 * Read a 32 bit unsigned integer value in big endian format from the memory buffer.
 */

/**
 * GST_READ_UINT32_LE:
 * @data: memory location
 *
 * Read a 32 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT32_BE(data)	(_GST_GET (data, 0, 32, 24) | \
					 _GST_GET (data, 1, 32, 16) | \
					 _GST_GET (data, 2, 32,  8) | \
					 _GST_GET (data, 3, 32,  0))

#define GST_READ_UINT32_LE(data)	(_GST_GET (data, 3, 32, 24) | \
					 _GST_GET (data, 2, 32, 16) | \
					 _GST_GET (data, 1, 32,  8) | \
					 _GST_GET (data, 0, 32,  0))

/**
 * GST_READ_UINT24_BE:
 * @data: memory location
 *
 * Read a 24 bit unsigned integer value in big endian format from the memory buffer.
 */
#define GST_READ_UINT24_BE(data)       (_GST_GET (data, 0, 32, 16) | \
                                         _GST_GET (data, 1, 32,  8) | \
                                         _GST_GET (data, 2, 32,  0))

/**
 * GST_READ_UINT24_LE:
 * @data: memory location
 *
 * Read a 24 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT24_LE(data)       (_GST_GET (data, 2, 32, 16) | \
                                         _GST_GET (data, 1, 32,  8) | \
                                         _GST_GET (data, 0, 32,  0))

/**
 * GST_READ_UINT16_BE:
 * @data: memory location
 *
 * Read a 16 bit unsigned integer value in big endian format from the memory buffer.
 */
/**
 * GST_READ_UINT16_LE:
 * @data: memory location
 *
 * Read a 16 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT16_BE(data)	(_GST_GET (data, 0, 16,  8) | \
					 _GST_GET (data, 1, 16,  0))

#define GST_READ_UINT16_LE(data)	(_GST_GET (data, 1, 16,  8) | \
					 _GST_GET (data, 0, 16,  0))

/**
 * GST_READ_UINT8:
 * @data: memory location
 *
 * Read an 8 bit unsigned integer value from the memory buffer.
 */
#define GST_READ_UINT8(data)            (_GST_GET (data, 0,  8,  0))

/**
 * GST_WRITE_UINT64_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 64 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT64_BE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 64, 56, num); \
                                          _GST_PUT (__put_data, 1, 64, 48, num); \
                                          _GST_PUT (__put_data, 2, 64, 40, num); \
                                          _GST_PUT (__put_data, 3, 64, 32, num); \
                                          _GST_PUT (__put_data, 4, 64, 24, num); \
                                          _GST_PUT (__put_data, 5, 64, 16, num); \
                                          _GST_PUT (__put_data, 6, 64,  8, num); \
                                          _GST_PUT (__put_data, 7, 64,  0, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT64_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 64 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT64_LE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 64,  0, num); \
                                          _GST_PUT (__put_data, 1, 64,  8, num); \
                                          _GST_PUT (__put_data, 2, 64, 16, num); \
                                          _GST_PUT (__put_data, 3, 64, 24, num); \
                                          _GST_PUT (__put_data, 4, 64, 32, num); \
                                          _GST_PUT (__put_data, 5, 64, 40, num); \
                                          _GST_PUT (__put_data, 6, 64, 48, num); \
                                          _GST_PUT (__put_data, 7, 64, 56, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT32_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 32 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT32_BE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 32, 24, num); \
                                          _GST_PUT (__put_data, 1, 32, 16, num); \
                                          _GST_PUT (__put_data, 2, 32,  8, num); \
                                          _GST_PUT (__put_data, 3, 32,  0, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT32_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 32 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT32_LE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 32,  0, num); \
                                          _GST_PUT (__put_data, 1, 32,  8, num); \
                                          _GST_PUT (__put_data, 2, 32, 16, num); \
                                          _GST_PUT (__put_data, 3, 32, 24, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT24_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 24 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT24_BE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 32,  16, num); \
                                          _GST_PUT (__put_data, 1, 32,  8, num); \
                                          _GST_PUT (__put_data, 2, 32,  0, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT24_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 24 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT24_LE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 32,  0, num); \
                                          _GST_PUT (__put_data, 1, 32,  8, num); \
                                          _GST_PUT (__put_data, 2, 32,  16, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT16_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 16 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT16_BE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 16,  8, num); \
                                          _GST_PUT (__put_data, 1, 16,  0, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT16_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 16 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT16_LE(data, num)  do { \
                                          void* __put_data = data; \
                                          _GST_PUT (__put_data, 0, 16,  0, num); \
                                          _GST_PUT (__put_data, 1, 16,  8, num); \
                                        } while (0)

/**
 * GST_WRITE_UINT8:
 * @data: memory location
 * @num: value to store
 *
 * Store an 8 bit unsigned integer value into the memory buffer.
 */
#define GST_WRITE_UINT8(data, num)      do { \
                                          _GST_PUT (data, 0,  8,  0, num); \
                                        } while (0)

/**
 * GST_READ_FLOAT_LE:
 * @data: memory location
 *
 * Read a 32 bit float value in little endian format from the memory buffer.
 *
 * Returns: The floating point value read from @data
 */
inline static float
GST_READ_FLOAT_LE(const uint8_t *data)
{
  union
  {
    uint32_t i;
    float f;
  } u;

  u.i = GST_READ_UINT32_LE (data);
  return u.f;
}

/**
 * GST_READ_FLOAT_BE:
 * @data: memory location
 *
 * Read a 32 bit float value in big endian format from the memory buffer.
 *
 * Returns: The floating point value read from @data
 */
inline static float
GST_READ_FLOAT_BE(const uint8_t *data)
{
    union
    {
        uint32_t i;
        float f;
    } u;

  u.i = GST_READ_UINT32_BE (data);
  return u.f;
}

/**
 * GST_READ_DOUBLE_LE:
 * @data: memory location
 *
 * Read a 64 bit double value in little endian format from the memory buffer.
 *
 * Returns: The double-precision floating point value read from @data
 */
inline static double
GST_READ_DOUBLE_LE(const uint8_t *data)
{
  union
  {
    uint64_t i;
    double d;
  } u;

  u.i = GST_READ_UINT64_LE (data);
  return u.d;
}

/**
 * GST_READ_DOUBLE_BE:
 * @data: memory location
 *
 * Read a 64 bit double value in big endian format from the memory buffer.
 *
 * Returns: The double-precision floating point value read from @data
 */
inline static double
GST_READ_DOUBLE_BE(const uint8_t *data)
{
  union
  {
    uint64_t i;
    double d;
  } u;

  u.i = GST_READ_UINT64_BE (data);
  return u.d;
}

/**
 * GST_WRITE_FLOAT_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 32 bit float value in little endian format into the memory buffer.
 */
inline static void
GST_WRITE_FLOAT_LE(uint8_t *data, float num)
{
    union
    {
        uint32_t i;
        float f;
    } u;

  u.f = num;
  GST_WRITE_UINT32_LE (data, u.i);
}

/**
 * GST_WRITE_FLOAT_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 32 bit float value in big endian format into the memory buffer.
 */
inline static void
GST_WRITE_FLOAT_BE(uint8_t *data, float num)
{
    union
    {
        uint32_t i;
        float f;
    } u;

  u.f = num;
  GST_WRITE_UINT32_BE (data, u.i);
}

/**
 * GST_WRITE_DOUBLE_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 64 bit double value in little endian format into the memory buffer.
 */
inline static void
GST_WRITE_DOUBLE_LE(uint8_t *data, double num)
{
    union
    {
        uint64_t i;
        double d;
    } u;

  u.d = num;
  GST_WRITE_UINT64_LE (data, u.i);
}

/**
 * GST_WRITE_DOUBLE_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 64 bit double value in big endian format into the memory buffer.
 */
inline static void
GST_WRITE_DOUBLE_BE(uint8_t *data, double num)
{
    union
    {
        uint64_t i;
        double d;
    } u;

  u.d = num;
  GST_WRITE_UINT64_BE (data, u.i);
}

#endif /* __GST_UTILS_H__ */
