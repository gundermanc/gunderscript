/**
 * lexer.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The Lexer object simply caches the initialization String and a current index
 * and returns the next "token" when the Next Token method is called. Tokens
 * are low level, non-language-specific patterns in text, such as symbols, (, )
 * {, }, etc. For more information on the Lexer's behavior, see
 * the Next Token method's description.

 * This object acts to unify the interface for acquiring new tokens. The object
 * can be initialized using either a file, or String. Each time next is called,
 * the method peruses the selected input source until it finds the next token.

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

#include "lexer.h"

/**
 * Creates a new lexer object.
 * inputType: an enum value specifying the input source for the lexer. Can be
 * STRING, or FILE.
 * inputFn: If inputType is a String, inputFn must be a string to lex. If
 * inputType is FILE, inputFn must be the path to a file containing input to
 * lex.
 * inputFnLen: The length of inputFn. No hand holding is done here. This value
 * must be a positive value, <= the size of the buffer.
 */
Lexer * lexer_new(LexerInput inputType, char * inputFn, size_t inputFnLen) {

  /* handle string inputs */
  if(inputType == LEXERINPUT_STRING) {

    Lexer * lexer = calloc(1, sizeof(Lexer));
    if(lexer == NULL) {
      return NULL;
    }

    lexer->input = calloc(inputFnLen + 1, sizeof(char));
    if(lexer->input == NULL) {
      free(lexer);
      return NULL;
    }

    strncpy(lexer->input, inputFn, inputFnLen);
    return lexer;
  }
  return NULL;
}

/**
 * Frees any memory associated with the Lexer Object and closes the input
 * file if there is one.
 * l: the lexer object to free.
 */
void lexer_free(Lexer * l) {
  free(l->input);
  free(l);
}

bool lexer_has_next(Lexer * l) {
  return 0;
}

static bool next_parse_whitespace(char * input, int * i, size_t len) {
  if(input[*i] == ' ') {
    /* skip all whitespace characters */
    for(; *i < len && (input[i] == ' '); i++);

    return true;
  }
  return false;
}

bool next_parse_strings(char * input, int * i, size_t len, LexerErr * err) {

  /* this is the beginning of a string */
  if(input[*i] == '"') {
    /* skip all whitespace characters */
    for(; *i < len && input[i] != '"'; i++);
  }
}

/**
 * Returns the next token string from this lexer if successful, returns NULL
 * if error occurs. Calls Set Lexer Error Method and sets the last error value
 * to UNTERMINATED_STRING if unmatched quotes occur.
 *
 * Passes line number of current Token out too somehow. Implementation specific
 *
 * Tokens are Strings made from
 * the object's initialization String, split up into the following things:
 * - String :: Each block of text surrounded by quotes is considered to be a
 *             String.
 * - Operators :: Each +,-,/,*,% +=, -=, *=, /=, %=, ==, <=, >=, ==, <, >,
 *                /*, [end comment], //, /n (newline), !=
 * - Symbols :: Each (, ), , {, }, [, ], ;
 *  - Keywords/Variables :: Each word is treated as a token.
 */
char * lexer_next(Lexer * l, size_t * len) {
  int i;
  LexerErr err;

  /* loop through characters one by one */
  for(i = 0; i < len; i++) {

    /* skip all whitespace characters */
    next_parse_whitespace(input, i, len);

    
  }
  return NULL;
}

char * lexer_current_token(Lexer * l, size_t * len) {
  return NULL;

}

void lexer_set_err(Lexer * l, LexerErr err) {
  l->err = err;
}

LexerErr lexer_get_err(Lexer * l) {
  return l->err;
}

LexerErr lexer_line_num(Lexer * l) {
  return l->lineNum;
}

LexerType lexer_token_type(char * token, size_t len) {
  /*
  if(token[0] == '"' && token[len-1] == '"') {
    return STRING;
    } else if(*/
  return LEXERTYPE_STRING;
}
