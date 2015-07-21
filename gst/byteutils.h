/*
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Liu Yangbin <yangbinx.liu@intel.com>
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

#ifndef __BYTE_UTILS_H__
#define __BYTE_UTILS_H__

/* Read a 64 bit unsigned integer value in big endian format from the memory buffer. */
#define GST_READ_UINT64_BE(data) \
    ((((uint64_t)(((const uint8_t*)(data))[0])) << 56) | \
     (((uint64_t)(((const uint8_t*)(data))[1])) << 48) | \
     (((uint64_t)(((const uint8_t*)(data))[2])) << 40) | \
     (((uint64_t)(((const uint8_t*)(data))[3])) << 32) | \
     (((uint64_t)(((const uint8_t*)(data))[4])) << 24) | \
     (((uint64_t)(((const uint8_t*)(data))[5])) << 16) | \
     (((uint64_t)(((const uint8_t*)(data))[6])) << 8)  | \
     (((uint64_t)(((const uint8_t*)(data))[7])) << 0))

/* Read a 64 bit unsigned integer value in little endian format from the memory buffer. */
#define GST_READ_UINT64_LE(data) \
    ((((uint64_t)(((const uint8_t*)(data))[7])) << 56) | \
     (((uint64_t)(((const uint8_t*)(data))[6])) << 48) | \
     (((uint64_t)(((const uint8_t*)(data))[5])) << 40) | \
     (((uint64_t)(((const uint8_t*)(data))[4])) << 32) | \
     (((uint64_t)(((const uint8_t*)(data))[3])) << 24) | \
     (((uint64_t)(((const uint8_t*)(data))[2])) << 16) | \
     (((uint64_t)(((const uint8_t*)(data))[1])) << 8)  | \
     (((uint64_t)(((const uint8_t*)(data))[0])) << 0))

/* Read a 32 bit unsigned integer value in big endian format from the memory buffer. */
#define GST_READ_UINT32_BE(data) \
    ((((uint32_t)(((const uint8_t*)(data))[0])) << 24) | \
     (((uint32_t)(((const uint8_t*)(data))[1])) << 16) | \
     (((uint32_t)(((const uint8_t*)(data))[2])) << 8)  | \
     (((uint32_t)(((const uint8_t*)(data))[3])) << 0))

/* Read a 32 bit unsigned integer value in little endian format from the memory buffer. */
#define GST_READ_UINT32_LE(data) \
    ((((uint32_t)(((const uint8_t*)(data))[3])) << 24) | \
     (((uint32_t)(((const uint8_t*)(data))[2])) << 16) | \
     (((uint32_t)(((const uint8_t*)(data))[1])) << 8)  | \
     (((uint32_t)(((const uint8_t*)(data))[0])) << 0))

/*  Read a 24 bit unsigned integer value in big endian format from the memory buffer. */
#define GST_READ_UINT24_BE(data) \
     ((((uint32_t)(((const uint8_t*)(data))[0])) << 16) | \
      (((uint32_t)(((const uint8_t*)(data))[1])) << 8)  | \
      (((uint32_t)(((const uint8_t*)(data))[2])) << 0))

/* Read a 24 bit unsigned integer value in little endian format from the memory buffer. */
#define GST_READ_UINT24_LE(data) \
     ((((uint32_t)(((const uint8_t*)(data))[2])) << 16) | \
      (((uint32_t)(((const uint8_t*)(data))[1])) << 8)  | \
      (((uint32_t)(((const uint8_t*)(data))[0])) << 0))

/* Read a 16 bit unsigned integer value in big endian format from the memory buffer. */
#define GST_READ_UINT16_BE(data) \
     ((((uint16_t)(((const uint8_t*)(data))[0])) << 8) | \
      (((uint16_t)(((const uint8_t*)(data))[1])) << 0))

/* Read a 16 bit unsigned integer value in little endian format from the memory buffer. */
#define GST_READ_UINT16_LE(data) \
     ((((uint16_t)(((const uint8_t*)(data))[1])) << 8) | \
      (((uint16_t)(((const uint8_t*)(data))[0])) << 0))

/* Read an 8 bit unsigned integer value from the memory buffer. */
#define GST_READ_UINT8(data) ((((uint8_t)(((const uint8_t*)(data))[0])) << 0))

/* Store a 64 bit unsigned integer value in big endian format into the memory buffer. */
#define GST_WRITE_UINT64_BE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint64_t)(num)) >> 56) & 0xff; \
        ((uint8_t*)(data))[1] = (((uint64_t)(num)) >> 48) & 0xff; \
        ((uint8_t*)(data))[2] = (((uint64_t)(num)) >> 40) & 0xff; \
        ((uint8_t*)(data))[3] = (((uint64_t)(num)) >> 32) & 0xff; \
        ((uint8_t*)(data))[4] = (((uint64_t)(num)) >> 24) & 0xff; \
        ((uint8_t*)(data))[5] = (((uint64_t)(num)) >> 16) & 0xff; \
        ((uint8_t*)(data))[6] = (((uint64_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[7] = (((uint64_t)(num)) >> 0)  & 0xff; \
    } while (0)

/* Store a 64 bit unsigned integer value in little endian format into the memory buffer. */
#define GST_WRITE_UINT64_LE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint64_t)(num)) >> 0)  & 0xff; \
        ((uint8_t*)(data))[1] = (((uint64_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[2] = (((uint64_t)(num)) >> 16) & 0xff; \
        ((uint8_t*)(data))[3] = (((uint64_t)(num)) >> 24) & 0xff; \
        ((uint8_t*)(data))[4] = (((uint64_t)(num)) >> 32) & 0xff; \
        ((uint8_t*)(data))[5] = (((uint64_t)(num)) >> 40) & 0xff; \
        ((uint8_t*)(data))[6] = (((uint64_t)(num)) >> 48) & 0xff; \
        ((uint8_t*)(data))[7] = (((uint64_t)(num)) >> 56) & 0xff; \
    } while (0)

/* Store a 32 bit unsigned integer value in big endian format into the memory buffer. */
#define GST_WRITE_UINT32_BE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint32_t)(num)) >> 24) & 0xff; \
        ((uint8_t*)(data))[1] = (((uint32_t)(num)) >> 16) & 0xff; \
        ((uint8_t*)(data))[2] = (((uint32_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[3] = (((uint32_t)(num)) >> 0)  & 0xff; \
    } while (0)

/* Store a 32 bit unsigned integer value in little endian format into the memory buffer. */
#define GST_WRITE_UINT32_LE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint32_t)(num)) >> 0)  & 0xff; \
        ((uint8_t*)(data))[1] = (((uint32_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[2] = (((uint32_t)(num)) >> 16) & 0xff; \
        ((uint8_t*)(data))[3] = (((uint32_t)(num)) >> 24) & 0xff; \
    } while (0)

/* Store a 24 bit unsigned integer value in big endian format into the memory buffer. */
#define GST_WRITE_UINT24_BE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint32_t)(num)) >> 16) & 0xff; \
        ((uint8_t*)(data))[1] = (((uint32_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[2] = (((uint32_t)(num)) >> 0)  & 0xff; \
    } while (0)

/* Store a 24 bit unsigned integer value in little endian format into the memory buffer. */
#define GST_WRITE_UINT24_LE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint32_t)(num)) >> 0)  & 0xff; \
        ((uint8_t*)(data))[1] = (((uint32_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[2] = (((uint32_t)(num)) >> 16) & 0xff; \
    } while (0)

/* Store a 16 bit unsigned integer value in big endian format into the memory buffer. */
#define GST_WRITE_UINT16_BE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint16_t)(num)) >> 8)  & 0xff; \
        ((uint8_t*)(data))[1] = (((uint16_t)(num)) >> 0)  & 0xff; \
    } while (0)

/* Store a 16 bit unsigned integer value in little endian format into the memory buffer. */
#define GST_WRITE_UINT16_LE(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint16_t)(num)) >> 0)  & 0xff; \
        ((uint8_t*)(data))[1] = (((uint16_t)(num)) >> 8)  & 0xff; \
    } while (0)

/* Store an 8 bit unsigned integer value into the memory buffer. */
#define GST_WRITE_UINT8(data, num) \
    do { \
        ((uint8_t*)(data))[0] = (((uint8_t)(num)) >> 0)  & 0xff; \
    } while (0)

/* Read a 32 bit float value in little endian format from the memory buffer. */
inline static float
GST_READ_FLOAT_LE(const uint8_t *data)
{
    uint32_t i = 0;
    float f = 0;

    i = GST_READ_UINT32_LE(data);
    memcpy(&f, &i, sizeof(uint32_t));
    return f;
}

/* Read a 32 bit float value in big endian format from the memory buffer. */
inline static float
GST_READ_FLOAT_BE(const uint8_t *data)
{
    uint32_t i = 0;
    float f = 0;

    i = GST_READ_UINT32_BE(data);
    memcpy(&f, &i, sizeof(uint32_t));
    return f;
}

/* Read a 64 bit double value in little endian format from the memory buffer. */
inline static double
GST_READ_DOUBLE_LE(const uint8_t *data)
{
    uint64_t i = 0;
    double d = 0;

    i = GST_READ_UINT64_LE(data);
    memcpy(&d, &i, sizeof(uint64_t));
    return d;
}

/* Read a 64 bit double value in big endian format from the memory buffer. */
inline static double
GST_READ_DOUBLE_BE(const uint8_t *data)
{
    uint64_t i = 0;
    double d = 0;

    i = GST_READ_UINT64_BE(data);
    memcpy(&d, &i, sizeof(uint64_t));
    return d;
}

/* Store a 32 bit float value in little endian format into the memory buffer. */
inline static void
GST_WRITE_FLOAT_LE(uint8_t *data, float num)
{
    float f = num;
    uint32_t i = 0;

    memcpy(&i, &f, sizeof(float));
    GST_WRITE_UINT32_LE (data, i);
}

/* Store a 32 bit float value in big endian format into the memory buffer. */
inline static void
GST_WRITE_FLOAT_BE(uint8_t *data, float num)
{
    float f = num;
    uint32_t i = 0;

    memcpy(&i, &f, sizeof(float));
    GST_WRITE_UINT32_BE (data, i);
}

/* Store a 64 bit double value in little endian format into the memory buffer. */
inline static void
GST_WRITE_DOUBLE_LE(uint8_t *data, double num)
{
    double d = num;
    uint64_t i = 0;

    memcpy(&i, &d, sizeof(double));
    GST_WRITE_UINT64_LE (data, i);
}

/* Store a 64 bit double value in big endian format into the memory buffer. */
inline static void
GST_WRITE_DOUBLE_BE(uint8_t *data, double num)
{
    double d = num;
    uint64_t i;

    memcpy(&i, &d, sizeof(double));
    GST_WRITE_UINT64_BE (data, i);
}

#endif /* __BYTE_UTILS_H__ */
