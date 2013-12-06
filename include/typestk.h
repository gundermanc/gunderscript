/**
 * typestk.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See typestk.c for full description.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TYPESTK__H__
#define TYPESTK__H__

#include <stdlib.h>
#include <string.h>
#include "vmdefs.h"
#include "gsbool.h"

typedef struct TypeStkData {
  char type;
  char data[VM_VAR_SIZE];
}TypeStkData;

typedef struct TypeStk {
  TypeStkData * stack;
  int depth;
  int blockSize;
  int size;
}TypeStk;

TypeStk * typestk_new(int initialDepth, int blockSize);

void typestk_free(TypeStk * stack);


bool typestk_push(TypeStk * stack, void * data, size_t dataSize, VarType type);

bool typestk_peek(TypeStk * stack, void * value, 
		  size_t valueSize, VarType * type);

bool typestk_pop(TypeStk * stack, void * value,
		 size_t valueSize, VarType * type);

int typestk_size(TypeStk * stack);

#endif /* TYPESTK__H__ */
