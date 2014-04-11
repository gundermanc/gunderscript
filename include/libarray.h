/**
 * libarray.h
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See libarray.c for description.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBARRAY__H__
#define LIBARRAY__H__

#include "gunderscript.h"

#define LIBARRAY_ARRAY_TYPE      "LIBARRAY.0"
#define LIBARRAY_ARRAY_TYPE_LEN  10
#define LIBARRAY_ENTRY_SIZE    (VM_VAR_SIZE + 1)
#define LIBARRAY_BLOCK_COUNT    10 

VMLibData * libarray_array_new(int size);

bool libarray_array_set(VM * vm, VMLibData * data, int index, 
			void * value, int valueSize, VarType type);

int libarray_array_size(VMLibData * data);

bool libarray_array_get_type(VMLibData * data, int index, VarType * type);

bool libarray_array_get(VMLibData * data, int index,
			void * value, int valueSize);

bool libarray_install(Gunderscript * gunderscript);

#endif /* LIBARRAY__H__ */
