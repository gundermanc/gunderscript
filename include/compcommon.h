/**
 * compcommon.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See compcommon.h for description.
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

#ifndef COMPCOMMON__H__
#define COMPCOMMON__H__

#include "gsbool.h"
#include "stk.h"
#include "vm.h"
#include "ht.h"
#include "buffer.h"
#include "lexer.h"

/* initial size of all hashtables */
#define COMPILER_INITIAL_HTSIZE   11
/* number of items to add to hashtable on rehash */
#define COMPILER_HTBLOCKSIZE      12
/* hashtable load factor upon which it will be rehashed */
#define COMPILER_HTLOADFACTOR     0.75

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
  COMPILERERR_EXPECTED_ENDSTATEMENT,
  COMPILERERR_STRING_TOO_LONG,
  COMPILERERR_UNKNOWN_OPERATOR,
  COMPILERERR_UNMATCHED_PARENTH,
  COMPILERERR_MALFORMED_ASSIGNMENT,
  COMPILERERR_UNDEFINED_VARIABLE,
  COMPILERERR_UNDEFINED_FUNCTION,
  COMPILERERR_INCORRECT_NUMARGS,
  COMPILERERR_MALFORMED_IFORLOOP,
  COMPILERERR_LEXER_ERR,
} CompilerErr;

/* english translations of compiler errors */
static const const char * const compilerErrorMessages [] = {
  "Success",
  "Memory allocation failed",
  "Expected function name, but none present",
  "Expected '(', but none present",
  "Expected variable name, but none present",
  "Unexpected token, check for missing operators or unneccessary symbols",
  "Expected '{', but none present",
  "Expected '}', but none present",
  "A function of this name already exists",
  "A variable of this name already exists",
  "Missing ';'",
  "String is too long (len >= 255)",
  "Unknown operator",
  "Mismatched parenthesis",
  "Malformed assignment statement",
  "Undefined variable",
  "Undefined function",
  "Incorrect number of arguments for this function",
  "Malformed loop or if statement",
  "Lex error: call compiler_lex_err() for the LexerErr",
};

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
  Buffer * outBuffer;             /* buffer builder that accepts the output */
  CompilerErr err;                /* error code value */
  int errorLineNum;               /* line number where error occurred */
  LexerErr lexerErr;              /* the error code passed by the lexer */
} Compiler;

/* a function struct */
typedef struct CompilerFunc {
  char * name;                    /* the string name of a function */
  int index;                      /* the index where the function's 
				   * bytecode begins */
  int numArgs;                    /* the number of arguments required */
  int numVars;                    /* the number of variables required */
  bool exported;
} CompilerFunc;

bool tokens_equal(char * token1, size_t num1,
		  char * token2, size_t num2);

OpCode operator_to_opcode(char * operator, size_t len);

bool symtblstk_push(Compiler * c);

HT * symtblstk_pop(Compiler * c);

HT * symtblstk_peek(Compiler * c, int offset);

int operator_precedence(char * operator, size_t operatorLen);

int topstack_precedence(TypeStk * stk, Stk * lenStk);

int topstack_type(TypeStk * stk, Stk * lenStk);

#endif /* COMPCOMMON__H__ */
