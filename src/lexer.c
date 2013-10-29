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

/* TODO: decide whether or not to remove operator checking...leave for compiler to do?? */
/**
 * Operators Array. This array contains the operators that are considered to be valid
 * while lexing.
 * Each operator can be any length in characters, but the array must be NULL terminated.
 */
const static char * gc_operatorsArray[] = {"+", "-", "/", "*", "%", "+=", "-=", "/=", "*=",
				     "==", "<=", ">=", ">", "<", "!=", NULL}; 

/**
 * Creates a set of operators with constant time lookup that will be used to
 * lex operators.
 * l: the current lexer instance
 * returns: true if the initialization is successful, and false if initialization
 * fails due to malloc errors.
 */
static bool initialize_operator_set(Lexer * l) {
  int i;
  l->operatorSet = set_new();

  /* if memory error allocating set, return error. */
  if(l->operatorSet == NULL) {
    return false;
  }

  /* return false;*/

  /* add operators to set to allow for quick checking against them */
   for(i = 0; gc_operatorsArray[i] != NULL; i++) {

    /* if malloc error occurs, exit */
    if(!set_add(l->operatorSet, gc_operatorsArray[i], 
		strlen(gc_operatorsArray[i]), NULL)) {
      set_free(l->operatorSet);
      return false;
    }
  }

  return true;
}

/**
 * Frees the operator set for the provided lexer instance.
 * l: the lexer instance who's operator set should be freed.
 */
static void kill_operator_set(Lexer * l) {
  set_free(l->operatorSet);
}

/* prevents and subsequent lexer_next calls */ 
static void finalize_lexer(Lexer * l) {
  l->index = l->input;
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

  /* allocate lexer, return NULL if fails */
  Lexer * lexer = calloc(1, sizeof(Lexer));
  if(lexer == NULL) {
    return NULL;
  }

  /* create operator set and allocate input storage string and handle failure */
  lexer->input = calloc(inputLen + 1, sizeof(char));
  if(/*!initialize_operator_set(lexer) ||*/ lexer->input == NULL) {

    /* TODO: this NULL check is here. Make sure it works. */
    free(lexer);
    return NULL;
  }

  
  strncpy(lexer->input, input, inputLen);
  lexer->inputLen = inputLen;

  return lexer;
}

/**
 * Frees any memory associated with the Lexer Object and closes the input
 * file if there is one.
 * l: the lexer object to free.
 */ 
void lexer_free(Lexer * l) {
  /*kill_operator_set(l);*/
  free(l->input);
  free(l);
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
    finalize_lexer(l);
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
	finalize_lexer(l);
	return true;
      }
    
    }

    /* error occurred, set token to null and quit */
    l->currToken = NULL;
    l->err = LEXERERR_UNTERMINATED_STRING;
    finalize_lexer(l);
    return true;
  }
  return false;
}

static bool is_letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_digit(char c) {
  return (c >= '0' && c <= '9');
}

/**
 * Returns whether or not a string is possibly a keyword, variable, or function
 * name. For a string to be a keyword or function name, it must start with a
 * letter (A-Z, a-z) and all characters within it must be either a letter or a
 * digit.
 */
/* TODO: perhaps remove this */
static bool is_valid_keyvar(char * input, size_t inputLen) {

  int i = 0;

  if(inputLen < 1) {
    return false;
  }

  if(!is_letter(input[0])) {
    return false;
  }

  for(i = 0; i < inputLen; i++) {
    if(!is_digit(input[i]) && !is_letter(input[i])) {
      return false;
    }
  }

  return true;
}

/* extract contiguous bodies of letters */
static bool next_parse_keyvars(Lexer * l) {

  if(is_letter(next_char(l, false))) {
    int beginStrIndex = l->index;

    /* extract entire word */
    while(remaining_chars(l) > 0 
	  && (is_letter(next_char(l, false)) || is_digit(next_char(l, false)))) {
      advance_char(l);
    }

    l->currTokenLen = (l->index - beginStrIndex);
    l->currToken = l->input + beginStrIndex;
    l->err = LEXERERR_SUCCESS;

    return true;
  }

  return false;
}

/* extract contiguous bodies of letters */
static bool next_parse_numbers(Lexer * l) {

  if(is_digit(next_char(l, false))) {
    int beginStrIndex = l->index;
    bool decimalDetected = false;

    /* move index to end of number */
    while(remaining_chars(l) > 0 
	  && (is_digit(next_char(l, false)) || next_char(l, false) == '.')) {

      /* prevent multiple decimal points in one number */
      if(next_char(l, false) == '.') {


	if(decimalDetected) {
	  l->err = LEXERERR_DUPLICATE_DECIMAL_PT;
	  finalize_lexer(l);
	  l->currToken = NULL;
	  l->currTokenLen = 0;
	  return true;
	}
	decimalDetected = true;
      }
      advance_char(l);
    }

    /* save number to currentToken pointer */
    l->currTokenLen = (l->index - beginStrIndex);
    l->currToken = l->input + beginStrIndex;
    l->err = LEXERERR_SUCCESS;

    return true;
  }

  return false;
}

/* superficially decides if this character is is of the "operator" type. Basically,
 * its not whitespace, letter, or digit.
 */
static bool is_operator(char c) {
  return (!is_digit(c) && !is_letter(c) && !is_white_space(c));
}

/* extract contiguous bodies of letters */
static bool next_parse_operators(Lexer * l) {

  if(is_operator(next_char(l, false))) {
    int beginStrIndex = l->index;
    char * token;
    size_t tokenLen;

    /* move index to end of symbols */
    while(remaining_chars(l) > 0 
	  && is_operator(next_char(l, false))) {
      advance_char(l);
    }

    tokenLen = (l->index - beginStrIndex);
    token  = l->input + beginStrIndex;

    /* TODO: either add an error code for invalid operator, or 
     * remove operator checking and leave this task to the the
     * compiler.
     */
    /* make sure symbol is in the operator set */
    /*   if(set_contains(l->operatorSet, token, tokenLen)) {*/
      l->currToken = token;
      l->currTokenLen = tokenLen;
      l->err = LEXERERR_SUCCESS;
      /* }*/

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

  /* Recursive Descent Parsing Loop:
   * Each iteration, the lexer attempts to handle the current set of characters
   * by feeding the current, previous, and next characters into a sub parser.
   * The sub parser will then look at the current character and decide whether
   * or not it can handle the current situation. For example, if the whitespace
   * parser sees a whitespace character, it will move the iterator index forward
   * until no more remain. It then returns true to signify that it could handle
   * the current situation, and the if/else chain restarts from the top. Lets say
   * the next symbol is a digit. The whitespace parser will be called again. Since
   * it doesn't know how to handle digits, it returns false and the digit is passed
   * down the chain until it reaches the number parser that can handle it.
   */
  for(; remaining_chars(l) > 0; ) {


    /* Recursive Descent Chain:
     * I know the McCabe's complexity is CRAZY high, but in this I case, I
     * believe that it is excusible because it greatly simplifies the rest of
     * the code document and makes the code much more understandable...the 
     * primary goal of good computer science.
     *
     * Basically, control just cascades down until it finds a method that
     * can handle the data.
     */
    if(next_parse_whitespace(l)) {

      /* do nothing, but restart if/else chain when whitespace is detected */

    } else if(next_parse_comments(l)) {
      if(l->err != LEXERERR_SUCCESS) {
	break;
      }
    } else if(next_parse_strings(l)) {
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_keyvars(l)){
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_numbers(l)){
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_operators(l)){
      *len = l->currTokenLen;
      return l->currToken;
    } else {

      /* none of the parsers can handle the current situation */
      /* temporary warning that helps with debugging */
      printf("WARNING: Control reached end of Recursive Parse Loop\n");
      break;
    }
  }
  return NULL;
}

char * lexer_current_token(Lexer * l, size_t * len) {
  *len = l->currTokenLen;
  return l->currToken;

}

LexerErr lexer_get_err(Lexer * l) {
  return l->err;
}

LexerErr lexer_line_num(Lexer * l) {
  return l->lineNum;
}

LexerType lexer_token_type(char * token, size_t len) {

  if(is_operator(token[0])) {
    if(token[len-1] == '"') {
      return LEXERTYPE_STRING;
    } else {
      
    }
  }

  return LEXERTYPE_UNKNOWN;
}
