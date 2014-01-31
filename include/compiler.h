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
#include "vm.h"

/* errors that can occur during compile time */
typedef enum {
  COMPILERERR_SUCCESS,
  COMPILERERR_ALLOC_FAILED,
  COMPILERERR_EXPECTED_FNAME,
  COMPILERERR_EXPECTED_OPARENTH,
  COMPILERERR_EXPECTED_VARNAME,
  COMPILERERR_UNEXPECTED_TOKEN,
  COMPILERERR_EXPECTED_OBRACKET,
  COMPILERERR_EXPECTED_CBRACKET,
  COMPILERERR_PREV_DEFINED_FUNC,
  COMPILERERR_PREV_DEFINED_VAR,
  COMPILERERR_EXPECTED_VAR_NAME,
  COMPILERERR_EXPECTED_ENDSTATEMENT,
  COMPILERERR_STRING_TOO_LONG,
  COMPILERERR_UNKNOWN_OPERATOR,
  COMPILERERR_UNMATCHED_PARENTH,
  COMPILERERR_MALFORMED_ASSIGNMENT,
  COMPILERERR_UNDEFINED_VARIABLE,
  COMPILERERR_UNDEFINED_FUNCTION,
} CompilerErr;


/* a compiler instance type */
typedef struct Compiler {

  /* stack of hashtables of symbol structs containing the index at which the
   * variable will be stored in the frame stack frame in the byte code.
   */
  Stk * symTableStk;
  /* an instance of virtual machine. this is used during compile time to see
   * what functions are available to the script.
   */
  VM * vm;
  HT * functionHT;                /* hashtable of function structs */
  SB * outBuffer;                 /* string builder that accepts the output */
  CompilerErr err;                /* error code value */
} Compiler;


/* a function struct */
typedef struct CompilerFunc {
  char * name;                    /* the string name of a function */
  int index;                      /* the index where the function's 
				   * bytecode begins */
  int numArgs;                    /* the number of arguments required */
} CompilerFunc;


Compiler * compiler_new(VM * vm);

bool compiler_build(Compiler * compiler, char * input, size_t inputLen);

void compiler_set_err(Compiler * compiler, CompilerErr err);

CompilerErr compiler_get_err(Compiler * compiler);

void compiler_free(Compiler * compiler);

#endif /* COMPILER__H__ */
