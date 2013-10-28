/**
 * lexer.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See lexer.c for description.

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

#include <stdlib.h>
#include <stdio.h>
#include "gsbool.h"

typedef enum {
  LEXERERR_SUCCESS,
  LEXERERR_UNTERMINATED_STRING,
  LEXERERR_UNTERMINATED_COMMENT,
  LEXERERR_NEWLINE_IN_STRING,
  /*  LEXERERR_SYNTAX_ERROR, */
} LexerErr;

typedef enum {
  LEXERTYPE_STRING
} LexerType;

typedef struct Lexer {
  char * input;
  size_t inputLen;
  char * currToken;
  size_t currTokenLen;
  LexerErr err;
  int index;
  int lineNum;
} Lexer;
