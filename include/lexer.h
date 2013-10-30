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
  LEXERERR_DUPLICATE_DECIMAL_PT,
  LEXERERR_TRAILING_DECIMAL_PT
} LexerErr;

typedef enum {
  LEXERTYPE_STRING,
  LEXERTYPE_NUMBER,
  LEXERTYPE_KEYVAR,
  LEXERTYPE_OPERATOR,
  LEXERTYPE_UNKNOWN
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


Lexer * lexer_new(char * input, size_t inputLen);

void lexer_free(Lexer * l);

char * lexer_next(Lexer * l, size_t * len);

char * lexer_current_token(Lexer * l, size_t * len);

LexerErr lexer_get_err(Lexer * l);

int lexer_line_num(Lexer * l);

LexerType lexer_token_type(char * token, size_t len, bool definitive);
