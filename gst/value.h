/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __VALUE_H__
#define __VALUE_H__

/***** padding for very extensible base classes *****/
#define PADDING_LARGE 20
/***** default padding of structures *****/
#define PADDING 4

#ifndef MIN(a, b)
#define MIN(a, b) (((a)>(b))?(b):(a))
#endif

#ifndef MAX(a, b)
#define MAX(a, b) (((a)>(b)?(a):(b)))
#endif

/*The maximum value which can be held in a signed char*/
#define G_MAXINT8	((signed char)  0x7f)
/*The maximum value which can be held in a unsigned short*/
#define G_MAXUINT16	((unsigned short) 0xffff)
/*The maximum value which can be held in a unsigned int*/
#define G_MAXUINT UINT_MAX

#ifndef G_MAXUINT32
    #define G_MAXUINT32	((unsigned int)0xffffffff)
#endif

#define G_N_ELEMENTS(array) (sizeof(array)/sizeof(array[0]))

#define VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME    3   /* frame-tag */
#define VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME        10  /* frame tag + magic number + frame width/height */

#endif /*end of value.h*/
