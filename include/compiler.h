/**
 * compiler.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See compiler.c for description.
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

#ifndef COMPILER__H__
#define COMPILER__H__

#include "compcommon.h"
#include "stk.h"
#include "ht.h"
#include "sb.h"
#include "vm.h"


Compiler * compiler_new(VM * vm);

bool compiler_build(Compiler * compiler, char * input, size_t inputLen);

void compiler_set_err(Compiler * compiler, CompilerErr err);

CompilerFunc * compiler_function(Compiler * compiler, char * name, size_t len);

CompilerErr compiler_get_err(Compiler * compiler);

void compiler_free(Compiler * compiler);

size_t compiler_bytecode_size(Compiler * compiler);

bool compiler_bytecode(Compiler * compiler, char * buffer, size_t bufferSize);

#endif /* COMPILER__H__ */
