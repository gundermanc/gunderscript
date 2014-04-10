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

#include <stdlib.h>
#include <stdio.h>
#include "gsbool.h"

#ifndef LEXER__H__
#define LEXER__H__

/* Lexer Error Codes */
typedef enum {
  LEXERERR_SUCCESS,
  LEXERERR_UNTERMINATED_STRING,
  LEXERERR_UNTERMINATED_COMMENT,
  LEXERERR_NEWLINE_IN_STRING_UNTERMINATED_ESCAPE,
  LEXERERR_DUPLICATE_DECIMAL_PT,
  LEXERERR_TRAILING_DECIMAL_PT
} LexerErr;

/* english translations of lexer errors */
static const char * const lexerErrorMessages [] = {
  "Success",
  "Unterminated string or char constant",
  "Unterminated multiline comment",
  "New line character in string, char, or unterminated escape sequence",
  "Muliple decimal points in numeric constant",
  "Trailing decimal point in number; all decimal points must"
    "be followed by digits",
};

/* Lexer Token Types */
typedef enum {
  LEXERTYPE_UNKNOWN,
  LEXERTYPE_STRING,
  LEXERTYPE_CHAR,
  LEXERTYPE_NUMBER,
  LEXERTYPE_KEYVAR,
  LEXERTYPE_BRACKETS,
  LEXERTYPE_PARENTHESIS,
  LEXERTYPE_OPERATOR,
  LEXERTYPE_ENDSTATEMENT,
  LEXERTYPE_ARGDELIM,
} LexerType;

/* Lexer Instance Struct */
typedef struct Lexer {
  char * input;
  size_t inputLen;
  char * currToken;
  size_t currTokenLen;
  LexerType currTokenType;
  char * nextToken;
  size_t nextTokenLen;
  LexerType nextTokenType;
  LexerErr err;
  int index;
  int lineNum;
} Lexer;


Lexer * lexer_new(char * input, size_t inputLen);

void lexer_free(Lexer * l);

char * lexer_next(Lexer * l, LexerType * type, size_t * len);

char * lexer_current_token(Lexer * l, LexerType * type, size_t * len);

LexerErr lexer_get_err(Lexer * l);

int lexer_line_num(Lexer * l);

LexerType lexer_token_type(char * token, size_t len, bool definitive);

char * lexer_peek(Lexer * lexer, LexerType * type, size_t * len);

const char * lexer_err_to_string(LexerErr err);

#endif /* LEXER__H__*/
