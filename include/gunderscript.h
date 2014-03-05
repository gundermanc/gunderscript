/**
 * gunderscript.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See gunderscript.c for description.
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
#ifndef GUNDERSCRIPT__H__
#define GUNDERSCRIPT__H__

#include <assert.h>
#include <stdlib.h>
#include "compiler.h"
#include "vm.h"

/* stores an instance of a Gunderscript environment */
typedef struct Gunderscript {
  Compiler * compiler;
  VM * vm;
} Gunderscript;

bool gunderscript_new(Gunderscript * instance, size_t stackSize,
		      int callbacksSize);
Compiler * gunderscript_compiler(Gunderscript * instance);

VM * gunderscript_vm(Gunderscript * instance);

void gunderscript_free(Gunderscript * instance);
#endif /* GUNDERSCRIPT__H__ */
