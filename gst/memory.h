/* GLIB sliced memory - fast threaded memory chunk allocator
 * Copyright (C) 2005 Tim Janik
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#define G_GNUC_MALLOC __attribute__((__malloc__))

#define g_malloc(size) malloc(size)
#define g_malloc0(n_bytes) calloc(1, n_bytes)
#define g_realloc(ptr, size) realloc(ptr, size)
#define g_realloc_n(mem, n_blocks, n_block_bytes) \
		g_realloc(mem, n_blocks * n_block_bytes)
#define g_try_realloc(ptr, size) realloc(ptr, size)

#define g_new(struct_type, n_structs)  \
    (struct_type*)malloc(sizeof(struct_type)*(n_structs))

#define g_new0(struct_type, n_structs)  \
    ({ struct_type* tmp = (struct_type*)malloc(sizeof(struct_type)*(n_structs)); \
    if(tmp) \
        memset(tmp, 0, sizeof(struct_type)*(n_structs));\
	tmp; })

#define g_slice_new(struct_type) \
    (struct_type*)malloc(sizeof(struct_type))

#define g_slice_new0(struct_type) \
    ({ struct_type* tmp = (struct_type*)malloc(sizeof(struct_type)); \
    if(tmp) \
        memset(tmp, 0, sizeof(struct_type));\
	tmp; })

#define g_slice_free(type, mem) \
    free(mem)

inline static void* 
g_memdup (const void* mem, uint32_t byte_size)
{
    void* new_mem;

    if (mem)
    {
        new_mem = malloc(byte_size);
        memcpy (new_mem, mem, byte_size);
    }
    else
        new_mem = NULL;

    return new_mem;
}

inline static void
g_free (void* mem)
{
    if(mem == NULL)
        return;
    else
        free(mem);
}

inline static uint32_t 
g_bit_storage(uint32_t value)
{
  uint32_t bits = 0;
  while (value >> bits)
     bits ++;
  
  return bits;
}

#endif  /*end of memory.h*/
