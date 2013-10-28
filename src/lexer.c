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
 *
 * This object acts to unify the interface for acquiring new tokens. The object
 * can be initialized using either a file, or String. Each time next is called,
 * the method peruses the selected input source until it finds the next token.
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

#include "lexer.h"
#include <string.h>
#include "gsbool.h"

/**
 * Operators Array
 * Each operator must be at most 3 characters long, and must be NULL terminated.
 */
const char * gc_operatorsArray[] = {"+\0", "-\0", "/\0", "*\0", "%\0", "+=", "-=", "/=", "*=",
				     "==", "<=", ">=", ">\0", "<\0", "!=", NULL}; 

/**
 * Creates a set of operators
 */
bool initialize_operator_set(Lexer * l) {
  int i;
  l->operatorSet = set_new();

  /* add operators to set to allow for quick checking against them */
  for(i = 0; gc_operatorsArray[i]; i++) {
    set_add(l->operatorSet, gc_operatorsArray[i], strlen(gc_operatorsArray[i]));
    printf("Indx: %i", i);
  }

  printf("\n\nContains? : %i\n\n", set_contains(l->operatorSet, "+=", 2));

  /* TODO: Add error checking */
  return true;
}

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
Lexer * lexer_new(char * input, size_t inputLen) {

  Lexer * lexer = calloc(1, sizeof(Lexer));
  if(lexer == NULL) {
    return NULL;
  }

  lexer->input = calloc(inputLen + 1, sizeof(char));
  if(lexer->input == NULL) {
    free(lexer);
    return NULL;
  }

  strncpy(lexer->input, input, inputLen);
  lexer->inputLen = inputLen;

  initialize_operator_set(lexer);

  return lexer;
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

static bool is_white_space(char c) {

  switch(c) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
    return true;
  }

  return false;
}

static void advance_char(Lexer * l) {
  l->index++;
}

static char next_char(Lexer * l, bool advance) {

  if(advance) {
    advance_char(l);
  }

  return l->input[l->index];
}

static char prev_char(Lexer * l) {
  return l->input[l->index - 1];
}

static char peek_char(Lexer * l) {
  return l->input[l->index + 1];
}

/**
 * Gets the number of characters remaining in the buffer
 */
static int remaining_chars(Lexer * l) {
  return l->inputLen - l->index;
}

static bool next_parse_whitespace(Lexer * l) {

  if(is_white_space(l->input[l->index])) {

    /* skip all whitespace characters */
    while(remaining_chars(l) > 0 && is_white_space(next_char(l, true)));

    return true;
  }

  return false;
}

static bool next_parse_comments(Lexer * l) {

  /* if there is another character after this one, check this one and the next one to see if
   * they are single line comment characters: "//". If so, skip the comment characters.
   */
  if(remaining_chars(l) > 0
     && next_char(l, false) == '/'
     && peek_char(l) == '/') {

    while(remaining_chars(l) > 0 && next_char(l, true) != '\n');

    return true;
  }

  /* if there is another character after the current one, check it to see if they
   * are single line line comment characters: "/*". If so, skip the comment chars.
   */
  if(remaining_chars(l) > 0
     && next_char(l, false) == '/'
     && peek_char(l) == '*') {

    while(remaining_chars(l) > 0) {
      if(prev_char(l) == '*' && next_char(l, false) == '/') {

	/* prevent the closing / from being re-evaluated next iteration */
	advance_char(l);
	return true;
      }
      advance_char(l);
    }

    l->err = LEXERERR_UNTERMINATED_COMMENT;
  }

  return false;
}

bool next_parse_strings(Lexer * l) {

  /* this is the beginning of a string */
  if(next_char(l, false) == '"') {
    int beginStrIndex = l->index;

    /* add characters to the string */
    l->index++;
    for(; remaining_chars(l) > 0; advance_char(l)) {

      /* encountered end of string, return it along with the quotes*/
      if(next_char(l, false) == '"' && prev_char(l) != '\\') {
	l->currTokenLen = (l->index - beginStrIndex) + 1;
	l->currToken = l->input + beginStrIndex;
	l->err = LEXERERR_SUCCESS;

	/* increment index again to prevent the end quote from being evaluated
	 * next iteration.
	 */
	advance_char(l);
	return true;
      }

      /* prevent newlines from being put in strings */
      if(next_char(l, false) == '\n') {
	l->err = LEXERERR_NEWLINE_IN_STRING;
	return true;
      }
    
    }

    /* error occurred, set token to null and quit */
    l->currToken = NULL;
    l->err = LEXERERR_UNTERMINATED_STRING;
    return true;
  }
  return false;
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
  LexerErr err;

  /* lexer loop */
  for(; remaining_chars(l) > 0; ) {


    /* begin parsing code */
    if(next_parse_whitespace(l)) {
      /* do nothing, but discontinue if/else chain when whitespace is detected */
    } else if(next_parse_comments(l)) {
      if(l->err != LEXERERR_SUCCESS) {
	break;
      }
    } else if(next_parse_strings(l)) {
      *len = l->currTokenLen;
      return l->currToken;
    } else {
 
      /* temporary warning that helps with debugging */
      printf("WARNING: Control reached end of Recursive Parse Loop\n");
      break;
    }
  }

  /* if control reaches this point, either an error occurred, or there are no
   *   more tokens left. finalize lexer and prevent any more next requests 
   */
  l->index = l->inputLen;
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
