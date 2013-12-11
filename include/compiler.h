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

#include "stk.h"
#include "ht.h"
#include "sb.h"


typedef enum {
  COMPILERERR_SUCCESS
} CompilerErr;


typedef struct Compiler {
  Stk * symTableStk;
  HT * functionHT;
  SB * outBuffer;
  CompilerErr err;
} Compiler;

void compiler_free(Compiler * compiler);

#endif /* COMPILER__H__ */
